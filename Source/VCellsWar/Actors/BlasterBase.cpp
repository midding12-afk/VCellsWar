// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "BlasterBase.h"

#include "GenericTeamAgentInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraComponent.h"
#include "StrategyEntityCharacter.h"

#include "Interface/StrategyEntityInterface.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"

// Sets default values
ABlasterBase::ABlasterBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// 1. Создаем сферу коллизии
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	
	// 💥 ГЛАВНЫЙ ААА-ФИКС ФИЗИКИ: Делаем сферу КОРНЕМ актора снаряда!
	// Теперь ProjectileMovement будет двигать саму физическую сферу в Chaos, 
	// и сервер железно увидит оверлапы со всеми серверными солдатами!
	RootComponent = SphereComponent;
	SphereComponent->SetSphereRadius(1000.f);
	
	// 2. Настраиваем дефолтную коллизию сферы для прошивающих пуль
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // Оверлапим солдат
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap); // Оверлапим статику ландшафта
	SphereComponent->SetGenerateOverlapEvents(true);
	

	// 3. Создаем и привязываем компонент движения
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereComponent; // Жестко привязываем обновление к сфере!
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f; // Отключаем гравитацию для лазеров
	// Приказывает ProjectileMovement использовать математический Sweep (сканирование линии)
	// при каждом перемещении актора на сервере, заставляя триггериться оверлапы! 
	ProjectileMovement->bSimulationEnabled = true;
	
}

// Called when the game starts or when spawned
void ABlasterBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (SphereComponent)
	{
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ABlasterBase::OnProjectileOverlap);
	}
}

// Called every frame
void ABlasterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ABlasterBase::InitBlasterShoot(FVector StartLocation, FVector Direction, FLinearColor Color, int32 NewTeamId)
{
	OwnerFactionID = NewTeamId;
	UWorld* World = GetWorld();
	if (!World) return;
	
	float ProjectileSpeed = 3000.f;  // Скорость бластера из ЗВ (30 метров в секунду)
	float MaxTraceDistance = 6000.f; // Максимальная дальность полёта в RTS (60 метров)
	

	// 1. Телепортируем физический корень (сферу) в точку выстрела
	SetActorLocationAndRotation(StartLocation, Direction.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);

	// 2. RTS ФИКС ДВИЖЕНИЯ НА СЕРВЕРЕ:
	// Принудительно будим и запускаем ProjectileMovement на СЕРВЕРЕ!
	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(RootComponent); // Еще раз цементируем корень
		ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed; // Задаем физический вектор скорости
		ProjectileMovement->Activate(true); // Принудительно включаем компонент в рантайме!
		
		ProjectileMovement->UpdateComponentVelocity(); 
		
		ProjectileSpeed = ProjectileMovement->GetMaxSpeed();
	}
	
	
	if (USphereComponent* MySphere = FindComponentByClass<USphereComponent>())
	{
		MySphere->SetUseCCD(true); // Оставляем непрерывную физику
		MySphere->SetGenerateOverlapEvents(true);
		MySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		// Возвращаем нормальный, адекватный RTS радиус пули
		MySphere->SetSphereRadius(20.0f, true); 

		// Переводим пулю в официальный тип объекта Physics Weapon / Projectile!
		// Это намертво сорвет оптимизационную слепоту сервера Chaos!
		MySphere->SetCollisionObjectType(ECC_PhysicsBody); // Либо ECC_PhysicsBody, если кастомного канала еще нет

		// Пересобираем ответы каналов пули:
		MySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		MySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);        // Жестко оверлапим солдат-Pawn!
		MySphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap); // Оверлапим карту для лога
		
		MySphere->UpdateOverlaps();
	}
	
	if (UNiagaraComponent* NiagaraComp = FindComponentByClass<UNiagaraComponent>())
	{
		NiagaraComp->SetNiagaraVariableLinearColor(TEXT("MainColor"), Color);
	}
	
	
	float CalculatedLifeTime = MaxTraceDistance / ProjectileSpeed; // Дефолтное время жизни (2 секунды)
	//if (CalculatedLifeTime <= 0.01f || CalculatedLifeTime > 5.0f) CalculatedLifeTime = 2.0f;

	// Запускаем таймер возврата в пул
	World->GetTimerManager().SetTimer(LifeTimerHandle, this, &ABlasterBase::ReturnToPool, CalculatedLifeTime, false);
}

void ABlasterBase::ReturnToPool()
{
	if (GetWorld())	GetWorld()->GetTimerManager().ClearTimer(LifeTimerHandle);
	
	ULocalGraphicsPoolSubsystem* Pool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	if (Pool) Pool->ReturnActorToPool(this);
}


void ABlasterBase::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Жесткий серверный шлюз. Физическая пуля считает урон строго на сервере матча!
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == GetOwner()) return;

	if (OtherActor->IsA(ABlasterBase::StaticClass())) return;

	if (AStrategyEntityCharacter* StrategyChar = Cast<AStrategyEntityCharacter>(OtherActor))
	{
		// Серверная пуля врежется СТРОГО в корневую капсулу серверного оригинала (высота Z=123/89)!
		int32 TargetFactionID = StrategyChar->GetRTSFactionIDDirect();

		//
		
		if (OwnerFactionID != TargetFactionID && OwnerFactionID != 255 && TargetFactionID != 255)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, *FString::Printf(TEXT("SERVER PROJECTILE OVERLAP: %d to %d"), OwnerFactionID, TargetFactionID));
			
			// Списываем ХП у серверного оригинала!
			// Проверка if (!HasAuthority()) внутри чарактера пропустит вызов, 
			// и солдат матча успешно улетит в пул резерва сервера!
			StrategyChar->GeinDamage(1.f);
			
			ReturnToPool();
		}
	}
}


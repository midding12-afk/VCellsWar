// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "PortalBase.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "VCellsWar/AI/StrategyAIController.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"
#include "VCellsWar/GameMods/MainGameGameState.h"

// Sets default values
APortalBase::APortalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SelectionDecalComponent->DecalSize = FVector(100.0f, 900.0f, 900.0f);
	
	
	
	// Шаг 2: Создаем и настраиваем гигантскую капсулу коллизии башни
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("PortalSphereCollision"));
	
	// Привязываем капсулу к нашему новому корню-хотспоту
	SphereComponent->SetupAttachment(RootComponent);
	
	SphereComponent->SetSphereRadius(450.0f);
	
	SphereComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SphereComponent->CanCharacterStepUpOn = ECB_No;
	SphereComponent->SetShouldUpdatePhysicsVolume(true);
	SphereComponent->SetCanEverAffectNavigation(false);
	SphereComponent->bDynamicObstacle = true;

	SphereComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -200));
	
}

void APortalBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APortalBase, NextSpawnTime);
	DOREPLIFETIME(APortalBase, BaseSpawnDelay);
	DOREPLIFETIME(APortalBase, PortalId);
}

// Called when the game starts or when spawned
void APortalBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
		
		if (GM)
		{
			GM->RegisterPortal(this);
		}
		
		ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
		
		ExecuteWaveSpawn();
	}
	else
	{
		AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->InvocLinksUpdate();
		}		
	}
}

// Called every frame
void APortalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortalBase::Server_SetNextSpawnDelay(float DelaySeconds)
{
	if (HasAuthority())
	{
		// Берем точную текущую секунду сервера и прибавляем задержку
		NextSpawnTime = GetWorld()->GetTimeSeconds() + DelaySeconds;
		
		// На хосте вызываем OnRep вручную для обновления локального UI
		OnRep_NextSpawnTime();
	}
}

void APortalBase::OnRep_NextSpawnTime()
{
	// Сеть доставила клиенту точную секунду спавна.
	// Здесь можно просто пнуть HUD, чтобы он обновил текст таймера.
	
	//GetServerWorldTimeSeconds();
}

void APortalBase::ScheduleNextWave()
{
	if (!HasAuthority() || !GetWorld()) return;

	// 1. По умолчанию берем базовую задержку
	float FinalDelay = BaseSpawnDelay;

	// 2. ИНТЕГРАЦИЯ С GAS (Из прошлых шагов): Проверяем, есть ли у владельца баффы на скорость спавна
	// Используем интерфейс или каст, который мы настраивали для GetEntityOwnerState()
	// В данном примере пишем через каст к нашему PlayerState:
	/*
	AMainGamePlayerState* RTSPlayerState = Cast<AMainGameState>(Execute_GetEntityOwnerState(this));
	if (RTSPlayerState && RTSPlayerState->GetAbilitySystemComponent())
	{
		// Считываем GAS-модификатор скорости из набора атрибутов игрока
		float Modifier = RTSPlayerState->GetAbilitySystemComponent()->GetNumericAttribute(URTSAttributeSet::GetSpawnCooldownModifierAttribute());
		FinalDelay *= Modifier; // Если модификатор 0.8, то 30 сек превратятся в 24 сек!
	}
	*/

	// 3. Записываем точную будущую секунду матча в реплицируемую переменную.
	// Клиентский Blueprint UI подхватит это число через GetServerWorldTimeSeconds() без спама пакетами!
	NextSpawnTime = GetWorld()->GetTimeSeconds() + FinalDelay;

	// 4. Взводим чистый серверный C++ таймер на эту задержку
	GetWorldTimerManager().SetTimer(
		WaveSpawnTimerHandle,
		this,
		&APortalBase::ExecuteWaveSpawn,
		FinalDelay,
		false // Галочку Looping НЕ ставим, так как задержка на следующей волне может измениться из-за GAS баффов!
	);
}

void APortalBase::ExecuteWaveSpawn()
{
	if (!HasAuthority() || !SoldierClass) return;

	// Получаем нашу изолированную серверную подсистему пула объектов
	//UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	
	
	
	if (ServerPool)
	{
		AMainGamePlayerState* PS = Execute_GetEntityOwnerState(this);
		APlayerController* PC = PS->GetPlayerController();
		if (!PS || !PC) return;
		
		FVector ForwardVec = GetActorForwardVector();
		FVector SpawnLocation = GetActorLocation();
		
		//TODO with GAS
		int32 Count = 100;	
		for (int Index = 0; Index < Count; Index++)
		{
			float AngleDegrees = 360.f/Count * Index;
			
			FRotator RotationAroundZ(0.0f, AngleDegrees, 0.0f);
			
			FVector RotatedForwardVec = RotationAroundZ.RotateVector(ForwardVec);
		
			FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation+RotatedForwardVec*150.f, FVector(1.0f, 1.0f, 1.0f));
		
			/*
			AActor* SpawnedActor = ServerPool->GetActorFromNetworkPool(SoldierClass, SpawnTransform);
			AStrategyEntityCharacter* Soldier = Cast<AStrategyEntityCharacter>(SpawnedActor);
			 		
			if (Soldier)
			{
				Soldier->SetEntityOwner(PS);			
			
			 	// Наращиваем счетчик поколения, чтобы убрать дергания у клиентов с пингом 500
			 	Soldier->SpawnGeneration++;
			 	
			 	if (Soldier->GetController() == nullptr)
			 	{
			 		// Этот метод создаст класс контроллера, который указан у вас в Class Defaults (AStrategyAIController)
			 		// и сразу же сделает ему Possess(Soldier) на стороне сервера!
			 		Soldier->SpawnDefaultController(); 
			 	}
			 	
			 	// Код выполняется на Сервере при рождении/активации юнита
			 	// if (AAIController* AIC = Cast<AAIController>(Soldier->GetController()))
			 	// {
			 	// 	AIC->SetGenericTeamId(FGenericTeamId(GetGenericTeamId()));
			 	// }
				Soldier->SetGenericTeamId(FGenericTeamId(GetGenericTeamId()));
				
			 	ServerPool->FinishSpawningNetworkUnit(Soldier, SpawnTransform);
			 	
				Soldier->LaunchFromPortal(RotatedForwardVec);
			}*/
			
			AActor* SpawnedActor = ServerPool->GetActorFromNetworkPool(SoldierClass, SpawnTransform);
			AStrategyEntityCharacter* Soldier = Cast<AStrategyEntityCharacter>(SpawnedActor);
            
			if (Soldier)
			{
				// Перезаписываем финальные параметры поверх инициализированной памяти
				Soldier->SetEntityOwner(PS);      
				Soldier->SetOwner(PC);
				
				if (AAIController* AIC = Cast<AAIController>(Soldier->GetController()))
				{
					AIC->SetOwner(PC);
				}
	
				// Насильно прописываем честную фракцию портала внутрь ИИ-контроллера и кэша тела! 
				Soldier->SetGenericTeamId(FGenericTeamId(GetGenericTeamId()));
    
				// Выталкиваем полностью готового к баллистическому полету юнита из ворот!
				Soldier->LaunchFromPortal(RotatedForwardVec);
			}
		}
	}	

	// Волна выпущена! Запускаем расчет времени для следующей волны
	ScheduleNextWave();
}





// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TacticalFlagBase.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "VCellsWar/Systems/StrategyGridSubsystem.h"

/*
ATacticalFlagBase::ATacticalFlagBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true); // Критически важно для плавного Drag-and-Drop в сети!

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	RootComponent = RootComp;

	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetupAttachment(RootComp);
	FlagMesh->SetCollisionProfileName(TEXT("Architecture")); // Настраиваем под клики мыши

	// TargetRadiusSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TargetRadiusSphere"));
	// TargetRadiusSphere->SetupAttachment(RootComp);
	// TargetRadiusSphere->SetCollisionProfileName(TEXT("NoCollision"));
}

void ATacticalFlagBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Синхронизируем физический размер сферы с параметром радиуса на старте
	// if (TargetRadiusSphere)
	// {
	// 	TargetRadiusSphere->SetSphereRadius(FlagRadius);
	// }
	
	if (!HasAuthority()) return;
	
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(GridUpdateTimerHandle))
	{
		TimerManager.SetTimer(GridUpdateTimerHandle, this, &ATacticalFlagBase::UpdateTroopsCounter, 1.0f, true);
	}
}

void ATacticalFlagBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATacticalFlagBase, FactionID);
	DOREPLIFETIME(ATacticalFlagBase, FlagRadius);
}

bool ATacticalFlagBase::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	// 1. Извлекаем PlayerController того человека, чей компьютер прямо сейчас запрашивает данные у сервера
	if (const APlayerController* ViewerPC = Cast<APlayerController>(RealViewer))
	{
		// 2. Достаем его PlayerState, где хранится его ID фракции / команды
		if (AMainGamePlayerState* ViewerPS = Cast<AMainGamePlayerState>(ViewerPC->PlayerState))
		{
			// Актор флага будет существовать в сети ТОЛЬКО если фракция смотрящего игрока 
			// хирургически точно совпадает с FactionID этого флага (то есть это его флаг)!
			// Для всех врагов и союзников метод вернет false, и движок полностью сотрет флаг с их экранов.
			return ViewerPS->GetGenericTeamId() == FactionID;
		}
	}

	// Дефолтное поведение для сервера и системных вызовов
	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}


void ATacticalFlagBase::OnRep_FlagRadius()
{
	// Когда радиус меняется на сервере, клиенты мгновенно обновляют визуальную сферу
	// if (TargetRadiusSphere)
	// {
	// 	TargetRadiusSphere->SetSphereRadius(FlagRadius);
	// }
}

void ATacticalFlagBase::UpdateTroopsCounter()
{
	UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>();
	if (GridSubsystem)
	{
		// Берем готовый С++ массив союзников в радиусе и просто считываем его размер .Num()!
		TArray<AActor*> NearbyAllies = GridSubsystem->FindAllAlliesInRadius(GetActorLocation(), FlagRadius, FactionID);
		CachedUnitsCount = NearbyAllies.Num();
	}
}

void ATacticalFlagBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int32 ATacticalFlagBase::GetCurrentUnitsInRadius() const
{
	return CachedUnitsCount;
}

bool ATacticalFlagBase::Server_UpdateFlagLocation_Validate(FVector NewWorldLocation)
{
	// ААА-Защита от читеров: проверяем, не пытается ли клиент телепортировать флаг сквозь стены 
	// или за пределы нашей MapSize. Сюда можно воткнуть Clamp по координатам!
	return true; 
}

void ATacticalFlagBase::Server_UpdateFlagLocation_Implementation(FVector NewWorldLocation)
{
	if (!HasAuthority()) return;

	// Сервер принудительно двигает актор. 
	// Благодаря SetReplicateMovement(true), движок сам плавно интерполирует (сгладит) 
	// перемещение флага на экранах всех остальных игроков в матче!
	SetActorLocation(NewWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
}*/
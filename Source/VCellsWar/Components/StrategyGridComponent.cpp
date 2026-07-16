// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "StrategyGridComponent.h"
#include "GameFramework/Actor.h"
#include "VCellsWar/Systems/StrategyGridSubsystem.h"

UStrategyGridComponent::UStrategyGridComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(false);
}

void UStrategyGridComponent::BeginPlay()
{
	Super::BeginPlay();

	// AActor* Owner = GetOwner();
	// if (!Owner || !GetWorld()) return;
	// if (GetWorld()->GetNetMode() == NM_Client) return;
	// if (!Owner->HasAuthority()) return;
	
	//ActivateGridTracking();
}

void UStrategyGridComponent::InitializeGridTracking()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;
	if (GetWorld()->GetNetMode() == NM_Client) return;
	if (!Owner->HasAuthority()) return;

	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		GridSubsystem->RegisterEntity(Owner);
		LastKnownSector = GridSubsystem->GetSectorCoords(Owner->GetActorLocation());
	}
}

void UStrategyGridComponent::DeinitializeGridTracking()
{
	if (!GetOwner()) return;
	
	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		GridSubsystem->UnregisterEntity(GetOwner());
	}
}

void UStrategyGridComponent::ActivateGridTracking()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || !Owner->HasAuthority()) return;
	
	if (GetWorld()->GetNetMode() == NM_Client) return;
	
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(GridUpdateTimerHandle))
	{
		TimerManager.SetTimer(GridUpdateTimerHandle, this, &UStrategyGridComponent::ManageGridSectorUpdate, 1.0f, true);
	}
}

// МЕТОД УСЫПЛЕНИЯ: Намертво тушит таймер (Дёргаем строго при остановке или уходе в пул!)
void UStrategyGridComponent::DeactivateGridTracking()
{
	if (!GetWorld()) return;

	// Полностью гасим таймер секторов, снимая фоновую нагрузку с CPU сервера
	GetWorld()->GetTimerManager().ClearTimer(GridUpdateTimerHandle);
	
}

void UStrategyGridComponent::ManageGridSectorUpdate()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || !Owner->HasAuthority()) return;
	
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		DeactivateGridTracking();
		return;
	}

	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		FIntPoint CurrentSector = GridSubsystem->GetSectorCoords(Owner->GetActorLocation());
		if (CurrentSector != LastKnownSector)
		{
			GridSubsystem->UpdateEntitySector(Owner);
			LastKnownSector = CurrentSector;
		}
	}
}
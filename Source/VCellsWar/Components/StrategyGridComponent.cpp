// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "StrategyGridComponent.h"
#include "GameFramework/Actor.h"
#include "VCellsWar/Systems/StrategyGridSubsystem.h"

UStrategyGridComponent::UStrategyGridComponent()
{
	// Нам НЕ нужен тяжелый Tick() компонента! Выключаем его в ноль для оптимизации
	PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyGridComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 1. Кешируем компонент движения
	CachedMoveComp = Owner->FindComponentByClass<UCharacterMovementComponent>();

	/*
	// 2. МГНОВЕННАЯ ПЕРВИЧНАЯ РЕГИСТРАЦИЯ:
	// Как только компонент проснулся на сервере (при спавне или выходе из ворот),
	// он сразу ставит объект на радары нашей Grid-подсистемы секторов!
	if (UWorld* World = GetWorld())
	{
		if (UStrategyGridSubsystem* GridSubsystem = World->GetSubsystem<UStrategyGridSubsystem>())
		{
			GridSubsystem->RegisterEntity(Owner);
			LastKnownSector = GridSubsystem->GetSectorCoords(Owner->GetActorLocation());
		}
		
		if (CachedMoveComp)
		{
			// 3. ЗАПУСКАЕМ ТРЕКЕР ДВИЖЕНИЯ:
			// Раз в 1с проверяем, сдвинулся ли юнит с места.
			// Это в сотни раз легче, чем нагружать физический движок или Tick.
			GetWorld()->GetTimerManager().SetTimer(
				MovementCheckTimerHandle,
				this,
				&UStrategyGridComponent::CheckMovementState,
				1.f,
				true
			);
		}
	}*/
}

void UStrategyGridComponent::ActivateGridTracking(const bool bCanMove)
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	// 1. МГНОВЕННАЯ РЕГИСТРАЦИЯ
	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		GridSubsystem->RegisterEntity(Owner);
		LastKnownSector = GridSubsystem->GetSectorCoords(Owner->GetActorLocation());
	}

	if (!bCanMove) return;
	
	// 2. ЗАПУСКАЕМ ТРЕКЕР ДВИЖЕНИЯ (раз в 0.25 секунды)
	GetWorld()->GetTimerManager().SetTimer(
		MovementCheckTimerHandle,
		this,
		&UStrategyGridComponent::CheckMovementState,
		5.f,
		true
	);
}

void UStrategyGridComponent::DeactivateGridTracking()
{
	if (!GetWorld() || !GetOwner()) return;

	// 1. Полностью гасим все таймеры компонента, чтобы спящий юнит не ел ресурсы CPU!
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(GridUpdateTimerHandle);
	TimerManager.ClearTimer(MovementCheckTimerHandle);

	// 2. Намертво выписываем солдата из подсистемы секторов матча
	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		GridSubsystem->UnregisterEntity(GetOwner());
	}
}


void UStrategyGridComponent::CheckMovementState()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	bool bIsMoving = false;

	// Проверяем движение: либо через скорость навигации, либо по физическому вектору скорости
	if (CachedMoveComp)
	{
		bIsMoving = CachedMoveComp->Velocity.SizeSquared() > 10.0f;
	}
	// else
	// {
	// 	bIsMoving = Owner->GetVelocity().SizeSquared() > 10.0f;
	// }

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// Если юнит побежал — включаем цикличный секундный таймер обновления секторов сетки
	if (bIsMoving)
	{
		if (!TimerManager.IsTimerActive(GridUpdateTimerHandle))
		{
			TimerManager.SetTimer(GridUpdateTimerHandle, this, &UStrategyGridComponent::ManageGridSectorUpdate, 3.0f, true);
		}
	}
	
	// полностью освобождая ресурсы процессора от лишних расчетов деления координат!
	else
	{
		if (TimerManager.IsTimerActive(GridUpdateTimerHandle))
		{
			TimerManager.ClearTimer(GridUpdateTimerHandle);
		}
	}
}

void UStrategyGridComponent::ManageGridSectorUpdate()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	if (UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>())
	{
		// Проверяем текущие координаты
		FIntPoint CurrentSector = GridSubsystem->GetSectorCoords(Owner->GetActorLocation());
		
		// Насильно пинаем подсистему обновить сектор ТОЛЬКО если граница действительно пересечена!
		if (CurrentSector != LastKnownSector)
		{
			GridSubsystem->UpdateEntitySector(Owner);
			LastKnownSector = CurrentSector; // Запоминаем новый квадрат
		}
	}
}

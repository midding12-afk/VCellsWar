// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "RTSPathVisualizerComponent.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/Actors/DecalLineBase.h"
#include "VCellsWar/Actors/DecalMoveTargetBase.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"


URTSPathVisualizerComponent::URTSPathVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true); 
	TargetDestination = FVector::ZeroVector;
	
}

void URTSPathVisualizerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Оптимизация трафика: шлем точку ТОЛЬКО игроку-владельцу отряда (COND_OwnerOnly)
	DOREPLIFETIME_CONDITION(URTSPathVisualizerComponent, TargetDestination, COND_OwnerOnly);
}

// ВЫЗЫВАЕТСЯ НА СЕРВЕРЕ (когда ИИ контроллер прокладывает или обновляет путь)
void URTSPathVisualizerComponent::SetNewMoveDestination(const FVector& NewDestination)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	// Если цель изменилась — обновляем переменную (движок сам отправит её клиентам по сети)
	if (TargetDestination != NewDestination)
	{
		TargetDestination = NewDestination;

		// ПРАВИЛЬНАЯ ПРОВЕРКА НА ЛИСТЕН-СЕРВЕР:
		// Проверяем, является ли Владелец (Owner) юнита локальным контроллером игрока на сервере
		if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetOwner()))
		{
			if (PC->IsLocalController())
			{
				// Этот код выполнится ТОЛЬКО для юнитов самого хоста (Listen-Server)
				OnRep_TargetDestination();
				return; // Выходим, клиентам движок отреплицирует всё сам
			}
		}
	}
}

// СРАБАТЫВАЕТ НА КЛИЕНТЕ ВЛАДЕЛЬЦА
void URTSPathVisualizerComponent::OnRep_TargetDestination()
{
	if (!GetWorld() || TargetDestination.IsZero()) 
	{
		DeactivateTracking();
		return;
	}

	// Запускаем таймер локального обновления линии движения
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(UpdateTimerHandle))
	{
		// Частоту можно сделать даже 0.1s или 0.05s, так как одна линия процессор вообще не грузит, зато будет плавно!
		TimerManager.SetTimer(UpdateTimerHandle, this, &URTSPathVisualizerComponent::ManagePathUpdate, 0.05f, true);
	}
}

// ЛОКАЛЬНЫЙ ТИК КЛИЕНТА (Отрисовка)
void URTSPathVisualizerComponent::ManagePathUpdate()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;
	
	if (Owner->IsHidden())
	{
		DeactivateTracking();
		return;
	}

	// 1. Проверяем, выделен ли юнит прямо сейчас
	if (IStrategyEntityInterface* EntityInterface = Cast<IStrategyEntityInterface>(Owner))
	{
		if (!EntityInterface->NativeRTSIsEntitySelected())
		{
			ClearActivePath();
			return; // Просто скрываем графику, но таймер пусть ждет (вдруг снова выделим)
		}
	}

	FVector MyLocation = Owner->GetActorLocation();

	// 2. Проверяем расстояние. Если мы уже почти у цели — тушим графику и таймер
	if (FVector::DistSquared(MyLocation, TargetDestination) < 40000.0f) // 100 единиц (1 метр)
	{
		DeactivateTracking();
		return;
	}

	// 3. РАБОТА С ПУЛОМ ГРАФИКИ
	ULocalGraphicsPoolSubsystem* GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	if (!GraphicsPool) return;

	// ---- ОТРИСОВКА ОДНОЙ ПРЯМОЙ ЛИНИИ ----
	ADecalLineBase* Decal = ActivePathLine.Get();
	if (!Decal)
	{
		AActor* PoolActor = GraphicsPool->GetActorFromPool(ADecalLineBase::StaticClass());
		Decal = Cast<ADecalLineBase>(PoolActor);
		if (Decal)
		{
			ActivePathLine = Decal;
		}
	}

	if (Decal)
	{
		// Линия идет строго от ног юнита до финальной реплицированной точки
		Decal->SetParametrs(MyLocation, TargetDestination);
	}

	// ---- ОТРИСОВКА МАРКЕРА ЦЕЛИ НА ЗЕМЛЕ ----
	if (!MyActiveDestination)
	{
		AActor* PoolActor = GraphicsPool->GetActorFromPool(ADecalMoveTargetBase::StaticClass());
		MyActiveDestination = Cast<ADecalMoveTargetBase>(PoolActor);
	}

	if (MyActiveDestination)
	{
		MyActiveDestination->SetActorLocation(TargetDestination);
	}
}

void URTSPathVisualizerComponent::DeactivateTracking()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}
	ClearActivePath();
	TargetDestination = FVector::ZeroVector;
}

void URTSPathVisualizerComponent::ClearActivePath()
{
	ULocalGraphicsPoolSubsystem* GraphicsPool = nullptr;
	if (GetWorld())
	{
		GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	}
	
	if (!GraphicsPool) return;

	if (ADecalLineBase* Line = ActivePathLine.Get())
	{
		GraphicsPool->ReturnActorToPool(Line);
		ActivePathLine = nullptr;
	}

	if (MyActiveDestination)
	{
		GraphicsPool->ReturnActorToPool(MyActiveDestination);
		MyActiveDestination = nullptr;
	}
}

void URTSPathVisualizerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Намертво тушим таймеры и возвращаем декали в пул графики на клиенте
	DeactivateTracking();

	Super::EndPlay(EndPlayReason);
}
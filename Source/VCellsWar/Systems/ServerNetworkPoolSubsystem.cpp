// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "ServerNetworkPoolSubsystem.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/StateTreeComponent.h"


bool UServerNetworkPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->GetNetMode() != NM_Client;
	//return World && (World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer));
}

AActor* UServerNetworkPoolSubsystem::GetActorFromNetworkPool(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, const int32 InFactionID, AMainGamePlayerState* InOwnerState)
{	
	if (!ActorClass || !GetWorld()) return nullptr;

	FNetworkActorPoolList* PoolList = NetworkObjectPoolMap.Find(ActorClass);

	// =========================================================================
	// ВЕТКА А: ЮНИТ ПРОСНУЛСЯ ИЗ МАССИВА СПЯЩИХ (Универсальная)
	// =========================================================================
	if (PoolList && PoolList->InactiveActors.Num() > 0)
	{
		AActor* RetrievedActor = PoolList->InactiveActors.Pop(EAllowShrinking::No);
		if (IsValid(RetrievedActor))
		{
			RetrievedActor->SetActorEnableCollision(true);
			// Насильно приказываем ядру Unreal Engine прямо сейчас заново зарегистрировать 
			// все физические и навигационные компоненты актора в потоке Chaos сервера!
			// Это полностью стирает анабиозный сон режима MOVE_None и отвязки от сцены!
			RetrievedActor->ReregisterAllComponents();
			
			// ВЫЗОВ ИНИЦИАЛИЗАЦИИ ДЛЯ СТАРЫХ ЮНИТОВ:
			// Пул просто пинает интерфейс. Объект САМ внутри себя включит 
			// нужные коллизии, сделает телепортацию физики и включит ИИ-мозги! 
			if (IStrategyEntityInterface* EntityInterface = Cast<IStrategyEntityInterface>(RetrievedActor))
			{
				// Передаем дефолтные заглушки, так как портал перезапишет фракцию строчкой позже!
				EntityInterface->NativeRTSInitialize(InFactionID, InOwnerState, SpawnTransform);
			}
			else
			{
				// Страховка для обычных объектов (бочек, декораций), у которых нет интерфейса
				RetrievedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
				RetrievedActor->SetActorEnableCollision(true);
			}

			// Будим сеть и визуал (Базовые методы AActor)
			SetActorNetworkActive(RetrievedActor, true);
			RetrievedActor->SetActorHiddenInGame(false);
			RetrievedActor->SetReplicates(true);
			RetrievedActor->SetReplicateMovement(true);	
			
			if (APawn* PawnActor = Cast<APawn>(RetrievedActor))
			{
				if (AController* AIC = PawnActor->GetController())
				{
					AIC->SetActorTickEnabled(true);

					if (UStateTreeComponent* STComp = AIC->FindComponentByClass<UStateTreeComponent>())
					{
						// Запускаем State Tree с чистого листа в новой локации боя!
						STComp->StartLogic();
					}
				}
			}

			return RetrievedActor;
		}
	}

	// =========================================================================
	// ВЕТКА Б: МГНОВЕННЫЙ СПАВН НОВОГО АКТОРA (Теперь с инициализацией!)
	// =========================================================================
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 1. Создаем "эмбрион" актора на сервере.
	AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(
		ActorClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	
	if (NewActor)
	{
		// 2. ФИНАЛИЗАЦИЯ: Пусть движок соберет Блупринт, применит CDо и Construction Script
		NewActor->FinishSpawning(SpawnTransform);

		// 3. ВЫЗОВ ИНИЦИАЛИЗАЦИИ ДЛЯ НОВЫХ ЮНИТОВ:
		// Как только Блупринт финализирован, мы принудительно вызываем инициализацию.
		// Метод настроит Custom-коллизию, запечет ServerMasterVersion и включит MOVE_Falling! 
		if (IStrategyEntityInterface* EntityInterface = Cast<IStrategyEntityInterface>(NewActor))
		{
			EntityInterface->NativeRTSInitialize(InFactionID, InOwnerState, SpawnTransform);
		}

		// 4. Сетевые шлюзы AActor
		NewActor->SetReplicates(true);
		NewActor->SetReplicateMovement(true);
		SetActorNetworkActive(NewActor, true);
	}

	return NewActor;
}



AActor* UServerNetworkPoolSubsystem::GetActorFromNetworkPoolDeferred(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass || !GetWorld()) return nullptr;

	// Ищем список спящих объектов для данного конкретного класса в нашей TMap
	FNetworkActorPoolList* PoolList = NetworkObjectPoolMap.Find(ActorClass);

	if (PoolList && PoolList->InactiveActors.Num() > 0)
	{
		// Достаем последнего актора из массива без сжатия памяти 
		AActor* RetrievedActor = PoolList->InactiveActors.Pop(EAllowShrinking::No);

		if (IsValid(RetrievedActor))
		{
			// Телепортируем невидимый актор в координаты спавна и сбрасываем физику прошлых жизней
			RetrievedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

			// Будим объект в глобальной сети матча
			//SetActorNetworkActive(RetrievedActor, true);

			// Возвращаем "замороженного" актора. Он готов к записи любых C++ и Блупринт данных!
			return RetrievedActor;
		}
	}

	// Если пул для этого класса пуст, вызываем универсальный отложенный спавн движка UE5!
	AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(
		ActorClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	
	if (NewActor)
	{
		NewActor->SetReplicates(true);
		NewActor->SetReplicateMovement(true);
		//SetActorNetworkActive(NewActor, true);
	}

	return NewActor;
}

void UServerNetworkPoolSubsystem::FinishSpawningNetworkUnit(AActor* ActorToFinish, const FTransform& SpawnTransform)
{
	if (!IsValid(ActorToFinish)) return;

	// 1. Если это абсолютно новый актор — финализируем его в движке.
	// Родной FinishSpawningActor сам внутри себя впервые включит репликацию для нового объекта.
	if (!ActorToFinish->HasActorBegunPlay())
	{
		UGameplayStatics::FinishSpawningActor(ActorToFinish, SpawnTransform);
	}

	// 2. ЖЕЛЕЗОБЕТОННЫЙ ФИКС: Включаем сетевую актуальность только ДЛЯ ПРОСНУВШИХСЯ из пула объектов!
	// Делаем это строго ПОСЛЕ того, как записал в актора все геймплейные переменные.
	if (ActorToFinish->HasActorBegunPlay())
	{
		SetActorNetworkActive(ActorToFinish, true);
	}

	// 3. Включаем визуал, коллизию и движение. 
	// Весь этот пакет данных (Локация + Владелец + Видимость + GAS теги) улетит клиентам за ОДИН СЕТЕВОЙ ТИК!
	ActorToFinish->SetActorHiddenInGame(false);
	ActorToFinish->SetActorEnableCollision(true);
	ActorToFinish->SetReplicateMovement(true); // Возвращаем репликацию движения

	if (UMovementComponent* MoveComp = ActorToFinish->FindComponentByClass<UMovementComponent>())
	{
		MoveComp->SetActive(true);
		
		if (UCharacterMovementComponent* CharMoveComp = Cast<UCharacterMovementComponent>(MoveComp))
		{
			CharMoveComp->SetMovementMode(MOVE_Falling);
		}
	}
	
	if (APawn* PawnActor = Cast<APawn>(ActorToFinish))
	{
		if (AController* AIC = PawnActor->GetController())
		{
			AIC->SetActorTickEnabled(true);

			if (UStateTreeComponent* STComp = AIC->FindComponentByClass<UStateTreeComponent>())
			{
				// Запускаем State Tree с чистого листа в новой локации боя!
				STComp->StartLogic();
			}
		}
	}
}

void UServerNetworkPoolSubsystem::ReturnActorToNetworkPool(AActor* ActorToReturn)
{
	if (!IsValid(ActorToReturn)) return;

	
	// 1. ОГЛУШАЕМ ИИ-МОЗГИ НА СЕРВЕРЕ ПЕРЕД УХОДОМ В ПУЛ
	if (APawn* PawnActor = Cast<APawn>(ActorToReturn))
	{
		if (AController* AIC = PawnActor->GetController())
		{
			AIC->StopMovement(); // Насильно тормозим бег навигации
		
			// ВМЕСТО UNPOSSESS: УСЫПЛЕНИЕ ДЕРЕВА СОСТОЯНИЙ STATE TREE
			// Ищем компонент State Tree внутри твоего AIController
			if (UStateTreeComponent* STComp = AIC->FindComponentByClass<UStateTreeComponent>())
			{
				// Намертво выключаем и сбрасываем логику дерева состояний.
				// После этого State Tree физически перестает тикать и вызывать любые таски/MakeShoot()!
				STComp->StopLogic(TEXT("Pooling Sleep"));
			}

			// Выключаем тик самого актора контроллера
			AIC->SetActorTickEnabled(false);
		}
	}

	// 2. СКРЫВАЕМ ВИЗУАЛ ДЛЯ СЕТИ
	// Поскольку репликация в этот нанокадр ЕЩЕ ВКЛЮЧЕНА, NetDriver 
	// мгновенно перешлет флаг Hidden=true на все клиенты, и прокси-куклы исчезнут!
	ActorToReturn->SetActorHiddenInGame(true);
	ActorToReturn->SetActorEnableCollision(false);
	ActorToReturn->SetReplicateMovement(false);

	if (ACharacter* CharActor = Cast<ACharacter>(ActorToReturn))
	{
		if (UCapsuleComponent* Capsule = CharActor->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}

	if (UMovementComponent* MoveComp = ActorToReturn->FindComponentByClass<UMovementComponent>())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetActive(false);
		if (UCharacterMovementComponent* CharMoveComp = Cast<UCharacterMovementComponent>(MoveComp))
		{
			CharMoveComp->SetMovementMode(MOVE_None);
			CharMoveComp->ClearAccumulatedForces();
		}
	}

	// 3. ЗАКРЫВАЕМ СЕТЕВОЙ ШЛЮЗ В САМОМ КОНЦЕ
	// Теперь, когда клиенты успешно скрыли манекены, мы можем убрать актор из сети
	SetActorNetworkActive(ActorToReturn, false);

	// Прячем в массив спящих
	TSubclassOf<AActor> ActorClass = ActorToReturn->GetClass();
	FNetworkActorPoolList& PoolList = NetworkObjectPoolMap.FindOrAdd(ActorClass);
	PoolList.InactiveActors.Add(ActorToReturn);
}




void UServerNetworkPoolSubsystem::SetActorNetworkActive(AActor* TargetActor, bool bIsActive)
{
	if (!TargetActor) return;

	if (bIsActive)
	{
		TargetActor->bAlwaysRelevant = true;
		TargetActor->NetCullDistanceSquared = 14400000000.0f; // Стандартная дистанция сети (около 120м)
	}
	else
	{
		// Полностью изолируем актора от отправки сетевых пакетов клиентам, пока он спит в пуле
		TargetActor->bAlwaysRelevant = false;
		TargetActor->NetCullDistanceSquared = 0.0f;
	}
}

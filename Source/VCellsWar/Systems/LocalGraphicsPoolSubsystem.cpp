// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LocalGraphicsPoolSubsystem.h"


bool ULocalGraphicsPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;

	// Отсекаем выделенный сервер, подсистема живет только там, где есть рендеринг графики
	if (FApp::CanEverRender() == false || IsRunningDedicatedServer())
	{
		return false;
	}
	return true;
}

AActor* ULocalGraphicsPoolSubsystem::GetActorFromPool(TSubclassOf<AActor> ActorClass)
{
	if (!ActorClass) return nullptr;

	// Ищем список спящих объектов для данного класса в нашей TMap
	FVisualActorPoolList* PoolList = ObjectPoolMap.Find(ActorClass);

	// Если список существует и в нем есть хотя бы один спящий актор
	if (PoolList && PoolList->InactiveActors.Num() > 0)
	{
		// Достаем последнего актора из массива (это быстрее для памяти, чем забирать нулевого)
		AActor* RetrievedActor = PoolList->InactiveActors.Pop(EAllowShrinking::No);

		if (IsValid(RetrievedActor))
		{			
			// Включаем отображение мешей/частиц и возвращаем коллизию
			RetrievedActor->SetActorHiddenInGame(false);
			RetrievedActor->SetActorEnableCollision(true);
			//RetrievedActor->SetActorTickEnabled(true);

			return RetrievedActor;
		}
	}

	// Если пул для этого класса пуст или актор оказался невалидным, спавним НОВЫЙ объект.
	// Задаем параметры спавна: этот актор будет 100% локальным (только для этого клиента)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	
	AActor* NewVisualActor = GetWorld()->SpawnActor<AActor>(ActorClass, FVector::Zero(), FRotator::ZeroRotator, SpawnParams);
	
	// На всякий случай аппаратно выключаем репликацию, так как пулу визуала она противопоказана
	if (NewVisualActor)
	{
		NewVisualActor->SetReplicates(false);
	}

	return NewVisualActor;
}

void ULocalGraphicsPoolSubsystem::ReturnActorToPool(AActor* ActorToReturn)
{
	if (!IsValid(ActorToReturn)) return;

	// Усыпляем актора: полностью прячем с экрана и отключаем коллизию, чтобы разгрузить физический движок Chaos
	ActorToReturn->SetActorHiddenInGame(true);
	ActorToReturn->SetActorEnableCollision(false);
	ActorToReturn->SetActorTickEnabled(false);

	// Получаем точный C++ класс возвращаемого объекта
	TSubclassOf<AActor> ActorClass = ActorToReturn->GetClass();

	// Добавляем актора в соответствующую ячейку нашей TMap структуры пула
	FVisualActorPoolList& PoolList = ObjectPoolMap.FindOrAdd(ActorClass);
	PoolList.InactiveActors.Add(ActorToReturn);
}

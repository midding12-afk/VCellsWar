// Copyright (c) 2026, Dmitry Tur. All rights reserved.
#include "StrategyGridSubsystem.h"
#include "NavigationSystem.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h" 
#include "StrategyGridSubsystem.h"

int32 UStrategyGridSubsystem::GetFactionIdFromActor(AActor* Actor) const
{
	if (Actor && Actor->GetClass()->ImplementsInterface(UStrategyEntityInterface::StaticClass()))
	{
		return IStrategyEntityInterface::Execute_GetEntityFactionID(Actor);
	}
	return 255; // Дефолтное безопасное значение для нейтралов/декораций
}

EStrategyEntityCategory UStrategyGridSubsystem::GetEntityCategoryFromActor(AActor* Actor) const
{
	if (Actor && Actor->GetClass()->ImplementsInterface(UStrategyEntityInterface::StaticClass()))
	{
		// Вызываем твой новый интерфейсный метод получения категории
		return IStrategyEntityInterface::Execute_GetEntityCategory(Actor);
	}
	return EStrategyEntityCategory::StaticBuild; // Безопасный фоллбэк
}

FIntPoint UStrategyGridSubsystem::GetSectorCoords(FVector Location) const
{
	int32 SectorX = FMath::FloorToInt(Location.X / SectorSize);
	int32 SectorY = FMath::FloorToInt(Location.Y / SectorSize);
	return FIntPoint(SectorX, SectorY);
}

void UStrategyGridSubsystem::RegisterEntity(AActor* Entity)
{
	if (!Entity || !Entity->HasAuthority()) return;
	IStrategyEntityInterface* Soldier = Cast<IStrategyEntityInterface>(Entity);
	if (!Soldier) return;
	
	if (ActorToSectorMap.Find(Entity)) return;

	int32 FactionId = GetFactionIdFromActor(Entity);
	EStrategyEntityCategory Category = GetEntityCategoryFromActor(Entity);
	FIntPoint SectorCoords = GetSectorCoords(Entity->GetActorLocation());

	FStrategyGridSector& GridSector = GlobalPlayersGrids.FindOrAdd(FactionId).Sectors.FindOrAdd(SectorCoords);
	TArray<AActor*>& TargetBucket = GridSector.GetBucket(Category);

	// Определяем базовый шаг выделения памяти для данной категории объектов
	const int32 AllocationStep = (Category == EStrategyEntityCategory::Troop) ? 512 : 32;

	// =========================================================================
	// АВТО-БУСТ ОЗУ (БЕЗ ФРИЗОВ И ПЕРЕРАСХОДА ПАМЯТИ)
	// .Max() возвращает физическую емкость (Capacity) выделенного куска Кучи.
	// Если бакет пуст ИЛИ текущая армия уперлась в потолок выделенной памяти,
	// мы РОВНО НА ОДИН ШАГ расширяем стек, исключая хаотичное раздувание массива!
	// =========================================================================
	if (TargetBucket.Num() == 0)
	{
		TargetBucket.Reserve(AllocationStep);
	}
	else if (TargetBucket.Num() >= TargetBucket.Max())
	{
		// Расширяем текущую емкость ровно на величину изначального шага 
		int32 NewStepCapacity = TargetBucket.Max() + AllocationStep;
		TargetBucket.Reserve(NewStepCapacity);
	}

	// ЗАПОМИНАЕМ ИНДЕКС: Юнит добавляется в конец бакета и жестко знает, в какую ячейку массива он сел
	Soldier->SetSelectedSectorGridIndex(TargetBucket.Add(Entity));

	ActorToSectorMap.Add(Entity, SectorCoords);
}

void UStrategyGridSubsystem::UnregisterEntity(AActor* Entity)
{
	if (!Entity) return;
	IStrategyEntityInterface* Soldier = Cast<IStrategyEntityInterface>(Entity);
	if (!Soldier) return;

	int32 FactionId = GetFactionIdFromActor(Entity);
	EStrategyEntityCategory Category = GetEntityCategoryFromActor(Entity);
	FIntPoint* FoundSector = ActorToSectorMap.Find(Entity);
	if (!FoundSector) return;

	if (FPlayerGrid* PlayerGrid = GlobalPlayersGrids.Find(FactionId))
	{
		if (FStrategyGridSector* GridSector = PlayerGrid->Sectors.Find(*FoundSector))
		{
			TArray<AActor*>& TargetBucket = GridSector->GetBucket(Category);
			int32 DeadUnitIdx = Soldier->GetSelectedSectorGridIndex();
			
			// Проверяем легитимность индекса перед выстрелом по памяти в конкретном бакете
			if (TargetBucket.IsValidIndex(DeadUnitIdx))
			{
				int32 LastIdx = TargetBucket.Num() - 1;
				AActor* ShiftingSwappedUnit = TargetBucket[LastIdx];

				// Наш священный Swap-and-Pop без сдвигов памяти и дефрагментации ОЗУ!
				TargetBucket.RemoveAtSwap(DeadUnitIdx, 1, EAllowShrinking::No);

				// СИНХРОНИЗАЦИЯ ИНДЕКСА ХВОСТА ВНУТРИ БАКЕТА:
				if (DeadUnitIdx < TargetBucket.Num() && IsValid(ShiftingSwappedUnit))
				{
					if (IStrategyEntityInterface* SwappedChar = Cast<IStrategyEntityInterface>(ShiftingSwappedUnit))
					{
						SwappedChar->SetSelectedSectorGridIndex(DeadUnitIdx);
					}
				}
			}

			Soldier->SetSelectedSectorGridIndex(-1);

			// Схлопываем сектор, только если абсолютно ВСЕ бакеты пусты!
			bool bIsSectorCompletelyEmpty = true;
			for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
			{
				EStrategyEntityCategory CurrentCat = static_cast<EStrategyEntityCategory>(i);
				if (GridSector->GetBucket(CurrentCat).Num() > 0)
				{
					bIsSectorCompletelyEmpty = false;
					break; // Если хоть в одном бакете кто-то есть — сектор живой, выходим из цикла
				}
			}

			// Если абсолютно все бакеты чисты — стираем ячейку из памяти
			if (bIsSectorCompletelyEmpty)
			{
				PlayerGrid->Sectors.Remove(*FoundSector);
			}
		}
	}
	ActorToSectorMap.Remove(Entity);
}

void UStrategyGridSubsystem::UpdateEntitySector(AActor* Entity)
{
	if (!Entity) return;

	FIntPoint NewSector = GetSectorCoords(Entity->GetActorLocation());
	FIntPoint* OldSectorPtr = ActorToSectorMap.Find(Entity);

	if (!OldSectorPtr)
	{
		RegisterEntity(Entity);
		return;
	}

	if (*OldSectorPtr != NewSector)
	{
		UnregisterEntity(Entity);
		RegisterEntity(Entity);
	}
}

AActor* UStrategyGridSubsystem::FindRandomEnemyEntityInRadius(AActor* ScanningEntity, float Radius)
{
	if (!IsValid(ScanningEntity)) return nullptr;

	// Этаж 1: Сначала ищем исключительно живую силу врага!
	const EStrategyEntityCategory TroopType = EStrategyEntityCategory::Troop;
	if (AActor* FoundTroop = FindRandomEnemyEntityInRadiusByType(ScanningEntity, Radius, &TroopType))
	{
		return FoundTroop;
	}

	// Этаж 2: Если солдат рядом нет — переключаемся на оборонительные турели 
	const EStrategyEntityCategory DefenseType = EStrategyEntityCategory::DefenseBuild;
	if (AActor* FoundDefense = FindRandomEnemyEntityInRadiusByType(ScanningEntity, Radius, &DefenseType))
	{
		return FoundDefense;
	}

	// Этаж 3: Если оборонительный рубеж чист — ломаем башни связи Делоне
	const EStrategyEntityCategory TowerType = EStrategyEntityCategory::Tower;
	if (AActor* FoundTower = FindRandomEnemyEntityInRadiusByType(ScanningEntity, Radius, &TowerType))
	{
		return FoundTower;
	}

	// Этаж 4: В самом конце, когда воевать больше не с кем, пилим постройки
	const EStrategyEntityCategory StaticBuildType = EStrategyEntityCategory::StaticBuild;
	return FindRandomEnemyEntityInRadiusByType(ScanningEntity, Radius, &StaticBuildType);
}

AActor* UStrategyGridSubsystem::FindRandomEnemyEntityInRadiusByType(AActor* ScanningEntity, float Radius, const EStrategyEntityCategory* SpecificCategory)
{
	if (!IsValid(ScanningEntity)) return nullptr;

	int32 MyFactionId = GetFactionIdFromActor(ScanningEntity);
	FVector MyLocation = ScanningEntity->GetActorLocation();
	FIntPoint CenterSector = GetSectorCoords(MyLocation);
	int32 SectorRange = FMath::Max(1, FMath::CeilToInt(Radius / SectorSize));

	// Наш инлайн-аллокатор на стеке для мгновенного сбора секторов без фризов ОЗУ
	TArray<const FStrategyGridSector*, TInlineAllocator<64>> ValidEnemySectors;
	int32 TotalEnemyCount = 0;

	// Фаза 1: Собираем сектора, где есть подходящие враги
	for (auto& PlayerGridPair : GlobalPlayersGrids)
	{
		int32 EnemyFactionId = PlayerGridPair.Key;
		if (EnemyFactionId == MyFactionId) continue;

		for (int32 x = CenterSector.X - SectorRange; x <= CenterSector.X + SectorRange; ++x)
		{
			for (int32 y = CenterSector.Y - SectorRange; y <= CenterSector.Y + SectorRange; ++y)
			{
				FIntPoint CurrentSectorCoords(x, y);
				const FStrategyGridSector* TargetSector = PlayerGridPair.Value.Sectors.Find(CurrentSectorCoords);
				
				if (TargetSector)
				{
					int32 SectorCount = 0;
					
					// Если категория ЗАДАНА — проверяем только её бакет!
					if (SpecificCategory)
					{
						SectorCount = TargetSector->GetBucket(*SpecificCategory).Num();
					}
					// Если категория НЕ ЗАДАНА (nullptr) — суммируем вообще все бакеты ячейки!
					else
					{
						for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
						{
							SectorCount += TargetSector->GetBucket(static_cast<EStrategyEntityCategory>(i)).Num();
						}
						
					}

					if (SectorCount > 0)
					{
						ValidEnemySectors.Add(TargetSector);
						TotalEnemyCount += SectorCount;
					}
				}
			}
		}
	}

	if (TotalEnemyCount == 0) return nullptr;
	
	
	float RadSq = Radius * Radius;
	auto CheckAndReturn = [&](const TArray<AActor*>& Bucket, AActor* ExcludeEnemy, FVector Loc) -> AActor*
	{
		for (AActor* BackupEnemy : Bucket)
		{
			if (IsValid(BackupEnemy) && BackupEnemy != ExcludeEnemy)
			{
				ATowerBase* EnemyTower = Cast<ATowerBase>(BackupEnemy);
				if (EnemyTower && EnemyTower->IsTowerLockedNoConnection(MyFactionId)) continue;

				if (FVector::DistSquared(Loc, BackupEnemy->GetActorLocation()) <= RadSq)
				{
					return BackupEnemy;
				}
			}
		}
		return nullptr;
	};

	// Фаза 2: Выбираем случайную цель из собранного пула
	int32 RandomGlobalIndex = FMath::RandRange(0, TotalEnemyCount - 1);

	for (const FStrategyGridSector* TargetSector : ValidEnemySectors)
	{
		int32 SectorCount = 0;
		if (SpecificCategory)
		{
			SectorCount = TargetSector->GetBucket(*SpecificCategory).Num();
		}
		else
		{
			for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
			{
				SectorCount += TargetSector->GetBucket(static_cast<EStrategyEntityCategory>(i)).Num();
			}
		}

		if (RandomGlobalIndex < SectorCount)
		{
			AActor* RandomEnemy = nullptr;

			if (SpecificCategory)
			{
				RandomEnemy = TargetSector->GetBucket(*SpecificCategory)[RandomGlobalIndex];
			}
			else
			{
				// Динамически вычитает размеры массивов, пока не попадет в нужный бакет.
				int32 RemainderIdx = RandomGlobalIndex;
				
				for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
				{
					EStrategyEntityCategory CurrentCat = static_cast<EStrategyEntityCategory>(i);
					const TArray<AActor*>& CurrentBucket = TargetSector->GetBucket(CurrentCat);
					
					if (RemainderIdx < CurrentBucket.Num())
					{
						RandomEnemy = CurrentBucket[RemainderIdx];
						break; // Нашли актора по адресу — выходим из цикла!
					}
					RemainderIdx -= CurrentBucket.Num();
				}
			}


			// Верификация честной круглой дистанции и линков башни
			if (IsValid(RandomEnemy))
			{
				ATowerBase* EnemyTower = Cast<ATowerBase>(RandomEnemy);
				bool bIsInvalidTower = false;
				if (EnemyTower)
				{
					bIsInvalidTower = EnemyTower->IsTowerLockedNoConnection(MyFactionId);
				}

				if (!bIsInvalidTower)
				{
					if (FVector::DistSquared(MyLocation, RandomEnemy->GetActorLocation()) <= RadSq)
					{
						return RandomEnemy;
					}
				}
			}

			
			// Бежим строго по тем бакетам, которые мы сканировали
			for (const FStrategyGridSector* BackupSector : ValidEnemySectors)
			{
				if (SpecificCategory)
				{
					// Если искали конкретный тип — чекаем только его бакет
					if (AActor* Found = CheckAndReturn(BackupSector->GetBucket(*SpecificCategory), RandomEnemy, MyLocation)) return Found;
				}
				else
				{
					// Если тип не важен — циклом автоматически прочесываем абсолютно все бакеты ячейки!
					for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
					{
						EStrategyEntityCategory CurrentCat = static_cast<EStrategyEntityCategory>(i);
						
						// Вызываем нашу инлайновую лямбду, передавая туда массив текущего бакета
						if (AActor* Found = CheckAndReturn(BackupSector->GetBucket(CurrentCat), RandomEnemy, MyLocation))
						{
							return Found; // Нашли хоть кого-то — мгновенно отдаем ИИ!
						}
					}
				}
			}

			return nullptr; 
		}

		RandomGlobalIndex -= SectorCount;
	}

	return nullptr;
}


TArray<AActor*> UStrategyGridSubsystem::FindAllAlliesInRadius(FVector CenterLocation, float Radius, uint8 FactionID, EStrategyEntityCategory Category)
{
	TArray<AActor*> FoundAllies;
	if (Radius <= 0.0f) return FoundAllies;

	const float RadiusSq = Radius * Radius;
	
	// Мгновенный прыжок в изолированный грид нашей фракции (пролетая мимо врагов)
	const FPlayerGrid* TargetPlayerGrid = GlobalPlayersGrids.Find(static_cast<int32>(FactionID));
	if (!TargetPlayerGrid) return FoundAllies;

	FIntPoint CenterCoords = GetSectorCoords(CenterLocation);
	int32 SectorRadiusRange = FMath::CeilToInt(Radius / SectorSize);

	// =========================================================================
	// ХИРУРГИЧЕСКИЙ АВТО-БУСТ ПАМЯТИ ИТОГОВОГО МАССИВА (PRE-ALLOCATION)
	// Вычисляем, сколько ВСЕГО ячеек сетки мы обойдем. 
	// И в один С++ вызов выделяем ровный стек памяти ОЗУ, чтобы массив больше 
	// НИ РАЗУ не реаллоцировался во время наполнения в циклах!
	// =========================================================================
	int32 TotalSectorsToScan = FMath::Square((SectorRadiusRange * 2) + 1);
	
	// Определяем средний ожидаемый шаг плотности объектов на одну ячейку
	int32 ExpectedDensityPerSector = 0;
	if (Category == EStrategyEntityCategory::Troop)
	{
		ExpectedDensityPerSector = 64; // Пехоты в секторе обычно много (плотная толпа)
	}
	else if (Category == EStrategyEntityCategory::MAX)
	{
		ExpectedDensityPerSector = 128; // Если ищем вообще всё подряд
	}
	else
	{
		ExpectedDensityPerSector = 4;  // Башен, турелей и заводов в одной ячейке 30х30м много не бывает
	}

	// Аллоцируем память одним монолитным куском в Куче (Heap)
	int32 EstimatedTotalSize = TotalSectorsToScan * ExpectedDensityPerSector;
	
	// Ставим разумный Clamp (минимум 16, максимум 2048), чтобы не перерасходовать ОЗУ на гигантских радиусах
	EstimatedTotalSize = FMath::Clamp(EstimatedTotalSize, 16, 2048);
	FoundAllies.Reserve(EstimatedTotalSize);

	// =========================================================================
	
	// Вспомогательная инлайновая лямбда-функция для честной круглой проверки
	auto ScanAndAddFromBucket = [&](const TArray<AActor*>& Bucket)
	{
		for (AActor* Entity : Bucket)
		{
			if (!IsValid(Entity)) continue;

			float CurrentDistSq = FVector::DistSquared(CenterLocation, Entity->GetActorLocation());
			if (CurrentDistSq <= RadiusSq)
			{
				// ХИРУРГИЧЕСКИЙ БУСТ: если уперлись в потолок текущей выделенной емкости
				if (FoundAllies.Num() >= FoundAllies.Max())
				{
					// Расширяем текущую емкость Max() строго на величину EstimatedTotalSize
					FoundAllies.Reserve(FoundAllies.Max() + EstimatedTotalSize);
				}

				FoundAllies.Add(Entity);
			}
		}
	};

	// Двухмерный обход соседних секторов только нашего игрока
	for (int32 x = CenterCoords.X - SectorRadiusRange; x <= CenterCoords.X + SectorRadiusRange; ++x)
	{
		for (int32 y = CenterCoords.Y - SectorRadiusRange; y <= CenterCoords.Y + SectorRadiusRange; ++y)
		{
			FIntPoint CurrentSectorKey(x, y);
			const FStrategyGridSector* TargetSector = TargetPlayerGrid->Sectors.Find(CurrentSectorKey);
			if (!TargetSector) continue;

			// Вариант А: Если запрошена специальная категория MAX — циклом собираем ВСЕ постройки и войска фракции
			if (Category == EStrategyEntityCategory::MAX)
			{
				for (uint8 i = 0; i < static_cast<uint8>(EStrategyEntityCategory::MAX); ++i)
				{
					EStrategyEntityCategory CurrentCat = static_cast<EStrategyEntityCategory>(i);
					ScanAndAddFromBucket(TargetSector->GetBucket(CurrentCat));
				}
			}
			// Вариант Б: Высокоскоростной точечный удар! Заглядываем строго в один конкретный бакет (по дефолту Troops)
			else
			{
				ScanAndAddFromBucket(TargetSector->GetBucket(Category));
			}
		}
	}

	return FoundAllies;
}


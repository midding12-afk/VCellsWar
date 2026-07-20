// Copyright (c) 2026, Dmitry Tur. All rights reserved.
#include "StrategyGridSubsystem.h"
#include "NavigationSystem.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h" 

int32 UStrategyGridSubsystem::GetFactionIdFromActor(AActor* Actor) const
{
	if (Actor && Actor->GetClass()->ImplementsInterface(UStrategyEntityInterface::StaticClass()))
	{
		return IStrategyEntityInterface::Execute_GetEntityFactionID(Actor);
	}
	return 255; // Дефолтное безопасное значение для нейтралов/декораций
}

FIntPoint UStrategyGridSubsystem::GetSectorCoords(FVector Location) const
{
	int32 SectorX = FMath::FloorToInt(Location.X / SectorSize);
	int32 SectorY = FMath::FloorToInt(Location.Y / SectorSize);
	return FIntPoint(SectorX, SectorY);
}

void UStrategyGridSubsystem::RegisterEntity(AActor* Entity)
{
	if (!Entity) return;
	IStrategyEntityInterface* Soldier = Cast<IStrategyEntityInterface>(Entity);
	if (!Soldier) return;
	
	if (ActorToSectorMap.Find(Entity)) return;

	int32 FactionId = GetFactionIdFromActor(Entity);
	FIntPoint SectorCoords = GetSectorCoords(Entity->GetActorLocation());

	FStrategyGridSector& GridSector = GlobalPlayersGrids.FindOrAdd(FactionId).Sectors.FindOrAdd(SectorCoords);

	// ААА-оптимизация: если сектор пустой, резервируем память под плотную толпу в 512 слотов сразу,
	// чтобы ОС больше ни разу не трогала Кучу (Heap) при наплыве армии!
	if (GridSector.FlatActorPointers.Num() == 0)
	{
		GridSector.FlatActorPointers.Reserve(512);
	}

	// ЗАПОМИНАЕМ ИНДЕКС: Юнит добавляется в конец и жестко знает, в какую ячейку массива он сел
	Soldier->SetSelectedSectorGridIndex(GridSector.FlatActorPointers.Add(Entity));

	ActorToSectorMap.Add(Entity, SectorCoords);
}

void UStrategyGridSubsystem::UnregisterEntity(AActor* Entity)
{
	if (!Entity) return;
	IStrategyEntityInterface* Soldier = Cast<IStrategyEntityInterface>(Entity);
	if (!Soldier) return;

	int32 FactionId = GetFactionIdFromActor(Entity);
	FIntPoint* FoundSector = ActorToSectorMap.Find(Entity);
	if (!FoundSector) return;

	if (FPlayerGrid* PlayerGrid = GlobalPlayersGrids.Find(FactionId))
	{
		if (FStrategyGridSector* GridSector = PlayerGrid->Sectors.Find(*FoundSector))
		{
			int32 DeadUnitIdx = Soldier->GetSelectedSectorGridIndex();
			
			// Проверяем легитимность индекса перед выстрелом по памяти
			if (GridSector->FlatActorPointers.IsValidIndex(DeadUnitIdx))
			{
				// Узнаем, какой живой солдат сейчас стоит в самом конце массива (хвосте)
				int32 LastIdx = GridSector->FlatActorPointers.Num() - 1;
				AActor* ShiftingSwappedUnit = GridSector->FlatActorPointers[LastIdx];

				// УДАЛЕНИЕ С ЗАМЕНОЙ ЗА O(1):
				// Удаляем умершего, хвост автоматически прыгает на его место за 0 наносекунд!
				GridSector->FlatActorPointers.RemoveAtSwap(DeadUnitIdx, 1, EAllowShrinking::No);

				// СИНХРОНИЗАЦИЯ ИНДЕКСА ХВОСТА:
				// Если перенос состоялся, перезаписываем выжившему солдату его новый индекс!
				if (DeadUnitIdx < GridSector->FlatActorPointers.Num() && IsValid(ShiftingSwappedUnit))
				{
					if (IStrategyEntityInterface* SwappedChar = Cast<IStrategyEntityInterface>(ShiftingSwappedUnit))
					{
						SwappedChar->SetSelectedSectorGridIndex(DeadUnitIdx);
					}
				}
			}

			// Сбрасываем индекс ушедшего в пул солдата в дефолт
			Soldier->SetSelectedSectorGridIndex(-1);

			if (GridSector->FlatActorPointers.Num() == 0)
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

	// Юнит физически пересёк границу сетки 30х30 метров
	if (*OldSectorPtr != NewSector)
	{
		UnregisterEntity(Entity);
		RegisterEntity(Entity);
	}
}

AActor* UStrategyGridSubsystem::FindRandomEnemyEntityInRadius(AActor* ScanningEntity, float Radius)
{
	if (!IsValid(ScanningEntity)) return nullptr;

	int32 MyFactionId = GetFactionIdFromActor(ScanningEntity);
	FVector MyLocation = ScanningEntity->GetActorLocation();
	FIntPoint CenterSector = GetSectorCoords(MyLocation);
	int32 SectorRange = FMath::Max(1, FMath::CeilToInt(Radius / SectorSize));

	// СВЕРХСКОРОСТНОЙ ИНЛАЙН НА СТЕКЕ ПОТОКА ПРОЦЕССОРА:
	// Буфер на 64 указателя секторов выделится за 0 наносекунд и идеально 
	// вместит в себя абсолютно весь FFA-замес до 8 игроков в 9 ячейках! 
	TArray<const FStrategyGridSector*, TInlineAllocator<64>> ValidEnemySectors;
	int32 TotalEnemyCount = 0;

	// ЭТАП 1: Собираем непустые вражеские FFA-сектора из соседних клеток за O(1)
	for (const TPair<int32, FPlayerGrid>& PlayerGridPair : GlobalPlayersGrids)
	{
		if (PlayerGridPair.Key == MyFactionId || PlayerGridPair.Key == 255) continue;

		const FPlayerGrid& EnemyGrid = PlayerGridPair.Value;

		for (int32 OffsetX = -SectorRange; OffsetX <= SectorRange; ++OffsetX)
		{
			for (int32 OffsetY = -SectorRange; OffsetY <= SectorRange; ++OffsetY)
			{
				FIntPoint TargetSectorCoords(CenterSector.X + OffsetX, CenterSector.Y + OffsetY);

				if (const FStrategyGridSector* GridSector = EnemyGrid.Sectors.Find(TargetSectorCoords))
				{
					int32 DirectCount = GridSector->FlatActorPointers.Num();
					if (DirectCount > 0)
					{
						ValidEnemySectors.Add(GridSector);
						TotalEnemyCount += DirectCount; // Быстро суммируем численность толпы
					}
				}
			}
		}
	}

	if (TotalEnemyCount == 0) return nullptr;

	// ЭТАП 2: ДВУХСТУПЕНЧАТЫЙ ПРЫЖОК К СЛУЧАЙНОЙ ЦЕЛИ ЗА ИСТИННОЕ O(1)
	/*int32 RandomGlobalIndex = FMath::RandRange(0, TotalEnemyCount - 1);

	for (const FStrategyGridSector* TargetSector : ValidEnemySectors)
	{
		int32 SectorCount = TargetSector->FlatActorPointers.Num();

		if (RandomGlobalIndex < SectorCount)
		{
			// МГНОВЕННЫЙ ВЫСТРЕЛ ПО ПРЯМОМУ ИНДЕКСУ:
			// Никаких итераторов и Pointer Chasing. Вытаскиваем случайного врага из тысяч за 1 такт!
			AActor* RandomEnemy = TargetSector->FlatActorPointers[RandomGlobalIndex];

			if (IsValid(RandomEnemy))
			{
				// Верификация честного круглого радиуса бластерного лазера
				float d = FVector::DistSquared(MyLocation, RandomEnemy->GetActorLocation());
					float dd = Radius * Radius;
				if (FVector::DistSquared(MyLocation, RandomEnemy->GetActorLocation()) <= (Radius * Radius))
				{
					return RandomEnemy;
				}
			}
			return RandomEnemy;
		}

		RandomGlobalIndex -= SectorCount;
	}

	return nullptr;*/
	
	int32 RandomGlobalIndex = FMath::RandRange(0, TotalEnemyCount - 1);

	for (const FStrategyGridSector* TargetSector : ValidEnemySectors)
	{
		int32 SectorCount = TargetSector->FlatActorPointers.Num();

		if (RandomGlobalIndex < SectorCount)
		{
			// МГНОВЕННЫЙ ВЫСТРЕЛ ПО ПРЯМОМУ ИНДЕКСУ:
			AActor* RandomEnemy = TargetSector->FlatActorPointers[RandomGlobalIndex];

			if (IsValid(RandomEnemy))
			{
				// Проверяем интерфейс блокировки башни
				ATowerBase* EnemyInterface = Cast<ATowerBase>(RandomEnemy);
				bool bIsInvalidTower = false;
				
				if (EnemyInterface)
				{
					bIsInvalidTower = EnemyInterface->IsTowerLockedNoConnection(MyFactionId);
				}

				// Если башня легитимна — проверяем честный круглый радиус и стреляем!
				if (!bIsInvalidTower)
				{
					if (FVector::DistSquared(MyLocation, RandomEnemy->GetActorLocation()) <= (Radius * Radius))
					{
						return RandomEnemy;
					}
				}
			}

			// =========================================================================
			// Если случайная башня фракции 254 заблокирована (или враг вне радиуса кадра),
			// мы делаем ОДИН быстрый проход по ВСЕМ валидным вражеским пулам в радиусе!
			// Это гарантированно найдет подбегающих солдат фракции 2 в этой же зоне.
			// =========================================================================
			for (const FStrategyGridSector* BackupSector : ValidEnemySectors)
			{
				for (AActor* BackupEnemy : BackupSector->FlatActorPointers)
				{
					if (IsValid(BackupEnemy) && BackupEnemy != RandomEnemy)
					{
						ATowerBase* BackupInterface = Cast<ATowerBase>(BackupEnemy);
						
						// Если этот бэкап-враг — тоже заблокированная башня, то молча скипаем её
						if (BackupInterface && BackupInterface->IsTowerLockedNoConnection(MyFactionId))
						{
							continue;
						}

						// Проверяем дистанцию: если солдат фракции 2 стоит рядом — он становится целью!
						if (FVector::DistSquared(MyLocation, BackupEnemy->GetActorLocation()) <= (Radius * Radius))
						{
							return BackupEnemy; // Нашли честную цель из пула ДРУГОЙ фракции! 
						}
					}
				}
			}

			// Если во всех вражеских пулах этой зоны вообще никого легитимного нет — возвращаем nullptr.
			// Никакого зацикливания, процессор свободен!
			return nullptr; 
		}

		RandomGlobalIndex -= SectorCount;
	}

	return nullptr;
}

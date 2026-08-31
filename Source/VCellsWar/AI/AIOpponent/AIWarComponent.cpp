// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "AIWarComponent.h"
#include "AISquad.h"
#include "VCellsWar/GameMods/MainGamePlayerController.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VCellsWar/Actors/TroopBase.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"
#include "VCellsWar/Systems/LocalVisualLinkSubsystem.h"
#include "VCellsWar/Systems/MatchStatisticsSubsystem.h"

UAIWarComponent::UAIWarComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Отключаем стандартный компонентный тик
}

void UAIWarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Кэшируем контроллер через цепочку: Компонент -> Генерал (AInfo) -> PlayerController
	if (AActor* OwnerActor = GetOwner())
	{
		MyAIController = Cast<AMainGamePlayerController>(OwnerActor->GetOwner());
	}
	
	UMatchStatisticsSubsystem* Stats = GetWorld()->GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	if (Stats)
	{
		MapSize = Stats->MapSize;
	}
}

UAISquad* UAIWarComponent::CreateNewSquad(EAISquadRole InitialRole, FVector TargetLocation)
{
	// Создаем легковесный UObject отряда в контексте этого компонента
	UAISquad* NewSquad = NewObject<UAISquad>(this);
	if (NewSquad)
	{
		NewSquad->InitSquad(MyFactionID, InitialRole, TargetLocation);

		// МГНОВЕННАЯ СВЯЗЬ: Подписываем Генерала на событие паники/кризиса этого отряда
		NewSquad->OnSquadCrisisReport.AddDynamic(this, &UAIWarComponent::HandleSquadCrisis);

		ActiveSquads.Add(NewSquad);
	}
	return NewSquad;
}


int32 UAIWarComponent::GetEnemyDangerousByHeatMap(TArray<int32>& HeatMap, int32 Index, int32 Radius, int32 MapLength) const
{
	// Жесткая защита от некорректных входных данных или пустого массива
	if (HeatMap.Num() == 0 || Radius < 0 || MapLength <= 0 || !HeatMap.IsValidIndex(Index))
	{
		return 0;
	}

	int32 TotalDanger = 0;

	// 1. Восстанавливаем 2D-координаты центральной ячейки из одномерного индекса.
	// Наша формула индексации: Index = (A * MapLength) + B;
	const int32 CenterA = Index / MapLength;
	const int32 CenterB = Index % MapLength;

	// 2. Сканируем квадратную область вокруг центра в пределах заданного радиуса
	for (int32 OffsetA = -Radius; OffsetA <= Radius; ++OffsetA)
	{
		for (int32 OffsetB = -Radius; OffsetB <= Radius; ++OffsetB)
		{
			const int32 CurrentA = CenterA + OffsetA;
			const int32 CurrentB = CenterB + OffsetB;

			// Проверяем, что мы не вылезли за географические границы сетки карты
			if (CurrentA >= 0 && CurrentA < MapLength && CurrentB >= 0 && CurrentB < MapLength)
			{
				// Считаем Манхэттенское расстояние (количество шагов по сетке)
				const int32 Distance = FMath::Max(FMath::Abs(OffsetA), FMath::Abs(OffsetB));//пусть диагональ тоже растояние 1

				// Если ячейка выходит за рамки искомого ромбовидного радиуса — пропускаем её
				//if (Distance > Radius) continue;

				// Вычисляем плоский индекс текущей ячейки соседей в памяти
				const int32 NeighborIndex = (CurrentA * MapLength) + CurrentB;

				if (HeatMap.IsValidIndex(NeighborIndex))
				{
					const int32 RawValue = HeatMap[NeighborIndex];

					// Нас интересуют ТОЛЬКО враги (отрицательные значения)
					if (RawValue < 0)
					{
						// Переводим отрицательное значение в положительный вес угрозы
						const int32 EnemyWeight = FMath::Abs(RawValue);

						// Применяем коэффициент затухания 0.5^N.
						// Коэффициенты: 
						// Distance = 0 или 1 -> x1 (смещение на 0 и 1 ячейку не уменьшает значимость)
						// Distance = 2      -> x0.5  (битовый сдвиг >> 1)
						// Distance = 3      -> x0.25 (битовый сдвиг >> 2) и т.д.
						if (Distance <= 1)
						{
							TotalDanger += EnemyWeight;
						}
						else
						{
							// Степень затухания для битового сдвига: при расстоянии 2 сдвиг должен быть на 1
							const int32 ShiftCount = Distance - 1;
							
							// Используем сверхбыстрый битовый сдвиг вправо вместо тяжелого FMath::Pow
							TotalDanger += (EnemyWeight >> ShiftCount);
						}
					}
				}
			}
		}
	}

	return TotalDanger;
}


void UAIWarComponent::TickWarLogics()
{
	AMainGamePlayerState* MyAIState = nullptr;
	if (AActor* DirectorActor = GetOwner())
	{
		if (AMainGamePlayerController* MyPC = Cast<AMainGamePlayerController>(DirectorActor->GetOwner()))
		{
			MyAIState = Cast<AMainGamePlayerState>(MyPC->PlayerState);
		}
	}
	if (!MyAIController)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			MyAIController = Cast<AMainGamePlayerController>(OwnerActor->GetOwner());
		}
	}
	if (!MyAIController || !MyAIState) return;

	// Быстрая зачистка расформированных пустых отрядов 
	for (int32 i = ActiveSquads.Num() - 1; i >= 0; --i)
	{
		if (!ActiveSquads[i] || !IsValid(ActiveSquads[i]))
		{
			ActiveSquads.RemoveAtSwap(i);
		}
		else if(ActiveSquads[i]->GetSquadSize() == 0)
		{
			ActiveSquads[i]->ClearSquad();
			ActiveSquads.RemoveAtSwap(i);
		}
	}
	
	// for (int32 i = ActiveSquads.Num() - 1; i >= 0; --i)
	// {
	// 	if (ActiveSquads[i]->GetSquadRole()==EAISquadRole::TowerAssault)
	// 		GEngine->AddOnScreenDebugMessage(10000+i, 2.f, FColor::White, *FString::Printf(TEXT("Squad %s assault tover in loc %s"), *ActiveSquads[i]->GetName(), *ActiveSquads[i]->GetTargetLocation().ToString()));
	// }
	
	AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
	if (!GM) return;
	
	// Получаем все башни из вашей субсистемы Делоне
	TArray<ATowerBase*> AllTowers = GM->GetTowers();

	TArray<ATowerBase*> AvailableTargets;
	
	ULocalVisualLinkSubsystem* VLSubsystem = GetWorld()->GetSubsystem<ULocalVisualLinkSubsystem>();
	if (!VLSubsystem) return;

	// 1. Собираем ВСЕ доступные для атаки башни
	for (ATowerBase* Tower : AllTowers)
	{
		if (Tower && IsValid(Tower) && Tower->GetGenericTeamId() != MyFactionID)
		{
			if (VLSubsystem->IsTowerInPlayerNetlink(Tower, MyAIState)) 
			{
				AvailableTargets.Add(Tower);
			}
		}
	}

	// Если атаковать нечего (зажали игрока на базе или башни кончились) — ИИ просто копит силы
	if (AvailableTargets.Num() == 0) return;

	UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>();
	
	if (!GridSubsystem) return;
	
	TArray<int32> HeatMap = GridSubsystem->GetHeatMapForPlayer(MyFactionID);
	
	//TODO добавить размытие хостайла по секторам
	
	ATowerBase* TargetTower = nullptr;
	int32 TargetTowerHostile = 0;
	
	int32 MapLength = FMath::DivideAndRoundUp((float)MapSize, GridSubsystem->GetSectorSize());
	
	for (ATowerBase* Tower : AvailableTargets)
	{
		FIntPoint point = GridSubsystem->GetSectorCoords(Tower->GetActorLocation());
		int32 index = point.X * MapLength + point.Y;
		
		int32 dangerous = GetEnemyDangerousByHeatMap(HeatMap, index, 1, MapLength);
		
		if (!IsEnemyTowerUnderMyAttack(Tower) && (!TargetTower || TargetTowerHostile > dangerous))
		{
			TargetTowerHostile = dangerous;
			TargetTower = Tower;
		}
	}
	
	if (TargetTower)
	{
		int32 RequiredSquadSize = (TargetTowerHostile * 2) + 10;

		ExecuteSmartAttack(TargetTower, RequiredSquadSize);
	}
	
	/*
	// 2. Сортируем цели: ИИ в приоритете должен выбирать БЛИЖАЙШИЕ к его порталам башни,
	// чтобы минимизировать время марша солдат.
	// (Для сортировки можно использовать координаты центра масс ваших порталов)
	FVector AICenterOfMass = GetAICenterOfMass(); 
	AvailableTargets.Sort([AICenterOfMass](const ATowerBase& A, const ATowerBase& B) {
		return FVector::DistSquared(A.GetActorLocation(), AICenterOfMass) < FVector::DistSquared(B.GetActorLocation(), AICenterOfMass);
	});

	// Нам достаточно выбрать одну наилучшую цель на текущем такте
	ATowerBase* BestTargetTower = AvailableTargets[0];

	// 3. МАТЕМАТИКА ВАШЕЙ ФОРМУЛЫ: Считаем, сколько врагов стоит в окрестностях этой башни
	// Используем вашу быструю грид-систему ячеек
	// int32 EnemyCountInSector = GridSubsystem->CountEnemiesInTowerSector(BestTargetTower);
	int32 EnemyCountInSector = 0; // Заглушка. Если башня пустая/нейтральная, тут будет 0

	// Динамически вычисляем размер нужной армии
	int32 RequiredSquadSize = (EnemyCountInSector * 2) + 10;

	// 4. ЗАПУСКАЕМ ЕДИНЫЙ АЛГОРИТМ МОБИЛИЗАЦИИ
	// Метод пойдет «пылесосить» ближайшие порталы, пока не наберет ровно RequiredSquadSize солдат!
	ExecuteSmartAttack(BestTargetTower, RequiredSquadSize);*/
}

void UAIWarComponent::ExecuteSmartAttack(ATowerBase* TargetTower, int32 TargetSquadSize)
{
	// Защита от некорректных данных
	if (!TargetTower || !MyAIController || TargetSquadSize <= 0) return;
	
	FVector AttackLocation = TargetTower->GetActorLocation();

	// Вычисляем порог в 75% для принятия решения 
	const int32 MinimumRequiredSize = TargetSquadSize*0.75f;

	// Массив для портальных отрядов, у которых есть излишки
	TArray<UAISquad*> TargetPortalSquads;
	
	// Переменная для БЫСТРОГО предварительного подсчета доступных сил по всей карте
	int32 TotalAvailableTroopsOnMap = 0;

	// Шаг 1: Просто сканируем размеры отрядов (без изменения памяти и паспортов солдат!)
	for (UAISquad* Squad : ActiveSquads)
	{
		if (Squad && Squad->GetSquadRole() == EAISquadRole::PortalSquad)
		{
			int32 FreeTroops = Squad->GetSquadSize();// - SafeRearGuardLimit;
			if (FreeTroops > 0)
			{
				TargetPortalSquads.Add(Squad);
				TotalAvailableTroopsOnMap += FreeTroops;
			}
		}
	}

	if (TotalAvailableTroopsOnMap < MinimumRequiredSize)
	{
		return;
	}

	// Шаг 2: Раз сил гарантированно хватает, теперь можно и отсортировать порталы по близости к цели
	TargetPortalSquads.Sort([AttackLocation](const UAISquad& A, const UAISquad& B) {
		return FVector::DistSquared(A.GetTargetLocation(), AttackLocation) < FVector::DistSquared(B.GetTargetLocation(), AttackLocation);
	});

	// Шаг 3: Создаем штурмовой отряд
	UAISquad* AttackSquad = CreateNewSquad(EAISquadRole::TowerAssault, AttackLocation);
	if (!AttackSquad) return;

	TArray<ATroopBase*> MobilizedBuffer;
	MobilizedBuffer.Reserve(TargetSquadSize);

	// Шаг 4: Начинаем чистый физический сбор солдат от ближних порталов к дальним
	for (UAISquad* PortalSquad : TargetPortalSquads)
	{
		// Как только набрали идеальные 100% или исчерпали все доступные войска (но гарантированно > 75%) — выходим
		if (MobilizedBuffer.Num() >= TargetSquadSize) break;

		int32 NeededCount = TargetSquadSize - MobilizedBuffer.Num();
		int32 AvailableCount = PortalSquad->GetSquadSize();// - SafeRearGuardLimit;
		int32 ExtractCount = FMath::Min(NeededCount, AvailableCount);

		if (ExtractCount > 0)
		{
			SharedTransferBuffer.Reset();
			PortalSquad->ExtractMembersBatch(ExtractCount, SharedTransferBuffer);
			MobilizedBuffer.Append(SharedTransferBuffer);

			// Если в процессе сбора от ближних порталов мы ОУЖЕ превысили 75%, 
			// и в этом конкретном портале солдаты кончились — мы можем проверить, стоит ли собирать дальше.
			// Но так как мы уже застрахованы верхним предохранителем, мы спокойно собираем до 100%, 
			// пока не закончатся порталы или лимит TargetSquadSize.
		}
	}

	// Шаг 5: Передаем собранную армию в штурмовой отряд и отправляем в бой
	if (MobilizedBuffer.Num() > 0)
	{
		AttackSquad->AddMembersBatch(MobilizedBuffer);

		TArray<AActor*> FinalMovementGroup;
		// 1. Выделяем память ОДИН РАЗ (очень важно для скорости)
		FinalMovementGroup.Reserve(AttackSquad->SquadMembers.Num());
		
		AttackSquad->SetTargetTower(TargetTower);

		// 2. Метод Append под капотом использует векторные инструкции процессора (SIMD)
		// Он мгновенно вливает кусок памяти указателей, приводя их к типу AActor* 
		FinalMovementGroup.Append(AttackSquad->SquadMembers);

		// 3. Передаем готовый массив в ваш метод контроллера
		MyAIController->Server_MoveSelectedUnits_Implementation(AttackLocation, FinalMovementGroup);
		
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, *FString::Printf(TEXT("RTS AI: Умная мобилизация завершена. Отправлено %d солдат на штурм башни."), FinalMovementGroup.Num()));
	}
}


void UAIWarComponent::HandleSquadCrisis(UAISquad* ReportingSquad)
{
	if (!ReportingSquad) return;

	// Мгновенная реакция на сигнал S.O.S.! 
	// Если отряд обороны башни тает на глазах, ищем ближайший тыловой отряд резерва
	if (ReportingSquad->GetSquadRole() == EAISquadRole::TowerDefense)
	{
		UAISquad* BestDonorSquad = nullptr;
		
		for (UAISquad* PotentialDonor : ActiveSquads)
		{
			if (!PotentialDonor || PotentialDonor == ReportingSquad) continue;

			// Ищем отряд, который просто стоит без дела в буфере накопления и имеет избыток сил
			if (PotentialDonor->GetSquadRole() == EAISquadRole::IdleBuffer && PotentialDonor->GetSquadSize() > 10)
			{
				BestDonorSquad = PotentialDonor;
				break;
			}
		}

		// Если донор найден — экстренно переливаем половину его сил в оборону
		if (BestDonorSquad)
		{
			int32 TroopsToTransfer = BestDonorSquad->GetSquadSize() / 2;
			TransferTroopsBetweenSquads(BestDonorSquad, ReportingSquad, TroopsToTransfer);
		}
	}
}

void UAIWarComponent::TransferTroopsBetweenSquads(UAISquad* FromSquad, UAISquad* ToSquad, int32 Count)
{
	if (!FromSquad || !ToSquad || Count <= 0) return;

	// 1. Сбрасываем общий буфер-переносчик (память процессора сохраняется, ноль аллокаций!)
	SharedTransferBuffer.Reset();

	// 2. Отщипываем солдат с хвоста массива FromSquad прямо в буфер по ссылке
	FromSquad->ExtractMembersBatch(Count, SharedTransferBuffer);

	// 3. Заливаем солдат из буфера в массив ToSquad
	ToSquad->AddMembersBatch(SharedTransferBuffer);

	// 4. Командуем переведенной пачке солдат бежать на новые координаты ToSquad
	TArray<AActor*> MovementGroup;
	MovementGroup.Reserve(SharedTransferBuffer.Num());
	for (ATroopBase* Troop : SharedTransferBuffer)
	{
		if (IsValid(Troop)) MovementGroup.Add(Troop);
	}

	if (MovementGroup.Num() > 0 && MyAIController)
	{
		MyAIController->Server_MoveSelectedUnits_Implementation(ToSquad->GetTargetLocation(), MovementGroup);
	}
}

void UAIWarComponent::RegisterNewSpawnedTroops(const TArray<ATroopBase*>& FreshTroops)
{
	if (FreshTroops.Num() == 0) return;

	// Ищем уже существующий отряд накопления (IdleBuffer)
	UAISquad* Accumulator = nullptr;
	for (UAISquad* Squad : ActiveSquads)
	{
		if (Squad && Squad->GetSquadRole() == EAISquadRole::IdleBuffer)
		{
			Accumulator = Squad;
			break;
		}
	}

	// Если игра только началась или прошлый буфер ушел в атаку — создаем новый Idle-отряд
	if (!Accumulator)
	{
		// Спавним отряд в координатах вашего главного портала
		FVector PortalLocation = FVector::ZeroVector; 
		Accumulator = CreateNewSquad(EAISquadRole::IdleBuffer, PortalLocation);
	}

	// Массово зачисляем новичков в отряд за один быстрый проход
	if (Accumulator)
	{
		Accumulator->AddMembersBatch(FreshTroops);
	}
}

bool UAIWarComponent::IsTowerOnFrontline(ATowerBase* Tower)
{
	// Быстрая проверка соседей по Делоне...
	return false;
}

bool UAIWarComponent::IsEnemyTowerUnderMyAttack(ATowerBase* Tower)
{
	for (UAISquad* Squad : ActiveSquads)
	{
		if (IsValid(Squad))
			if (Squad->GetSquadRole()==EAISquadRole::TowerAssault && (Squad->TargetTowerRef.Get() == Tower || Squad->GetTargetLocation().Equals(Tower->GetActorLocation(), 200.0f)))
			{
				return true;
			}
	}
	
	return false;
}

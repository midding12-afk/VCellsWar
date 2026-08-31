// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "AISquad.h"
#include "VCellsWar/Actors/TroopBase.h"
#include "VCellsWar/Actors/TowerBase.h"

UAISquad::UAISquad()
{
	
}

void UAISquad::InitSquad(int32 InFactionID, EAISquadRole InInitialRole, FVector InTargetLocation)
{
	FactionID = InFactionID;
	CurrentRole = InInitialRole;
	CurrentTargetLocation = InTargetLocation;
	
	SquadMembers.Reset();
	InitialSquadSize = 0;
}

void UAISquad::AddMember(ATroopBase* NewTroop)
{
	if (!IsValid(NewTroop)) return;

	// Записываем солдата в наш плотный массив
	int32 NewIndex = SquadMembers.Add(NewTroop);
	
	// Заполняем "паспорт" внутри самого солдата
	NewTroop->MyAISquad = this;
	NewTroop->SquadLocalIndex = NewIndex;

	// Запоминаем стартовый размер для калькуляции будущих потерь
	InitialSquadSize = FMath::Max(InitialSquadSize, SquadMembers.Num());
}

void UAISquad::Server_NotifyMemberDeath(int32 DeadIndex)
{
	// Жесткая защита от выхода за границы памяти
	if (!SquadMembers.IsValidIndex(DeadIndex)) return;

	// Сбрасываем "паспорт" у умирающего солдата (хороший тон для пулов объектов)
	if (SquadMembers[DeadIndex])
	{
		SquadMembers[DeadIndex]->MyAISquad = nullptr;
		SquadMembers[DeadIndex]->SquadLocalIndex = -1;
	}

	// Алгоритм Swap-and-Pop за абсолютные O(1)
	if (DeadIndex < SquadMembers.Num() - 1)
	{
		// Берем живого бойца с самого хвоста массива
		ATroopBase* MovedTroop = SquadMembers.Last();
		
		// Переставляем его на место погибшего
		SquadMembers[DeadIndex] = MovedTroop;
		
		// Мгновенно обновляем индекс в паспорте перенесенного выжившего солдата
		MovedTroop->SquadLocalIndex = DeadIndex;
	}

	// Удаляем последний элемент, уменьшая физический размер массива без сдвига памяти
	SquadMembers.RemoveAtSwap(SquadMembers.Num() - 1);

	// --- ПОЛУГЛУПАЯ ЛОГИКА РАПОРТОВ ПО СОБЫТИЯМ ---
	// Если отряд понес тяжелые потери и пересек порог паники
	if (InitialSquadSize > 5 && SquadMembers.Num() < FMath::CeilToInt(InitialSquadSize * PanicThresholdPercent))
	{
		// Мы не принимаем решение сами. Мы просто "кричим" Генералу через делегат.
		// Генерал поймает это событие мгновенно, прервав свой 2-секундный сон, если нужно!
		if (OnSquadCrisisReport.IsBound())
		{
			OnSquadCrisisReport.Broadcast(this);
		}
	}
}

void UAISquad::ClearSquad()
{
	UnsubscribeFromCurrentTower();
	
	// Перед очисткой отряда сбрасываем паспорта у всех выживших
	for (ATroopBase* Troop : SquadMembers)
	{
		if (Troop && IsValid(Troop))
		{
			Troop->MyAISquad = nullptr;
			Troop->SquadLocalIndex = -1;
		}
	}
	
	SquadMembers.Reset();
	InitialSquadSize = 0;
	OnSquadCrisisReport.Clear(); // Отвязываем все подписки
	MarkAsGarbage();
	
}

void UAISquad::AddMembersBatch(const TArray<ATroopBase*>& NewTroops)
{
	if (NewTroops.Num() == 0) return;

	// 1. Заранее резервируем память в TArray, чтобы избежать постоянных переаллокаций
	SquadMembers.Reserve(SquadMembers.Num() + NewTroops.Num());

	// 2. Интегрируем бойцов в отряд
	for (ATroopBase* Troop : NewTroops)
	{
		if (Troop && IsValid(Troop))
		{
			// Добавляем в конец и сразу прописываем "паспорт"
			int32 NewIndex = SquadMembers.Add(Troop);
			Troop->MyAISquad = this;
			Troop->SquadLocalIndex = NewIndex;
		}
	}

	// 3. Корректируем начальный размер отряда, так как к нам пришло пополнение.
	// Это важно, чтобы полуглупая логика паники рассчитывала процент потерь от новой численности.
	InitialSquadSize = SquadMembers.Num();
}

void UAISquad::ExtractMembersBatch(int32 CountToExtract, TArray<ATroopBase*>& OutExtractedTroops)
{
	if (CountToExtract <= 0 || SquadMembers.Num() == 0) return;

	// Ограничиваем запрос фактическим количеством солдат в отряде
	int32 ActualExtractCount = FMath::Min(CountToExtract, SquadMembers.Num());
	
	// Подготавливаем выходной массив
	OutExtractedTroops.Reserve(ActualExtractCount);

	// Забираем солдат строго с хвоста массива.
	for (int32 i = 0; i < ActualExtractCount; ++i)
	{
		ATroopBase* TroopToExtract = SquadMembers.Last();
		
		if (TroopToExtract && IsValid(TroopToExtract))
		{
			// Стираем старый "паспорт" — боец больше не принадлежит этому отряду
			TroopToExtract->MyAISquad = nullptr;
			TroopToExtract->SquadLocalIndex = -1;

			OutExtractedTroops.Add(TroopToExtract);
		}

		// Удаляем последний элемент из нашего отряда за O(1)
		SquadMembers.RemoveAtSwap(SquadMembers.Num() - 1);
	}

	// Корректируем базовый размер отряда после планового уменьшения сил Генералом,
	// чтобы отряд не впал в панику из-за того, что Генерал сам забрал у него людей.
	InitialSquadSize = SquadMembers.Num();
}

void UAISquad::UnsubscribeFromCurrentTower()
{
	// Если прошлая башня еще жива в памяти, аккуратно убираем нашу подписку
	if (TargetTowerRef.IsValid())
	{
		TargetTowerRef->OnTowerFactionChanged.RemoveDynamic(this, &UAISquad::OnTargetTowerFactionChanged);
	}
	TargetTowerRef = nullptr;
}

void UAISquad::SetTargetTower(ATowerBase* NewTower)
{
	// 1. Если мы уже были подписаны на какую-то другую башню — отписываемся
	UnsubscribeFromCurrentTower();

	if (!NewTower || !IsValid(NewTower)) return;

	TargetTowerRef = NewTower;
	SetTargetLocation(NewTower->GetActorLocation());

	// 2. САМОПОДПИСКА: Отряд сам вешает свой колбэк на событие башни!
	NewTower->OnTowerFactionChanged.AddDynamic(this, &UAISquad::OnTargetTowerFactionChanged);
}

void UAISquad::OnTargetTowerFactionChanged(ATowerBase* Tower, int32 NewFactionID)
{
	// Жесткая защита: проверяем, что ивент прилетел именно от той башни, которую мы штурмуем
	if (!Tower || Tower != TargetTowerRef.Get()) return;

	// Сценарий А: Мы (ИИ) успешно захватили эту башню!
	if (NewFactionID == FactionID)
	{
		// Отряд сам меняет свою роль на Оборону и фиксирует позицию.
		// Генералу не улетело ни одного лишнего тика или запроса!
		SetSquadRole(EAISquadRole::TowerDefense);
		
	}
	// Сценарий Б: Нашу башню, которую мы защищали, отжал игрок
	else if (CurrentRole == EAISquadRole::TowerDefense)
	{
		// Башня потеряна. Сбрасываем роль в Idle. 
		// На следующем такте Генерал увидит этот свободный отряд и решит, куда его направить
		SetSquadRole(EAISquadRole::IdleBuffer);
		UnsubscribeFromCurrentTower(); // От этой башни отписываемся, она больше не наша
		
	}
}

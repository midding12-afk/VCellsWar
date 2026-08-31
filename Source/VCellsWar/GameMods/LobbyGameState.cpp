// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LobbyGameState.h"
#include "Net/UnrealNetwork.h"

ALobbyGameState::ALobbyGameState()
{
	NodeCount = 3; // Стартовое дефолтное количество нод на на игрока
	bReplicates = true;
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Принудительно синхронизируем всегда, даже если у владельца число совпало
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyGameState, NodeCount, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyGameState, MapSeed, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyGameState, MapSize, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyGameState, AIPortalsCount, COND_None, REPNOTIFY_Always);
}

void ALobbyGameState::SetNodeCount(int32 NewCount)
{
	if (HasAuthority())
	{
		NodeCount = FMath::Clamp(NewCount, 3, 10); // Ограничиваем рамками баланса стратегии
		
		// Вызываем OnRep вручную на сервере, чтобы хост тоже увидел обновление своего превью
		OnRep_NodeCount();
	}
}

void ALobbyGameState::SetMapSeed(int32 NewMapSeed)
{
	if (HasAuthority())
	{
		MapSeed = NewMapSeed;
		OnRep_MapSeed();
	}
}

int32 ALobbyGameState::GetMapSeed() const
{
	return MapSeed;
}

void ALobbyGameState::SetMapSize(int32 NewSize)
{
	if (HasAuthority())
	{
		MapSize = NewSize;
		OnRep_MapSize();
	}
}

void ALobbyGameState::SetAIPortalsCount(int32 NewCount)
{
	if (HasAuthority())
	{
		AIPortalsCount = NewCount;
	}
}

void ALobbyGameState::OnRep_NodeCount()
{
	// Этот код выполнится на компьютерах ВСЕХ игроков, когда число нод изменится
	OnNodeCountChangedBP.Broadcast(NodeCount);
}

void ALobbyGameState::OnRep_MapSeed()
{
	OnMapSeedChangedBP.Broadcast(MapSeed);
}

void ALobbyGameState::OnRep_MapSize()
{
	OnMapSizeChangedBP.Broadcast(MapSize);
}

void ALobbyGameState::OnRep_AIPortalsCount()
{
	OnAIPortalsCountChangedBP.Broadcast(AIPortalsCount);
}

void ALobbyGameState::UpdateNodePositions(const TArray<FVector2D>& NewPositions)
{
	if (HasAuthority())
	{
		NodesPositions = NewPositions;
	}
}

TArray<FVector2D> ALobbyGameState::GetNodePositions()
{
	return NodesPositions;
}


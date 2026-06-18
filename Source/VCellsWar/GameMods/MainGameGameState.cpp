// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameState.h"

#include "Net/UnrealNetwork.h"

void AMainGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, MapSeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, MapSize, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, AllPlayerCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, AllNodesCountOnInit, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, CachedDeloneEdgesTowerID, COND_None, REPNOTIFY_Always);
}

void AMainGameGameState::OnRep_MapSeed()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_MapSize()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_AllPlayerCount()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_AllNodesCountOnInit()
{
	ReplicatedAllNodesCountOnInit.Broadcast(AllNodesCountOnInit);
}

void AMainGameGameState::OnRep_CachedDeloneEdgesTowerID()
{
	OnCachedDeloneEdgesTowerIDChangedBP.Broadcast(CachedDeloneEdgesTowerID);
}

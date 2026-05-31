// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameState.h"

#include "Net/UnrealNetwork.h"

void AMainGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, MapSeed, COND_None, REPNOTIFY_Always);
}

void AMainGameGameState::OnRep_MapSeed()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed);
}


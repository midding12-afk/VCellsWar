// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TowerBase.h"

#include "Net/UnrealNetwork.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"
#include "VCellsWar/GameMods/MainGameGameState.h"

ATowerBase::ATowerBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATowerBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
		
		if (GM)
		{
			GM->RegisterTower(this);
		}
	}
	else
	{
		AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->InvocLinksUpdate();
		}		
	}
}

void ATowerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATowerBase, TowerId);
}

void ATowerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// void ATowerBase::Server_SetTowerId(int32 NewTowerId)
// {
// 	if (HasAuthority())
// 	{
// 		TowerId = NewTowerId;
// 	}
// }

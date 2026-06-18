// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TowerBase.h"

#include "Net/UnrealNetwork.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"

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

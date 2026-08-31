// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TroopBase.h"

#include "VCellsWar/AI/AIOpponent/AISquad.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"

void ATroopBase::GeinDamage(float Damage, int32 InstigatorTeamID)
{
	if (!HasAuthority()) return;
	
	if (FMath::FRand()<0.5f) return; //TODO %armor from PC GAS
	
	Super::GeinDamage(Damage, InstigatorTeamID);
	
	NativeRTSDeinitialize();
	
	UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	if (ServerPool)
	{
		ServerPool->ReturnActorToNetworkPool(this);
	}
	
	if (MyAISquad)
	{
		MyAISquad->Server_NotifyMemberDeath(SquadLocalIndex);
	}
}

void ATroopBase::SetNewRtsTargetFlag(class ATacticalFlagBase* NewFlag)
{
	if (GetAssignmentState() == ETroopAssignmentState::MarchingToFlag && CurrentTargetFlag)
	{
		CurrentTargetFlag->DegreaseIncomingTroopsCount();
	}
	
	CurrentTargetFlag = NewFlag;
	if (CurrentTargetFlag)
	{
		AssignmentState = ETroopAssignmentState::MarchingToFlag;
		
		CurrentTargetFlag->RegisterIncomingTroopForMovement(this);
		
		LastCnownFlagLocation=CurrentTargetFlag->GetActorLocation();
	}
	else
	{
		AssignmentState = ETroopAssignmentState::Idle;
		ServerLocalIndex = -1;
	}
}

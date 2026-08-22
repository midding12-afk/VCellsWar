// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TroopBase.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"

void ATroopBase::GeinDamage(float Damage, int32 InstigatorTeamID)
{
	if (!HasAuthority()) return;
	Super::GeinDamage(Damage, InstigatorTeamID);
	
	NativeRTSDeinitialize();
	
	UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	if (ServerPool)
	{
		ServerPool->ReturnActorToNetworkPool(this);
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

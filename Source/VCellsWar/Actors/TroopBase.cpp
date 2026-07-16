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

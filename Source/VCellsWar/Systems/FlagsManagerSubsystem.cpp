// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "FlagsManagerSubsystem.h"

#include "VCellsWar/GameMods/MainGamePlayerController.h"

void UFlagsManagerSubsystem::RegistryFlagAsLocalTemp(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	int32 flagID = Flag->FlagID;
	if (!LocalTempFlagList.Contains(flagID))
	{
		LocalTempFlagList.Add(flagID, Flag);
	}
}

void UFlagsManagerSubsystem::RegistryFlagAsLocalPermanent(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* LocalPC = World->GetFirstPlayerController())
		{
			if (AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(LocalPC))
			{
				PC->ReplaceTempLastActiveChainNode(Flag);
			}
		}
	}
	
	int32 flagID = Flag->FlagID;
	if ( TObjectPtr<ATacticalFlagBase>* tempFlag = LocalTempFlagList.Find(flagID))
	{
		LocalTempFlagList.Remove(flagID);
		tempFlag->Get()->Destroy();
	}
}

void UFlagsManagerSubsystem::RegistryFlagOnServer(int32 PlayerId, ATacticalFlagBase* Flag)
{
	if (!Flag) return;

	int32 flagID = Flag->FlagID;

	FServerFlags& PlayerFlagsStruct = ServerFlagMapByPlayer.FindOrAdd(PlayerId);

	PlayerFlagsStruct.ServerFlagList.Add(flagID, Flag);
}

ATacticalFlagBase* UFlagsManagerSubsystem::GetFlag(int32 PlayerId, int32 FlagId)
{
	if (ServerFlagMapByPlayer.Contains(PlayerId))
	{
		FServerFlags* listF =  ServerFlagMapByPlayer.Find(PlayerId);
		if (ATacticalFlagBase* Fl = *listF->ServerFlagList.Find(FlagId))
		{
			return Fl;
		}
	}
	return nullptr;
}

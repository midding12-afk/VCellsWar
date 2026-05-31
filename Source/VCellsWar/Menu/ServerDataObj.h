// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FindSessionsCallbackProxy.h"

#include "ServerDataObj.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class VCELLSWAR_API UServerDataObj : public UObject
{
	GENERATED_BODY()
	
public:

	FBlueprintSessionResult SessionResult; 
    

	UPROPERTY(BlueprintReadOnly) FString ServerName = "0:Name";
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers;
	UPROPERTY(BlueprintReadOnly) int32 Ping;
	UPROPERTY(BlueprintReadOnly) FString HostSteamID;
};






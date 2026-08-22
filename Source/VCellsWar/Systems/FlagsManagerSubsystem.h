// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VCellsWar/TacticalFlag/TacticalFlagBase.h"
#include "FlagsManagerSubsystem.generated.h"


/**
 * 
 */

UENUM(BlueprintType)
enum class ETroopAssignmentState : uint8
{
	Idle                 UMETA(DisplayName = "Idle"),
	MarchingToFlag       UMETA(DisplayName = "Marching To Flag"),
	DefendingFlag        UMETA(DisplayName = "Defending Flag")
};

USTRUCT(BlueprintType)
struct FServerFlags
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<int32, TObjectPtr<ATacticalFlagBase>> ServerFlagList;
};

UCLASS()
class VCELLSWAR_API UFlagsManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TMap<int32, TObjectPtr<ATacticalFlagBase>> LocalTempFlagList;
	
	UFUNCTION()
	void RegistryFlagAsLocalTemp(ATacticalFlagBase* Flag);
	
	UFUNCTION()
	void RegistryFlagAsLocalPermanent(ATacticalFlagBase* Flag);
	
	UPROPERTY()
	TMap<int32, FServerFlags> ServerFlagMapByPlayer;
	
	UFUNCTION()
	void RegistryFlagOnServer(int32 PlayerId, ATacticalFlagBase* Flag);
	
	ATacticalFlagBase* GetFlag(int32 PlayerId, int32 FlagId);
};

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchStatisticsSubsystem.generated.h"

/**
 * 
 */
/*USTRUCT(BlueprintType)
struct FPlayerMatchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString PlayerName;
	UPROPERTY(BlueprintReadOnly) int32 Kills = 0;
	UPROPERTY(BlueprintReadOnly) int32 BuildingsDestroyed = 0;
	UPROPERTY(BlueprintReadOnly) int32 GoldMined = 0;
};*/

UCLASS()
class VCELLSWAR_API UMatchStatisticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Match Setup")
	int32 NodesPerPlayer = 3;
	
	UPROPERTY(BlueprintReadWrite, Category = "Match Setup")
	int32 AllPlayerCount = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Match Setup")
	int32 MapSeed = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Match Setup")
	int32 MapSize = 25000;
	
	UPROPERTY(BlueprintReadWrite, Category = "Match Setup")
	int32 AIPortalsCount = 1;
	
	/*UPROPERTY(BlueprintReadWrite, Category = "Match Statistics")
	TArray<FPlayerMatchStats> EndGameScores;*/
	
	UFUNCTION(BlueprintCallable, Category = "Match Statistics")
	void ResetStatistics() { /*EndGameScores.Empty();*/ }
	
	TArray<FVector2D> NodesPositions;
};

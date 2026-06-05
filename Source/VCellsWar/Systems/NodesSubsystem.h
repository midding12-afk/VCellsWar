// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NodesSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API UNodesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	TArray<FVector2D> GenNodesList(int32 Seed, int32 MapSize, int32 NodesCount, int32 PlayersCount);
};

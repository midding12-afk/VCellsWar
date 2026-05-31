// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MainGameGameState.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API AMainGameGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapSeedReplicated, int32, ReplicatedSeed);

	UPROPERTY(BlueprintAssignable, Category = "Strategy | Map")
	FOnMapSeedReplicated OnMapSeedReplicatedBP;
	
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSeed, BlueprintReadOnly, Category = "Map")
	int32 MapSeed =-1;

	UFUNCTION()
	void OnRep_MapSeed();
	
};

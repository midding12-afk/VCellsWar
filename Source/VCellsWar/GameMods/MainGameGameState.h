// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VCellsWar/Systems/VoronoiSubsystem.h" 
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
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMapSeedReplicated, int32, ReplicatedSeed, int32, ReplicatedSize, int32, ReplicatedAllPlayerCount);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReplicatedAllNodesCountOnInit, int32, AllNodesCountOnInit);

	UPROPERTY(BlueprintAssignable, Category = "Strategy | Map")
	FOnMapSeedReplicated OnMapSeedReplicatedBP;
	
	UPROPERTY(BlueprintAssignable, Category = "Strategy | Map")
	FReplicatedAllNodesCountOnInit ReplicatedAllNodesCountOnInit;	
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSeed, BlueprintReadOnly, Category = "Map")
	int32 MapSeed =-1;
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSize, BlueprintReadOnly, Category = "Map")
	int32 MapSize;
	
	UPROPERTY(ReplicatedUsing = OnRep_AllPlayerCount, BlueprintReadOnly, Category = "Match Setup")
	int32 AllPlayerCount = 2;
	
	UPROPERTY(ReplicatedUsing = OnRep_AllNodesCountOnInit, BlueprintReadOnly, Category = "Map")
	int32 AllNodesCountOnInit = 0;

	UFUNCTION()
	void OnRep_MapSeed();
	
	UFUNCTION()
	void OnRep_MapSize(); 
	
	UFUNCTION()
	void OnRep_AllPlayerCount(); 
	
	UFUNCTION()
	void OnRep_AllNodesCountOnInit(); 
	
	UPROPERTY(ReplicatedUsing = OnRep_CachedDeloneEdgesTowerID, BlueprintReadOnly, Category = "Map")
	TArray<FDeloneGraphEdge> CachedDeloneEdgesTowerID;
	
	UFUNCTION()
	void OnRep_CachedDeloneEdgesTowerID(); 
		
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCachedDeloneEdgesTowerIDChanged, TArray<FDeloneGraphEdge>, DeloneEdgesTowerID);
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnCachedDeloneEdgesTowerIDChanged OnCachedDeloneEdgesTowerIDChangedBP;
};

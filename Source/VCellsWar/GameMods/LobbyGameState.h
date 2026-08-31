// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

/**
 * 
 */

UCLASS()
class VCELLSWAR_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Функция для сервера, чтобы безопасно менять количество нод
	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	void SetNodeCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	int32 GetNodeCount() const { return NodeCount; }
	
	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	void SetMapSeed(int32 NewMapSeed);

	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	int32 GetMapSeed() const;
	
	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	void SetMapSize(int32 NewSize);

	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	int32 GetMapSize() const { return MapSize; }
	
	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	void SetAIPortalsCount(int32 NewCount);
	
	UFUNCTION(BlueprintCallable, Category = "Map Settings|UI")
	int32 GetAIPortalsCount() const {return AIPortalsCount;}

protected:
	// Реплицируемая переменная количества нод с RepNotify
	UPROPERTY(ReplicatedUsing = OnRep_NodeCount, BlueprintReadOnly, Category = "Map Settings")
	int32 NodeCount;
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSeed, BlueprintReadOnly, Category = "Map Settings")
	int32 MapSeed;
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSize, BlueprintReadOnly, Category = "Map Settings")
	int32 MapSize = 0;
	
	UPROPERTY(ReplicatedUsing = OnRep_AIPortalsCount, BlueprintReadOnly, Category = "Map Settings")
	int32 AIPortalsCount = 0;
 
	UFUNCTION()
	void OnRep_NodeCount();
	
	UFUNCTION()
	void OnRep_MapSeed();
	
	UFUNCTION()
	void OnRep_MapSize();
	
	UFUNCTION()
	void OnRep_AIPortalsCount();
	
	TArray<FVector2D> NodesPositions;

public:
	// Делегат, на который подпишется блупринт главного виджета лобби
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeCountChanged, int32, NewNodeCount);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnNodeCountChanged OnNodeCountChangedBP;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapSeedChanged, int32, NewMapSeed);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnMapSeedChanged OnMapSeedChangedBP;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapSizeChanged, int32, NewMapSeed);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnMapSizeChanged OnMapSizeChangedBP;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIPortalsCountChanged, int32, NewAIPortalsCount);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnAIPortalsCountChanged OnAIPortalsCountChangedBP;
	
	UFUNCTION(BlueprintCallable, Category = "Voronoi")
	void UpdateNodePositions(const TArray<FVector2D>& NewPositions);
	
	TArray<FVector2D> GetNodePositions();
};
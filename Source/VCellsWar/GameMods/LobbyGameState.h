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

protected:
	// Реплицируемая переменная количества нод с RepNotify
	UPROPERTY(ReplicatedUsing = OnRep_NodeCount, BlueprintReadOnly, Category = "Map Settings")
	int32 NodeCount;
	
	UPROPERTY(ReplicatedUsing = OnRep_MapSeed, BlueprintReadOnly, Category = "Map Settings")
	int32 MapSeed;

	UFUNCTION()
	void OnRep_NodeCount();
	
	UFUNCTION()
	void OnRep_MapSeed();

public:
	// Делегат, на который подпишется блупринт главного виджета лобби
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeCountChanged, int32, NewNodeCount);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnNodeCountChanged OnNodeCountChangedBP;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapSeedChanged, int32, NewMapSeed);
	
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnMapSeedChanged OnMapSeedChangedBP;
};
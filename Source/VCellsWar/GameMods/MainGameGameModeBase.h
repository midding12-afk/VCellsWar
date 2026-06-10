// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VCellsWar/Actors/PortalBase.h"
#include "MainGameGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API AMainGameGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	void BeginPlay() override;
	
	virtual void OnPostLogin(AController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> StrategyNodeClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> StrategyPortalClass;
	
	UFUNCTION(BlueprintCallable, Category = "Strategy | Map Generation")
	AActor* SpawnActorInLocation(const TSubclassOf<AActor> ActorToSpawn, const FVector2D Location) const;
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> CharacterUnitClass;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Strategy | Map Generation")
	void SpawnNodesFromSubsystem();
	void SpawnUnitFromPortal(APortalBase* PortalActor);
	
	// API для регистрации: вызываются акторами из их BeginPlay / EndPlay
	void RegisterPortal(AStrategyEntityBase* NewPortal);
	void UnregisterPortal(AStrategyEntityBase* OldPortal);
	
	FVector GetPointOnMapInLocation(FVector2D Location2D) const;
	
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
protected:
	// Функция глобального таймера логики (например, доход или спавн)
	void ProcessStrategyLogicTick();
	
	void SpawnNewPortal(AController* NewPlayer);
	
	int32 SplayerSpawnedPortalsCounter = 0;
private:
	// Чистые C++ массивы указателей. Поскольку GameMode живет только на сервере,
	// макрос UPROPERTY() нужен здесь ТОЛЬКО для того, чтобы сборщик мусора (Garbage Collector) 
	// случайно не удалил эти объекты из памяти. Репликация здесь НЕ ВКЛЮЧАЕТСЯ.
	UPROPERTY()
	TArray<AStrategyEntityBase*> ActivePortals;

	FTimerHandle StrategyLogicTimerHandle;
};

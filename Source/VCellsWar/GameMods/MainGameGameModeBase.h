// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VCellsWar/Actors/PortalBase.h"
#include "VCellsWar/Actors/TowerBase.h"
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
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> StrategyNodeClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> StrategyPortalClass;
	
	UFUNCTION(BlueprintCallable, Category = "Strategy | Map Generation")
	AActor* SpawnActorInLocation(const TSubclassOf<AActor> ActorToSpawn, const FVector2D Location) const;
	
	UPROPERTY(EditDefaultsOnly, Category = "Strategy | Map Settings")
	TSubclassOf<AActor> CharacterUnitClass;
	
public:
	virtual void Logout(AController* Exiting) override;
	
	UFUNCTION(BlueprintCallable, Category = "Strategy | Map Generation")
	void SpawnNodesFromSubsystem();
	void SpawnUnitFromPortal(APortalBase* PortalActor);
	
	// API для регистрации: вызываются акторами из их BeginPlay / EndPlay
	void RegisterPortal(APortalBase* NewPortal);
	void UnregisterPortal(APortalBase* OldPortal);
	
	void RegisterTower(ATowerBase* NewTower);
	void UnregisterTower(ATowerBase* OldTower);
	
	FVector GetPointOnMapInLocation(FVector2D Location2D) const;
protected:	
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;

	// Функция глобального таймера логики (например, доход или спавн)
	void ProcessStrategyLogicTick();
	
	void SpawnNewPortal(AController* NewPlayer);
	
	int32 SplayerSpawnedPortalsCounter = 0;
private:
	// Чистые C++ массивы указателей. Поскольку GameMode живет только на сервере,
	// макрос UPROPERTY() нужен здесь ТОЛЬКО для того, чтобы сборщик мусора (Garbage Collector) 
	// случайно не удалил эти объекты из памяти. Репликация здесь НЕ ВКЛЮЧАЕТСЯ.
	UPROPERTY()
	TArray<APortalBase*> ActivePortals;
	
	UPROPERTY()
	TArray<ATowerBase*> ActiveTower;

	FTimerHandle StrategyLogicTimerHandle;
	
	int32 LinkedStructuresCounter = 0;
	
	void UpdateVoronoiAndLinks(bool NeedToUpdateCellsMap = false);
	
	bool bAllTowersSpawned=false;
};

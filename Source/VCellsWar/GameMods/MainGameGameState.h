// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VCellsWar/Systems/VoronoiSubsystem.h" 
#include "MainGameGameState.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FRTSShotData
{
	GENERATED_BODY()

	// Откуда вылетел лазер (
	UPROPERTY()
	FVector_NetQuantize ShotOrigin;

	// Вектор направления выстрела
	UPROPERTY()
	FVector_NetQuantizeNormal ShotDirection;

	// ID игрока/фракции (0-7), который совершил выстрел!
	// Занимает всего 1 байт памяти в сетевом пакете.
	UPROPERTY()
	uint8 PlayerID = 0;
};


UCLASS()
class VCELLSWAR_API AMainGameGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	AMainGameGameState();
	
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
	
	void UpdateCachedDeloneEdgesTowerID(TArray<FDeloneGraphEdge> NewCachedDeloneEdgesTowerID);
	
	UFUNCTION()
	void OnRep_CachedDeloneEdgesTowerID(); 
	
	void InvocLinksUpdate() {OnRep_CachedDeloneEdgesTowerID();};
		
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCachedDeloneEdgesTowerIDChanged, TArray<FDeloneGraphEdge>, DeloneEdgesTowerID);
	UPROPERTY(BlueprintAssignable, Category = "Map Settings|UI")
	FOnCachedDeloneEdgesTowerIDChanged OnCachedDeloneEdgesTowerIDChangedBP;
	
	/**
	 * КРИТИЧЕСКИЙ МЕТОД ДЛЯ ИИ СОЛДАТ:
	 * Любая таска State Tree или GAS способность атаки на сервере просто 
	 * бросает сюда координаты своего выстрела. Код выполняется мгновенно!
	 */
	UFUNCTION(BlueprintCallable)
	void Server_RegisterRTSShot(const FVector& Origin, const FVector& Direction, uint8 PlayerID);
	
	void AddTeamIDColor(int32 TeamId, FLinearColor TeamColor);
	
	virtual void Tick(float DeltaTime) override;
	
private:
	// Временный серверный буфер, который копит выстрелы текущего кадра
	UPROPERTY()
	TArray<FRTSShotData> AccumulatedShots;
	
	UPROPERTY(Replicated)
	TArray<FLinearColor> AllTeamsColors;
	
	/**
	 * Сервер вызывает этот метод у себя, но движок UE5 автоматически пакует 
	 * весь массив выстрелов в ОДИН сетевой пакет и рассылает его всем клиентам в лобби!
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_BroadcastRTSShotsBatch(const TArray<FRTSShotData>& ShotsBatch);
	
};

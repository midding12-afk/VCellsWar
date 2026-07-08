// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityBase.h"
#include "GameFramework/Actor.h"
#include "Interface/StructureNetIDInterface.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"
#include "StrategyEntityCharacter.h"
#include "PortalBase.generated.h"

UCLASS()
class VCELLSWAR_API APortalBase : public AStrategyEntityBase, public IStructureNetIDInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_NextSpawnTime, BlueprintReadOnly, Category = "RTS | Logic")
	float NextSpawnTime = 0.0f;
	
	UPROPERTY(ReplicatedUsing = OnRep_NextSpawnTime, EditAnywhere, BlueprintReadOnly, Category = "RTS | Logic")
	float BaseSpawnDelay = 30.0f;

	UFUNCTION()
	void OnRep_NextSpawnTime();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Logic")
	TSubclassOf<AStrategyEntityCharacter> SoldierClass;
	
	FTimerHandle WaveSpawnTimerHandle;
	
	// Внутренний метод сервера, который заводит следующий цикл и считает GAS модификаторы
	void ScheduleNextWave();

	// Физическая функция спавна, которая дергает серверный пул
	void ExecuteWaveSpawn();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void Server_SetNextSpawnDelay(float DelaySeconds);
	
	float GetNextSpawnTime() { return NextSpawnTime; }
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS | Logic")
	int32 PortalId = -1;
	
	virtual int32 GetStructureNetID_Implementation() override {return PortalId;};
	virtual void Server_SetStructureNetID_Implementation(int32 NewID) override {if (HasAuthority()) PortalId =  NewID;};
	
	void Server_SpawnWave(int32 Count);
private:
	UServerNetworkPoolSubsystem* ServerPool;
};

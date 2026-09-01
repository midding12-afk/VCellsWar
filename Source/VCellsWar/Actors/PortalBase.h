// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityBase.h"
#include "GameFramework/Actor.h"
#include "Interface/StructureNetIDInterface.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"
#include "StrategyEntityCharacter.h"
#include "PortalBase.generated.h"

class USphereComponent;

UCLASS()
class VCELLSWAR_API APortalBase : public AStrategyEntityBase, public IStructureNetIDInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ExecuteWaveSpawn(int32 TroopsCount);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Logic")
	TSubclassOf<AStrategyEntityCharacter> SoldierClass;
	
	FTimerHandle WaveSpawnTimerHandle;
	
	
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> SphereComponent;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//float GetNextSpawnTime() { return NextSpawnTime; }
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS | Logic")
	int32 PortalId = -1;
	
	virtual int32 GetStructureNetID_Implementation() override {return PortalId;};
	virtual void Server_SetStructureNetID_Implementation(int32 NewID) override {if (HasAuthority()) PortalId =  NewID;};
	
	
	UPROPERTY()
	class AAIGeneralDirector* EnemyAiDirector;
	UPROPERTY()
	TWeakObjectPtr<class UAISquad> LocalPortalSquad;
private:
	UServerNetworkPoolSubsystem* ServerPool;
};

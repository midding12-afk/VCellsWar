// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityPawn.h"
#include "Interface/StructureNetIDInterface.h"
#include "Components/CapsuleComponent.h"
#include "TowerBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ATowerBase : public AStrategyEntityPawn, public IStructureNetIDInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATowerBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS | Logic")
	int32 TowerId = -1;
	
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* DummyRootComponent;*/
	
	UPROPERTY(ReplicatedUsing=OnRep_HealthBarProgress)
	uint8 HealthBarProgress = 0;
	
	UPROPERTY(ReplicatedUsing=OnRep_HealthBarTeamIDColor)
	uint8 HealthBarTeamIDColor = 255;
	
	UFUNCTION()
	void OnRep_HealthBarProgress();
	
	UFUNCTION()
	void OnRep_HealthBarTeamIDColor();
	
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicTowerMaterial;
	
	UPROPERTY()
	int32 MaxHealthWithoutOwner = 100;
	
	UPROPERTY()
	int32 CurrentOwningProgressInHealthPoint = 0;
	

public:	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//void Server_SetTowerId(int32 NewTowerId);
	//int32 GetTowerId() const {return TowerId;};
	
	virtual int32 GetStructureNetID_Implementation() override {return TowerId;};
	virtual void Server_SetStructureNetID_Implementation(int32 NewID) override {if (HasAuthority()) TowerId =  NewID;};
	
	virtual void GeinDamage(float Damage, int32 InstigatorTeamID) override;
	
	bool IsTowerLockedNoConnection(int32 MyFactionId);
};

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interface/StrategyEntityInterface.h"
#include "StrategyEntityPawn.generated.h"

UCLASS()
class VCELLSWAR_API AStrategyEntityPawn : public APawn, public IStrategyEntityInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AStrategyEntityPawn();
	
	virtual FGenericTeamId GetGenericTeamId() const override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	// Самый главный указатель. Реплицируется от сервера к клиентам.
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerState, BlueprintReadOnly, Category = "Strategy | Ownership")
	AMainGamePlayerState* OwningPlayerState;
	
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerColor, BlueprintReadOnly, Category = "Strategy | Ownership")
	FLinearColor OwningPlayerColor = FLinearColor::Gray*0.3f;

	UFUNCTION()
	void OnRep_OwningPlayerState();
	
	UFUNCTION()
	void OnRep_OwningPlayerColor();
	
	// Физическая переменная цвета, которая реплицируется по сети
	// UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Visual")
	// FLinearColor TeamColor;
	//
	// UFUNCTION()
	// void OnRep_TeamColor();
	
	int SelectedSectorGridIndex = -1;
public:
	// Нам НЕ нужны макросы UFUNCTION здесь! Они автоматически унаследовались из интерфейса.
	// virtual void SetTeamColor_Implementation(FLinearColor NewColor) override {TeamColor = NewColor;};
	virtual FLinearColor GetTeamColor_Implementation() const override {return OwningPlayerColor;};
	
	virtual AMainGamePlayerState* GetEntityOwnerState_Implementation() override {return  OwningPlayerState;};
	//virtual void SetEntityOwner(AMainGamePlayerState* NewOwnerState) override { OwningPlayerState = NewOwnerState; };
	//virtual bool IsOwnedByLocalPlayer_Implementation() const override;

	virtual void SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState) override;
	
	virtual int32 GetEntityFactionID_Implementation() const override {return GetGenericTeamId();};
	
	virtual void SetSelectedSectorGridIndex(int NewSelectedSectorGridIndex) override {SelectedSectorGridIndex = NewSelectedSectorGridIndex;};
	virtual int GetSelectedSectorGridIndex() override {return SelectedSectorGridIndex;};
};

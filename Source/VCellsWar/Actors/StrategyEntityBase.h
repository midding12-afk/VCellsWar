// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interface/StrategyEntityInterface.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "StrategyEntityBase.generated.h"



UCLASS()
class VCELLSWAR_API AStrategyEntityBase : public AActor, public IStrategyEntityInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	AStrategyEntityBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EStrategyEntityCategory GetEntityCategory_Implementation() const override {return EStrategyEntityCategory::StaticBuild;};
protected:
	virtual void BeginPlay() override;
	
	// Самый главный указатель. Реплицируется от сервера к клиентам.
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerState, BlueprintReadOnly, Category = "Strategy | Ownership")
	AMainGamePlayerState* OwningPlayerState;
	
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerColor, BlueprintReadOnly, Category = "Strategy | Ownership")
	FLinearColor OwningPlayerColor = FLinearColor::Gray;

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
	
	FGenericTeamId StructureTeamId;
	
	int SelectedSectorGridIndex = -1;
	
	// С++ компонент декали, который проецирует зеленый круг на землю под солдатом
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Selection")
	class UDecalComponent* SelectionDecalComponent;
	
public:
	// Нам НЕ нужны макросы UFUNCTION здесь! Они автоматически унаследовались из интерфейса.
	//virtual void SetTeamColor_Implementation(FLinearColor NewColor) override {TeamColor = NewColor;};
	virtual FLinearColor GetTeamColor_Implementation() const override {return OwningPlayerColor;};
	
	virtual AMainGamePlayerState* GetEntityOwnerState_Implementation() override {return  OwningPlayerState;};
	//virtual void SetEntityOwner(AMainGamePlayerState* NewOwnerState) override { OwningPlayerState = NewOwnerState; };
	//virtual bool IsOwnedByLocalPlayer_Implementation() const override;
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override {StructureTeamId =  TeamID;};
	
	virtual FGenericTeamId GetGenericTeamId() const override { return StructureTeamId; }
	

	virtual void SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState) override;
	
	virtual int32 GetEntityFactionID_Implementation() const override {return GetGenericTeamId();};
	
	virtual void SetSelectedSectorGridIndex(int NewSelectedSectorGridIndex) override {SelectedSectorGridIndex = NewSelectedSectorGridIndex;};
	virtual int GetSelectedSectorGridIndex() override {return SelectedSectorGridIndex;};
	
	virtual bool NativeRTSIsEntitySelected() const override;
	
	UFUNCTION(BlueprintCallable, Category = "RTS | Selection")
	virtual void SelectEntity() override;

	UFUNCTION(BlueprintCallable, Category = "RTS | Selection")
	virtual void DeselectEntity() override;
	
	virtual void NativeRTSInitialize(int32 InFactionID, class AMainGamePlayerState* InOwnerState, const FTransform& InSpawnTransform) override;
	virtual void NativeRTSDeinitialize() override;
};

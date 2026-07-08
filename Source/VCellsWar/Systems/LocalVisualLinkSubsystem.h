// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VoronoiSubsystem.h" 
#include "VCellsWar/Actors/BeamBase.h"
#include "LocalVisualLinkSubsystem.generated.h"

USTRUCT()
struct FLinkDataState
{
	GENERATED_BODY()

	int32 Start = -1;

	int32 End = -1;
	
	UPROPERTY()
	ABeamBase* BeamPtr = nullptr;
	
	bool isActive = false;
    
	FLinkDataState() {}
	FLinkDataState(int32 InA, int32 InB, ABeamBase* InBeam) : Start(InA), End(InB), BeamPtr(InBeam) {}
	FLinkDataState(int32 InA, int32 InB) : Start(InA), End(InB) {}
    
	bool operator==(const FLinkDataState& Other) const
	{
		return (Start == Other.Start && End == Other.End);
	}
	
	bool operator==(const FDeloneGraphEdge& Other) const
	{
		return (Start == Other.Start && End == Other.End);
	}
	
};

UCLASS()
class VCELLSWAR_API ULocalVisualLinkSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION(BlueprintCallable)
	void UpdateTowerIDMap();

protected:
	void TryInitGameStateBridge();
	FTimerHandle GameStateCheckTimerHandle;
	
	UFUNCTION()
	void HandleDeloneEdgesChanged(TArray<FDeloneGraphEdge> DeloneEdgesTowerID);
	
	UPROPERTY()
	TMap<int32, AActor*> TowerIDMap;
	
	UPROPERTY()
	TArray<FLinkDataState> BeamList;
	
};

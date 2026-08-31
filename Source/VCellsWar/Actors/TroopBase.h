// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityCharacter.h"
#include "VCellsWar/Systems/FlagsManagerSubsystem.h"
#include "TroopBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ATroopBase : public AStrategyEntityCharacter
{
	GENERATED_BODY()
public:	

	virtual void GeinDamage(float Damage, int32 InstigatorTeamID) override;
	
	class ATacticalFlagBase* GetCurrentTargetFlag() {return CurrentTargetFlag;};
	void SetNewRtsTargetFlag(class ATacticalFlagBase* NewFlag);
	
	//void SetServerLocalIndex(int32 newID) {AttachedToFlagIndex = newID;};
	
	FORCEINLINE ETroopAssignmentState GetAssignmentState() const { return AssignmentState; }
	FORCEINLINE void SetAssignmentState(ETroopAssignmentState NewState) { AssignmentState = NewState; }
	
	bool IsTargetFlagMoved() {return (CurrentTargetFlag && LastCnownFlagLocation!=CurrentTargetFlag->GetActorLocation());};
	
	UPROPERTY()
	class UAISquad* MyAISquad;
	
	UPROPERTY()
	int32 SquadLocalIndex = -1;
private:
	ETroopAssignmentState AssignmentState = ETroopAssignmentState::Idle;
	
	UPROPERTY()
	class ATacticalFlagBase* CurrentTargetFlag = nullptr;
	int32 ServerLocalIndex = -1;
	
	UPROPERTY()
	FVector LastCnownFlagLocation;
	
	
};

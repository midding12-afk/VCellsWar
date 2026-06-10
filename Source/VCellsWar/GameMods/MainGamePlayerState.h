// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MainGamePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API AMainGamePlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	FLinearColor GetTeamColor() const;
	void SetTeamColor(FLinearColor NewTeamColor);
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Strategy | Player")
	FLinearColor TeamColor = FLinearColor::Gray;

	UFUNCTION()
	void OnRep_TeamColor();

};

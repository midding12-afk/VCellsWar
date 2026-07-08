// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ALobbyPlayerState : public APlayerState, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
public:
	ALobbyPlayerState();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		
protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Lobby")
	FLinearColor TeamColor;	
	
	UPROPERTY(ReplicatedUsing = OnRep_bIsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;
	
	UFUNCTION()
	void OnRep_TeamColor();
	
	UFUNCTION()
	void OnRep_bIsReady();
	
	virtual void CopyProperties(APlayerState* NewPlayerState) override;
	
public:
	FLinearColor GetTeamColor() const;
	void SetTeamColor(FLinearColor NewTeamColor);
	
	bool GetIsReady() const;
	void SetIsReady(bool NewIsReady);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamColorChanged, FLinearColor, NewColor);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsReadyChanged, bool, bIsReady);
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnTeamColorChanged OnTeamColorChangedBP;
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnIsReadyChanged OnIsReadyChangedBP;
	
	// UFUNCTION(BlueprintCallable)
	// int32 GetTeamIndex() const { return TeamIndex; };
	// UFUNCTION(BlueprintCallable)
	// void SetTeamIndex(int32 NewTeamIndex){TeamIndex = NewTeamIndex;};
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override {TeamIndex = TeamID;}
	
	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamIndex; }
	
protected:
	UPROPERTY()
	FGenericTeamId TeamIndex;
};

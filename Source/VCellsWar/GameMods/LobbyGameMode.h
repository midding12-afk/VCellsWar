// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ALobbyGameMode();

	void TryAssignColorToPlayer(AController* PlayerController, FLinearColor RequestedColor);
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	bool CheckAllPlayersReady();
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void StartMatch(const FString& MapPath);
	
protected:
	
	virtual void OnPostLogin(AController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;
};

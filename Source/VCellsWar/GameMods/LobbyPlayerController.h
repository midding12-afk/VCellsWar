// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyGameMode.h"
#include "LobbyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestFactionColor(FLinearColor NewColor);
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestReadyCheckBoxChange(bool bIsReady);
	
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartGame();
	
	UFUNCTION(Server, Reliable)
	void Server_RequestFactionColor(FLinearColor NewColor);
	
	UFUNCTION(Server, Reliable)
	void Server_RequestReadyCheckBoxChange(bool bIsReady);
	
	UFUNCTION(Server, Reliable)
	void Server_RequestStartGame();
	
	UFUNCTION(Client, Reliable)
	void Client_RefreshLobbyUI_PlayerList();
	
	UFUNCTION(Client, Reliable)
	void Client_RefreshLobbyUI_MapPrev();
	
	UFUNCTION(Client, Reliable)
	void Client_ShowNotificationMessageUI(const FText& Message);
	
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshLobbyUI_PlayerList);
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnRefreshLobbyUI_PlayerList OnRefreshLobbyUI_PlayerList;
	/////
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshLobbyUI_MapPrev);
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnRefreshLobbyUI_MapPrev OnRefreshLobbyUI_MapPrev;
	/////
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationMessageUI, FText, Message);
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnNotificationMessageUI OnNotificationMessageUI;
};

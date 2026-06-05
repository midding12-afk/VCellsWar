// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LobbyPlayerController.h"

#include "LobbyPlayerState.h"
#include "Kismet/KismetSystemLibrary.h"

void ALobbyPlayerController::Server_RequestFactionColor_Implementation(FLinearColor NewColor)
{
	UKismetSystemLibrary::PrintString(GetWorld(), TEXT("RequestFactionColor"));
	
	ALobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (LobbyGM)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("RequestFactionColor GM call"));
		LobbyGM->TryAssignColorToPlayer(this, NewColor);
	}
}

void ALobbyPlayerController::RequestFactionColor(FLinearColor NewColor)
{
	Server_RequestFactionColor(NewColor);
}

void ALobbyPlayerController::RequestReadyCheckBoxChange(bool bIsReady)
{
	Server_RequestReadyCheckBoxChange(bIsReady);
}

void ALobbyPlayerController::RequestStartGame()
{
	Server_RequestStartGame();
}

void ALobbyPlayerController::Server_RequestStartGame_Implementation()
{
	if (HasAuthority())
	{
		ALobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
		if (LobbyGM)
		{
			UKismetSystemLibrary::PrintString(GetWorld(), TEXT("RequestFactionColor GM call"));
			LobbyGM->StartMatch("/Game/VCellsWar/Maps/L_MainGame");
		}
	}
}

void ALobbyPlayerController::Client_ShowNotificationMessageUI_Implementation(const FText& Message)
{
	OnNotificationMessageUI.Broadcast(Message);
}

void ALobbyPlayerController::Server_RequestReadyCheckBoxChange_Implementation(bool bIsReady)
{
	ALobbyPlayerState* TargetPS = GetPlayerState<ALobbyPlayerState>();
	if (TargetPS)
	{
		if (TargetPS->GetTeamColor() == FLinearColor::White)
		{
			TargetPS->SetIsReady(false);
			Client_ShowNotificationMessageUI(FText::FromString("Before you're ready, you need to choose a color."));
		}
		else
		{
			TargetPS->SetIsReady(bIsReady);
		}
	}
		
	ALobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (LobbyGM)
	{
		LobbyGM->CallUpdatePlayerListOnAllPlayers();
	}
	
}

void ALobbyPlayerController::Client_RefreshLobbyUI_PlayerList_Implementation()
{
	OnRefreshLobbyUI_PlayerList.Broadcast();
}


void ALobbyPlayerController::Client_RefreshLobbyUI_MapPrev_Implementation()
{
	OnRefreshLobbyUI_MapPrev.Broadcast();
}

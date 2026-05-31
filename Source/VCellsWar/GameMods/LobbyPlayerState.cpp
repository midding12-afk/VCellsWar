// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LobbyPlayerState.h"
#include "MainGamePlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerState::ALobbyPlayerState()
{
	bReplicates = true; // Включаем репликацию для этого эктора
	TeamColor = FLinearColor::White; // Дефолтный цвет
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Указываем, что TeamColor должен синхронизироваться со всеми клиентами
	//DOREPLIFETIME(ALobbyPlayerState, TeamColor);
	
	//DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	
	// Вместо DOREPLIFETIME используем CONDITION_Notify
	// REPCOND_None означает реплицировать всегда и всем, 
	// а REPNOTIFY_Always заставляет OnRep срабатывать ДАЖЕ если значение переменной на клиенте совпадает с сервером
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyPlayerState, TeamColor, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyPlayerState, bIsReady, COND_None, REPNOTIFY_Always);
}

FLinearColor ALobbyPlayerState::GetTeamColor() const
{
	return TeamColor;
}

void ALobbyPlayerState::SetTeamColor(FLinearColor NewTeamColor)
{
	UKismetSystemLibrary::PrintString(GetWorld(), TEXT("SetTeamColor"));
	if (HasAuthority())
	{
		TeamColor = NewTeamColor;
		
		// НЮАНС UE: На самом сервере функция OnRep_TeamColor автоматически НЕ вызывается.
		// Чтобы серверный игрок (хост) тоже сразу увидел изменения в своем UI,
		// вызываем её вручную на сервере.
		OnRep_TeamColor();
	}
}

bool ALobbyPlayerState::GetIsReady() const
{
	return bIsReady;
}

void ALobbyPlayerState::SetIsReady(bool NewIsReady)
{
	if (HasAuthority())
	{
		bIsReady = NewIsReady;
		UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Set bIsReady to %s"), *GetPlayerName()));
		
		OnRep_bIsReady();
	}
}

void ALobbyPlayerState::OnRep_TeamColor()
{
	UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("SetTeamColor broadcast to %s"), *GetPlayerName()));
	
	OnTeamColorChangedBP.Broadcast(TeamColor);
}

void ALobbyPlayerState::OnRep_bIsReady()
{
	UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("bIsReady broadcast to %s"), *GetPlayerName()));
	OnIsReadyChangedBP.Broadcast(bIsReady);
}

void ALobbyPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	// 1. ОБЯЗАТЕЛЬНО вызываем родительский метод. 
	// Он скопирует ID игрока, пинг и его имя из Steam, чтобы они не стерлись
	Super::CopyProperties(NewPlayerState);

	if (NewPlayerState)
	{
		// 2. Приводим прилетевший базовый NewPlayerState к классу основной игры
		AMainGamePlayerState* MainGamePS = Cast<AMainGamePlayerState>(NewPlayerState);
		
		if (MainGamePS)
		{
			// 3. Копируем цвет из ТЕКУЩЕГО стейта лобби (this->TeamColor) 
			// в соответствующий метод или переменную игрового стейта матча
			MainGamePS->SetTeamColor(this->TeamColor); 
			
		}
	}
}

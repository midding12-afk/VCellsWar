// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerState.h"

#include "Net/UnrealNetwork.h"

void AMainGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGamePlayerState, TeamColor, COND_None, REPNOTIFY_Always);
}

FLinearColor AMainGamePlayerState::GetTeamColor() const
{
	return TeamColor;
}

void AMainGamePlayerState::SetTeamColor(FLinearColor NewColor)
{
	// Метод вызовется сервером в процессе Seamless Travel
	TeamColor = NewColor;
}

void AMainGamePlayerState::OnRep_TeamColor()
{
	
}

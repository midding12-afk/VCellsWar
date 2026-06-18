// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityBase.h"
#include "Net/UnrealNetwork.h"


AStrategyEntityBase::AStrategyEntityBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Включаем сетевую репликацию для объекта!
	bReplicates = true;
	bAlwaysRelevant = true; 
	//SetReplicateMovement(true); // Юниты будут двигаться, постройкам можно выключить в наследниках
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	//AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	OwningPlayerState = nullptr;
}

void AStrategyEntityBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AStrategyEntityBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Синхронизируем владельца со всеми клиентами в игре
	DOREPLIFETIME(AStrategyEntityBase, OwningPlayerState);
}


void AStrategyEntityBase::OnRep_OwningPlayerState()
{
	if (OwningPlayerState)
	{
		
	}
}

void AStrategyEntityBase::OnRep_OwningPlayerColor()
{
	
}

void AStrategyEntityBase::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}


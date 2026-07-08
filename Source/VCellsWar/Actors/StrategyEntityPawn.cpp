// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityPawn.h"

#include "GenericTeamAgentInterface.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AStrategyEntityPawn::AStrategyEntityPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Включаем сетевую репликацию для объекта!
	bReplicates = true;
	bAlwaysRelevant = true; 
	SetReplicateMovement(true); // Юниты будут двигаться, постройкам можно выключить в наследниках
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	OwningPlayerState = nullptr;
}

// Called when the game starts or when spawned
void AStrategyEntityPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStrategyEntityPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AStrategyEntityPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AStrategyEntityPawn, OwningPlayerState);
}

void AStrategyEntityPawn::OnRep_OwningPlayerState()
{
}

void AStrategyEntityPawn::OnRep_OwningPlayerColor()
{
}

void AStrategyEntityPawn::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}

FGenericTeamId AStrategyEntityPawn::GetGenericTeamId() const
{
	// Если у солдата есть контроллер ИИ — забираем ID команды у него
	if (IGenericTeamAgentInterface* TeamController = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamController->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}


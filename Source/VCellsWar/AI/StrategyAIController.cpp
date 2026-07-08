// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyAIController.h"
#include "Components/StateTreeComponent.h" // Для работы с компонентом дерева
#include "StateTreeEvents.h"               // Для корректной рефлексии FStateTreeEvent
#include "GameplayTagsManager.h"

#include "AbilitySystemComponent.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Actors/BlasterBase.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
#include "VCellsWar/GameMods/MainGameGameState.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"

AStrategyAIController::AStrategyAIController()
{
	
}

void AStrategyAIController::MakeShoot()
{
	// 1. Проверяем базовую валидность: мы должны быть на сервере, и у нас обязана быть цель для атаки!
	if (!HasAuthority() || !IsValid(CombatTargetActor)) return;

	// 2. Достаем физическое тело (Pawn), которым сейчас управляет этот ИИ-контроллер
	AStrategyEntityCharacter* MySoldier = Cast<AStrategyEntityCharacter>(GetPawn());
	if (!IsValid(MySoldier)) return;
	
	
	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;
	
	TSubclassOf<AActor> BlasterClass = Settings->VisualBlasterClass.LoadSynchronous();
	ULocalGraphicsPoolSubsystem* GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	
	if (!GS) return;
	
	 AActor* ABlast = GraphicsPool->GetActorFromPool(BlasterClass);
	 if (ABlast)
	 {
		ABlasterBase* Blast = Cast<ABlasterBase>(ABlast);	
		FVector ShotOrigin = MySoldier->GetActorLocation();//+FVector(0.0,0.0,1.f);
		FVector TargetLocation = CombatTargetActor->GetActorLocation();//+FVector(0.0,0.0,1.f);
		FVector Direction = (TargetLocation - ShotOrigin).GetSafeNormal();
		Direction = FMath::VRandCone(Direction, 0.2f);
	 	
	 	Blast->SetActorHiddenInGame(true);
		
		Blast->InitBlasterShoot(ShotOrigin,Direction,MySoldier->GetTeamColor_Implementation(),MySoldier->GetGenericTeamId());
		GS->Server_RegisterRTSShot(ShotOrigin, Direction, MySoldier->GetGenericTeamId());
	 }				
}




void AStrategyAIController::BeginPlay()
{
	Super::BeginPlay();

	
}

void AStrategyAIController::Command_MoveTo(FVector TargetLocation)
{
	// 1. ЖЕЛЕЗОБЕТОННЫЙ RTS ФИКС: Просто сохраняем точку клика в память контроллера!
	MoveTargetLocation = TargetLocation;

	// 2. Пуляем в State Tree самый обычный, легкий ивент. 
	// Его задача теперь — просто переключить ветку дерева из Idle в Moving!
	UStateTreeComponent* STComp = FindComponentByClass<UStateTreeComponent>();
	if (IsValid(STComp))
	{
		FStateTreeEvent Event;
		Event.Tag = FGameplayTag::RequestGameplayTag(TEXT("Gameplay.Event.MoveTo"));
		
		// Нам БОЛЬШЕ НЕ НУЖЕН Payload.InitializeAs! Сеть и память чисты.
		STComp->SendStateTreeEvent(Event);
	}
}

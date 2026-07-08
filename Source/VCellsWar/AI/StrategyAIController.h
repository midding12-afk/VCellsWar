// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StrategyAIController.generated.h"

/**
 * 
 */
// Опережающее объявление компонентов восприятия
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
	class VCELLSWAR_API AStrategyAIController : public AAIController//, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AStrategyAIController();
	
	void Command_MoveTo(FVector TargetLocation);

	// Текущий враг, которого солдат держит на мушке
	UPROPERTY(BlueprintReadWrite, Category = "RTS | Combat")
	AActor* CombatTargetActor = nullptr;
	
	// Текущая точка назначения солдата на сервере
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Movement")
	FVector MoveTargetLocation;
	
	UFUNCTION(BlueprintCallable, Category = "RTS | Attack")
	void MakeShoot();
	
	UPROPERTY(BlueprintReadWrite)
	float AccumulatedTime;

protected:
	virtual void BeginPlay() override;
	
};

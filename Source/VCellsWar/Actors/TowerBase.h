// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityPawn.h"
#include "TowerBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ATowerBase : public AStrategyEntityPawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATowerBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

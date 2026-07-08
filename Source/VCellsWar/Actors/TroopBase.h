// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityCharacter.h"
#include "TroopBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API ATroopBase : public AStrategyEntityCharacter
{
	GENERATED_BODY()
public:	
	UFUNCTION(BlueprintCallable)
	virtual void GeinDamage(float Damage) override;
};

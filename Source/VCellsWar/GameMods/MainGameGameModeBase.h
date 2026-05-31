// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API AMainGameGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	void BeginPlay() override;
};

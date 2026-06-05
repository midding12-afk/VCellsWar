// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainGamePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API AMainGamePlayerController : public APlayerController
{
	GENERATED_BODY()
public:	
	virtual void BeginPlay() override;
	
	// Этот метод гарантирует, что павн ПОЛНОСТЬЮ перешел под контроль клиента на новой карте
	virtual void AcknowledgePossession(APawn* P) override;
	
	void TeleportLocalCameraToCenter(FVector CenterLocation);
protected:
	UFUNCTION(Client, Reliable)
	void Client_TeleportCamera(FVector TargetLocation);
};

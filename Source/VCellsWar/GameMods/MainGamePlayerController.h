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
	
	void TeleportLocalCameraTo(FVector2D CenterLocation);
protected:
	UFUNCTION(Client, Reliable, BlueprintCallable, meta = (CPP_Default_Z = -1.0f))
	void Client_TeleportCamera(FVector2D TargetLocation, float Z);
};

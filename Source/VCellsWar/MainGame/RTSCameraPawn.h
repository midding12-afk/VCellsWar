// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "RTSCameraPawn.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class VCELLSWAR_API ARTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTSCameraPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	// Переопределяем встроенный метод UE для привязки управления
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Компоненты камеры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS Camera")
	UCameraComponent* CameraComponent;

	// --- ENHANCED INPUT ---
public:
	UPROPERTY(EditDefaultsOnly, Category = "RTS Input")
	UInputMappingContext* CameraMappingContext;
protected:
	UPROPERTY(EditDefaultsOnly, Category = "RTS Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Input")
	UInputAction* ZoomAction;

	// Настройки скорости
	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera Settings")
	float MoveSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Camera Settings")
	float ZoomSpeed = 50.0f;

	// Внутренние функции движения
	void Move(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
	void CheckEdgePan(float DeltaTime); // Движение камеры мышкой у краев экрана
	
	float GetCalkMoveSpeed();
};

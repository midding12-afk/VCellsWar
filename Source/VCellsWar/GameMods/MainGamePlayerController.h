// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h" 
#include "MainGamePlayerController.generated.h"

/**
 * 
 */
// Опережающее объявление классов Enhanced Input
class UInputMappingContext;
class UInputAction;

UCLASS()
class VCELLSWAR_API AMainGamePlayerController : public APlayerController
{
	GENERATED_BODY()
public:	
	AMainGamePlayerController();
	
	
	// Этот метод гарантирует, что павн ПОЛНОСТЬЮ перешел под контроль клиента на новой карте
	virtual void AcknowledgePossession(APawn* P) override;
	
	void TeleportLocalCameraTo(FVector2D CenterLocation);
	
	// Наш динамический список юнитов, которых игрок обвел рамкой на экране.
	// UPROPERTY() обязателен, чтобы Garbage Collector не стер указатели!
	UPROPERTY(BlueprintReadWrite, Category = "RTS | Selection")
	TArray<AActor*> MySelectedUnits;
	
	UFUNCTION(BlueprintCallable)
	void SetSelectedList(TArray<AActor*> NewSelectedUnits);
	
	// Флаг: зажата ли рамка прямо сейчас
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Selection")
	bool bIsSelecting = false;

	// Точка экрана, где игрок нажал мышку (Координаты X, Y)
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Selection")
	FVector2D StartSelectionPoint;
protected:
	UFUNCTION(Client, Reliable, BlueprintCallable, meta = (CPP_Default_Z = -1.0f))
	void Client_TeleportCamera(FVector2D TargetLocation, float Z);
	
	virtual void SetupInputComponent() override;
	
	virtual void BeginPlay() override;
	
	// СЕТЕВОЙ ШЛЮЗ ДЛЯ КЛИЕНТОВ (Срабатывает при репликации пешки)
	virtual void OnRep_Pawn() override;

	// СЕТЕВОЙ ШЛЮЗ ДЛЯ СЕРВЕРА / STANDALONE (Срабатывает при захвате пешки на сервере)
	virtual void OnPossess(APawn* InPawn) override;

private:
	// Наша общая безопасная функция, которая физически включает Enhanced Input
	void InitializeRTSInput();
	
protected:
	// Функции ввода для Левой Кнопки Мыши (LMB)
	// void OnLeftClickPressed();
	// void OnLeftClickReleased();
	
	// Локальная C++ функция, которую вы привяжете к клику правой кнопкой мыши (RMB)
	// void OnRightClickPressed();
	
	/**
	 * СЕТЕВОЙ СЕРВЕРНЫЙ МОСТ (Server RPC):
	 * Вызывается на клиенте, но физическое тело выполняется СТРОГО на сервере [1.5].
	 * Флаг Reliable гарантирует, что приказ долетит без потерь пакетов [1.5].
	 * Флаг WithValidation позволяет серверу отсекать пакеты от читеров [1.5].
	 */
	UFUNCTION(Server, Reliable, WithValidation, Category = "RTS | Network Commands")
	void Server_MoveSelectedUnits(FVector TargetLocation, const TArray<AActor*>& ActorsToMove);
	
	// АССЕТЫ УПРАВЛЕНИЯ: Выставим их в Blueprint-наследнике PlayerController
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputMappingContext* DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputAction* ActionLeftClick;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputAction* ActionRightClick;
	
	// Сигнатуры функций Enhanced Input жестко требуют аргумент const FInputActionValue& Value!
	void OnLeftClickStarted(const FInputActionValue& Value);
	void OnLeftClickCompleted(const FInputActionValue& Value);
	void OnRightClickPressed(const FInputActionValue& Value);
};

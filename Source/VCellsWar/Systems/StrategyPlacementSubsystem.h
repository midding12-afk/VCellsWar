// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "StrategyPlacementSubsystem.generated.h"

/**
 * 
 */

class ATacticalFlagBase;

/** УНИВЕРСАЛЬНЫЙ ТАКТИЧЕСКИЙ ПАКЕТ ДАННЫХ СТРОИТЕЛЬСТВА */
USTRUCT(BlueprintType)
struct FPlacementBuildingData
{
	GENERATED_BODY()

	/** Реальный реплицируемый С++ класс здания (Флаг, Завод), который родится на Сервере */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Construction")
	TSubclassOf<AActor> RealServerBuildingClass;

	/** Легкий локальный класс-призрак (Ghost/Preview) для ведения за мышкой на Клиенте */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Construction")
	TSubclassOf<AActor> LocalClientPreviewClass;

	/** Физический радиус габаритов постройки для С++ калькулятора коллизий и наложения */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Construction")
	float PlacementCheckRadius = 150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Construction")
	bool bIsChainBuilding = false;
};

/** МОДУЛЬНАЯ ААА ПОДСИСТЕМА РАЗМЕЩЕНИЯ И СТРОИТЕЛЬСТВА ИГРОКА */
UCLASS()
class VCELLSWAR_API UStrategyPlacementSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UStrategyPlacementSubsystem();

	/** Накатываем ручной Tick подсистемы (вызывается из PlayerController) */
	void TickPlacement(float DeltaTime, class APlayerController* PC);

	/** Вызывается из UI кнопок. Заводит режим размещения! */
	UFUNCTION(BlueprintCallable, Category = "RTS|Placement")
	void StartPlacementMode(FPlacementBuildingData BuildingData);

	/** Клик ЛКМ. Фиксирует постройку и шлет RPC через контроллер */
	UFUNCTION(BlueprintCallable, Category = "RTS|Placement")
	void ConfirmPlacement(class APlayerController* PC);

	/** Клик ПКМ. Отменяет постройку. */
	UFUNCTION(BlueprintCallable, Category = "RTS|Placement")
	void CancelPlacement();

	/** Проверка: активен ли сейчас режим ведения постройки мыши? */
	UFUNCTION(BlueprintPure, Category = "RTS|Placement")
	bool IsPlacingActive() const { return bIsPlacingActive; }

private:
	/** Внутренний С++ калькулятор доступности точки по геометрии мира */
	bool CheckPlacementLegality(const FVector& TestLocation);

	/** Обновляет параметры материала фантома на клиенте (Зеленый / Красный) */
	void UpdatePreviewMaterials(bool bIsLegallyValid);

	/** Стейт-переменные менеджера подсистемы */
	bool bIsPlacingActive = false;
	bool bCurrentLocationIsValid = false;
	FPlacementBuildingData CurrentActiveBuildingData;

	UPROPERTY()
	TObjectPtr<AActor> CurrentLocalPreviewActor;
};
// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StrategyGridComponent.generated.h"

UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class VCELLSWAR_API UStrategyGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UStrategyGridComponent();
	virtual void BeginPlay() override;
	void ActivateGridTracking(const bool bCanMove = false);
	void DeactivateGridTracking();

protected:	

	// Воркер, который будет дергать подсистему секторов раз в секунду
	void ManageGridSectorUpdate();

	// Наш динамический С++ трекер движения. 
	// Мы будем проверять скорость компонента перемещения без привязки к тяжелому тику.
	void CheckMovementState();

private:
	// Ссылка на компонент движения владельца
	UPROPERTY()
	UCharacterMovementComponent* CachedMoveComp;

	// Хэндлы наших оптимизированных таймеров
	FTimerHandle GridUpdateTimerHandle;
	FTimerHandle MovementCheckTimerHandle;

	// Координаты сектора, в котором юнит стоял при прошлой проверке
	FIntPoint LastKnownSector;
};
// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyGridComponent.generated.h"

class UNavigationPath;

UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class VCELLSWAR_API UStrategyGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UStrategyGridComponent();

	void ActivateGridTracking();
	void DeactivateGridTracking();
	
	virtual void BeginPlay() override;
	
	void InitializeGridTracking();
	void DeinitializeGridTracking();

protected:	

	/** Основной секундный воркер обновления вокселей */
	void ManageGridSectorUpdate();

private:
	// Идентификатор циклического секундного таймера обновления секторов
	FTimerHandle GridUpdateTimerHandle;

	// Координаты сектора, в котором юнит находился при прошлой проверке
	FIntPoint LastKnownSector;
};
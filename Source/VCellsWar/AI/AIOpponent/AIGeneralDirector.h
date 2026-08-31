// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "AIGeneralDirector.generated.h"

/**
 * 
 */

UCLASS()
class VCELLSWAR_API AAIGeneralDirector : public AInfo
{
	GENERATED_BODY()

public:
	AAIGeneralDirector();

	virtual void BeginPlay() override;

	/** Военный модуль ИИ (управление солдатами в поле) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS AI | Components")
	class UAIWarComponent* WarComponent;

	/** Глобальный таймер макро-логики (2 секунды) */
	FTimerHandle MacroLogicTimerHandle;

	/** Главный цикл макро-анализа карты */
	void Server_AnalyzeBattlefield();

	// Будущие компоненты, которые вы добавите позже абсолютно безболезненно:
	// UPROPERTY(VisibleAnywhere, Category = "AI Components")
	// class UAIEconomyComponent* EconomyComponent;

	// UPROPERTY(VisibleAnywhere, Category = "AI Components")
	// class UAIBuildingComponent* BuildingComponent;
};
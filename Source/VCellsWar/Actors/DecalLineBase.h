// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DecalLineBase.generated.h"

UCLASS()
class VCELLSWAR_API ADecalLineBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ADecalLineBase();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Selection")
	class UDecalComponent* LineDecalComponent;

private:
	// Кэшированные настройки для быстрой работы SetParametrs
	float CachedThickness = 3.0f;
	float CachedLengthMultiplier = 1.0f;
	float CachedProjectionHeight = 500.0f;

public:	
	/**
	 * Вызывается один раз при инициализации линии (или при взятии из пула).
	 * Задает постоянные параметры внешнего вида.
	 */
	void InitLineSettings(
		UMaterialInterface* NewMaterial,
		float InThickness = 3.0f,
		float InLengthMultiplier = 1.0f,
		float InProjectionHeight = 500.0f
	);

	/**
	 * Быстро обновляет позицию и растягивает декаль между точками.
	 * Использует сохраненные ранее настройки толщины, высоты и коэффициента.
	 */
	void SetParametrs(const FVector& PointStart, const FVector& PointEnd);
	
	void RemoveDecal();
};

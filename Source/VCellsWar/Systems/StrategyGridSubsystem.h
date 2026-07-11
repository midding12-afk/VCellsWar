// Copyright (c) 2026, Dmitry Tur. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "StrategyGridSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FStrategyGridSector
{
	GENERATED_BODY()

	// Идеально плотный, непрерывный массив указателей на солдат в Куче ОЗУ.
	// 2000 указателей солдат = всего 16 КБ памяти, которые монолитно сидят в L3-кэше процессора!
	UPROPERTY()
	TArray<AActor*> FlatActorPointers;
};

USTRUCT(BlueprintType)
struct FPlayerGrid
{
	GENERATED_BODY()

	// Наша плоская хэш-карта секторов для конкретного игрока FFA (Поиск за O(1))
	UPROPERTY(BlueprintReadOnly, Category = "StrategyGrid")
	TMap<FIntPoint, FStrategyGridSector> Sectors;
};

UCLASS()
class VCELLSWAR_API UStrategyGridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Возврат двухмерных координат сектора на основе мировых координат X и Y
	UFUNCTION(BlueprintPure, Category = "StrategyGrid | Utility")
	FIntPoint GetSectorCoords(FVector Location) const;

	/** Регистрация при спавне/выходе из пула. Добавляет юнита в сектор за O(1) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void RegisterEntity(AActor* Entity);

	/** Разрегистрация при смерти/уходе в пул. Стирает юнита за O(1) без сдвигов памяти */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void UnregisterEntity(AActor* Entity);

	/** Обновление положения при движении. Вызываем периодически из тика/таймера чарактера */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void UpdateEntitySector(AActor* Entity);

	/** 🚀 ИСТИННЫЙ FFA-СКАНЕР ЗА O(1): Находит абсолютно случайного врага среди всех фракций без лагов */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	AActor* FindRandomEnemyEntityInRadius(AActor* ScanningEntity, float Radius);

private:
	// Вспомогательный метод получения ID фракции через интерфейс
	int32 GetFactionIdFromActor(AActor* Actor) const;

	// Наша основная трехмерная сетка фракций для FFA матчей до 8 игроков
	UPROPERTY()
	TMap<int32, FPlayerGrid> GlobalPlayersGrids;

	// Плоская карта-помощник для мгновенного поиска текущих координат сектора актера
	UPROPERTY()
	TMap<AActor*, FIntPoint> ActorToSectorMap;

	// Размер сектора на карте (3000 единиц = 30 метров)
	UPROPERTY()
	float SectorSize = 3000.f;
};

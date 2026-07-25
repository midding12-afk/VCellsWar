// Copyright (c) 2026, Dmitry Tur. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "StrategyGridSubsystem.generated.h"


UENUM(BlueprintType)
enum class EStrategyEntityCategory : uint8
{
	Troop         UMETA(DisplayName = "Мобильные войска (Пехота/Техника)"),
	StaticBuild   UMETA(DisplayName = "Производственные/Экономические здания"),
	Tower         UMETA(DisplayName = "Башни связи Делоне"),
	DefenseBuild  UMETA(DisplayName = "Оборонительные обитаемые турели/укрипления"),
	
	
	MAX           UMETA(Hidden) // Автоматически хранит общее количество категорий 
};

USTRUCT(BlueprintType)
struct FStrategyGridSector
{
	GENERATED_BODY()

	// Каждая категория сидит в своём собственном плотном векторе в L3-кэше процессора!
	UPROPERTY() TArray<AActor*> Troops;
	UPROPERTY() TArray<AActor*> StaticBuilds;
	UPROPERTY() TArray<AActor*> Towers;
	UPROPERTY() TArray<AActor*> DefenseBuilds;

	/** Легковесный хелпер для быстрого получения нужного массива по Enum */
	FORCEINLINE TArray<AActor*>& GetBucket(EStrategyEntityCategory Category)
	{
		switch (Category)
		{
			case EStrategyEntityCategory::Troop:        return Troops;
			case EStrategyEntityCategory::StaticBuild: return StaticBuilds;
			case EStrategyEntityCategory::Tower:       return Towers;
			case EStrategyEntityCategory::DefenseBuild: return DefenseBuilds;
		}
		return Troops; // Дефолтный безопасный фоллбэк
	}

	FORCEINLINE const TArray<AActor*>& GetBucket(EStrategyEntityCategory Category) const
	{
		switch (Category)
		{
			case EStrategyEntityCategory::Troop:        return Troops;
			case EStrategyEntityCategory::StaticBuild: return StaticBuilds;
			case EStrategyEntityCategory::Tower:       return Towers;
			case EStrategyEntityCategory::DefenseBuild: return DefenseBuilds;
		}
		return Troops;
	}
};

USTRUCT(BlueprintType)
struct FPlayerGrid
{
	GENERATED_BODY()

	// Наша плоская хэш-карта секторов для конкретного игрока FFA 
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

	/** Регистрация при спавне/выходе из пула. Распределяет юнита в нужный бакет за O(1) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void RegisterEntity(AActor* Entity);

	/** Разрегистрация при смерти/уходе в пул. Стирает юнита из бакета за O(1) через Swap-and-Pop */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void UnregisterEntity(AActor* Entity);

	/** Обновление положения при движении. Вызываем периодически из тика/таймера чарактера */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	void UpdateEntitySector(AActor* Entity);

	/** ИСТИННЫЙ FFA-СКАНЕР ЗА O(1): Ищет случайного врага ТОЛЬКО среди мобильных войск и турелей */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StrategyGrid | Operations")
	AActor* FindRandomEnemyEntityInRadius(AActor* ScanningEntity, float Radius);

	/**  Ищет случайного врага ТОЛЬКО конкретного типа. 
	 *  Если тип не указан (nullptr), собирает абсолютно любые вражеские объекты в радиусе кадра! 
	 */
	AActor* FindRandomEnemyEntityInRadiusByType(AActor* ScanningEntity, float Radius, const EStrategyEntityCategory* SpecificCategory = nullptr);

	/** ФИЛЬТР ДЛЯ ТАКТИЧЕСКИХ ФЛАГОВ: Выдергивает за 0 наносекунд только мобильные отряды своей фракции */
	UFUNCTION(BlueprintCallable, Category = "RTS|Grid", meta = (AutoCreateRefTerm = "Category", CPP_Default_Input = "EStrategyEntityCategory::Troop"))
	TArray<AActor*> FindAllAlliesInRadius(FVector CenterLocation, float Radius, uint8 FactionID, EStrategyEntityCategory Category = EStrategyEntityCategory::Troop);	

private:
	// Вспомогательный метод получения ID фракции через интерфейс
	int32 GetFactionIdFromActor(AActor* Actor) const;

	/** Вспомогательный метод безопасного извлечения категории из интерфейса сущности */
	EStrategyEntityCategory GetEntityCategoryFromActor(AActor* Actor) const;

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

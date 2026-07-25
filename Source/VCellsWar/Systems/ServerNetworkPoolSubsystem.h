// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ServerNetworkPoolSubsystem.generated.h"

/**
 * 
 */

// Вспомогательная структура для хранения массива спящих сетевых акторов конкретного класса
USTRUCT()
struct FNetworkActorPoolList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> InactiveActors;
};

UCLASS()
class VCELLSWAR_API UServerNetworkPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Подсистема создается строго на сервере
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** 
	 * ШАГ 1: УНИВЕРСАЛЬНЫЙ геттер. Принимает ЛЮБОЙ класс, унаследованный от AActor.
	 * meta = (DetermineOutputType = "ActorClass") автоматически изменит тип выходного пина в Блупринтах!
	 */
	UFUNCTION(BlueprintCallable, Category = "RTS | Server Network Pool", meta = (DetermineOutputType = "ActorClass"))
	AActor* GetActorFromNetworkPoolDeferred(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);
	
	UFUNCTION(BlueprintCallable, Category = "RTS | Server Network Pool", meta = (DetermineOutputType = "ActorClass"))
	AActor* GetActorFromNetworkPool(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, const int32 InFactionID = 255, AMainGamePlayerState* InOwnerState = nullptr);
	
	/**
	 * ШАГ 2: УНИВЕРСАЛЬНАЯ финализация спавна для любого актора.
	 * Включает видимость, коллизию и активирует компоненты.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTS | Server Network Pool")
	void FinishSpawningNetworkUnit(AActor* ActorToFinish, const FTransform& SpawnTransform);

	/**
	 * УНИВЕРСАЛЬНОЕ усыпление. Принимает любой AActor, гасит его сеть, физику и прячет в пул.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTS | Server Network Pool")
	void ReturnActorToNetworkPool(AActor* ActorToReturn);

protected:
	// Карта памяти сетевого пула сервера: Класс Актора -> Список его спящих копий
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FNetworkActorPoolList> NetworkObjectPoolMap;

private:
	// Вспомогательный метод для безопасного управления сетевыми флагами (без жесткого каста к чарактеру)
	void SetActorNetworkActive(AActor* TargetActor, bool bIsActive);
};

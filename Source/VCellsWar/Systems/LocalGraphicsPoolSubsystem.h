// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LocalGraphicsPoolSubsystem.generated.h"

USTRUCT()
struct FVisualActorPoolList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> InactiveActors;
};

UCLASS()
class VCELLSWAR_API ULocalGraphicsPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	
	UFUNCTION(BlueprintCallable, Category = "RTS | Visual Pool", meta = (DetermineOutputType = "ActorClass"))
	AActor* GetActorFromPool(TSubclassOf<AActor> ActorClass);

	
	UFUNCTION(BlueprintCallable, Category = "RTS | Visual Pool")
	void ReturnActorToPool(AActor* ActorToReturn);

protected:

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FVisualActorPoolList> ObjectPoolMap;
};

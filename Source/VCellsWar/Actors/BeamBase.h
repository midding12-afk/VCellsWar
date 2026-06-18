// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BeamBase.generated.h"

UCLASS()
class VCELLSWAR_API ABeamBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABeamBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite)
	FVector Start = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(BlueprintReadWrite)
	FVector End = FVector(0.0f, 0.0f, 0.0f);
	
	UPROPERTY(BlueprintReadWrite)
	float TowerHeight = 1950.f;
	
	UPROPERTY(BlueprintReadWrite)
	FLinearColor StartColor = FLinearColor(0.0f, 0.0f, 0.0f);
	UPROPERTY(BlueprintReadWrite)
	FLinearColor EndColor = FLinearColor(0.0f, 0.0f, 0.0f);
	
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateBeam();
};

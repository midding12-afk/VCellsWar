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
	// Sets default values for this actor's properties
	ADecalLineBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Selection")
	class UDecalComponent* LineDecalComponent;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetParametrs(const FVector& PointStart, const FVector& PointEnd);
};

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DecalMoveTargetBase.generated.h"

UCLASS()
class VCELLSWAR_API ADecalMoveTargetBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADecalMoveTargetBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Selection")
	class UDecalComponent* SelectionDecalComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

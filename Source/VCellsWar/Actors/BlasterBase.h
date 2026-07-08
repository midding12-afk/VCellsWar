// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "BlasterBase.generated.h"

UCLASS()
class VCELLSWAR_API ABlasterBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlasterBase();

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY()
	int32 OwnerFactionID = -1;
	
	//FVector VectorVelocity = FVector::Zero();
	
	
	// Метод серверного перехвата пересечений
	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UPROPERTY()
	USphereComponent* SphereComponent;
	
	UPROPERTY()
	UProjectileMovementComponent* ProjectileMovement;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void InitBlasterShoot(FVector StartLocation, FVector Direction, FLinearColor Color, int32 NewTeamId);
	
private:
	void ReturnToPool();
	
	FTimerHandle LifeTimerHandle;
};

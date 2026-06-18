// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "BeamBase.h"

// Sets default values
ABeamBase::ABeamBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ABeamBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABeamBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


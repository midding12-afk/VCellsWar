// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TowerBase.h"

ATowerBase::ATowerBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATowerBase::BeginPlay()
{
	Super::BeginPlay();
}

void ATowerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "PortalBase.h"

#include "Net/UnrealNetwork.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"

// Sets default values
APortalBase::APortalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void APortalBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APortalBase, NextSpawnTime);
	DOREPLIFETIME(APortalBase, PortalId);
}

// Called when the game starts or when spawned
void APortalBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
		
		if (GM)
		{
			GM->RegisterPortal(this);
		}
	}
}

// Called every frame
void APortalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortalBase::Server_SetNextSpawnDelay(float DelaySeconds)
{
	if (HasAuthority())
	{
		// Берем точную текущую секунду сервера и прибавляем задержку
		NextSpawnTime = GetWorld()->GetTimeSeconds() + DelaySeconds;
		
		// На хосте вызываем OnRep вручную для обновления локального UI
		OnRep_NextSpawnTime();
	}
}

void APortalBase::OnRep_NextSpawnTime()
{
	// Сеть доставила клиенту точную секунду спавна.
	// Здесь можно просто пнуть HUD, чтобы он обновил текст таймера.
	
	//GetServerWorldTimeSeconds();
}



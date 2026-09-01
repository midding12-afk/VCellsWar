// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "PortalBase.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "TroopBase.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "VCellsWar/AI/StrategyAIController.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"
#include "VCellsWar/GameMods/MainGameGameState.h"
#include "VCellsWar/AI/AIOpponent/AIGeneralDirector.h"
#include "VCellsWar/AI/AIOpponent/AIWarComponent.h"
#include "VCellsWar/GameMods/MainGamePlayerController.h"

// Sets default values
APortalBase::APortalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SelectionDecalComponent->DecalSize = FVector(100.0f, 900.0f, 900.0f);
	
	
	
	// Шаг 2: Создаем и настраиваем гигантскую капсулу коллизии башни
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("PortalSphereCollision"));
	
	// Привязываем капсулу к нашему новому корню-хотспоту
	SphereComponent->SetupAttachment(RootComponent);
	
	SphereComponent->SetSphereRadius(450.0f);
	
	SphereComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SphereComponent->CanCharacterStepUpOn = ECB_No;
	SphereComponent->SetShouldUpdatePhysicsVolume(true);
	SphereComponent->SetCanEverAffectNavigation(false);
	SphereComponent->bDynamicObstacle = true;

	SphereComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -200));
	
}

void APortalBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APortalBase, PortalId);
}

// Called when the game starts or when spawned
void APortalBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AMainGamePlayerState* PS = Execute_GetEntityOwnerState(this);
		APlayerController* PC = PS->GetPlayerController();
		if (PC)
		{
			if (AMainGamePlayerController* MPC = Cast<AMainGamePlayerController>(PC))
			{
				if (MPC->EnemyAiDirector)
					EnemyAiDirector = MPC->EnemyAiDirector;
			}
		}
		
		AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
		
		if (GM)
		{
			GM->RegisterPortal(this);
		}
		
		ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
		
		ExecuteWaveSpawn(10);
	}
	else
	{
		AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->InvocLinksUpdate();
		}		
	}
	
}

// Called every frame
void APortalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortalBase::ExecuteWaveSpawn(int32 TroopsCount)
{
	if (!HasAuthority() || !SoldierClass) return;

	// Получаем нашу изолированную серверную подсистему пула объектов
	//UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	
	if (ServerPool)
	{
		AMainGamePlayerState* PS = Execute_GetEntityOwnerState(this);
		APlayerController* PC = PS->GetPlayerController();
		if (!PS || !PC) return;
		
		FVector ForwardVec = GetActorForwardVector();
		FVector SpawnLocation = GetActorLocation();
		
		int32 Count = TroopsCount;	
		for (int Index = 0; Index < Count; Index++)
		{
			float AngleDegrees = 360.f/Count * Index;
			
			FRotator RotationAroundZ(0.0f, AngleDegrees, 0.0f);
			
			FVector RotatedForwardVec = RotationAroundZ.RotateVector(ForwardVec);
		
			FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation+RotatedForwardVec*150.f, FVector(1.0f, 1.0f, 1.0f));
			
			AActor* SpawnedActor = ServerPool->GetActorFromNetworkPool(SoldierClass, SpawnTransform, FGenericTeamId(GetGenericTeamId()), PS);
			ATroopBase* Soldier = Cast<ATroopBase>(SpawnedActor);
            
			if (Soldier)
			{
				// Перезаписываем финальные параметры поверх инициализированной памяти     
				Soldier->SetOwner(PC);
				
				if (AAIController* AIC = Cast<AAIController>(Soldier->GetController()))
				{
					AIC->SetOwner(PC);
				}
    
				// Выталкиваем полностью готового к баллистическому полету юнита из ворот!
				Soldier->LaunchFromPortal(RotatedForwardVec);
				
				if (EnemyAiDirector && EnemyAiDirector->WarComponent)
				{
					if (!LocalPortalSquad.IsValid()) LocalPortalSquad = EnemyAiDirector->WarComponent->CreateNewSquad(EAISquadRole::PortalSquad, GetActorLocation());
					
					if (LocalPortalSquad.IsValid())
					{
						LocalPortalSquad->AddMember(Soldier);
					}
				}
			}
		}
	}	

}





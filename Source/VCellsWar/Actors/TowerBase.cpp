// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "TowerBase.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/GameMods/MainGameGameModeBase.h"
#include "VCellsWar/GameMods/MainGameGameState.h"

ATowerBase::ATowerBase()
{
	PrimaryActorTick.bCanEverTick = false;
	

	
	// Шаг 2: Создаем и настраиваем гигантскую капсулу коллизии башни
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCylinder"));
	
	// Привязываем капсулу к нашему новому корню-хотспоту
	CapsuleComponent->SetupAttachment(RootComponent);

	float HalfHeight = 88.0f * 12.0f; 
	CapsuleComponent->InitCapsuleSize(34.0f * 7.0f, HalfHeight);
	
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;

	CapsuleComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HalfHeight));
	
	
	SelectionDecalComponent->DecalSize = FVector(100.0f, 900.0f, 900.0f);
	
	
	
	// Создаем невидимую плоскую коробку строго под габариты фундамента башни!
	UBoxComponent* NavObstacleBox = CreateDefaultSubobject<UBoxComponent>(TEXT("NavObstacleBox"));
	NavObstacleBox->SetupAttachment(RootComponent);
	
	// Выставляем размер коробки-заглушки (например, квадрат 1.2 x 1.2 метра, высота 2 метра)
	NavObstacleBox->SetBoxExtent(FVector(350.0f, 350.0f, 100.0f));
	NavObstacleBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	// Настраиваем её физический профиль как Obstacle
	NavObstacleBox->SetCollisionProfileName(TEXT("Obstacle"));
	NavObstacleBox->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly); // Нужна только для преграды пути

	// Приказываем этой коробке динамически прорезать идеальную квадратную 
	// дыру в ландшафте NavMesh прямо под собой! 
	NavObstacleBox->SetCanEverAffectNavigation(true);
	NavObstacleBox->bDynamicObstacle = true;
}

void ATowerBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AMainGameGameModeBase* GM = GetWorld()->GetAuthGameMode<AMainGameGameModeBase>();
		
		if (GM)
		{
			GM->RegisterTower(this);
		}
	}
	else
	{
		AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
		if (GS)
		{
			GS->InvocLinksUpdate();
		}		
	}
	
	
	
	/*FTimerHandle TestSelectionTimerHandle;
	GetWorldTimerManager().SetTimer(
		TestSelectionTimerHandle, 
		this, 
		&ATowerBase::SelectEntity, 
		1.1f,                                  
		false                                  
	);*/
}

void ATowerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATowerBase, TowerId);
}

void ATowerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// void ATowerBase::Server_SetTowerId(int32 NewTowerId)
// {
// 	if (HasAuthority())
// 	{
// 		TowerId = NewTowerId;
// 	}
// }

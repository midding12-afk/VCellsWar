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
	
	UStaticMeshComponent* BlueprintTowerMesh = FindComponentByClass<UStaticMeshComponent>();

	if (BlueprintTowerMesh)
	{

		// СОЗДАЕМ ДИНАМИЧЕСКИЙ МАТЕРИАЛ ДЛЯ ПРОЦЕДУРНЫХ ШКАЛ ХП
		// Превращаем дефолтный материал блупринт-меша в динамический экземпляр
		 DynamicTowerMaterial = BlueprintTowerMesh->CreateAndSetMaterialInstanceDynamic(0);

		if (DynamicTowerMaterial)
		{
			// Выставляем стартовые неоновые параметры шкалы высоты Z
			DynamicTowerMaterial->SetScalarParameterValue(TEXT("HealthProgress"), (float)HealthBarProgress / 255.0f);
			
			if (AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState()))
				DynamicTowerMaterial->SetVectorParameterValue(TEXT("HealthBarColor"), GS->GetTeamColor(HealthBarTeamIDColor));
			else
				DynamicTowerMaterial->SetVectorParameterValue(TEXT("HealthBarColor"), FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
		}
	}

}

void ATowerBase::OnRep_HealthBarProgress()
{
	if (DynamicTowerMaterial)
	{
		DynamicTowerMaterial->SetScalarParameterValue(TEXT("HealthProgress"), (float)HealthBarProgress / 255.0f);
	}
}

void ATowerBase::OnRep_HealthBarTeamIDColor()
{
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	if (GS && DynamicTowerMaterial)
	{		
		DynamicTowerMaterial->SetVectorParameterValue(TEXT("HealthBarColor"), GS->GetTeamColor(HealthBarTeamIDColor));
	}
}

void ATowerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATowerBase, TowerId);
	DOREPLIFETIME(ATowerBase, HealthBarProgress);
	DOREPLIFETIME(ATowerBase, HealthBarTeamIDColor);
}

void ATowerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATowerBase::GeinDamage(float Damage, int32 InstigatorTeamID)
{
	if (!HasAuthority()) return;
	Super::GeinDamage(Damage, InstigatorTeamID);
	
	if (GetGenericTeamId() == InstigatorTeamID) return;
	
	float MaxHealth = MaxHealthWithoutOwner; //TODO + from gas or upgrades
	
	if (OwningPlayerState)
	{
		CurrentOwningProgressInHealthPoint -= Damage;
		
		if (CurrentOwningProgressInHealthPoint <= 0)
		{
			HealthBarTeamIDColor = 255;
			SetEntityOwner(nullptr);
			SetOwner(nullptr);
			CurrentOwningProgressInHealthPoint = 0;
			OnRep_HealthBarTeamIDColor();
			AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
			if (GS)
			{
				GS->Multicast_InvocLinksUpdate();
			}	
		}			
	}
	else
	{
		if (HealthBarTeamIDColor == InstigatorTeamID)
		{
			CurrentOwningProgressInHealthPoint += Damage;
			
			if (CurrentOwningProgressInHealthPoint >= MaxHealth)
			{
				CurrentOwningProgressInHealthPoint = MaxHealth;
				
				AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
				if (GS)
				{
					//HealthBarTeamIDColor=InstigatorTeamID;
					
					AMainGamePlayerState* PS = GS->GetPlayerState(InstigatorTeamID);
					if (!PS) return;
					SetEntityOwner(PS);
					APlayerController* PC = PS->GetPlayerController();
					SetOwner(PC);
					OnRep_HealthBarTeamIDColor();

					if (GS)
					{
						GS->Multicast_InvocLinksUpdate();
					}	
				}					
			}
		}
		else
		{
			
			CurrentOwningProgressInHealthPoint -= Damage;
		
			if (CurrentOwningProgressInHealthPoint < 0)
			{
				CurrentOwningProgressInHealthPoint = 0;
				HealthBarTeamIDColor=InstigatorTeamID;
				OnRep_HealthBarTeamIDColor();
			}
		}
	}
	
	HealthBarProgress = FMath::Clamp(FMath::RoundToInt(CurrentOwningProgressInHealthPoint/MaxHealth * 255.f),0,255);
	OnRep_HealthBarProgress();	
}

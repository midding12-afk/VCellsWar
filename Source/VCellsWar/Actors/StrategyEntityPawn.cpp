// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityPawn.h"

#include "GenericTeamAgentInterface.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/RTSVisualSettings.h"

// Sets default values
AStrategyEntityPawn::AStrategyEntityPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Включаем сетевую репликацию для объекта!
	bReplicates = true;
	bAlwaysRelevant = true; 
	SetReplicateMovement(true); // Юниты будут двигаться, постройкам можно выключить в наследниках
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	OwningPlayerState = nullptr;
	
	USceneComponent* DummyRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
	RootComponent = DummyRootComponent;	
	
	
	// Создаем компонент декали выделения
	SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
	SelectionDecalComponent->SetupAttachment(RootComponent);

	// Разворачиваем декаль строго вертикально вниз (лицом к земле)
	SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// Настраиваем 3D-размер проекционного куба декали (X - глубина луча, Y и Z - радиус круга)
	// Размер 64, 45, 45 идеально накроет землю под стандартной капсулой
	SelectionDecalComponent->DecalSize = FVector(64.0f, 90.0f, 90.0f);
	
	SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0));

	// По умолчанию кольцо полностью выключено и скрыто в игре
	SelectionDecalComponent->SetVisibility(false);
	SelectionDecalComponent->SetHiddenInGame(true);
}

// Called when the game starts or when spawned
void AStrategyEntityPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStrategyEntityPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AStrategyEntityPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AStrategyEntityPawn, OwningPlayerState);
}

void AStrategyEntityPawn::OnRep_OwningPlayerState()
{
}

void AStrategyEntityPawn::OnRep_OwningPlayerColor()
{
}

void AStrategyEntityPawn::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}

FGenericTeamId AStrategyEntityPawn::GetGenericTeamId() const
{
	// Если у солдата есть контроллер ИИ — забираем ID команды у него
	if (IGenericTeamAgentInterface* TeamController = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamController->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void AStrategyEntityPawn::SelectEntity()
{
	if (SelectionDecalComponent)
	{
		const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
		if (!Settings) return;
	
		UMaterialInterface* DecalMaterial = Settings->SelectionDecalMaterial.LoadSynchronous();
		
		// Если материал кольца задан — принудительно накатываем его на декаль
		if (DecalMaterial && SelectionDecalComponent->GetDecalMaterial() == nullptr)
		{
			SelectionDecalComponent->SetDecalMaterial(DecalMaterial);
		}

		// Будим и показываем зеленое кольцо под ногами!
		SelectionDecalComponent->SetVisibility(true);
		SelectionDecalComponent->SetHiddenInGame(false);
	}
}

void AStrategyEntityPawn::DeselectEntity()
{
	if (SelectionDecalComponent)
	{
		// Полностью тушим и скрываем кольцо, когда игрок сбросил выделение
		SelectionDecalComponent->SetVisibility(false);
		SelectionDecalComponent->SetHiddenInGame(true);
	}
}

bool AStrategyEntityPawn::NativeRTSIsEntitySelected() const
{
	return SelectionDecalComponent && SelectionDecalComponent->IsVisible();
}


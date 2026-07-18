// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityBase.h"

#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Systems/RTSMinimapSubsystem.h"


AStrategyEntityBase::AStrategyEntityBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Включаем сетевую репликацию для объекта!
	bReplicates = true;
	bAlwaysRelevant = true; 
	//SetReplicateMovement(true); // Юниты будут двигаться, постройкам можно выключить в наследниках
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	//AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
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

void AStrategyEntityBase::BeginPlay()
{
	Super::BeginPlay();
	
}


void AStrategyEntityBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Синхронизируем владельца со всеми клиентами в игре
	DOREPLIFETIME(AStrategyEntityBase, OwningPlayerState);
}

void AStrategyEntityBase::NativeRTSInitialize(int32 InFactionID, AMainGamePlayerState* InOwnerState, const FTransform& InSpawnTransform)
{
	// 1. Запекаем базовые RTS параметры памяти
	SetEntityOwner(InOwnerState);
	SetGenericTeamId(FGenericTeamId(InFactionID));
	
	bReplicates = true;
	bAlwaysRelevant = true; 
	
	
	// 3. СИНХРОННАЯ ТЕЛЕПОРТАЦИЯ ФИЗИКИ CHAOS
	// Так как коллизия уже включена строчкой выше, метод обновит матрицы Chaos со 100% точностью 
	SetActorTransform(InSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	
	URTSMinimapSubsystem* Minimap = GetWorld()->GetSubsystem<URTSMinimapSubsystem>();
	if (Minimap)
	{
		Minimap->RegisterEntity(this);
	}
}

void AStrategyEntityBase::NativeRTSDeinitialize()
{	
	URTSMinimapSubsystem* Minimap = GetWorld()->GetSubsystem<URTSMinimapSubsystem>();
	if (Minimap)
	{
		Minimap->UnregisterEntity(this);
	}
}

void AStrategyEntityBase::OnRep_OwningPlayerState()
{
	if (OwningPlayerState)
	{
		
	}
}

void AStrategyEntityBase::OnRep_OwningPlayerColor()
{
	
}

void AStrategyEntityBase::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}

void AStrategyEntityBase::SelectEntity()
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

void AStrategyEntityBase::DeselectEntity()
{
	if (SelectionDecalComponent)
	{
		// Полностью тушим и скрываем кольцо, когда игрок сбросил выделение
		SelectionDecalComponent->SetVisibility(false);
		SelectionDecalComponent->SetHiddenInGame(true);
	}
}

bool AStrategyEntityBase::NativeRTSIsEntitySelected() const
{
	return SelectionDecalComponent && SelectionDecalComponent->IsVisible();
}


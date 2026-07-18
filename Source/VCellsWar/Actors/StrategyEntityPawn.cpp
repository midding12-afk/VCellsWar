// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityPawn.h"

#include "GenericTeamAgentInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Components/RTSPathVisualizerComponent.h"
#include "VCellsWar/Components/StrategyGridComponent.h"
#include "VCellsWar/Systems/RTSMinimapSubsystem.h"

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
	
	PathVisualizerComponent = CreateDefaultSubobject<URTSPathVisualizerComponent>(TEXT("PathVisualizerComponent"));
	
	if (!GetWorld()) return;
	if (GetWorld()->GetNetMode() == NM_Client) return;
	if (HasAuthority()) return;
	GridTrackingComponent = CreateDefaultSubobject<UStrategyGridComponent>(TEXT("GridTrackingComponent"));
	
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

void AStrategyEntityPawn::NativeRTSInitialize(int32 InFactionID, AMainGamePlayerState* InOwnerState, const FTransform& InSpawnTransform)
{
	// 1. Запекаем базовые RTS параметры памяти
	SetEntityOwner(InOwnerState);
	SetGenericTeamId(FGenericTeamId(InFactionID));
	
	bReplicates = true;
	bAlwaysRelevant = true; 
	
	if (UCapsuleComponent* Capsule = GetComponentByClass<UCapsuleComponent>())
	{
		// Включаем полный физический просчет QueryAndPhysics
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					
		// Выставляем жесткий текстовый профиль Custom, чтобы движок разрешил ручную матрицу флажков 
		Capsule->SetCollisionProfileName(TEXT("Custom")); 
		Capsule->SetCollisionObjectType(ECC_Pawn); 

		// Выставляем флажки ответов СТРОГО по вашей картинке-скриншоту:
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);

		// Trace Responses (Ответы на трассировку):
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);  // Visibility = Ignore
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);       // Camera = Block

		// Object Responses (Ответы на типы объектов):
		Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);   // Ландшафт намертво заблокирован! 
		Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // Оверлап для нашей серверной пули!
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);         // Оверлап для других солдат
		Capsule->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);   // Оверлап для снарядов PhysicsBody
		Capsule->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);       // Техника = Block
		Capsule->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);  // Разрушаемые объекты = Block

		// Задаем физический RTS размер капсулы, чтобы они не сливались в матрёшку 
		//Capsule->SetCapsuleSize(35.0f, 88.0f, true);

		Capsule->SetGenerateOverlapEvents(true);
					
		// Пинок для Chaos, чтобы он мгновенно обновил матрицы коллизий в памяти
		Capsule->UpdateOverlaps(); 
	}
	
	// 3. СИНХРОННАЯ ТЕЛЕПОРТАЦИЯ ФИЗИКИ CHAOS
	// Так как коллизия уже включена строчкой выше, метод обновит матрицы Chaos со 100% точностью 
	SetActorTransform(InSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	// 4. ВСЕЛЯЕМ ИИ-МОЗГИ НА СЕРВЕРЕ
	// ИИ вселится строго в правильной точке ворот портала в Chaos, без создания зомби-клонов 
	if (HasAuthority() && GetController() == nullptr)
	{
		SpawnDefaultController();
	}

	if (GridTrackingComponent)
	{
		// Просыпаемся! Юнит снова на радарах сетки, таймеры взведены
		GridTrackingComponent->InitializeGridTracking(); 
	}
	
	URTSMinimapSubsystem* Minimap = GetWorld()->GetSubsystem<URTSMinimapSubsystem>();
	if (Minimap)
	{
		Minimap->RegisterEntity(this);
	}
}

void AStrategyEntityPawn::NativeRTSDeinitialize()
{
	// В самом конце метода NativeRTSInitialize:
	if (GridTrackingComponent)
	{
		// Просыпаемся! Юнит снова на радарах сетки, таймеры взведены
		GridTrackingComponent->DeinitializeGridTracking();
	}
	
	URTSMinimapSubsystem* Minimap = GetWorld()->GetSubsystem<URTSMinimapSubsystem>();
	if (Minimap)
	{
		Minimap->UnregisterEntity(this);
	}
}

void AStrategyEntityPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AStrategyEntityPawn, OwningPlayerState);
	
	DOREPLIFETIME(AStrategyEntityPawn, PathVisualizerComponent);
}

void AStrategyEntityPawn::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	IGenericTeamAgentInterface::SetGenericTeamId(NewTeamID);
	
	CachedFactionID = NewTeamID.GetId();
	
	// 3. Синхронизируем ИИ-контроллер, если он уже привязан к нам
	if (AController* AIC = GetController())
	{
		if (IGenericTeamAgentInterface* TeamController = Cast<IGenericTeamAgentInterface>(AIC))
		{
			TeamController->SetGenericTeamId(NewTeamID);
		}
	}
}

void AStrategyEntityPawn::OnRep_OwningPlayerState()
{
	if (UObject* AsUObject = Cast<UObject>(this))
	{
		Execute_OnOwnerChanged(AsUObject, OwningPlayerState);
	}
}

void AStrategyEntityPawn::OnRep_OwningPlayerColor()
{
	// if (UObject* AsUObject = Cast<UObject>(this))
	// {
	// 	Execute_OnOwnerChanged(AsUObject, OwningPlayerState);
	// }
}


void AStrategyEntityPawn::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{	
	OwningPlayerState = NewOwnerState;

	OwningPlayerColor = OwningPlayerState ? OwningPlayerState->GetTeamColor(): FLinearColor::Gray;
	
	GridTrackingComponent->DeinitializeGridTracking();
	
	SetGenericTeamId(NewOwnerState ? NewOwnerState->GetGenericTeamId() : FGenericTeamId(254));
	
	GridTrackingComponent->InitializeGridTracking();
}

FGenericTeamId AStrategyEntityPawn::GetGenericTeamId() const
{
	/*// Если у солдата есть контроллер ИИ — забираем ID команды у него
	if (IGenericTeamAgentInterface* TeamController = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamController->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;*/
	return CachedFactionID;
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
	
	if (PathVisualizerComponent)
	{
		PathVisualizerComponent->SetNewMoveDestination(FVector::Zero());
	}
}

bool AStrategyEntityPawn::NativeRTSIsEntitySelected() const
{
	return SelectionDecalComponent && SelectionDecalComponent->IsVisible();
}


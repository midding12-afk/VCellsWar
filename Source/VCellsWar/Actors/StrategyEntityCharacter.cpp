// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityCharacter.h"

#include "GenericTeamAgentInterface.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/DecalComponent.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Components/RTSPathVisualizerComponent.h"
#include "VCellsWar/Components/StrategyGridComponent.h"
#include "VCellsWar/Systems/RTSMinimapSubsystem.h"

// Sets default values
AStrategyEntityCharacter::AStrategyEntityCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	OwningPlayerState = nullptr;
		
	
	// Создаем компонент декали выделения
	SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
	SelectionDecalComponent->SetupAttachment(RootComponent);

	// Разворачиваем декаль строго вертикально вниз (лицом к земле)
	SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// Настраиваем 3D-размер проекционного куба декали (X - глубина луча, Y и Z - радиус круга)
	// Размер 64, 45, 45 идеально накроет землю под стандартной капсулой
	SelectionDecalComponent->DecalSize = FVector(64.0f, 90.0f, 90.0f);
	
	if (UCapsuleComponent* RootCapsule = GetCapsuleComponent())
	{
		float HalfHeight = RootCapsule->GetUnscaledCapsuleHalfHeight();
		SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight));
	}

	// По умолчанию кольцо полностью выключено и скрыто в игре
	SelectionDecalComponent->SetVisibility(false);
	SelectionDecalComponent->SetHiddenInGame(true);
	
	GridTrackingComponent = CreateDefaultSubobject<UStrategyGridComponent>(TEXT("GridTrackingComponent"));
	
	PathVisualizerComponent = CreateDefaultSubobject<URTSPathVisualizerComponent>(TEXT("PathVisualizerComponent"));
	
	// Страховка: принудительно выключаем его нативный тяжелый Tick, 
	// так как наш компонент работает на сверхскоростных изолированных таймерах! [1.5]
	if (GridTrackingComponent)
	{
		GridTrackingComponent->PrimaryComponentTick.bCanEverTick = false;
		
	}
		
}

// Called when the game starts or when spawned
void AStrategyEntityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	InitCharacter();
	
	/*FTimerHandle TestSelectionTimerHandle;
	GetWorldTimerManager().SetTimer(
		TestSelectionTimerHandle, 
		this, 
		&AStrategyEntityCharacter::SelectEntity, 
		0.1f,                                  
		false                                  
	);*/
}

void AStrategyEntityCharacter::InitCharacter()
{
	// 1. СЕТЕВЫЕ СТАНДАРТЫ
	bReplicates = true;
	bAlwaysRelevant = true; 
	
	// 2. МАТРИЦА КОЛЛИЗИЙ С КОРНЕВОЙ КАПСУЛЫ (СТРОГО ПО ВАШЕМУ СКРИНШОТУ)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
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

	// 3. ГРАФИЧЕСКИЙ ВИЗУАЛ (Встроенный Скелетный Меш)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Меш призрачный для пуль 
		MeshComp->SetGenerateOverlapEvents(false);
					
		// Срываем блупринтовые оптимизации сна анимаций при выходе из пула 
		MeshComp->SetVisibility(true, true);
		MeshComp->bHiddenInGame = false;
		
		
		MeshComp->bReceivesDecals = false; 
		
	}
	
	// СБРОС ВЫДЕЛЕНИЯ ПРИ ПЕРЕРОЖДЕНИИ:
	// Гарантирует, что воскресший из пула солдат выйдет из ворот со скрытым кольцом!
	if (SelectionDecalComponent)
	{
		SelectionDecalComponent->SetVisibility(false);
		SelectionDecalComponent->SetHiddenInGame(true);
	}
	
	
}

void AStrategyEntityCharacter::NativeRTSInitialize(int32 InFactionID, AMainGamePlayerState* InOwnerState, const FTransform& InSpawnTransform)
{
	// 4. ВСЕЛЯЕМ ИИ-МОЗГИ НА СЕРВЕРЕ
	// ИИ вселится строго в правильной точке ворот портала в Chaos, без создания зомби-клонов 
	if (HasAuthority() && GetController() == nullptr)
	{
		SpawnDefaultController();
	}
	
	// 1. Запекаем базовые RTS параметры памяти
	SetEntityOwner(InOwnerState);
	SetGenericTeamId(FGenericTeamId(InFactionID));
	
	SpawnGeneration++;

	// 2. ВЫЗЫВАЕМ НАШ ИСПРАВЛЕННЫЙ МЕТОД:
	// Капсула мгновенно получает статус QueryAndPhysics и Custom-ответы каналов.
	// Движок Chaos снова полноценно видит это тело в мире сервера! 
	InitCharacter();

	// 3. СИНХРОННАЯ ТЕЛЕПОРТАЦИЯ ФИЗИКИ CHAOS
	// Так как коллизия уже включена строчкой выше, метод обновит матрицы Chaos со 100% точностью 
	SetActorTransform(InSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	
	// 5. НАСТРОЙКА КОМПОНЕНТА ДВИЖЕНИЯ ПОСЛЕ ВСЕЛЕНИЯ ИИ
	if (UCharacterMovementComponent* CharMoveComp = GetCharacterMovement())
	{
		CharMoveComp->SetActive(true);

		// Полностью стираем инерцию и скорости прошлых жизней юнита из пула
		CharMoveComp->Velocity = FVector::ZeroVector;
		CharMoveComp->ClearAccumulatedForces();

		// Ставим режим падения, так как портал вытолкнет их вверх и под углом.
		// В момент удара о землю сработает наш отрефакторенный метод Landed() и приклеит ноги к NavMesh! 
		CharMoveComp->SetMovementMode(MOVE_Falling);
		
		// Принудительно перерегистрируем сам компонент движения, чтобы он мгновенно 
		// активировал свои коллизии Chaos и навигацию до применения LaunchFromPortal! 
		CharMoveComp->ReregisterComponent();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->UpdateOverlaps(); // Финальный пинок триггеров в новой точке пространства
	}
	
	// В самом конце метода NativeRTSInitialize:
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

void AStrategyEntityCharacter::NativeRTSDeinitialize()
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


// Called every frame
void AStrategyEntityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if (HasAuthority()) 
	//	GEngine->AddOnScreenDebugMessage(uint64(this), 0.0f, FColor::White, *FString::Printf(TEXT("I am ID: %d at %s"), CachedFactionID, *GetActorLocation().ToString()));
}

void AStrategyEntityCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AStrategyEntityCharacter, OwningPlayerState);
	
	//DOREPLIFETIME(AStrategyEntityCharacter, CachedFactionID);
	
	DOREPLIFETIME(AStrategyEntityCharacter, PathVisualizerComponent);
}

void AStrategyEntityCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
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

void AStrategyEntityCharacter::LaunchFromPortal(FVector PortalForwardDirection)
{
	if (!HasAuthority()) return;

	// Вычисляем вектор броска: берем направление портала вперед 
	// и добавляем мощный импульс вверх (ось Z)
	FVector LaunchVelocity = (PortalForwardDirection * 500.0f) + FVector(0.0f, 0.0f, 700.0f);

	// Встроенная функция CharacterMovement, которая подбросит человечка.
	// Движок автоматически продублирует этот бросок на экранах всех клиентов, плавно сгладив полет!
	LaunchCharacter(LaunchVelocity, false, false);
}


void AStrategyEntityCharacter::OnRep_OwningPlayerState()
{
	
}

// void AStrategyEntityCharacter::OnRep_OwningPlayerColor()
// {
// }

void AStrategyEntityCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// ЭТОТ МОМЕНТ СРАБОТАЕТ, КОГДА ЧЕЛОВЕЧЕК КАСНЕТСЯ ЗЕМЛИ
	if (HasAuthority())
	{

		// --- ФИНАЛЬНАЯ RTS ОПТИМИЗАЦИЯ ---
		// Раз наш бумажный человечек уже на земле и не будет прыгать/падать во время боя,
		// мы можем отключить постоянную проверку падения, переключив его в плоский режим навигации.
		UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (MoveComp)
		{
			// Переключаем в режим ходьбы по NavMesh, отключая просчет капризной трехмерной физики пола
			MoveComp->SetMovementMode(MOVE_NavWalking);
			
			MoveComp->Velocity.Z = 0.0f;
		}
	}
}

void AStrategyEntityCharacter::OnRep_SpawnGeneration()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// САМАЯ ВАЖНАЯ СТРОЧКА: 
		// Очищает буфер старых координат интерполяции. Движок полностью стирает из памяти клиента 
		// информацию о том, где этот солдат бегал в своей "прошлой жизни".
		MoveComp->ResetPredictionData_Client(); 
		
		// Обнуляем локальную скорость компонента движения на клиенте, чтобы его не уносило по старой инерции
		MoveComp->Velocity = FVector::ZeroVector;
	}
	
	// Только ПОСЛЕ очистки истории мы безопасно показываем меш солдата игроку
	SetActorHiddenInGame(false);
}

void AStrategyEntityCharacter::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}


FGenericTeamId AStrategyEntityCharacter::GetGenericTeamId() const
{

	return CachedFactionID;
}

void AStrategyEntityCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Как только контроллер привязался к телу на сервере:
	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(NewController))
	{
		// Запоминаем ID фракции прямо в плоскую память тела солдата!
		CachedFactionID = TeamAgent->GetGenericTeamId().GetId();
	}
}

void AStrategyEntityCharacter::SelectEntity()
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

void AStrategyEntityCharacter::DeselectEntity()
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

bool AStrategyEntityCharacter::NativeRTSIsEntitySelected() const
{
	return SelectionDecalComponent && SelectionDecalComponent->IsVisible();
}


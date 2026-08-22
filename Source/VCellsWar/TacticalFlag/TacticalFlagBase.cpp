// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "TacticalFlagBase.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"
#include "StrategyFlagUiInterface.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Actors/DecalLineBase.h"
#include "VCellsWar/Actors/TroopBase.h"
#include "VCellsWar/GameMods/MainGamePlayerController.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "VCellsWar/Systems/FlagsManagerSubsystem.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"
#include "VCellsWar/Systems/StrategyGridSubsystem.h"

ATacticalFlagBase::ATacticalFlagBase()
{
	// Нам больше НЕ НУЖЕН тяжелый Tick актора! Всё перенесено на секундный таймер!
	PrimaryActorTick.bCanEverTick = true;
	
	// Важнейший RTS-шаг оптимизации: по умолчанию усыпляем его при старте, 
	// чтобы он не тратил такты до проведения сетевых проверок!
	PrimaryActorTick.bStartWithTickEnabled = false; 
	bReplicates = true;
	SetReplicateMovement(true);
	

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	RootComponent = RootComp;

	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetupAttachment(RootComp);
	FlagMesh->SetCollisionProfileName(TEXT("Architecture"));

	// Настраиваем ААА-декаль сонара
	RadiusDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("RadiusDecal"));
	RadiusDecal->SetupAttachment(RootComp);
	// Разворачиваем декаль строго вниз, чтобы она правильно проецировалась на ландшафт
	//RadiusDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	LightBeamNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LightBeamNiagara"));
	LightBeamNiagara->SetupAttachment(RootComp);

	
	// НАСТРОЙКА ИНТЕРАКТИВНОГО 3D UI НАД ФЛАГОМ
	InfoWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InfoWidgetComponent"));
	InfoWidgetComponent->SetupAttachment(RootComp);
	InfoWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 10000.0f));
	InfoWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	
	// 1. НАМЕРТВО ВКЛЮЧАЕМ АВТО-МАСШТАБ ПОД РАЗМЕР ВЕРСТКИ
	// (Заставляет компонент прочитать Desired Size из твоего Блупринт-виджета)
	InfoWidgetComponent->SetDrawAtDesiredSize(true);

	// 2. БРОНЕБОЙНЫЙ ААА-ФИКС ДЛЯ SCREEN SPACE:
	// Если авто-масштаб сбоит из-за пустых контейнеров, мы выставляем 
	// жесткие дефолтные габариты холста (например, 300 на 150 пикселей).
	// Как только Slate увидит эти цифры — виджет мгновенно загорится над флагом!
	InfoWidgetComponent->SetDrawSize(FVector2D(300.0f, 150.0f));

	// 3. Выравнивание Pivot в центр по Х (0.5) и в самый низ по Y (1.0)
	InfoWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));

	// Настройка кликабельности и коллизий
	InfoWidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
	InfoWidgetComponent->SetCollisionObjectType(ECC_WorldDynamic);
	InfoWidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InfoWidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InfoWidgetComponent->SetGenerateOverlapEvents(false);

	
	
	RotatingMovementComp = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComp"));
	
	// Нацеливаем компонент вращать строго наш FlagMesh, оставляя корень, декаль сонара и UI неподвижными на земле!
	RotatingMovementComp->SetUpdatedComponent(FlagMesh);
	
	// Задаем скорость вращения: 15 градусов в секунду вокруг вертикальной оси Z (Yaw)
	// (Число можно крутить прямо из Блупринта флага в панели Details компонента)
	RotatingMovementComp->RotationRate = FRotator(0.0f, 60.0f, 0.0f);
	
	// Компонент полностью автономен, сам реплицирует движение на клиентах и летает без нагрузки на ОЗУ!
	RotatingMovementComp->bRotationInLocalSpace = true;
	
	
	// Создаем коллайдер
	SelectionCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("SelectionCollider"));

	// Прикрепляем к текущему Root (например, к вашей декали или SceneComponent)
	SelectionCollider->SetupAttachment(RootComponent);

	// Настраиваем коллизию для мышки
	SelectionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Только для лучей/кликов, без физики физических тел
	SelectionCollider->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SelectionCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // Игнорируем всё

	// Включаем блокировку для канала видимости, по которому кликает мышка (обычно Visibility или Camera)
	SelectionCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	
}

void ATacticalFlagBase::BeginPlay()
{
	Super::BeginPlay();
	UpdateDecalSize(FlagRadius);
	
	if (FlagMesh && SelectionCollider)
	{
		// 1. Получаем локальные габариты статик меша (его коробку Bounds)
		// Это работает, даже если меш сложной формы или импортирован со смещением
		FBoxSphereBounds MeshBounds = FlagMesh->CalcBounds(FlagMesh->GetComponentTransform());

		// 2. Извлекаем радиус и высоту из габаритов
		// BoxExtent.X и Y определяют ширину, берем максимальное значение для радиуса капсулы
		float NewRadius = FMath::Max(MeshBounds.BoxExtent.X, MeshBounds.BoxExtent.Y);
		
		// BoxExtent.Z — это половина высоты меша. 
		// Капсуле в UE нужна именно полувысота (HalfHeight)
		float NewHalfHeight = MeshBounds.BoxExtent.Z;

		// 3. Добавляем небольшой зазор (например, +10%), чтобы по флагу было проще кликать
		NewRadius *= 1.1f;
		NewHalfHeight *= 1.1f;

		// 4. Задаем размеры капсуле
		SelectionCollider->SetCapsuleSize(NewRadius, NewHalfHeight);

		// 5. Сдвигаем капсулу вверх, так как ее центр в UE находится посередине, 
		// а меш флага обычно стоит основанием на земле (в нуле координат)
		SelectionCollider->SetRelativeLocation(FVector(0.0f, 0.0f, NewHalfHeight));
	}
	
	UFlagsManagerSubsystem* FlagManager = GetWorld()->GetSubsystem<UFlagsManagerSubsystem>();
	
	if (bIsLocalTempVersion)
	{
		if (FlagManager)
		{
			FlagManager->RegistryFlagAsLocalTemp(this);
		}
		return;
	}
	
	if (HasAuthority())
	{
		if (FlagManager)
		{
			FlagManager->RegistryFlagOnServer(FactionID,this);
		}
	}
	else
	{
		if (FlagManager)
		{
			FlagManager->RegistryFlagAsLocalPermanent(this);
		}
	}
	
	if (InfoWidgetComponent && InfoWidgetClass)
	{
		InfoWidgetComponent->SetWidgetClass(InfoWidgetClass);
		
		// С++ ПОДПИСКА НА ДЕЛЕГАТ ВИДЖЕТА ПРИ РОЖДЕНИИ:
		if (UUserWidget* SpawnedWidget = InfoWidgetComponent->GetUserWidgetObject())
		{
			// Проверяем: унаследовал ли виджет наш С++ интерфейс со встроенным геттером?
			if (IStrategyFlagUiInterface* UiInterface = Cast<IStrategyFlagUiInterface>(SpawnedWidget))
			{
				UiInterface->SetSelfFlagActor(this);
				
				// Вызываем геттер, забираем делегат и принудительно биндим на него нашу С++ функцию!
				UiInterface->GetRadiusSliderChangedDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnRadiusSliderChanged);
				UiInterface->GetRadiusSliderChangedDelegateLocal().AddDynamic(this, &ATacticalFlagBase::HandleOnRadiusSliderChangedLocal);
				UiInterface->GetButtonDeletePressedDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnButtonDeletePressed);
				
				UiInterface->GetOnUiCheckboxInFilterChangedSignatureDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnUiCheckboxInFilterChanged);
				UiInterface->GetOnUiCheckboxOutFilterChangedSignatureDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnUiCheckboxOutFilterChanged);
				UiInterface->GetOnUiCountInFilterChangedSignatureDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnUiCountInFilterChanged);
				UiInterface->GetOnUiCountOutFilterChangedSignatureDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnUiCountOutFilterChanged);
				
				UiInterface->GetButtonDeleteSourcePressedDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnButtonDeleteSourcePressed);
				UiInterface->GetButtonDeleteDestinationPressedDelegate().AddDynamic(this, &ATacticalFlagBase::HandleOnButtonDeleteDestinationPressed);
			}
		}
	}

	// ЗАПУСКАЕМ СЕРВЕРНЫЙ СЧЕТЧИК СТРОГО РАЗ В СЕКУНДУ
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TimerHandle_UpdateCounter, this, &ATacticalFlagBase::UpdateTroopsCounter, 1.0f, true);
	}
	
	UWorld* World = GetWorld();
	if (!World) return;

	if (APlayerController* LocalPC = World->GetFirstPlayerController())
	{
		if (AMainGamePlayerState* LocalPS = Cast<AMainGamePlayerState>(LocalPC->PlayerState))
		{
			uint8 LocalPlayerFactionID = LocalPS->GetGenericTeamId();
			
			SetActorTickEnabled(LocalPlayerFactionID == FactionID);
			
			if (FlagManager && LocalPlayerFactionID == FactionID)
			{
				FlagManager->RegistryFlagAsLocalPermanent(this);
			}
		}
	}
	
	UpdateAllFlagDestinations();
	UpdateAllFlagSources();

	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, *FString::Printf(TEXT("FLAG BUILDET ID=%d"), FlagID));
}

void ATacticalFlagBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	for (auto It = FlagDestinationConnections.CreateIterator(); It; ++It)
	{
		ADecalLineBase* DeadLine = It.Value();
    
		if (DeadLine && IsValid(DeadLine))
		{
			DeadLine->RemoveDecal();
		}

		It.RemoveCurrent();
	}
}

void ATacticalFlagBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World || !InfoWidgetComponent) return;

	// 1. Достаем камеру локального игрока на этом ПК
	APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!LocalPC || !LocalPC->PlayerCameraManager) return;

	// 2. Считаем честное 3D расстояние от глаз игрока до нашего 3D виджет-компонента
	FVector CameraLocation = LocalPC->PlayerCameraManager->GetCameraLocation();
	FVector WidgetLocation = InfoWidgetComponent->GetComponentLocation();
	float DistanceToCamera = FVector::Dist(CameraLocation, WidgetLocation);

	// 3. С++ МАТЕМАТИКА МАСШТАБА (Аналог Map Range Clamped от 1000 до 4000)
	// Вблизи (1000см) масштаб = 1.0f, на дистанции (4000см) масштаб плавно сжимается до 0.4f
	float TargetScale = FMath::GetMappedRangeValueClamped(FVector2D(1000.0f, 4000.0f), FVector2D(1.0f, 0.4f), DistanceToCamera);

	// 4. С++ МАТЕМАТИКА ПРОЗРАЧНОСТИ (Аналог Map Range Clamped от 4000 до 5500)
	// На дистанции выше 4000 интерфейс начинает плавно растворяться, вшиваясь в полный ноль на 5500см
	float TargetOpacity = FMath::GetMappedRangeValueClamped(FVector2D(4000.0f, 5500.0f), FVector2D(1.0f, 0.0f), DistanceToCamera);

	// Извлекаем слепой инстанс виджета, проверяем интерфейс и вбрасываем готовые числа!
	if (UUserWidget* FlagUserWidget = InfoWidgetComponent->GetUserWidgetObject())
	{
		if (FlagUserWidget->GetClass()->ImplementsInterface(UStrategyFlagUiInterface::StaticClass()))
		{
			IStrategyFlagUiInterface::Execute_UpdateUiSizing(FlagUserWidget, TargetScale, TargetOpacity);
		}
	}
}

void ATacticalFlagBase::AddDestination(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	if (!DestinationFlags.Contains(Flag))
	{
		DestinationFlags.Add(Flag);
		Flag->AddSource(this);
		UpdateFlagDestination(Flag);
	}
}

void ATacticalFlagBase::AddSource(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	if (!SourceFlags.Contains(Flag))
	{
		SourceFlags.Add(Flag);
		Flag->AddDestination(this);
	}
}


void ATacticalFlagBase::UpdateAllFlagDestinations()
{
	for (int32 i = DestinationFlags.Num() - 1; i >= 0; --i)
	{
		// Проверяем, жив ли еще флаг
		if (IsValid(DestinationFlags[i]))
		{
			// Если жив, получаем прямой указатель и вызываем логику
			ATacticalFlagBase* Flag = DestinationFlags[i].Get();
        
			UpdateFlagDestination(Flag);
			Flag->UpdateFlagSource(this);
		}
		else
		{
			// Если ссылка невалидна — просто удаляем запись из массива
			DestinationFlags.RemoveAt(i);
		}
	}
	
	// Запускаем итератор по нашей карте связей
	for (auto It = FlagDestinationConnections.CreateIterator(); It; ++It)
	{
		// Проверяем ключ (TWeakObjectPtr). Если флаг был уничтожен, Key.IsValid() вернет false
		if (!It.Key().IsValid() || !DestinationFlags.Contains(It.Key()))
		{
			// 1. Получаем указатель на линию-декаль, которая осталась «сиротой»
			ADecalLineBase* DeadLine = It.Value();
        
			if (DeadLine && IsValid(DeadLine))
			{
				DeadLine->RemoveDecal();
			}

			// 2. Безопасно удаляем саму запись из TMap через итератор
			It.RemoveCurrent();
		}
	}
}

void ATacticalFlagBase::UpdateAllFlagSources()
{
	for (int32 i = SourceFlags.Num() - 1; i >= 0; --i)
	{
		// Проверяем валидность флага источника
		if (IsValid(SourceFlags[i]))
		{
			ATacticalFlagBase* Flag = SourceFlags[i].Get();
        
			Flag->UpdateFlagDestination(this);
		}
		else
		{
			// Удаляем битую ссылку из массива
			SourceFlags.RemoveAt(i);
		}
	}
}

void ATacticalFlagBase::UpdateFlagDestination(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	
	
	APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (LocalPC && LocalPC->IsLocalController())
	{
		if (AMainGamePlayerState* LocalPS = Cast<AMainGamePlayerState>(LocalPC->PlayerState))
		{
			if (LocalPS && FactionID != LocalPS->GetGenericTeamId()) 
			{
				return; //на сервере не спавним декали флагов других игроков
			}
		}
	}
	
	if (!FlagDestinationConnections.Contains(Flag))
	{
		ULocalGraphicsPoolSubsystem* GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
		if (!GraphicsPool) return;
		
		AActor* PoolActor = GraphicsPool->GetActorFromPool(ADecalLineBase::StaticClass());
		ADecalLineBase* Decal = Cast<ADecalLineBase>(PoolActor);
		if (Decal)
		{
			const URTSVisualSettings* VisualSettings = GetDefault<URTSVisualSettings>();
			if (VisualSettings)
			{
				// 2. Синхронно загружаем нужный материал из настроек 
				UMaterialInterface* TargetMaterial = VisualSettings->FlagMoveDestinationDecalMaterial.LoadSynchronous();

				if (TargetMaterial && Decal)
				{
					// 3. Задаем настройки один раз при извлечении из пула
					// Передаем: Материал, Толщину, Коэффициент длины (например, 2.f для башен), Высоту проекции
					Decal->InitLineSettings(TargetMaterial, 100.0f, 2.f, 500.0f);
					
					FlagDestinationConnections.Add(Flag, Decal);
				}
			}
		}		
	}
	
	if (ADecalLineBase** DecalPtr = FlagDestinationConnections.Find(Flag))
	{
		// DecalPtr — это адрес ячейки, где лежит сам указатель на линию.
		// Сначала проверяем, что в этой ячейке не nullptr, и что сам актер еще «жив»
		if (*DecalPtr && IsValid(*DecalPtr))
		{
			FVector StartPoint = GetActorLocation();
			FVector EndPoint = Flag->GetActorLocation();
			// Разыменовываем через (*DecalPtr), чтобы вызвать функцию самой линии
			(*DecalPtr)->SetParametrs(StartPoint, EndPoint);
		}
	}	
}

void ATacticalFlagBase::UpdateFlagSource(ATacticalFlagBase* Flag)
{
}

void ATacticalFlagBase::RemoveDestination(ATacticalFlagBase* Flag)
{
	// 1. Проверяем входной параметр на валидность
	if (!Flag) return;

	// Переменная для сохранения найденного флага перед его удалением из массива
	ATacticalFlagBase* FoundFlag = nullptr;

	// 2. Ищем флаг в массиве вручную через обратный цикл (для безопасности удаления)
	for (int32 i = DestinationFlags.Num() - 1; i >= 0; --i)
	{
		// Извлекаем чистый указатель из TObjectPtr
		if (DestinationFlags[i].Get() == Flag)
		{
			// Сохраняем указатель на объект, чтобы вызвать у него метод позже
			FoundFlag = DestinationFlags[i].Get();
			
			// Удаляем запись из нашего массива
			DestinationFlags.RemoveAt(i);
			break; // Флаг уникальный, дальше цикл крутить не нужно
		}
	}

	// 3. Если флаг был найден и успешно удален из нашего массива — оповещаем его
	if (FoundFlag && IsValid(FoundFlag))
	{
		FoundFlag->RemoveSource(this);
	}

	// 3. Ищем и удаляем связанную декаль-линию из TMap
	// Передаем Flag — движок сам создаст временный TWeakObjectPtr для поиска ключа
	if (ADecalLineBase** DecalPtr = FlagDestinationConnections.Find(Flag))
	{
		if (*DecalPtr && IsValid(*DecalPtr))
		{
			(*DecalPtr)->RemoveDecal();
		}

		// Удаляем саму запись «Флаг -> Линия» из таблицы связей
		FlagDestinationConnections.Remove(Flag);
	}
}

void ATacticalFlagBase::RemoveSource(ATacticalFlagBase* Flag)
{
	// 1. Проверяем входной параметр на валидность
	if (!Flag) return;

	// Переменная для сохранения найденного флага перед его удалением из массива
	ATacticalFlagBase* FoundFlag = nullptr;

	// 2. Ищем флаг в массиве вручную через обратный цикл (для безопасности удаления)
	for (int32 i = SourceFlags.Num() - 1; i >= 0; --i)
	{
		// Извлекаем чистый указатель из TObjectPtr
		if (SourceFlags[i].Get() == Flag)
		{
			// Сохраняем указатель на объект, чтобы вызвать у него метод позже
			FoundFlag = SourceFlags[i].Get();
			
			// Удаляем запись из нашего массива
			SourceFlags.RemoveAt(i);
			break; // Флаг уникальный, дальше цикл крутить не нужно
		}
	}

	// 3. Если флаг был найден и успешно удален из нашего массива — оповещаем его
	if (FoundFlag && IsValid(FoundFlag))
	{
		FoundFlag->RemoveDestination(this);
	}
}


void ATacticalFlagBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATacticalFlagBase, FactionID);
	DOREPLIFETIME(ATacticalFlagBase, FlagRadius);
	DOREPLIFETIME(ATacticalFlagBase, CurrentUnitsCount); // Синхронизируем счетчик с клиентом
	
	DOREPLIFETIME(ATacticalFlagBase, SourceFlags);
	DOREPLIFETIME(ATacticalFlagBase, DestinationFlags);
	DOREPLIFETIME(ATacticalFlagBase, MinimumRetainedUnitsCount);
	DOREPLIFETIME(ATacticalFlagBase, MaximumCapacityLimit);
	DOREPLIFETIME(ATacticalFlagBase, bIsGlobalRallyPoint);
}

void ATacticalFlagBase::SetFlagVisualVisibility(bool bIsVisible)
{
	// Насильно тушим или зажигаем все графические компоненты флага на текущем ПК
	
	if (FlagMesh) FlagMesh->SetVisibility(bIsVisible, true);
	if (RadiusDecal) RadiusDecal->SetVisibility(bIsVisible);
	if (InfoWidgetComponent) InfoWidgetComponent->SetVisibility(bIsVisible, true);
	

	// Для Niagara эффектов используем деактивацию/активацию, чтобы они не жрали частицы за кадром
	TArray<UNiagaraComponent*> NiagaraComps;
	GetComponents<UNiagaraComponent>(NiagaraComps);
	for (UNiagaraComponent* NC : NiagaraComps)
	{
		if (NC)
		{
			NC->SetVisibility(bIsVisible);
			if (!bIsVisible) NC->Deactivate();
		}
	}
}

void ATacticalFlagBase::UpdateTroopsCounter()
{
	if (!HasAuthority()) return;

	UStrategyGridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UStrategyGridSubsystem>();
	if (GridSubsystem)
	{
		// Используем бакетный радар союзников!
		// По дефолту он ищет ТОЛЬКО мобильные войска (Troops), полностью игнорируя здания,
		// и выделяет память в ОЗУ идеальными контролируемыми шагами!
		TArray<AActor*> NearbyAllies = GridSubsystem->FindAllAlliesInRadius(GetActorLocation(), FlagRadius, FactionID);
		
		// Записываем размер буфера. Изменение переменной мгновенно стриггерит OnRep на клиенте!
		CurrentUnitsCount = NearbyAllies.Num();
		
		//GEngine->AddOnScreenDebugMessage(88, 1.0f, FColor::Green, *FString::Printf(TEXT("FLAG COUNTER UPDATE R=%.1f  C=%d"), FlagRadius, CurrentUnitsCount));
		
		OnRep_CurrentUnitsCount();
		
		
		
		TSet<AActor*> NearbySet(NearbyAllies);

		// ЭТАП А: Очистка серверного массива флага от тех, кто сбежал или умер
		for (int32 i = BoundTroops.Num() - 1; i >= 0; --i)
		{
			ATroopBase* BoundTroop = BoundTroops[i];
			if (!BoundTroop || !IsValid(BoundTroop) || !NearbySet.Contains(BoundTroop))
			{
				UnbindTroopByIndex(i);
			}
		}

		// ЭТАП Б: Регистрация пришедших вручную или завершивших марш солдат
		for (AActor* Actor : NearbyAllies)
		{
			ATroopBase* Troop = Cast<ATroopBase>(Actor);
			if (!Troop || !IsValid(Troop)) continue;

			if (Troop->GetCurrentTargetFlag() == nullptr)
			{
				BindTroop(Troop); // Подхватываем "свободного" солдата
			}
			else if (Troop->GetCurrentTargetFlag() == this && Troop->GetAssignmentState() == ETroopAssignmentState::MarchingToFlag)
			{
				// Солдат дошел маршем: уменьшаем счетчик ожидания и привязываем к массиву флага
				DecrementIncomingTroops();
				BindTroop(Troop); 
			}
		}

		// ЭТАП В: Расчет реальных излишков с учетом забронированных солдат в пути
		int32 TotalAssigned = BoundTroops.Num() + IncomingTroopsCount;
		int32 SurplusCount = TotalAssigned - (bIsFilterOutCount ? MinimumRetainedUnitsCount : 0);
		
		//GEngine->AddOnScreenDebugMessage(FlagID, 3.0f, FColor::Red, *FString::Printf(TEXT("Id: %d; BoundTroops %d; IncomingTroopsCount %d"), FlagID, BoundTroops.Num(), IncomingTroopsCount));

		// Если лишних солдат нет — выходим
		if (SurplusCount <= 0 || DestinationFlags.Num() == 0)
		{
			Server_ExecuteDeferredMovement();
			return;
		}

		// Защита: отправляем не больше, чем физически стоит в массиве флага
		SurplusCount = FMath::Min(SurplusCount, BoundTroops.Num());
	
		// Запуск нарезки и отправки групп
		ProcessSurplusSending(SurplusCount);		
	}
	
	Server_ExecuteDeferredMovement();
}

void ATacticalFlagBase::BindTroop(ATroopBase* Troop)
{
	if (!Troop) return;
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, *FString::Printf(TEXT("Troop bind to %d"), FlagID));
	int32 NewIndex = BoundTroops.Add(Troop);
	//Troop->SetServerLocalIndex(NewIndex);
	Troop->SetNewRtsTargetFlag(this);
	Troop->SetAssignmentState(ETroopAssignmentState::DefendingFlag);
}

void ATacticalFlagBase::UnbindTroopByIndex(int32 Index)
{
	if (!BoundTroops.IsValidIndex(Index)) return;

	if (BoundTroops[Index] && IsValid(BoundTroops[Index]))
	{
		BoundTroops[Index]->SetNewRtsTargetFlag(nullptr);
	}

	// Сверхбыстрый Swap-and-Pop алгоритм
	// if (Index < BoundTroops.Num() - 1)
	// {
	// 	BoundTroops.Last()->SetServerLocalIndex(Index);
	// }
	BoundTroops.RemoveAtSwap(Index);
}

void ATacticalFlagBase::ProcessSurplusSending(int32 SurplusCount)
{
	int32 NumDestinations = DestinationFlags.Num();
	if (NumDestinations == 0 || SurplusCount <= 0) return;

	AMainGamePlayerController* MyPC = Cast<AMainGamePlayerController>(GetOwner());
	if (!MyPC) return;

	// Массив для хранения флагов, которые РЕАЛЬНО могут принять солдат прямо сейчас,
	// и количества свободных мест на каждом из них.
	TArray<TPair<ATacticalFlagBase*, int32>> ValidDestinations;
	ValidDestinations.Reserve(NumDestinations);

	int32 TotalAvailableSpace = 0;

	// Шаг 1: Сканируем флаги назначения и выясняем, у кого есть свободные места
	for (int32 i = 0; i < NumDestinations; ++i)
	{
		ATacticalFlagBase* TargetFlag = DestinationFlags[i].Get();
		if (!TargetFlag || !IsValid(TargetFlag)) continue;

		// Считаем, сколько солдат уже закреплено за целевым флагом (стоят + идут к нему)
		int32 TargetTotalAssigned = TargetFlag->BoundTroops.Num() + TargetFlag->IncomingTroopsCount;

		// Вычисляем свободное место до лимита вместимости
		int32 FreeSpace = TargetFlag->bIsFilterInCount ? (TargetFlag->MaximumCapacityLimit - TargetTotalAssigned) : 100;

		if (FreeSpace > 0)
		{
			ValidDestinations.Add(TPair<ATacticalFlagBase*, int32>(TargetFlag, FreeSpace));
			TotalAvailableSpace += FreeSpace;
		}
	}

	// Если на всех флагах назначения достигнут MaximumCapacityLimit — отправлять некуда, выходим
	if (ValidDestinations.Num() == 0) return;

	// Ограничиваем общую отправку общим свободным местом на всех целевых флагах
	int32 TotalTroopsToSend = FMath::Min(SurplusCount, TotalAvailableSpace);

	// Шаг 2: Равномерно распределяем солдат по доступным флагам с учетом их лимитов
	while (TotalTroopsToSend > 0 && ValidDestinations.Num() > 0)
	{
		// Считаем базовую порцию на один свободный флаг на этом круге распределения
		int32 BasePortion = FMath::Max(1, TotalTroopsToSend / ValidDestinations.Num());

		for (int32 i = ValidDestinations.Num() - 1; i >= 0; --i)
		{
			if (TotalTroopsToSend <= 0) break;

			ATacticalFlagBase* TargetFlag = ValidDestinations[i].Key;
			int32& FreeSpace = ValidDestinations[i].Value;

			// Берем порцию, но не больше, чем осталось свободного места на этом флаге
			int32 SliceSize = FMath::Min(BasePortion, FreeSpace);
			SliceSize = FMath::Min(SliceSize, TotalTroopsToSend);

			if (SliceSize <= 0) continue;

			// Собираем пачку солдат с хвоста массива BoundTroops через Swap-and-Pop
			//TArray<AActor*> TroopGroup;
			//TroopGroup.Reserve(SliceSize);
			int32 TroopGroupSize = 0;

			for (int32 j = 0; j < SliceSize; ++j)
			{
				if (BoundTroops.Num() == 0) break;

				ATroopBase* TroopToMarch = BoundTroops.Last();

				// Прописываем "паспорт" марша К НОВОМУ флагу ДО отвязки
				TroopToMarch->SetNewRtsTargetFlag(TargetFlag);
				TroopToMarch->SetAssignmentState(ETroopAssignmentState::MarchingToFlag);

				//TroopGroup.Add(TroopToMarch);
				TroopGroupSize++;
				// Удаляем из серверного массива текущего флага за O(1)
				BoundTroops.RemoveAtSwap(BoundTroops.Num() - 1);
			}

			if (TroopGroupSize > 0)
			{
				// Фиксируем бронь и обновляем локальные счетчики свободного места
				TargetFlag->IncomingTroopsCount += TroopGroupSize;
				FreeSpace -= TroopGroupSize;
				TotalTroopsToSend -= TroopGroupSize;

				// Передаем пачку флагу назначения на отложенное исполнение движения
				//TargetFlag->AddIncomingTroopsBatch(TroopGroup);
			}

			// Если этот флаг полностью забился до лимита, убираем его из списка доступных
			if (FreeSpace <= 0)
			{
				ValidDestinations.RemoveAtSwap(i);
			}
		}
	}
}

/*void ATacticalFlagBase::ProcessSurplusSending(int32 SurplusCount)
{
	int32 NumDestinations = DestinationFlags.Num();
	int32 TroopsPerFlag = SurplusCount / NumDestinations;
	int32 RemainderTroops = SurplusCount % NumDestinations;

	AMainGamePlayerController* MyPC = Cast<AMainGamePlayerController>(GetOwner());
	if (!MyPC) return;

	for (int32 i = 0; i < NumDestinations; ++i)
	{
		ATacticalFlagBase* TargetFlag = DestinationFlags[i].Get();
		if (!TargetFlag || !IsValid(TargetFlag)) continue;

		int32 SliceSize = TroopsPerFlag + (RemainderTroops > 0 ? 1 : 0);
		RemainderTroops--;

		if (SliceSize <= 0) continue;

		TArray<AActor*> TroopGroup;
		TroopGroup.Reserve(SliceSize);

		// Нарезаем пачку солдат С КОНЦА серверного массива флага
		for (int32 j = 0; j < SliceSize; ++j)
		{
			if (BoundTroops.Num() == 0) break;
			
			ATroopBase* TroopToMarch = BoundTroops.Last();
			
			// Прописываем «паспорт» марша К НОВОМУ флагу ДО отвязки, чтобы не затереть данные
			TroopToMarch->SetNewRtsTargetFlag(TargetFlag);
			TroopToMarch->SetAssignmentState(ETroopAssignmentState::MarchingToFlag);
			
			TroopGroup.Add(TroopToMarch);
			
			// Удаляем его из списков текущего флага
			BoundTroops.RemoveAtSwap(BoundTroops.Num() - 1);
		}

		if (TroopGroup.Num() > 0)
		{
			// Фиксируем бронь на новом флаге, спасаясь от оверспама
			TargetFlag->IncomingTroopsCount += TroopGroup.Num();

			// Запуск группового перемещения с построениями из вашего контроллера
			//MyPC->Server_MoveSelectedUnits(TargetFlag->GetActorLocation(), TroopGroup);
		}
	}
}*/

void ATacticalFlagBase::Server_ExecuteDeferredMovement()
{
	// Предохранитель: если новых регистраций не было, или массив пуст — ничего не делаем.
	if (!bNeedsFormationUpdate || IncomingTroopsArray.Num() == 0) 
	{
		return;
	}

	AMainGamePlayerController* MyPC = Cast<AMainGamePlayerController>(GetOwner());
	if (!MyPC) 
	{
		IncomingTroopsArray.Reset();
		bNeedsFormationUpdate = false;
		return;
	}

	// Готовим финальный массив для метода контроллера (требует тип AActor*)
	TArray<AActor*> FinalMovementGroup;
	
	// Выделяем память под сумму двух массивов за одну аллокацию
	FinalMovementGroup.Reserve(BoundTroops.Num() + IncomingTroopsArray.Num());

	// 1. ПЕРВЫМИ добавляем тех, кто уже на месте. 
	// Они гарантированно займут центр и внутренние кольца спирали!
	// for (ATroopBase* Troop : BoundTroops)
	// {
	// 	if (Troop && IsValid(Troop))
	// 	{
	// 		FinalMovementGroup.Add(Troop);
	// 	}
	// }
	FinalMovementGroup.Append(BoundTroops);

	// 2. ВТОРЫМИ добавляем новичков, которые только начали марш с других башен.
	// Они займут внешние витки спирали и будут бежать к внешним границам построения.
	// for (ATroopBase* Troop : IncomingTroopsArray)
	// {
	// 	if (Troop && IsValid(Troop))
	// 	{
	// 		FinalMovementGroup.Add(Troop);
	// 	}
	// }
	FinalMovementGroup.Append(IncomingTroopsArray);

	// Отдаем ОДИН монолитный приказ на движение для ВСЕЙ объединенной группы
	if (FinalMovementGroup.Num() > 0)
	{
		MyPC->Server_MoveSelectedUnits_Implementation(GetActorLocation(), FinalMovementGroup);
	}

	// Сбрасываем данные для следующего такта логики
	IncomingTroopsArray.Reset();
	bNeedsFormationUpdate = false; 
}

void ATacticalFlagBase::RegisterIncomingTroopForMovement(ATroopBase* Troop)
{
	if (!Troop || !IsValid(Troop) || !HasAuthority()) return;

	// Добавляем солдата в накопительный массив для следующего такта движения
	IncomingTroopsArray.Add(Troop);

	// Поднимаем флаг: состав группы изменился, в конце тика нужна команда на движение!
	bNeedsFormationUpdate = true;
}

void ATacticalFlagBase::Server_UpdateFlagRadius_Implementation(float NewFlagRadius)
{
	if (!HasAuthority()) return;
	FlagRadius = NewFlagRadius;
	UpdateDecalSize(FlagRadius);
}

bool ATacticalFlagBase::Server_UpdateFlagRadius_Validate(float NewFlagRadius)
{
	return true;
}

void ATacticalFlagBase::OnRep_FlagRadius()
{
	UpdateDecalSize(FlagRadius);
}

void ATacticalFlagBase::OnRep_CurrentUnitsCount()
{
	if (OnUnitsCountChanged.IsBound())
	{
		OnUnitsCountChanged.Broadcast(CurrentUnitsCount);
	}
	
	

	if (InfoWidgetComponent)
	{
		if (UUserWidget* FlagUserWidget = InfoWidgetComponent->GetUserWidgetObject())
		{
			if (FlagUserWidget->GetClass()->ImplementsInterface(UStrategyFlagUiInterface::StaticClass()))
			{
				IStrategyFlagUiInterface::Execute_RefreshSoldierCount(FlagUserWidget, CurrentUnitsCount);
			}
		}
	}
}

void ATacticalFlagBase::OnRep_NetworkLinksChanged()
{
	UpdateAllFlagDestinations();
	UpdateAllFlagSources();
}

void ATacticalFlagBase::OnRep_bIsGlobalRallyPoint()
{
}

void ATacticalFlagBase::UpdateDecalSize(float Radius)
{
	if (RadiusDecal)
	{
		RadiusDecal->DecalSize = (FVector(Radius, Radius, 500.0f));
		if (UMaterialInstanceDynamic* DynamicMat = RadiusDecal->CreateDynamicMaterialInstance())
		{
			DynamicMat->SetScalarParameterValue(FName(TEXT("Radius")), Radius);
		}
		RadiusDecal->MarkRenderStateDirty(); 
	}
}

bool ATacticalFlagBase::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (const APlayerController* ViewerPC = Cast<APlayerController>(RealViewer))
	{
		if (AMainGamePlayerState* ViewerPS = Cast<AMainGamePlayerState>(ViewerPC->PlayerState))
		{
			return ViewerPS->GetGenericTeamId() == FactionID;
		}
	}
	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

bool ATacticalFlagBase::Server_UpdateFlagLocation_Validate(FVector NewWorldLocation) { return true; }
void ATacticalFlagBase::Server_UpdateFlagLocation_Implementation(FVector NewWorldLocation)
{
	if (!HasAuthority()) return;
	SetActorLocation(NewWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void ATacticalFlagBase::HandleOnRadiusSliderChanged(float NewRadiusValue)
{
	Server_UpdateFlagRadius(NewRadiusValue);
}

void ATacticalFlagBase::HandleOnRadiusSliderChangedLocal(float NewRadiusValue)
{
	UpdateDecalSize(NewRadiusValue);
}

void ATacticalFlagBase::HandleOnButtonDeletePressed()
{
	Server_DestroyFlag();
}

void ATacticalFlagBase::HandleOnButtonDeleteSourcePressed()
{
	Server_DeleteSource();
}

void ATacticalFlagBase::Server_DeleteSource_Implementation()
{
	for (int32 i = SourceFlags.Num() - 1; i >= 0; --i)
	{
		ATacticalFlagBase* flag = SourceFlags[i];

		if (IsValid(flag))
		{
			flag->RemoveDestination(this);			
		}
		else
		{
			SourceFlags.RemoveAt(i);
		}
	}
}

bool ATacticalFlagBase::Server_DeleteSource_Validate()
{
	return true;
}

void ATacticalFlagBase::HandleOnButtonDeleteDestinationPressed()
{
	Server_DeleteDestination();
}

void ATacticalFlagBase::Server_DeleteDestination_Implementation()
{	
	for (int32 i = DestinationFlags.Num() - 1; i >= 0; --i)
	{
		ATacticalFlagBase* flag = DestinationFlags[i];

		if (IsValid(flag))
		{
			flag->RemoveSource(this);			
		}
		else
		{
			DestinationFlags.RemoveAt(i);
		}
	}
}

bool ATacticalFlagBase::Server_DeleteDestination_Validate()
{
	return true;
}


void ATacticalFlagBase::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	
	UpdateAllFlagDestinations();
	UpdateAllFlagSources();
	
	if (HasAuthority())
	{
		for (ATroopBase* Troop : BoundTroops)
		{
			Troop->SetAssignmentState(ETroopAssignmentState::MarchingToFlag);
		}
		
		IncomingTroopsArray.Append(BoundTroops);

		BoundTroops.Reset();
		bNeedsFormationUpdate = true;
		Server_ExecuteDeferredMovement();
	}
}

void ATacticalFlagBase::HandleOnUiCheckboxInFilterChanged(bool NewState)
{
	bIsFilterInCount = NewState;
}

void ATacticalFlagBase::HandleOnUiCheckboxOutFilterChanged(bool NewState)
{
	bIsFilterOutCount = NewState;
}

void ATacticalFlagBase::HandleOnUiCountInFilterChanged(int32 NewCount)
{
	MaximumCapacityLimit = NewCount;
}

void ATacticalFlagBase::HandleOnUiCountOutFilterChanged(int32 NewCount)
{
	MinimumRetainedUnitsCount = NewCount;
}

void ATacticalFlagBase::Server_OnUiCheckboxOutFilterChanged_Implementation(bool NewState)
{
}

bool ATacticalFlagBase::Server_OnUiCheckboxOutFilterChanged_Validate(bool NewState)
{
	return  true;
}

void ATacticalFlagBase::Server_OnUiCheckboxInFilterChanged_Implementation(bool NewState)
{
}

bool ATacticalFlagBase::Server_OnUiCheckboxInFilterChanged_Validate(bool NewState)
{
	return  true;
}

void ATacticalFlagBase::Server_OnUiCountInFilterChanged_Implementation(int32 NewCount)
{
}

bool ATacticalFlagBase::Server_OnUiCountInFilterChanged_Validate(int32 NewCount)
{
	return true;
}

void ATacticalFlagBase::Server_OnUiCountOutFilterChanged_Implementation(int32 NewCount)
{
}

bool ATacticalFlagBase::Server_OnUiCountOutFilterChanged_Validate(int32 NewCount)
{
	return true;
}

void ATacticalFlagBase::Server_DestroyFlag_Implementation()
{
	if (!HasAuthority()) return;
	
	for (auto It = FlagDestinationConnections.CreateIterator(); It; ++It)
	{
		ADecalLineBase* DeadLine = It.Value();
    
		if (DeadLine && IsValid(DeadLine))
		{
			DeadLine->RemoveDecal();
		}

		It.RemoveCurrent();
	}
	
	for (int32 i = SourceFlags.Num() - 1; i >= 0; --i)
	{
		ATacticalFlagBase* flag = SourceFlags[i];

		if (IsValid(flag))
		{
			flag->RemoveDestination(this);			
		}
		else
		{
			SourceFlags.RemoveAt(i);
		}
	}
	
	for (int32 i = DestinationFlags.Num() - 1; i >= 0; --i)
	{
		ATacticalFlagBase* flag = DestinationFlags[i];

		if (IsValid(flag))
		{
			flag->RemoveSource(this);			
		}
		else
		{
			DestinationFlags.RemoveAt(i);
		}
	}
	
	for (ATroopBase* Troop : BoundTroops)
	{
		Troop->SetNewRtsTargetFlag(nullptr);
	}
	
	Destroy();
}

bool ATacticalFlagBase::Server_DestroyFlag_Validate()
{
	return  true;
}



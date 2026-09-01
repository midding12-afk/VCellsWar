// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "VCellsWar/AI/StrategyAIController.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerState.h"
#include "VCellsWar/MainGame/RTSCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "NavigationSystem.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"
#include "VCellsWar/Systems/FlagsManagerSubsystem.h"
#include "VCellsWar/Systems/ServerNetworkPoolSubsystem.h"
#include "VCellsWar/Systems/StrategyPlacementSubsystem.h"
#include "VCellsWar/TacticalFlag/TacticalFlagBase.h"

AMainGamePlayerController::AMainGamePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMainGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
		
	if (IsLocalController())
	{
		FInputModeGameAndUI InputModeData;
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputModeData.SetHideCursorDuringCapture(false);
		SetInputMode(InputModeData);

		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		SetShowMouseCursor(true);
	}
	
	
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, *FString::Printf(TEXT("BP ")));
}
void AMainGamePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ТРИГГЕР ДЛЯ СЕРВЕРА / LISTEN SERVER / STANDALONE:
	// Как только сервер вселился в камеру, мы проверяем: если это локальный игрок-хост,
	// принудительно запускаем инициализацию ввода вручную!
	if (IsLocalController())
	{
		InitializeRTSInput();
	}
}

void AMainGamePlayerController::Server_MoveFlag_Implementation(ATacticalFlagBase* Flag, FVector Location)
{
	Flag->SetActorLocation(Location);
	Flag->OnRep_ReplicatedMovement();
}

void AMainGamePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// ТРИГГЕР ДЛЯ СЕТЕВЫХ КЛИЕНТОВ:
	// Срабатывает автоматически на ПК клиента, когда до него долетел пакет о владении пешкой.
	if (IsLocalController())
	{
		InitializeRTSInput();
	}
}

void AMainGamePlayerController::InitializeRTSInput()
{
	// Монолитная и безопасная точка включения контекста управления!
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				// Очищаем старый контекст, чтобы защититься от дубликатов при Seamless Travel
				Subsystem->RemoveMappingContext(DefaultMappingContext);

				// Накатываем чистый RTS-контекст управления
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				
			}
		}
	}
}

void AMainGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Делаем безопасный каст стандартного InputComponent к продвинутому EnhancedInputComponent
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// ПРИВЯЗКА ЛЕВОГО КЛИКА (Рамка выделения):
		if (ActionLeftClick)
		{
			// Started срабатывает ровно в миллисекунду НАЖАТИЯ кнопки (IE_Pressed) 
			EnhancedInputComponent->BindAction(ActionLeftClick, ETriggerEvent::Started, this, &AMainGamePlayerController::OnLeftClickStarted);
			
			// Completed срабатывает ровно в миллисекунду ОТПУСКАНИЯ кнопки (IE_Released) 
			EnhancedInputComponent->BindAction(ActionLeftClick, ETriggerEvent::Completed, this, &AMainGamePlayerController::OnLeftClickCompleted);
		}

		// ПРИВЯЗКА ПРАВОГО КЛИКА (Приказ атаки/движения):
		if (ActionRightClick)
		{
			// Triggered идеально подходит для одиночных или зажатых кликов действий 
			EnhancedInputComponent->BindAction(ActionRightClick, ETriggerEvent::Started, this, &AMainGamePlayerController::OnRightClickPressed);
		}
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, *FString::Printf(TEXT("INPUT")));
}

void AMainGamePlayerController::OnLeftClickStarted(const FInputActionValue& Value)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::White, *FString::Printf(TEXT("LMC From %d"), Cast<IGenericTeamAgentInterface>(PlayerState)->GetGenericTeamId().GetId()));
	
	if (UStrategyPlacementSubsystem* PlacementSubsystem = GetLocalPlayer()->GetSubsystem<UStrategyPlacementSubsystem>())
	{
		if (PlacementSubsystem->IsPlacingActive())
		{
			PlacementSubsystem->ConfirmPlacement(this); // Нажали ЛКМ при стройке — зафиксировать!
			return;
		}
	}

	
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		StartSelectionPoint = FVector2D(MouseX, MouseY);
		bIsSelecting = true;
	}
	
}

void AMainGamePlayerController::OnLeftClickCompleted(const FInputActionValue& Value)
{
	bIsSelecting = false;
}

void AMainGamePlayerController::OnRightClickPressed(const FInputActionValue& Value)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::White, *FString::Printf(TEXT("RMC From %d"), Cast<IGenericTeamAgentInterface>(PlayerState)->GetGenericTeamId().GetId()));
	
	if (UStrategyPlacementSubsystem* PlacementSubsystem = GetLocalPlayer()->GetSubsystem<UStrategyPlacementSubsystem>())
	{
		if (PlacementSubsystem->IsPlacingActive())
		{
			PlacementSubsystem->CancelPlacement(); 
			return;
		}
	}
	
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		if (HitResult.bBlockingHit)
		{
			Server_MoveSelectedUnits(HitResult.Location, MySelectedUnits);
		}
	}
}


void AMainGamePlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	
	// Проверяем, что мы управляем именно нашей RTS камерой
	ARTSCameraPawn* RTSCamera = Cast<ARTSCameraPawn>(P);
    
	if (RTSCamera && IsLocalController())
	{
		// Получаем подсистему ввода локального игрока
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// На всякий случай очищаем старые контексты (например, контекст лобби, если он завис)
			Subsystem->ClearAllMappings();

			// Берем указатель на контекст прямо из настроек вашей камеры
			// (Для этого сделайте переменную CameraMappingContext в ARTSCameraPawn публичной public:)
			if (RTSCamera->CameraMappingContext)
			{
				Subsystem->AddMappingContext(RTSCamera->CameraMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("Контроллер: Контекст RTS-камеры успешно переподключен после SeamlessTravel!"));
			}
		}
	}
}

void AMainGamePlayerController::TeleportLocalCameraTo(FVector2D CenterLocation)
{
	Client_TeleportCamera(CenterLocation, -1);
}



void AMainGamePlayerController::Client_TeleportCamera_Implementation(FVector2D TargetLocation, float Z)
{
	APawn* MyCameraPawn = GetPawn();
	if (MyCameraPawn)
	{
		// Клиент сам двигает свою нереплицируемую камеру в пространстве
		FVector Location(TargetLocation.X, TargetLocation.Y, Z>=0 ? Z : MyCameraPawn->GetActorLocation().Z);
		
		MyCameraPawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
        
		// Сбрасываем ротацию контроллера в ноль, чтобы выровнять камеру (наш прошлый фикс)
		SetControlRotation(FRotator::ZeroRotator);
	}
}

void AMainGamePlayerController::SetSelectedList(TArray<AActor*> NewSelectedUnits)
{
	for (AActor* Actor : MySelectedUnits)
	{
		if (IStrategyEntityInterface* EntityInterface = Cast<IStrategyEntityInterface>(Actor))
		{
			EntityInterface->DeselectEntity(); 
		}
	}
	
	
	MySelectedUnits = NewSelectedUnits;
	
	
	for (AActor* Actor : MySelectedUnits)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Selected: %s"), *Actor->GetName()));
		if (IStrategyEntityInterface* EntityInterface = Cast<IStrategyEntityInterface>(Actor))
		{
			EntityInterface->SelectEntity(); 
		}
	}
}


bool AMainGamePlayerController::Server_MoveSelectedUnits_Validate(FVector TargetLocation, const TArray<AActor*>& ActorsToMove)
{
	// Античит-подстраховка: сервер проверяет, не прислал ли клиент сломанные координаты (например, NaN)
	// Если вернуть false, движок мгновенно разорвет сетевое соединение с этим игроком за читы.
	return !TargetLocation.ContainsNaN();
}

void AMainGamePlayerController::Server_MoveSelectedUnits_Implementation(FVector TargetLocation, const TArray<AActor*>& ActorsToMove)
{
	if (ActorsToMove.Num() == 0) return;
	
	IGenericTeamAgentInterface* PlayerTeamAgent = Cast<IGenericTeamAgentInterface>(PlayerState);
	if (!PlayerTeamAgent) return; 

	FGenericTeamId PlayerTeamID = PlayerTeamAgent->GetGenericTeamId();
	
	// =========================================================================
	// НАСТРОЙКИ ФОРМАЦИИ КРУГА 
	// =========================================================================
	// Дистанция между солдатами в кругу (в сантиметрах). 
	// 80-100 единиц — идеальный баланс, чтобы они стояли рядом, но не терлись плечами!
	const float SeparationDistance = 90.0f; 
	
	// Золотой угол Фибоначчи (примерно 137.51 градусов). 
	// Обеспечивает математически идеальное и равномерное заполнение круга!
	const float GoldenAngle = 2.39996f; 

	int32 UnitIndex = 0;

	for (AActor* SelectedUnit : ActorsToMove)
	{
		if (!IsValid(SelectedUnit)) continue;

		APawn* UnitPawn = Cast<APawn>(SelectedUnit);
		if (!UnitPawn) continue;

		// АНТИЧИТ ПРОВЕРКА ФРАКЦИИ
		IGenericTeamAgentInterface* UnitTeamAgent = Cast<IGenericTeamAgentInterface>(SelectedUnit);
		if (!UnitTeamAgent || UnitTeamAgent->GetGenericTeamId() != PlayerTeamID)
		{
			continue; 
		}

		AStrategyAIController* AIC = Cast<AStrategyAIController>(UnitPawn->GetController());
		if (AIC)
		{
			// =========================================================================
			// МАТЕМАТИЧЕСКИЙ РАСЧЕТ ИНДИВИДУАЛЬНОЙ ТОЧКИ В КРУГУ
			// =========================================================================
			FVector IndividualTarget = TargetLocation;

			// Центр отряда (самый первый юнит) побежит ровно в точку клика TargetLocation.
			// Все последующие юниты начнут равномерно рассаживаться кругами вокруг него.
			if (UnitIndex > 0)
			{
				// Вычисляем радиус текущего витка спирали. 
				// Корень из индекса заставляет круг заполняться плотно, без дыр в центре!
				float Radius = SeparationDistance * FMath::Sqrt(static_cast<float>(UnitIndex));
				
				// Вычисляем угол поворота для текущего солдата
				float Angle = UnitIndex * GoldenAngle;

				// Находим смещение по осям X и Y относительно центра клика
				float OffsetX = Radius * FMath::Cos(Angle);
				float OffsetY = Radius * FMath::Sin(Angle);

				IndividualTarget.X += OffsetX;
				IndividualTarget.Y += OffsetY;

				// 🛠️ СТРАХОВКА НАВИГАЦИИ (Проекция на NavMesh):
				// Поскольку холмистый ландшафт процедурный, точка круга X и Y может слегка зависнуть в воздухе.
				// Проверяем, существует ли под этой точкой реальная земля NavMesh, чтобы бот не тупил!
				if (UWorld* World = GetWorld())
				{
					FNavLocation NavLocation;
					if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
					{
						// Ищем ближайшую легитимную точку на NavMesh в радиусе 1.5 метров по вертикали
						if (NavSys->ProjectPointToNavigation(IndividualTarget, NavLocation, FVector(10.f, 10.f, 150.f)))
						{
							IndividualTarget = NavLocation.Location;
						}
					}
				}
			}

			// Отдаем приказ бежать в персональное место в круговой формации!
			AIC->Command_MoveTo(IndividualTarget);
			
			// Инкрементируем счетчик, чтобы следующий солдат сел на следующий виток спирали
			UnitIndex++;
		}
	}
}


void AMainGamePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Достукиваемся до подсистемы локального игрока и отдаем ей рулевое управление мыши!
	if (IsLocalController())
	{
		if (UStrategyPlacementSubsystem* PlacementSubsystem = GetLocalPlayer()->GetSubsystem<UStrategyPlacementSubsystem>())
		{
			PlacementSubsystem->TickPlacement(DeltaTime, this);
		}
	}
}


void AMainGamePlayerController::Server_SpawnRtsFlag_Implementation(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation, int32 NewFlagNum, int32 SourceID, int32 TargetID)
{
	if (!HasAuthority()) return;
	if (!BuildingClass) return;

	AMainGamePlayerState* PS = Cast<AMainGamePlayerState>(PlayerState);
	if (!PS) return;

	uint8 PlayerFactionID = PS->GetGenericTeamId();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	
	UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	if (ServerPool)
	{
		FTransform SpawnTransform(FRotator::ZeroRotator, BuildSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
		
		AActor* SpawnedActor = ServerPool->GetActorFromNetworkPoolDeferred(BuildingClass, SpawnTransform);//, PlayerFactionID, PS);
		
		if (!SpawnedActor) return;
		
		if (ATacticalFlagBase* Entity = Cast<ATacticalFlagBase>(SpawnedActor))
		{     
			Entity->FactionID = PlayerFactionID;	
			
			Entity->SetOwner(this);
			Entity->FlagID = NewFlagNum;
			
			if (!IsLocalController()) 
			{
				if (auto ClientFlag = Cast<ATacticalFlagBase>(SpawnedActor))
				{
					ClientFlag->SetFlagVisualVisibility(false);
				}
			}
			
			if (UFlagsManagerSubsystem* FlagManager = GetWorld()->GetSubsystem<UFlagsManagerSubsystem>())
			{
				if (SourceID>0)
				{
					if (ATacticalFlagBase* Flag = FlagManager->GetFlag(PlayerFactionID,SourceID))
						Entity->AddSource(Flag);
				}
			
				if (TargetID>0)
				{
					if (ATacticalFlagBase* Flag = FlagManager->GetFlag(PlayerFactionID,TargetID))
						Entity->AddDestination(Flag);
				}
			}
		}
		
		ServerPool->FinishSpawningNetworkUnit(SpawnedActor, SpawnTransform);
	}
}


bool AMainGamePlayerController::Server_SpawnRtsFlag_Validate(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation, int32 NewFlagNum, int32 SourceID, int32 TargetID)
{
	return true;
}

void AMainGamePlayerController::Server_JastMakeLinkRtsFlag_Implementation(int32 SourceID, int32 TargetID)
{
	AMainGamePlayerState* PS = Cast<AMainGamePlayerState>(PlayerState);
	if (!PS) return;

	uint8 PlayerFactionID = PS->GetGenericTeamId();
	
	if (SourceID>0 && TargetID>0)
		if (UFlagsManagerSubsystem* FlagManager = GetWorld()->GetSubsystem<UFlagsManagerSubsystem>())
			if (ATacticalFlagBase* FlagS = FlagManager->GetFlag(PlayerFactionID,SourceID))
				if (ATacticalFlagBase* FlagD = FlagManager->GetFlag(PlayerFactionID,TargetID))
				{
					FlagD->AddSource(FlagS);
					FlagS->AddDestination(FlagD);
				}
}

bool AMainGamePlayerController::Server_JastMakeLinkRtsFlag_Validate(int32 SourceID, int32 TargetID)
{
	return  true;
}

bool AMainGamePlayerController::Server_SpawnRtsBuilding_Validate(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation)
{
	// Железобетонный серверный античит-фильтр (проверка ресурсов/лимитов на спавн этого BuildingClass)
	return true; 
}

void AMainGamePlayerController::Server_SpawnRtsBuilding_Implementation(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation)
{
	if (!HasAuthority() || !BuildingClass) return;

	AMainGamePlayerState* PS = Cast<AMainGamePlayerState>(PlayerState);
	if (!PS) return;

	//uint8 PlayerFactionID = PS->GetGenericTeamId();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	
	UServerNetworkPoolSubsystem* ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	if (ServerPool)
	{
		FTransform SpawnTransform(FRotator::ZeroRotator, BuildSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
		
		AActor* SpawnedActor = ServerPool->GetActorFromNetworkPoolDeferred(BuildingClass, SpawnTransform);//, PlayerFactionID, PS);
		
		if (!SpawnedActor) return;
		  
		if (AStrategyEntityCharacter* Entity = Cast<AStrategyEntityCharacter>(SpawnedActor))
		{     
			Entity->SetOwner(this);	
		}
		
		ServerPool->FinishSpawningNetworkUnit(SpawnedActor, SpawnTransform);
	}
}

void AMainGamePlayerController::HandleUniversalPlacementClick(AActor* HitActor, const FVector& ClickedLocation, const FPlacementBuildingData& BuildingData)
{
	if (bIsFlagMoveMode)
	{
		if (MovableFlag)
		{
			Server_MoveFlag(MovableFlag, ClickedLocation);
		}
		
		bIsFlagMoveMode = false;
		return;
	}
	// Достукиваемся до подсистемы, чтобы плавно управлять фантомами
	UStrategyPlacementSubsystem* PlacementSubsystem = GetLocalPlayer()->GetSubsystem<UStrategyPlacementSubsystem>();
	if (!PlacementSubsystem) return;

	// РАЗВОДИМ ПАТОКИ ПО ТИПУ ДАННЫХ ИЗ НАСТРОЕК СТРОИТЕЛЬСТВА
	if (!BuildingData.bIsChainBuilding || !BuildingData.RealServerBuildingClass->IsChildOf(ATacticalFlagBase::StaticClass()))
	{
		if (BuildingData.RealServerBuildingClass->IsChildOf(ATacticalFlagBase::StaticClass()))
		{
			if (HitActor && HitActor->IsA(ATacticalFlagBase::StaticClass()))
			{
				//todo select flag
				return;
			}
			LocalFlagCounter++;
			Server_SpawnRtsFlag(BuildingData.RealServerBuildingClass, ClickedLocation, LocalFlagCounter,0,0);
		}
		else
		{
			// ВЕТКА 1: Обычная постройка (Завод, Турель). Никаких бесконечных цепей!
			Server_SpawnRtsBuilding(BuildingData.RealServerBuildingClass, ClickedLocation);
		}		
	}
	else
	{
		// ВЕТКА 2: Наш продвинутый цепной флаг снабжения!
		ATacticalFlagBase* ClickedFlag = Cast<ATacticalFlagBase>(HitActor);
		
		// Передаем управление методу цепей (он унаследует всю нашу прошлую логику переброса якорей)
		HandleChainPlacement(ClickedFlag, ClickedLocation, BuildingData);
		
		// Мгновенно перезапускаем ведение фантома, чтобы цепь плелась без пауз!
		//PlacementSubsystem->ConfirmPlacement(this);
		PlacementSubsystem->StartPlacementMode(BuildingData);
	}
}


void AMainGamePlayerController::StartFlagMovement(ATacticalFlagBase* NewFlag)
{
	
}

void AMainGamePlayerController::SetFlagMoveMode(ATacticalFlagBase* Flag)
{
	if (!Flag) return;
	bIsFlagMoveMode=true;
	MovableFlag = Flag;
}

void AMainGamePlayerController::SetSpawnMode(EPortalSpawnMode SM)
{
	AMainGamePlayerState* PS = Cast<AMainGamePlayerState>(PlayerState);
	if (!PS) return;
	
	PS->Server_SetPortalSpawnMode(SM);
}


void AMainGamePlayerController::HandleChainPlacement(ATacticalFlagBase* ClickedFlag, const FVector& Location, const FPlacementBuildingData& BuildingData)
{
	TSubclassOf<AActor> ClassToSpawn = BuildingData.RealServerBuildingClass;
	if (!ClassToSpawn) return;

	UStrategyPlacementSubsystem* PlacementSubsystem = GetLocalPlayer()->GetSubsystem<UStrategyPlacementSubsystem>();
	if (!PlacementSubsystem) return;

	
	if (ClickedFlag)
	{
		if (LastActiveChainNode && LastActiveChainNode!=ClickedFlag)
		{
			int32 SourceID = (LinkDirection == EChainLinkDirection::Forward_SourceToTarget && LastActiveChainNode) ? LastActiveChainNode->FlagID : ClickedFlag->FlagID;
			int32 TargetID = (LinkDirection == EChainLinkDirection::Backward_TargetToSource && LastActiveChainNode) ? LastActiveChainNode->FlagID : ClickedFlag->FlagID;
			
			Server_JastMakeLinkRtsFlag(SourceID, TargetID);
		}
		
		
		// Клик 1 по готовому флагу: он сразу становится легитимным якорем
		LastActiveChainNode = ClickedFlag;
	}
	else
	{
		int32 SourceID = (LinkDirection == EChainLinkDirection::Forward_SourceToTarget && LastActiveChainNode) ? LastActiveChainNode->FlagID : 0;
		int32 TargetID = (LinkDirection == EChainLinkDirection::Backward_TargetToSource && LastActiveChainNode) ? LastActiveChainNode->FlagID : 0;
		
		// Клик 1 по пустой земле
		LocalFlagCounter++;
		//LastActiveChainNode = nullptr; // Настоящего актора пока нет, он летит из сети
		
		
		// Спавним времянки строго на клиенте внутри текущего мира локального игрока!
		if (UWorld* World = GetWorld())
		{
			FTransform SpawnTransform(FRotator::ZeroRotator, Location, FVector(1.0f, 1.0f, 1.0f));
			ATacticalFlagBase* CurrentLocalTempActor = World->SpawnActorDeferred<ATacticalFlagBase>(BuildingData.RealServerBuildingClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			
			if (CurrentLocalTempActor)
			{
				AMainGamePlayerState* PS = Cast<AMainGamePlayerState>(PlayerState);
				if (!PS) return;

				uint8 PlayerFactionID = PS->GetGenericTeamId();
				CurrentLocalTempActor->FactionID = PlayerFactionID;	
			
				CurrentLocalTempActor->SetOwner(this);
				CurrentLocalTempActor->FlagID = LocalFlagCounter;
				CurrentLocalTempActor->bIsLocalTempVersion = true;
				
				CurrentLocalTempActor->FinishSpawning(FTransform::Identity, true);
				LastActiveChainNode = CurrentLocalTempActor;
			}
		}
		
				
		// Просим сервер заспавнить флаг и привязать к нему наш токен!
		Server_SpawnRtsFlag(ClassToSpawn, Location, LocalFlagCounter, SourceID, TargetID);
	}

}

void AMainGamePlayerController::UpdateLastActiveChainNode(ATacticalFlagBase* NewFlag)
{
	LastActiveChainNode = NewFlag;
}

void AMainGamePlayerController::ReplaceTempLastActiveChainNode(ATacticalFlagBase* NewFlag)
{
	if (LastActiveChainNode && NewFlag && LastActiveChainNode->FlagID == NewFlag->FlagID)
	{
		LastActiveChainNode = NewFlag;
	}
}


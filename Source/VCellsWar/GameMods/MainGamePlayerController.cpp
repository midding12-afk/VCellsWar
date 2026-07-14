// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "VCellsWar/AI/StrategyAIController.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerState.h"
#include "VCellsWar/MainGame/RTSCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"

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
	
	// if (IsLocalController())
	// {
	// 	// В этой точке GetLocalPlayer() ГАРАНТИРОВАННО возвращает валидный указатель во всех режимах!
	// 	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	// 	{
	// 		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	// 		{
	// 			if (DefaultMappingContext)
	// 			{
	// 				// Накатываем контекст управления со 100% гарантией работы
	// 				Subsystem->AddMappingContext(DefaultMappingContext, 0);
	// 				UE_LOG(LogTemp, Log, TEXT("EnhancedInput: Контекст успешно активирован в PawnClientRestart!"));
	// 			}
	// 		}
	// 	}
	// }
	
	
	
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
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		if (HitResult.bBlockingHit)
		{
			Server_MoveSelectedUnits(HitResult.Location, MySelectedUnits);
		}
	}
}

/*void AMainGamePlayerController::OnLeftClickPressed()
{
	// Локально на клиенте запоминаем точку старта рамки
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		StartSelectionPoint = FVector2D(MouseX, MouseY);
		bIsSelecting = true;
	}
}

void AMainGamePlayerController::OnLeftClickReleased()
{
	// Когда игрок отпустил мышку, рамка закрывается. 
	// Логику захвата акторов мы перенесем в HUD, поэтому здесь просто опускаем флаг.
	bIsSelecting = false;
}


void AMainGamePlayerController::OnRightClickPressed()
{
	if (MySelectedUnits.Num() == 0) return; // На клиенте проверка работает!

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		if (HitResult.bBlockingHit)
		{
			// Передаем координаты КЛИКА и наш ЛОКАЛЬНЫЙ массив выделенных юнитов!
			Server_MoveSelectedUnits(HitResult.Location, MySelectedUnits);
		}
	}
}*/

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

/*void AMainGamePlayerController::Server_MoveSelectedUnits_Implementation(FVector TargetLocation, const TArray<AActor*>& ActorsToMove)
{
	
	// ТЕПЕРЬ ВСЁ ИДЕАЛЬНО: Сервер читает честный список ActorsToMove, прилетевший из сети!
	if (ActorsToMove.Num() == 0) return;
	
	// 1. УЗНАЕМ КОМАНДУ ИГРОКА, КОТОРЫЙ ПРИСЛАЛ RPC:
	// Кастуем PlayerState текущего контроллера к интерфейсу команд движка.
	// (Ваш PlayerState должен реализовывать IGenericTeamAgentInterface, возвращая свой TeamIndex 0-7)
	IGenericTeamAgentInterface* PlayerTeamAgent = Cast<IGenericTeamAgentInterface>(PlayerState);
	if (!PlayerTeamAgent) return; // Если у игрока нет фракции, рубим приказ

	FGenericTeamId PlayerTeamID = PlayerTeamAgent->GetGenericTeamId();
	
	for (AActor* SelectedUnit : ActorsToMove)
	{
		if (IsValid(SelectedUnit))
		{
			APawn* UnitPawn = Cast<APawn>(SelectedUnit);
						
			if (UnitPawn)
			{				
				// 3. АНТИЧИТ КАСТ К ИНТЕРФЕЙСУ КОМАНД ДВИЖКА:
				// Проверяем физическое тело юнита, прилетевшего от клиента
				IGenericTeamAgentInterface* UnitTeamAgent = Cast<IGenericTeamAgentInterface>(SelectedUnit);
			
				GEngine->AddOnScreenDebugMessage(uint64(this), 1.f, FColor::White, *FString::Printf(TEXT("From %d to %d"), PlayerTeamID.GetId(), UnitTeamAgent->GetGenericTeamId().GetId()));
				
				if (UnitTeamAgent)
				{
					// Если ID фракции юнита НЕ совпадает с ID фракции приславшего игрока — 
					// значит, читер попытался сжульничать и выделить чужую армию. Игнорируем этот объект!
					if (UnitTeamAgent->GetGenericTeamId() != PlayerTeamID)
					{
						continue; 
					}
				}
				else
				{
					// Если объект вообще не поддерживает интерфейс команд, бежать он не может
					continue;
				}
					
				
				AStrategyAIController* AIC = Cast<AStrategyAIController>(UnitPawn->GetController());
				if (AIC)
				{
					AIC->Command_MoveTo(TargetLocation);
				}
			}
		}
	}
}*/

// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "VCellsWar/MainGame/RTSCameraPawn.h"

void AMainGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Принудительно выставляем режим инпута для RTS при старте карты матча
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
    
	SetShowMouseCursor(true);
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

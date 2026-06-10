// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "RTSCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Камера не должна блокировать спавн юнитов в лобби, отключаем коллизию с ними
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 1200.0f; // Высота полета над землей
	SpringArmComponent->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // Наклон вниз
	SpringArmComponent->bDoCollisionTest = false; // Чтобы камера не дергалась, если пролетает сквозь деревья

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	
	CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
	CameraComponent->bUsePawnControlRotation = false;

	// Отключаем синхронизацию движения камеры по сети ради оптимизации
	SetReplicates(false);
	SetReplicateMovement(false);
}

void ARTSCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// Подключаем контекст управления (только для локального игрока)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			// КРИТИЧЕСКИ ВАЖНО ДЛЯ РЕДАКТОРА:
			// Сбрасываем ротацию контроллера в ноль (Pitch=0, Yaw=0, Roll=0).
			// Это сотрет наклон камеры, который редактор насильно передал из вьюпорта при нажатии Play.
			PC->SetControlRotation(FRotator::ZeroRotator);
			

			// Дополнительно страхуем сам Pawn, возвращая его ротацию в дефолтное положение
			SetActorRotation(FRotator::ZeroRotator);
			//SetActorLocation(FVector::ZeroVector);
			
			// if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			// {
			// 	Subsystem->AddMappingContext(CameraMappingContext, 0);
			// }
			PC->SetShowMouseCursor(true); // В RTS курсор должен гореть всегда
		}
	}
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Каждый кадр проверяем, не прижал ли игрок мышку к краю экрана
	CheckEdgePan(DeltaTime);
}

void ARTSCameraPawn::Move(const FInputActionValue& Value)
{
	// Логика движения по WASD (выполняется только локально)
	FVector2D MoveVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		FVector Forward = GetActorForwardVector();
		Forward.Z = 0.0f; // Двигаемся строго по плоскости земли
		Forward.Normalize();

		FVector Right = GetActorRightVector();

		AddActorWorldOffset(Forward * MoveVector.Y * GetCalkMoveSpeed() * GetWorld()->GetDeltaSeconds(), true);
		AddActorWorldOffset(Right * MoveVector.X * GetCalkMoveSpeed() * GetWorld()->GetDeltaSeconds(), true);
	}
}

void ARTSCameraPawn::Zoom(const FInputActionValue& Value)
{
	// Плавный зум колесиком мыши
	float ZoomValue = Value.Get<float>();
	if (SpringArmComponent)
	{
		float NewLen = SpringArmComponent->TargetArmLength + (ZoomValue * ZoomSpeed);
		SpringArmComponent->TargetArmLength = FMath::Clamp(NewLen, 400.0f, 30000.0f); // Ограничения зума
	}
}

void ARTSCameraPawn::CheckEdgePan(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;

	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	float MouseX, MouseY;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		FVector MoveDirection(0.0f, 0.0f, 0.0f);
		float Margin = 20.0f; // Зона активации в пикселях от края экрана

		if (MouseX <= Margin) MoveDirection.Y = -1.0f; // Левый край
		if (MouseX >= ViewportSizeX - Margin) MoveDirection.Y = 1.0f; // Правый край
		if (MouseY <= Margin) MoveDirection.X = 1.0f; // Верхний край
		if (MouseY >= ViewportSizeY - Margin) MoveDirection.X = -1.0f; // Нижний край

		if (!MoveDirection.IsZero())
		{
			MoveDirection.Normalize();
			AddActorWorldOffset(MoveDirection * GetCalkMoveSpeed() * DeltaTime, true);
		}
	}
}

float ARTSCameraPawn::GetCalkMoveSpeed()
{
	return MoveSpeed * SpringArmComponent->TargetArmLength*0.0001f;
}

void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Приводим базовый компонент ввода к современной системе Enhanced Input
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 1. Привязываем движение по WASD (или стрелочкам). 
		// Используем ETriggerEvent::Triggered, чтобы функция Move вызывалась каждый кадр, пока кнопка зажата.
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARTSCameraPawn::Move);

		// 2. Привязываем зум колесиком мыши
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARTSCameraPawn::Zoom);
	}
}

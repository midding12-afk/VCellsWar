// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyEntityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AStrategyEntityCharacter::AStrategyEntityCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Включаем сетевую репликацию для объекта!
	bReplicates = true;
	bAlwaysRelevant = true; 
	//SetReplicateMovement(true); // Юниты будут двигаться, постройкам можно выключить в наследниках
	
	// --- ОПТИМИЗАЦИЯ CHARACTER MOVEMENT ДЛЯ RTS ---
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->bOrientRotationToMovement = true; // Человечек сам разворачивается туда, куда идет
		MoveComp->RotationRate = FRotator(0.0f, 600.0f, 0.0f); // Быстрый разворот

		// Отключаем тяжелые калькуляции, ненужные плоским бумажным человечкам
		//MoveComp->bMaintainAnchorToBase = false;
		MoveComp->bCanWalkOffLedges = true;
		MoveComp->bImpartBaseVelocityX = false;
		MoveComp->bImpartBaseVelocityY = false;
		MoveComp->bImpartBaseVelocityZ = false;
		
		// Задаем базовую скорость ходьбы
		MoveComp->MaxWalkSpeed = 400.0f;
		MoveComp->GravityScale = 1.5f; // Делаем гравитацию чуть тяжелее, чтобы они падали бодрее
	}
	
	// АВТО-СПАВН ИИ МОЗГА НА СЕРВЕРЕ:
	// Эта строчка приказывает движку: как только GameMode спавнит этого павна на сервере,
	// сервер обязан мгновенно заспавнить для него невидимый AIController и вселить его внутрь павна.
	// Без этой строчки юниты будут стоять как овощи и не смогут ходить по командам.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	OwningPlayerState = nullptr;
}

// Called when the game starts or when spawned
void AStrategyEntityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStrategyEntityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AStrategyEntityCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AStrategyEntityCharacter, OwningPlayerState);
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

void AStrategyEntityCharacter::OnRep_OwningPlayerColor()
{
}

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
		}
	}
}

void AStrategyEntityCharacter::SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState)
{
	if (!NewOwnerState) return;
	
	OwningPlayerState = NewOwnerState;
	OwningPlayerColor = OwningPlayerState->GetTeamColor();
}

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/StrategyEntityInterface.h"
#include "StrategyEntityCharacter.generated.h"

UCLASS()
class VCELLSWAR_API AStrategyEntityCharacter : public ACharacter, public IStrategyEntityInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AStrategyEntityCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	// Самый главный указатель. Реплицируется от сервера к клиентам.
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerState, BlueprintReadOnly, Category = "Strategy | Ownership")
	AMainGamePlayerState* OwningPlayerState;

	UFUNCTION()
	void OnRep_OwningPlayerState();
	
	virtual void Landed(const FHitResult& Hit) override;
	
	// Физическая переменная цвета, которая реплицируется по сети
	// UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Visual")
	// FLinearColor TeamColor;
	//
	// UFUNCTION()
	// void OnRep_TeamColor();
public:
	// Нам НЕ нужны макросы UFUNCTION здесь! Они автоматически унаследовались из интерфейса.
	// virtual void SetTeamColor_Implementation(FLinearColor NewColor) override {TeamColor = NewColor;};
	virtual FLinearColor GetTeamColor_Implementation() const override {return OwningPlayerState ? OwningPlayerState->GetTeamColor() : FLinearColor::Gray;};
	
	virtual AMainGamePlayerState* GetEntityOwnerState_Implementation() override {return  OwningPlayerState;};
	//virtual void SetEntityOwner(AMainGamePlayerState* NewOwnerState) override { OwningPlayerState = NewOwnerState; };
	//virtual bool IsOwnedByLocalPlayer_Implementation() const override;
protected:
	virtual void SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState) override;
	
public:
	
	void LaunchFromPortal(FVector PortalForwardDirection);
};

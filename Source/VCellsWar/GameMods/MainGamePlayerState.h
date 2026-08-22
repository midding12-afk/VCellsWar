// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "MainGamePlayerState.generated.h"

/**
 * 
 */
class UAbilitySystemComponent;

 
UCLASS()
class VCELLSWAR_API AMainGamePlayerState : public APlayerState, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AMainGamePlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	FLinearColor GetTeamColor() const;
	void SetTeamColor(FLinearColor NewTeamColor);
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override {TeamIndex = TeamID;}
	
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamIndex; }
	
	// Реализуем обязательный C++ метод интерфейса GAS
	UFUNCTION(BlueprintCallable, Category = "RTS | GAS")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "RTS | Combat | GAS")
	void SetGASAvatarForSoldier(AActor* SoldierAvatar);

	// Выносим выдачу способностей в отдельный чистый метод
	void Override_GiveFactionDefaultAbilities();
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Strategy | Player")
	FLinearColor TeamColor = FLinearColor::Gray;

	UFUNCTION()
	void OnRep_TeamColor();
	
	UPROPERTY(Replicated)
	FGenericTeamId TeamIndex;
	
	// Объявляем сам компонент. В RTS он живет строго на сервере и реплицируется клиентам
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | GAS", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	virtual void BeginPlay() override;

	// СВЯЩЕННЫЙ МАССИВ RTS-СПОСОБНОСТЕЙ ИГРОКА:
	// Сюда мы выберем нашу GA_BlasterAttack, а также будущие апгрейды и технологии
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | GAS")
	TArray<TSubclassOf<class UGameplayAbility>> FactionDefaultAbilities;

};

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "MainGamePlayerState.generated.h"


UENUM(BlueprintType)
enum class EPortalSpawnMode : uint8
{
	ToWorld,         // Выдавать солдат физически на карту
	ToVirtualBuffer  // Направлять поток в виртуальный буфер подпространства
};
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

	
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamColor, BlueprintReadOnly, Category = "Strategy | Player")
	FLinearColor TeamColor = FLinearColor::Gray;

	UFUNCTION()
	void OnRep_TeamColor();
	
	UPROPERTY(Replicated)
	FGenericTeamId TeamIndex;
	

	virtual void BeginPlay() override;

	////////////////////////////////////////////////////////
	///		GAS
	////////////////////////////////////////////////////////
public:
	// Реализация интерфейса системы способностей Unreal Engine
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FORCEINLINE class URTSAttributeSet* GetRTSAttributeSet() const { return RTSAttributeSet; }
	FORCEINLINE EPortalSpawnMode GetPortalSpawnMode() const { return CurrentSpawnMode; }

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "RTS | Economy")
	void Server_SetPortalSpawnMode(EPortalSpawnMode NewMode);
	
	/** Возвращает суммарное производство энергии всеми генераторами */
	UFUNCTION(BlueprintCallable, Category = "RTS | Energy")
	float GetTotalEnergyProduction() const;

	/** Возвращает суммарное потребление энергии всеми домиками/турелями */
	UFUNCTION(BlueprintCallable, Category = "RTS | Energy")
	float GetTotalEnergyConsumption() const;

	/** Проверка: Перегружена ли сеть прямо сейчас? (Расход > Выработки) */
	UFUNCTION(BlueprintCallable, Category = "RTS | Energy")
	bool IsPowerGridOverloaded() const;
	
	void OnVirtualTroopsAttributeChanged(const struct FOnAttributeChangeData& Data);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTroopsInBufferCountChanged, int32, TroopsInBufferCount);
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby|UI")
	FOnTroopsInBufferCountChanged OnTroopsInBufferCountChanged;
	
	// Нативный OnRep метод движка для PlayerState. На КЛИЕНТАХ он гарантированно 
	// срабатывает, когда данные игрока отреплицировались.
	virtual void OnRep_PlayerId() override;


private:
	/** Вспомогательный метод, чтобы не дублировать код подписки */
	void BindGASAttributeCallbacks();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | GAS")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	class URTSAttributeSet* RTSAttributeSet;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Economy")
	EPortalSpawnMode CurrentSpawnMode = EPortalSpawnMode::ToWorld;
	
	////////////////////////////////////////////////////////
	///		/GAS
	////////////////////////////////////////////////////////
};

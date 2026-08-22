// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalFlagBase.generated.h"

class UDecalComponent;
class UNiagaraComponent;
class UWidgetComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType)
class VCELLSWAR_API ATacticalFlagBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ATacticalFlagBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	/** Универсальный метод для скрытия всего визуала флага на текущем экране */
	void SetFlagVisualVisibility(bool bIsVisible);
	
	virtual void Tick(float DeltaTime) override;
	
	void AddDestination(ATacticalFlagBase* Flag);
	void AddSource(ATacticalFlagBase* Flag);
	

protected:
	virtual void BeginPlay() override;	
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** ВИЗУАЛЬНЫЕ КОМПОНЕНТЫ ФЛАГА */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;

	/** Меш-пин флага на земле */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FlagMesh;

	/** Декаль радиуса действия (Крутящийся радар/сонар) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> RadiusDecal;

	/** Столб света, бьющий вверх */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> LightBeamNiagara;

	/** Голографический 3D-интерфейс над флагом */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InfoWidgetComponent;

public:
	/** СЕТЕВЫЕ ПАРАМЕТРЫ */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Flag")
	uint8 FactionID = 0;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Flag")
	uint8 FlagID = -1;

	/** Динамический радиус действия флага */
	UPROPERTY(ReplicatedUsing = OnRep_FlagRadius, EditAnywhere, BlueprintReadWrite, Category = "RTS|Flag")
	float FlagRadius = 1200.0f;

	/** РЕПЛИЦИРУЕМЫЙ СЧЕТЧИК ВОЙСК (Для отображения в UI на клиенте!) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentUnitsCount, BlueprintReadOnly, Category = "RTS|Flag")
	int32 CurrentUnitsCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RTS|Flag|Input")
	bool bIsBeingDragged = false;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpdateFlagLocation(FVector NewWorldLocation);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpdateFlagRadius(float NewFlagRadius);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitsCountChanged, int32, NewCount);
	
	UPROPERTY(BlueprintAssignable, Category = "RTS|Events")
	FOnUnitsCountChanged OnUnitsCountChanged;
	
	UFUNCTION() // UFUNCTION обязателен, так как делегат динамический!
	void HandleOnRadiusSliderChanged(float NewRadiusValue);
	
	UFUNCTION()
	void HandleOnRadiusSliderChangedLocal(float NewRadiusValue);
	
	UFUNCTION()
	void HandleOnButtonDeletePressed();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DestroyFlag();
	
	UFUNCTION()
	void HandleOnButtonDeleteSourcePressed();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DeleteSource();
	
	UFUNCTION()
	void HandleOnButtonDeleteDestinationPressed();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DeleteDestination();
	
	virtual void OnRep_ReplicatedMovement() override;
	
	UFUNCTION()
	void HandleOnUiCheckboxInFilterChanged(bool NewState);
	
	UFUNCTION()
	void HandleOnUiCheckboxOutFilterChanged(bool NewState);
	
	UFUNCTION()
	void HandleOnUiCountInFilterChanged(int32 NewCount);
	
	UFUNCTION()
	void HandleOnUiCountOutFilterChanged(int32 NewCount);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnUiCheckboxInFilterChanged(bool NewState);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnUiCheckboxOutFilterChanged(bool NewState);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnUiCountInFilterChanged(int32 NewCount);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnUiCountOutFilterChanged(int32 NewCount);
	
	
	UPROPERTY()
	bool bIsLocalTempVersion = false;
	
	void UpdateAllFlagDestinations();
	void UpdateAllFlagSources();
	
	void UpdateFlagDestination(ATacticalFlagBase* Flag);
	void UpdateFlagSource(ATacticalFlagBase* Flag);
	
	void RemoveDestination(ATacticalFlagBase* Flag);
	void RemoveSource(ATacticalFlagBase* Flag);
	
	void DegreaseIncomingTroopsCount() {if (IncomingTroopsCount>0) IncomingTroopsCount--;};

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Collision", meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* SelectionCollider;
	
	UFUNCTION() 
	void OnRep_FlagRadius();
	
	UFUNCTION() 
	void OnRep_CurrentUnitsCount();
	
	/** Метод-слушатель сети: срабатывает на клиентах при изменении структуры графа снабжения */
	UFUNCTION()
	void OnRep_NetworkLinksChanged();
	
	/** Метод-слушатель сети: зажигает или тушит визуал глобального спавн-хаба */
	UFUNCTION()
	void OnRep_bIsGlobalRallyPoint();
	
	/** Наш ААА-компонент для реактивного вращения меша в обход тяжелого Tick актора */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class URotatingMovementComponent> RotatingMovementComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	TSubclassOf<UUserWidget> InfoWidgetClass;
	
	/** СЕТЕВОЙ МАССИВ: Флаги, которые качают солдат СЮДА (Наши источники) */
	UPROPERTY(ReplicatedUsing = OnRep_NetworkLinksChanged, BlueprintReadOnly, Category = "RTS|Logistics")
	TArray<TObjectPtr<ATacticalFlagBase>> SourceFlags;

	/** СЕТЕВОЙ МАССИВ: Флаги, куда ЭТОТ флаг перенаправляет избыток солдат (Наши назначения) */
	UPROPERTY(ReplicatedUsing = OnRep_NetworkLinksChanged, BlueprintReadOnly, Category = "RTS|Logistics")
	TArray<TObjectPtr<ATacticalFlagBase>> DestinationFlags;
	
	UPROPERTY()
	TMap<TWeakObjectPtr<ATacticalFlagBase>, class ADecalLineBase*> FlagDestinationConnections;
	
	/** Жесткий лимит удержания: сколько солдат должно МИНИМУМ оставаться здесь на защите */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Logistics|Limits", meta = (ClampMin = "0"))
	int32 MinimumRetainedUnitsCount = 5;

	/** Максимальная емкость: выше этого капа флаг временно блокирует приток новых подкреплений */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Logistics|Limits", meta = (ClampMin = "1"))
	int32 MaximumCapacityLimit = 30;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Logistics|Limits", meta = (ClampMin = "1"))
	bool bIsFilterInCount = false;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Logistics|Limits", meta = (ClampMin = "1"))
	bool bIsFilterOutCount = false;
	
	/** СЕТЕВОЙ ТРИГГЕР: Назначен ли этот флаг глобальной точкой сбора спавна всей фракции */
	UPROPERTY(ReplicatedUsing = OnRep_bIsGlobalRallyPoint, BlueprintReadOnly, Category = "RTS|Rally")
	bool bIsGlobalRallyPoint = false;
	
	// UPROPERTY(BlueprintReadOnly, Category = "RTS|Logistics")
	// TArray<TObjectPtr<ATroopBase>> AttachedTroops;
	//
	// UPROPERTY(BlueprintReadOnly, Category = "RTS|Logistics")
	// TArray<TObjectPtr<ATroopBase>> IncommingTroops;
	
	// Серверные структуры для менеджмента солдат (Чисто серверные данные)
	UPROPERTY()
	TArray<class ATroopBase*> BoundTroops;
	UPROPERTY()
	TArray<class ATroopBase*> IncomingTroopsArray;
	
	int32 IncomingTroopsCount = 0;
	
	bool bNeedsFormationUpdate = false;

public:
	// Логика менеджмента солдат (Серверная авторитарность)
	void BindTroop(ATroopBase* Troop);
	void UnbindTroopByIndex(int32 Index);
	
	FORCEINLINE void DecrementIncomingTroops() { IncomingTroopsCount = FMath::Max(0, IncomingTroopsCount - 1); }
	
	void RegisterIncomingTroopForMovement(ATroopBase* Troop);

private:
	void ProcessSurplusSending(int32 SurplusCount);
	void Server_ExecuteDeferredMovement();


	/** СЕРВЕРНЫЙ ХИРУРГИЧЕСКИЙ ТАЙМЕР */
	FTimerHandle TimerHandle_UpdateCounter;
	
	/** Вызывается строго раз в секунду на сервере */
	void UpdateTroopsCounter();

	/** Утилита настройки размеров декали под радиус */
	void UpdateDecalSize(float Radius);
	
};

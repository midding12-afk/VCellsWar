// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h" 
#include "MainGamePlayerController.generated.h"

/**
 * 
 */
// Опережающее объявление классов Enhanced Input
class UInputMappingContext;
class UInputAction;
struct FPlacementBuildingData; 
class ATacticalFlagBase;

UENUM(BlueprintType)
enum class EChainLinkDirection : uint8
{
	/** Предыдущий клик — это Источник, следующий — это Назначение */
	Forward_SourceToTarget,
	
	/** Предыдущий клик — это Назначение, следующий — это Источник */
	Backward_TargetToSource
};

UCLASS()
class VCELLSWAR_API AMainGamePlayerController : public APlayerController
{
	GENERATED_BODY()
public:	
	AMainGamePlayerController();
	
	
	// Этот метод гарантирует, что павн ПОЛНОСТЬЮ перешел под контроль клиента на новой карте
	virtual void AcknowledgePossession(APawn* P) override;
	
	void TeleportLocalCameraTo(FVector2D CenterLocation);
	
	// Наш динамический список юнитов, которых игрок обвел рамкой на экране.
	// UPROPERTY() обязателен, чтобы Garbage Collector не стер указатели!
	UPROPERTY(BlueprintReadWrite, Category = "RTS | Selection")
	TArray<AActor*> MySelectedUnits;
	
	UFUNCTION(BlueprintCallable)
	void SetSelectedList(TArray<AActor*> NewSelectedUnits);
	
	// Флаг: зажата ли рамка прямо сейчас
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Selection")
	bool bIsSelecting = false;

	// Точка экрана, где игрок нажал мышку (Координаты X, Y)
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Selection")
	FVector2D StartSelectionPoint;
	
	/** УНИВЕРСАЛЬНЫЙ СЕРВЕРНЫЙ RPC-ШЛЮЗ СТРОИТЕЛЬСТВА */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RTS|Network")
	void Server_SpawnRtsBuilding(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RTS|Network")
	void Server_SpawnRtsFlag(TSubclassOf<AActor> BuildingClass, FVector BuildSpawnLocation, int32 NewFlagNum, int32 SourceID, int32 TargetID);
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "RTS|Network")
	void Server_JastMakeLinkRtsFlag(int32 SourceID, int32 TargetID);

	virtual void PlayerTick(float DeltaTime) override;
	
	// СЕТЕВОЙ ШЛЮЗ ДЛЯ КЛИЕНТОВ (Срабатывает при репликации пешки)
	virtual void OnRep_Pawn() override;

	/** ГЛАВНЫЙ УНИВЕРСАЛЬНЫЙ ШЛЮЗ ВВОДА: Сюда подсистема шлет ЛЮБОЙ клик ЛКМ */
	void HandleUniversalPlacementClick(AActor* HitActor, const FVector& ClickedLocation, const FPlacementBuildingData& BuildingData);
	
	UFUNCTION(BlueprintCallable, Category = "RTS|Flag")
	void UpdateLastActiveChainNode(ATacticalFlagBase* NewFlag);
	void ReplaceTempLastActiveChainNode(ATacticalFlagBase* NewFlag);
	
	UPROPERTY(BlueprintReadWrite, Category = "RTS|Flag")
	EChainLinkDirection LinkDirection = EChainLinkDirection::Forward_SourceToTarget;
	
	void StartFlagMovement(ATacticalFlagBase* NewFlag);
	
	
	UFUNCTION(BlueprintCallable, Category = "RTS|Flag")
	void SetFlagMoveMode(ATacticalFlagBase* Flag);
	
	UPROPERTY()
	class AAIGeneralDirector* EnemyAiDirector;

private:

	/** МЕТОД 2: Бесконечный двунаправленный конвейер логистических цепей флагов */
	void HandleChainPlacement(class ATacticalFlagBase* ClickedFlag, const FVector& Location, const FPlacementBuildingData& BuildingData);
	
	UPROPERTY()
	int32 LocalFlagCounter = 0;
	UPROPERTY()
	bool bIsInfiniteChainActive	= false;
	
	UPROPERTY()
	ATacticalFlagBase* LastActiveChainNode;
	
	
protected:
	UPROPERTY(BlueprintReadWrite, Category = "RTS|Flag")
	bool bIsFlagMoveMode = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "RTS|Flag")
	ATacticalFlagBase* MovableFlag = nullptr;
	
	UFUNCTION(Client, Reliable, BlueprintCallable, meta = (CPP_Default_Z = -1.0f))
	void Client_TeleportCamera(FVector2D TargetLocation, float Z);
	
	virtual void SetupInputComponent() override;
	
	virtual void BeginPlay() override;
	

	// СЕТЕВОЙ ШЛЮЗ ДЛЯ СЕРВЕРА / STANDALONE (Срабатывает при захвате пешки на сервере)
	virtual void OnPossess(APawn* InPawn) override;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RTS|Network")
	void Server_MoveFlag(ATacticalFlagBase* Flag, FVector Location);

private:
	// Наша общая безопасная функция, которая физически включает Enhanced Input
	void InitializeRTSInput();
	
	// переменная для отслеживания шагов постройки цепочек снабжения
	UPROPERTY()
	TObjectPtr<ATacticalFlagBase> StoredFirstFlagNode = nullptr;

	bool bIsBuildingChainActive = false;
	

public:

	UFUNCTION(Server, Reliable, WithValidation, Category = "RTS | Network Commands")
	void Server_MoveSelectedUnits(FVector TargetLocation, const TArray<AActor*>& ActorsToMove);
protected:	
	// АССЕТЫ УПРАВЛЕНИЯ: Выставим их в Blueprint-наследнике PlayerController
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputMappingContext* DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputAction* ActionLeftClick;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS | Input")
	UInputAction* ActionRightClick;
	
	// Сигнатуры функций Enhanced Input жестко требуют аргумент const FInputActionValue& Value!
	void OnLeftClickStarted(const FInputActionValue& Value);
	void OnLeftClickCompleted(const FInputActionValue& Value);
	void OnRightClickPressed(const FInputActionValue& Value);
};

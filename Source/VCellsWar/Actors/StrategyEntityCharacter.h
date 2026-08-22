// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/StrategyEntityInterface.h"
#include "VCellsWar/Systems/StrategyGridSubsystem.h"
#include "StrategyEntityCharacter.generated.h"

UCLASS()
class VCELLSWAR_API AStrategyEntityCharacter : public ACharacter, public IStrategyEntityInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AStrategyEntityCharacter();
	
	virtual FGenericTeamId GetGenericTeamId() const override;
	void PossessedBy(AController* NewController) override;


	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void InitCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	virtual EStrategyEntityCategory GetEntityCategory_Implementation() const override {return EStrategyEntityCategory::Troop;};
protected:
	// Самый главный указатель. Реплицируется от сервера к клиентам.
	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerState, BlueprintReadOnly, Category = "Strategy | Ownership")
	AMainGamePlayerState* OwningPlayerState;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Strategy | Ownership")
	FLinearColor OwningPlayerColor = FLinearColor::Gray;

	UFUNCTION()
	void OnRep_OwningPlayerState();
	
	
	UFUNCTION()
	void OnRep_SpawnGeneration();
	
	UPROPERTY()//(BlueprintReadOnly, Category = "RTS | Team")
	int32 CachedFactionID = 255;
	
public:
	virtual void Landed(const FHitResult& Hit) override;
	
	// Нам НЕ нужны макросы UFUNCTION здесь! Они автоматически унаследовались из интерфейса.
	virtual FLinearColor GetTeamColor_Implementation() const override {return OwningPlayerColor;};
	
	virtual AMainGamePlayerState* GetEntityOwnerState_Implementation() override {return  OwningPlayerState;};
	virtual void SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState) override;
	

	UPROPERTY(ReplicatedUsing=OnRep_SpawnGeneration)
	int32 SpawnGeneration = 0;
	void LaunchFromPortal(FVector PortalForwardDirection);
	
	virtual int32 GetEntityFactionID_Implementation() const override {return GetGenericTeamId();};
	
	FORCEINLINE int32 GetRTSFactionIDDirect() const { return CachedFactionID; }
	
	virtual void NativeRTSInitialize(int32 InFactionID, class AMainGamePlayerState* InOwnerState, const FTransform& InSpawnTransform) override;
	virtual void NativeRTSDeinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "RTS | Selection")
	virtual void SelectEntity() override;

	UFUNCTION(BlueprintCallable, Category = "RTS | Selection")
	virtual void DeselectEntity() override;
	
	virtual void SetSelectedSectorGridIndex(int NewSelectedSectorGridIndex) override {SelectedSectorGridIndex = NewSelectedSectorGridIndex;};
	virtual int GetSelectedSectorGridIndex() override {return SelectedSectorGridIndex;};
	
	virtual bool NativeRTSIsEntitySelected() const override;

protected:
	// С++ компонент декали, который проецирует зеленый круг на землю под солдатом
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS | Selection")
	class UDecalComponent* SelectionDecalComponent;

	// Ссылка на материал кольца (будет настраиваться в Блупринте)
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS | Selection")
	//class UMaterialInterface* SelectionDecalMaterial;
	
	int SelectedSectorGridIndex = -1;
	
	UPROPERTY()
	class UStrategyGridComponent* GridTrackingComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Components")
	class URTSPathVisualizerComponent* PathVisualizerComponent;
	
};

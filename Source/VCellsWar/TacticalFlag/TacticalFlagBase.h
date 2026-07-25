// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalFlagBase.generated.h"

//class UStaticMeshComponent;

UCLASS() 
class VCELLSWAR_API ATacticalFlagBase : public AActor
{
	GENERATED_BODY()
	
/*
public:	
	ATacticalFlagBase();
	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

protected:
	virtual void BeginPlay() override;
	

	/** БАЗОВЫЕ КОМПОНЕНТЫ #1#
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FlagMesh;

	/** Сфера для визуализации/физики радиуса в мире (опционально) #1#
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> TargetRadiusSphere;

	
public:
	/** СЕТЕВЫЕ ПАРАМЕТРЫ ФЛАГА #1#
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RTS|Flag")
	uint8 FactionID = -1;

	/** Динамический радиус действия флага #1#
	UPROPERTY(ReplicatedUsing = OnRep_FlagRadius, EditAnywhere, BlueprintReadWrite, Category = "RTS|Flag")
	float FlagRadius = 500.0f;

	UFUNCTION()
	void OnRep_FlagRadius();
	void UpdateTroopsCounter();

	/** Публичный С++ геттер: возвращает текущее число своих юнитов внутри FlagRadius #1#
	UFUNCTION(BlueprintCallable, Category = "RTS|Flag")
	int32 GetCurrentUnitsInRadius() const;

	/** СЕТЕВОЙ DRAG-AND-DROP (ПЕРЕТАСКИВАНИЕ) #1#
	/** Переключатель: зажат ли этот флаг мышкой локального игрока прямо сейчас #1#
	UPROPERTY(BlueprintReadWrite, Category = "RTS|Flag|Input")
	bool bIsBeingDragged = false;

	/** Запрос серверу на физическое перемещение флага в рантайме #1#
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpdateFlagLocation(FVector NewWorldLocation);

private:
	/** Переменная для кэширования числа юнитов (чтобы не насиловать хэш-грид каждый кадр) #1#
	int32 CachedUnitsCount = 0;
	float CountTimer = 0.0f;
	const float CountInterval = 1.f; // Обновляем циферку 
	
	FTimerHandle GridUpdateTimerHandle;*/
};
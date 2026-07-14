// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VCellsWar/Actors/DecalMoveTargetBase.h"
#include "VCellsWar/Actors/DecalLineBase.h"
#include "RTSPathVisualizerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VCELLSWAR_API URTSPathVisualizerComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	URTSPathVisualizerComponent();

	// На сервере вызываем эту функцию, чтобы обновить цель (передаем результат из AIC->GetImmediateMoveDestination())
	void SetNewMoveDestination(const FVector& NewDestination);

	// Очистка при остановке/уходе в пул
	void DeactivateTracking();
	void ClearActivePath();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Обязательный метод для регистрации репликации в C++
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Сработает на клиенте, когда сервер обновит точку назначения
	UPROPERTY(ReplicatedUsing = OnRep_TargetDestination)
	FVector TargetDestination;

	UFUNCTION()
	void OnRep_TargetDestination();

	// Локальный тик обновления положения одной линии
	void ManagePathUpdate();
	

private:
	FTimerHandle UpdateTimerHandle;

	// Теперь нам нужна всего ОДНА линия-декаль вместо массива
	TWeakObjectPtr<class ADecalLineBase> ActivePathLine;
	
	UPROPERTY()
	class ADecalMoveTargetBase* MyActiveDestination = nullptr;

/*  вариант для ии контроллера(нет на клиентах)
public:	
	// Sets default values for this component's properties
	URTSPathVisualizerComponent();
	
	void ActivateTracking();
	void DeactivateTracking();
	void ClearActivePath();

protected:
	void ManagePathUpdate();	

	//UPROPERTY()
	//TArray<ADecalLineBase*, TInlineAllocator<8>> MyActivePathLines;
	TArray<TWeakObjectPtr<ADecalLineBase>, TInlineAllocator<8>> MyActivePathLines;
	
	UPROPERTY()
	ADecalMoveTargetBase* MyActiveDestination;
private:	
	FTimerHandle UpdateTimerHandle;
	*/

		
};

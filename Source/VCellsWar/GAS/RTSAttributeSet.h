// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RTSAttributeSet.generated.h"

// Автоматический макрос Unreal Engine для генерации геттеров и сеттеров GAS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class VCELLSWAR_API URTSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	URTSAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 1. Виртуальный пул солдат в подпространстве */
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Resources", ReplicatedUsing = OnRep_VirtualTroopsReserve)
	FGameplayAttributeData VirtualTroopsReserve;
	ATTRIBUTE_ACCESSORS(URTSAttributeSet, VirtualTroopsReserve);

	/** 2. Прирост солдат за макро-цикл (Домики будут увеличивать этот атрибут) */
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Resources", ReplicatedUsing = OnRep_TroopIncomePerCycle)
	FGameplayAttributeData TroopIncomePerCycle;
	ATTRIBUTE_ACCESSORS(URTSAttributeSet, TroopIncomePerCycle);

	/** 3. Суммарный РАСХОД энергии (Потребление инфраструктуры: домики, турели) */
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Energy", ReplicatedUsing = OnRep_CurrentEnergy)
	FGameplayAttributeData CurrentEnergy;
	ATTRIBUTE_ACCESSORS(URTSAttributeSet, CurrentEnergy);

	/** 4. Суммарный ПРИТОК энергии (Максимальное производство генераторов) */
	UPROPERTY(BlueprintReadOnly, Category = "RTS | Energy", ReplicatedUsing = OnRep_MaxEnergy)
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(URTSAttributeSet, MaxEnergy);

protected:
	UFUNCTION() virtual void OnRep_VirtualTroopsReserve(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_TroopIncomePerCycle(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_CurrentEnergy(const FGameplayAttributeData& OldValue);
	UFUNCTION() virtual void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);
};

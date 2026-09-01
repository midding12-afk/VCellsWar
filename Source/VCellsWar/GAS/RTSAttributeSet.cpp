// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "RTSAttributeSet.h"
#include "Net/UnrealNetwork.h"

URTSAttributeSet::URTSAttributeSet()
{
	// Инициализация стартовых макро-параметров матча
	InitVirtualTroopsReserve(0.0f);
	InitTroopIncomePerCycle(10.0f); // Базово 10 солдат за цикл 30с
	InitCurrentEnergy(0.0f);        // Стартовый расход равен 0 (зданий еще нет)
	InitMaxEnergy(0.0f);            // Стартовое производство равно 0 (генераторов еще нет)
}

void URTSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Обеспечиваем автоматическую сетевую репликацию GAS ресурсов
	DOREPLIFETIME_CONDITION_NOTIFY(URTSAttributeSet, VirtualTroopsReserve, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URTSAttributeSet, TroopIncomePerCycle, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URTSAttributeSet, CurrentEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URTSAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
}

void URTSAttributeSet::OnRep_VirtualTroopsReserve(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(URTSAttributeSet, VirtualTroopsReserve, OldValue); }
void URTSAttributeSet::OnRep_TroopIncomePerCycle(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(URTSAttributeSet, TroopIncomePerCycle, OldValue); }
void URTSAttributeSet::OnRep_CurrentEnergy(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(URTSAttributeSet, CurrentEnergy, OldValue); }
void URTSAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(URTSAttributeSet, MaxEnergy, OldValue); }

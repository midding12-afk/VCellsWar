// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"
#include "VCellsWar/GAS/RTSAttributeSet.h"

AMainGamePlayerState::AMainGamePlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	RTSAttributeSet = CreateDefaultSubobject<URTSAttributeSet>(TEXT("RTSAttributeSet"));
}

void AMainGamePlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	
	if (HasAuthority() && AbilitySystemComponent && RTSAttributeSet)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, GetOwner());
		
	}
	
	// Пробуем подписаться (сработает на сервере/хосте)
	BindGASAttributeCallbacks();
}


// === ЛОГИКА ДЛЯ СЕТЕВЫХ КЛИЕНТОВ (КОГДА ДАННЫЕ ПРИЛЕТЕЛИ ПО СЕТИ) ===
void AMainGamePlayerState::OnRep_PlayerId()
{
	Super::OnRep_PlayerId();

	if (AbilitySystemComponent)
	{
		// Связываем GAS на стороне клиента с его локальным контроллером-владельцем
		AbilitySystemComponent->InitAbilityActorInfo(this, GetOwner());
		
		// Данные долетели — привязываем колбэки интерфейса!
		BindGASAttributeCallbacks();
	}
}

void AMainGamePlayerState::BindGASAttributeCallbacks()
{
	// Проверяем, что компоненты GAS и атрибутов физически существуют в памяти на этой машине
	if (AbilitySystemComponent && RTSAttributeSet)
	{
		// Очищаем старые привязки, чтобы не плодить дубликаты при ServerTravel
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URTSAttributeSet::GetVirtualTroopsReserveAttribute()).RemoveAll(this);

		// Вешаем C++ колбэк на изменение пула солдат
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(URTSAttributeSet::GetVirtualTroopsReserveAttribute())
			.AddUObject(this, &AMainGamePlayerState::OnVirtualTroopsAttributeChanged);

		// Мягкий пинок для GUI: сразу бродкастим стартовое значение, чтобы виджет не был пустым при загрузке карты
		float CurrentVal = RTSAttributeSet->GetVirtualTroopsReserve();
		if (OnTroopsInBufferCountChanged.IsBound())
		{
			OnTroopsInBufferCountChanged.Broadcast(FMath::TruncToInt(CurrentVal));
		}
	}
}

void AMainGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGamePlayerState, TeamColor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(AMainGamePlayerState, CurrentSpawnMode);
}

FLinearColor AMainGamePlayerState::GetTeamColor() const
{
	return TeamColor;
}

void AMainGamePlayerState::SetTeamColor(FLinearColor NewColor)
{
	// Метод вызовется сервером в процессе Seamless Travel
	TeamColor = NewColor;
}

void AMainGamePlayerState::OnRep_TeamColor()
{
	
}

UAbilitySystemComponent* AMainGamePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMainGamePlayerState::Server_SetPortalSpawnMode_Implementation(EPortalSpawnMode NewMode)
{
	if (!HasAuthority()) return;
	
	CurrentSpawnMode = NewMode;
}

float AMainGamePlayerState::GetTotalEnergyProduction() const
{
	return RTSAttributeSet ? RTSAttributeSet->GetMaxEnergy() : 0.0f;
}

float AMainGamePlayerState::GetTotalEnergyConsumption() const
{
	return RTSAttributeSet ? RTSAttributeSet->GetCurrentEnergy() : 0.0f;
}

bool AMainGamePlayerState::IsPowerGridOverloaded() const
{
	if (!RTSAttributeSet) return false;
	// Сеть перегружена, если Текущее потребление превысило Максимальную выработку
	return RTSAttributeSet->GetCurrentEnergy() > RTSAttributeSet->GetMaxEnergy();
}

void AMainGamePlayerState::OnVirtualTroopsAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Data.NewValue — это новое float-значение, которое только что записалось в атрибут
	int32 TotalTroopsInt = FMath::TruncToInt(Data.NewValue);

	// Активируем ваш делегат! Сигнал мгновенно улетает в Блюпринт вашего GUI
	if (OnTroopsInBufferCountChanged.IsBound())
	{
		OnTroopsInBufferCountChanged.Broadcast(TotalTroopsInt);
	}
}





// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGamePlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Net/UnrealNetwork.h"

AMainGamePlayerState::AMainGamePlayerState()
{
	// 1. Создаем единственный компонент GAS в памяти игрока
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	// Настраиваем сетевой режим репликации под RTS (Минимальный трафик)
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void AMainGamePlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	// Инициализируем GAS на сервере
	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
		// 💥 RTS СЕТЕВОЙ ФИКС: 
		// Если это ЛИСТЕН-СЕРВЕР (Хост, Игрок 0), его данные валидны СРАЗУ. Выдаем пакет способностей!
		/*if (GetWorld() && (GetWorld()->IsNetMode(NM_ListenServer) || GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerState == this))
		{
			Override_GiveFactionDefaultAbilities();
		}*/
	}
}


void AMainGamePlayerState::Override_GiveFactionDefaultAbilities()
{
	// Метод вызывается строго на сервере в момент полной готовности данных фракции
	if (!HasAuthority() || !AbilitySystemComponent) return;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : FactionDefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
			AbilitySystemComponent->GiveAbility(AbilitySpec);
		}
	}
}

void AMainGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGamePlayerState, TeamColor, COND_None, REPNOTIFY_Always);
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

void AMainGamePlayerState::SetGASAvatarForSoldier(AActor* SoldierAvatar)
{
	if (AbilitySystemComponent && IsValid(SoldierAvatar))
	{
		// Передаем себя (PlayerState) как постоянного владельца экономики и тегов,
		// а в качестве Аватара (физического воплощения) подставляем нашего солдата!
		AbilitySystemComponent->InitAbilityActorInfo(this, SoldierAvatar);
	}
}




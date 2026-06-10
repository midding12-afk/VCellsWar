// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VCellsWar/GameMods/MainGamePlayerState.h"
#include "StrategyEntityInterface.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UStrategyEntityInterface : public UInterface
{
	GENERATED_BODY()
};

class VCELLSWAR_API IStrategyEntityInterface
{
	GENERATED_BODY()

public:
	
	// virtual FLinearColor GetTeamColor() const = 0;
	// virtual void SetTeamColor(FLinearColor NewColor) = 0;
	
	// UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Interface")
	// void SetTeamColor(FLinearColor NewColor);
	// virtual void SetTeamColor_Implementation(FLinearColor NewColor) {}


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Interface")
	FLinearColor GetTeamColor() const;
	virtual FLinearColor GetTeamColor_Implementation() const { return FLinearColor::White; }
	
	
	// 1. Сама функция для Блупринтов и C++ вызовов
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Interface")
	bool IsOwnedByLocalPlayer();//не const из-за бага вызова из PB

	// 2. БАЗОВАЯ РЕАЛИЗАЦИЯ: Пишем её прямо здесь через суффикс _Implementation.
	// Делаем её виртуальной, чтобы C++ наследники могли её переопределить при желании.
	virtual bool IsOwnedByLocalPlayer_Implementation()//не const из-за бага вызова из PB
	{
		UObject* AsUObject = Cast<UObject>(this);
		if (!AsUObject) return false;
		
		AMainGamePlayerState* OwnerState = Execute_GetEntityOwnerState(AsUObject);
		if (!OwnerState) return false;
		
		// 3. Безопасно получаем мир
		UWorld* World = OwnerState->GetWorld();
		if (!World) return false;

		// Если этот код сейчас выполняется на СЕРВЕРЕ (для выделения ИИ или логики урона),
		// сервер проверяет владение через NetConnection контроллера.
		if (World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer))
		{
			// На сервере мы берем локальный PlayerController хоста (индекс 0)
			APlayerController* ServerLocalPC = World->GetFirstPlayerController();
			if (ServerLocalPC && ServerLocalPC->PlayerState)
			{
				return OwnerState == Cast<AMainGamePlayerState>(ServerLocalPC->PlayerState);
			}
		}

		// Если код выполняется на КЛИЕНТЕ (отрисовка HUD, клик мышки игрока):
		// Вместо капризного GetFirstPlayerController мы берем ЛОКАЛЬНОГО игрока напрямую из движка.
		// Это самый надежный способ в UE5, который никогда не возвращает nullptr на клиентах!
		APlayerController* LocalPC = World->GetFirstPlayerController();
		if (LocalPC && LocalPC->PlayerState)
		{
			// Шаг 2. Делаем строгое сравнение. 
			// Cast извлекает чистый указатель из контейнера TObjectPtr движка
			return OwnerState == Cast<AMainGamePlayerState>(LocalPC->PlayerState);
		}

		// Резервный профессиональный способ через GameInstance (если контроллер еще инициализируется)
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (ULocalPlayer* FirstLocalPlayer = GI->GetFirstGamePlayer())
			{
				APlayerController* TargetPC = FirstLocalPlayer->GetPlayerController(World);
				if (TargetPC && TargetPC->PlayerState)
				{
					return OwnerState == Cast<AMainGamePlayerState>(TargetPC->PlayerState);
				}
			}
		}
		
		return false;
	}

	// Вспомогательный чистый метод, чтобы интерфейс мог заглянуть в память актора
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Interface")
	AMainGamePlayerState* GetEntityOwnerState();//не const из-за бага вызова из PB
		
	//virtual void SetEntityOwner(AMainGamePlayerState* NewOwnerState) = 0;
	void SetEntityOwner(AMainGamePlayerState* NewOwnerState)
	{
		SetEntityOwner_Internal(NewOwnerState);

		// Триггерим BlueprintNativeEvent событие!
		// Так как мы находимся внутри интерфейса и у нас нет сырого AActor*, 
		// мы принудительно кастим текущий указатель к UObject для рефлексии движка.
		if (UObject* AsUObject = Cast<UObject>(this))
		{
			Execute_OnOwnerChanged(AsUObject, NewOwnerState);
		}
	}
	
protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Interface")
	void OnOwnerChanged(AMainGamePlayerState* NewOwner);

	// должна быть виртуальной, чтобы её переопределяли C++ наследники!
	virtual void OnOwnerChanged_Implementation(AMainGamePlayerState* NewOwner) {}
	
	virtual AMainGamePlayerState* GetEntityOwnerState_Implementation() = 0;//Не const из-за бага вызова из PB
	
	
	// СКРЫТЫЙ КОНТРАКТ: Чистая виртуальная функция для наследников.
	// Каждый класс (Актор здания или Pawn юнита) реализует её за 1 секунду, просто записывая переменную.
	virtual void SetEntityOwner_Internal(AMainGamePlayerState* NewOwnerState) = 0;


};

// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LobbyPlayerDataObj.generated.h"

// Опережающее объявление (Forward Declaration), чтобы не захламлять инклюды
class ALobbyPlayerState;

UCLASS(BlueprintType, Blueprintable)
class VCELLSWAR_API ULobbyPlayerDataObj : public UObject
{
	GENERATED_BODY()
	
public:
	// Конструктор по умолчанию
	ULobbyPlayerDataObj();

	// Ссылка на PlayerState конкретного игрока.
	// Обязательно ставим BlueprintReadOnly, чтобы виджет строки мог прочитать её.
	// А также мета-теги ExposeOnSpawn и InstanceEditable, чтобы пин появился внутри ноды Construct Object.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby Data", meta = (ExposeOnSpawn = "true", InstanceEditable = "true"))
	ALobbyPlayerState* LobbyPlayerStateRef;
};
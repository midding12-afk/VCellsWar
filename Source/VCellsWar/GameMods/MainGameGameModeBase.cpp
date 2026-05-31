// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameModeBase.h"

#include "MainGameGameState.h"
#include "VCellsWar/Systems/MatchStatisticsSubsystem.h"

void AMainGameGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Сервер берет сид из подсистемы и записывает в GameState
	UMatchStatisticsSubsystem* Stats = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (Stats && GS)
	{
		// 2. Сервер записывает сид в GameState (это запускает репликацию для клиентов)
		GS->MapSeed = Stats->MapSeed; 

		// 3. КРИТИЧЕСКИ ВАЖНО: Вручную вызываем OnRep для Сервера/Хоста!
		// Это заставит делегат "выстрелить" локально на сервере, 
		// и серверный генератор Dynamic Mesh тоже построит горы.
		GS->OnRep_MapSeed();
	}
}

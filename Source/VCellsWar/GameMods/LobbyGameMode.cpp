// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LobbyGameMode.h"

#include "LobbyGameState.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyPlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LobbyPlayerController.h"
#include "VCellsWar/Systems/MatchStatisticsSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerStateClass = ALobbyPlayerState::StaticClass();

	bUseSeamlessTravel = true;
}

void ALobbyGameMode::TryAssignColorToPlayer(AController* PlayerController, FLinearColor RequestedColor)
{
	if (!PlayerController || !GameState) return;
	UKismetSystemLibrary::PrintString(GetWorld(), TEXT("TryAssignColorToPlayer checking"));
	// 1. ПРОВЕРКА: Проверяем, не занят ли этот цвет кем-то другим
	// Перебираем всех игроков, которые сейчас находятся в лобби
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ALobbyPlayerState* StrategyPS = Cast<ALobbyPlayerState>(PS);
		if (StrategyPS)
		{
			// Если у кого-то уже есть ТОЧНО ТАКОЙ ЖЕ цвет (с небольшой погрешностью для float)
			if (StrategyPS->GetTeamColor().Equals(RequestedColor, 0.01f))
			{
				UE_LOG(LogTemp, Warning, TEXT("Сервер: Цвет уже занят игроком %s! Отклонено."), *StrategyPS->GetPlayerName());
                
				// Опционально: можно отправить обратно этому клиенту Client-RPC с ошибкой "Цвет занят"
				return; 
			}
		}
	}
	
	UKismetSystemLibrary::PrintString(GetWorld(), TEXT("TryAssignColorToPlayer approved"));
	// 2. ПРИСВОЕНИЕ: Если цикл завершился и цвет никто не занял, отдаем его игроку
	ALobbyPlayerState* TargetPS = PlayerController->GetPlayerState<ALobbyPlayerState>();
	if (TargetPS)
	{
		TargetPS->SetTeamColor(RequestedColor); // Записываем цвет в реплицируемую переменную
        
		UE_LOG(LogTemp, Log, TEXT("Сервер: Игрок %s успешно получил выбранный цвет."), *TargetPS->GetPlayerName());
        
		// Так как переменная TeamColor в PlayerState помечена как Replicated, 
		// она автоматически улетит ко всем клиентам, и их UI обновится.
	}
	
	CallUpdatePlayerListOnAllPlayers();
}

bool ALobbyGameMode::CheckAllPlayersReady()
{
	if (!GameState || GameState->PlayerArray.Num() == 0) return false;

	// Перебираем массив игроков в поисках тех, кто еще не готов
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
		if (LobbyPS)
		{
			// Встроенная переменная bIsReady из APlayerState (или ваша кастомная)
			if (!LobbyPS->GetIsReady())
			{
				return false; // Нашелся не готовый игрок
			}
		}
	}

	return true; // Все игроки без исключения готовы к старту
}

void ALobbyGameMode::StartMatch(const FString& MapPath)
{
	
	// Запускать переход имеет право только сервер
	if (!HasAuthority()) return;

	// Финальная серверная проверка готовности перед путешествием карт
	if (CheckAllPlayersReady())
	{
		// 3. ПЕРЕД СТАРТОМ: Сохраняем настройки генерации из GameState в подсистему GameInstance.
		// Так как этот код выполняется на Сервере (Хосте), подсистема хоста запишет параметры.
		ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GetWorld()->GetGameState());
		UMatchStatisticsSubsystem* StatsSubsystem = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
    
		if (LobbyGS && StatsSubsystem)
		{
			// Забираем число нод, выбранное в ComboBox, и консервируем в подсистему перед сносом карты лобби
			StatsSubsystem->NodesPerPlayer = LobbyGS->GetNodeCount();
			StatsSubsystem->MapSeed = LobbyGS->GetMapSeed();
			
			StatsSubsystem->NodesPositions = LobbyGS->GetNodePositions();
			StatsSubsystem->MapSize = LobbyGS->GetMapSize();
			
			StatsSubsystem->AllPlayerCount = GameState->PlayerArray.Num() < 2 ? 2 : GameState->PlayerArray.Num();//to do + AI?
			
		}		
		
		
		//UE_LOG(LogTemp, Log, TEXT("LobbyGM: Все готовы! Запуск бесшовного перехода на игровую карту..."));
		
		// Напоминание: путь должен быть в формате "/Game/Maps/MyStrategyMap"
		// Параметр "?listen" обязателен, чтобы сервер открыл сетевой порт для удержания клиентов
		FString TravelURL = FString::Printf(TEXT("%s?listen?NetDriverName=SteamSocketsNetDriver"), *MapPath);
		//FString TravelURL = FString::Printf(TEXT("/Game/VCellsWar/Maps/L_MainGame?listen?NetDriverName=SteamSocketsNetDriver"), *MapPath);
			
		GetWorld()->ServerTravel(TravelURL);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyGM: Попытка старта отклонена — не все игроки готовы."));
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* BasePC = It->Get();
			
			ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(BasePC);
			
			if (LobbyPC)
			{
				LobbyPC->Client_ShowNotificationMessageUI(FText::FromString("Waiting for all players to be ready"));
			}
		}
	}
}

void ALobbyGameMode::CallUpdatePlayerListOnAllPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* BasePC = It->Get();
			
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(BasePC);
			
		if (LobbyPC)
		{
			// Мы можем вызвать кастомную Client-RPC функцию в вашем PlayerController, 
			// которая заставит локальный UI игрока перерисовать список участников лобби.
			LobbyPC->Client_RefreshLobbyUI_PlayerList();
		}
	}
}

void ALobbyGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (!NewPlayer) return;

	// 2. Получаем кастомный PlayerState только что вошедшего игрока
	ALobbyPlayerState* LobbyPS = NewPlayer->GetPlayerState<ALobbyPlayerState>();
	if (LobbyPS)
	{
		// 3. Логируем успешное подключение (в собранной игре вы увидите реальный никнейм из Steam)
		UE_LOG(LogTemp, Log, TEXT("LobbyGM: Игрок '%s' успешно вошел в лобби!"), *LobbyPS->GetPlayerName());

		// 4. Оповещаем UI всех клиентов об обновлении списка игроков.
		// Так как GameMode живет только на сервере, мы не можем напрямую вызвать функцию виджета.
		// Вместо этого мы перебираем всех уже подключенных игроков через их PlayerController'ы
		CallUpdatePlayerListOnAllPlayers();
        
		/* 
		   ПРИМЕЧАНИЕ ДЛЯ ПЕРЕХОДА ИЗ МАТЧА В МАТЧ:
		   Если вы используете Seamless Travel для возврата из сыгранного матча обратно в лобби,
		   вместо OnPostLogin для старых игроков сработает метод HandleSeamlessTravelPlayer().
		   Если вам нужно будет инициализировать их заново, эту логику дублируют и туда.
		*/
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Если игрок вышел из лобби, его занятый цвет освобождается автоматически.
	// Здесь можно вызвать кастомное обновление UI у оставшихся игроков через GameState, 
	// чтобы заблокированные кнопки цветов снова стали активными.
	UE_LOG(LogTemp, Log, TEXT("LobbyGM: Игрок покинул лобби. Перепроверка параметров..."));
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* BasePC = It->Get();
			
		ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(BasePC);
			
		if (LobbyPC)
		{
			LobbyPC->Client_RefreshLobbyUI_PlayerList();
		}
	}
}
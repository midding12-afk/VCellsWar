// Copyright (c) 2026, Dmitry Tur. All rights reserved..


#include "MultiplayerSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
	:OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnSessionCreated)),
	OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnSessionFound)),
	OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMultiplayerSessionSubsystem::OnSessionJoin))
{
	
}

void UMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	
	if (onlineSubsystem && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("Subsystem name = %s"), *onlineSubsystem->GetSubsystemName().ToString()));

		OnlineSessionPtr = onlineSubsystem->GetSessionInterface();
	}
	
	
}

bool UMultiplayerSessionSubsystem::CheckSubsystem()
{
	return IOnlineSubsystem::Get() && IOnlineSubsystem::Get()->GetSubsystemName().ToString()=="Steam";
}


void UMultiplayerSessionSubsystem::CreateSession(int MaxPlayerCount)
{
	if (!OnlineSessionPtr.IsValid()) return;


	// 1. Проверяем, существует ли уже сессия, и удаляем её
	auto ExistingSession = OnlineSessionPtr->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		OnlineSessionPtr->DestroySession(NAME_GameSession);
		// В реальном проекте лучше дождаться делегата OnDestroySessionComplete
		// прежде чем создавать новую, но для теста пойдет и так.
	}

	OnlineSessionPtr->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	// 2. Используем правильный класс настроек
	FOnlineSessionSettings SessionSettings;

	SessionSettings.bIsLANMatch = false;               // false для Steam
	SessionSettings.NumPublicConnections = MaxPlayerCount;          // Сколько игроков влезет
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;      // Важно для Steam!
	SessionSettings.bShouldAdvertise = true;           // Чтобы другие видели сервер - что можно найти
	SessionSettings.bUsesPresence = true;              // Обязательно для работы поиска через Steam - поиск по региону только
	SessionSettings.bUseLobbiesIfAvailable = true;     // Для UE5 и Steam рекомендуется

	//SessionSettings.bUseLobbiesVoiceChatIfAvailable = true; // Иногда помогает "протолкнуть" лобби в сеть

	SessionSettings.Set(FName(TEXT("MyGameTestName")), FString(TEXT("VCellsWarMultiplayerTest")), EOnlineDataAdvertisementType::ViaOnlineService);


	// 4. Получаем UniqueNetId
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (LocalPlayer)
	{
		// В современных версиях GetPreferredUniqueNetId возвращает обертку, нужен GetUniqueNetId()
		OnlineSessionPtr->CreateSession(*LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, SessionSettings);

		
	}
}

void UMultiplayerSessionSubsystem::FindSession()
{
	if (!OnlineSessionPtr.IsValid())
	{
		return;
	}


	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (LocalPlayer)
	{
		OnlineSessionSearch = MakeShared<FOnlineSessionSearch>();
		OnlineSessionPtr->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

		OnlineSessionSearch->bIsLanQuery = false;
		OnlineSessionSearch->MaxSearchResults = 100;
			//onlineSessionSearch.QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
			// Правильный современный синтаксис
			//onlineSessionSearch.QuerySettings.Set(FOnlineSearchSettings::SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		OnlineSessionSearch->QuerySettings.Set(FName(TEXT("PRESENCE")), true, EOnlineComparisonOp::Equals);

		OnlineSessionSearch->QuerySettings.Set(FName(TEXT("MyGameTestName")), FString(TEXT("VCellsWarMultiplayerTest")), EOnlineComparisonOp::Equals);


		// Обязательно добавить этот флаг для Steam!
		OnlineSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

		OnlineSessionPtr->FindSessions(*LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), OnlineSessionSearch.ToSharedRef());

		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("START SEARCH")));
	}
	
}


void UMultiplayerSessionSubsystem::OnSessionCreated(FName InSessionName, bool InWasCreated)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("session name = %s, is created = %d"), *InSessionName.ToString(), InWasCreated));

	//GetWorld()->ServerTravel(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson?listen"), true);
	GetWorld()->ServerTravel(TEXT("/Game/VCellsWar/Maps/L_LobbyScreen?listen?NetDriverName=SteamSocketsNetDriver"), true);
}										

void UMultiplayerSessionSubsystem::OnSessionFound(bool bWasSuccessful)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("FINISH SEARCH")));

	if (!bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("CANT found sessions")));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("found count = %d"), OnlineSessionSearch->SearchResults.Num()));
	
	// Создаем массив, который отдадим в UI
	TArray<UServerDataObj*> ServerObjectsList;

	for (const FOnlineSessionSearchResult& result : OnlineSessionSearch->SearchResults)
	{
		FString sessionId = result.GetSessionIdStr();
		FString userName = result.Session.OwningUserName;

		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("user name = %s, id = %s"), *userName, *sessionId));

		FString nameGame;
		result.Session.SessionSettings.Get(FName(TEXT("MyGameTestName")), nameGame);

		if (OnlineSessionPtr.IsValid() && nameGame == TEXT("VCellsWarMultiplayerTest"))
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, TEXT("Обходим баг JoinSession: Прямое подключение..."));

			// 1. Создаем новый экземпляр вашего класса данных
			// В качестве Outer передаем саму подсистему (this)
			UServerDataObj* ServerData = NewObject<UServerDataObj>(this);
			
			if (ServerData)
			{
				// 2. Заполняем структуру (чтобы кнопка "Подключиться" в строке работала)
				ServerData->SessionResult.OnlineResult = result;
				
				// 3. Извлекаем данные для UI напрямую из структуры Steam
				ServerData->Ping = result.PingInMs;
				ServerData->MaxPlayers = result.Session.SessionSettings.NumPublicConnections;
				
				// Вычисляем текущих игроков: Максимум минус количество свободных слотов
				ServerData->CurrentPlayers = ServerData->MaxPlayers - result.Session.NumOpenPublicConnections;
				
				ServerData->ServerName = FString::Printf(TEXT("%d: %s"), ServerObjectsList.Num()+1, *result.Session.OwningUserName);
				
				ServerData->HostSteamID = result.Session.OwningUserId->ToString();

				// 4. Добавляем готовый объект в массив
				ServerObjectsList.Add(ServerData);
			}
			
						
			// // 1. Получаем уникальный сетевой ID владельца сессии (хоста)
			// FUniqueNetIdPtr HostNetId = result.Session.OwningUserId;
			//
			// if (HostNetId.IsValid() && HostNetId->IsValid())
			// {
			// 	APlayerController* playerController = GetWorld()->GetFirstPlayerController();
			// 	if (playerController)
			// 	{
			// 		// 2. Формируем специальную строку подключения для Steam P2P: "steam.SteamID_Хоста"
			// 		// ToString() вернет числовой SteamID владельца сервера
			// 		FString HostSteamID = HostNetId->ToString();
			// 		FString ConnectionString = FString::Printf(TEXT("steam.%s"), *HostSteamID);
			//
			// 		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Traveling to Host ID: %s"), *ConnectionString));
			//
			// 		// 3. Отправляем клиента напрямую на хост, минуя стадию JoinSession
			// 		playerController->ClientTravel(ConnectionString, ETravelType::TRAVEL_Absolute);
			// 		return;
			// 	}
			// }
		}
	}
	
	// "Стреляем" делегатом, передавая Блупринту массив УЖЕ ГОТОВЫХ объектов
	OnFindSessionsCompleteBP.Broadcast(ServerObjectsList);
}


void UMultiplayerSessionSubsystem::JoinSession(FString HostSteamID)
{
	FString ConnectionString = FString::Printf(TEXT("steam.%s"), *HostSteamID);

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Traveling to Host ID: %s"), *ConnectionString));

	// 3. Отправляем клиента напрямую на хост, минуя стадию JoinSession
	GetWorld()->GetFirstPlayerController()->ClientTravel(ConnectionString, ETravelType::TRAVEL_Absolute);
}



//void UMultiplayerSessionSubsystem::OnSessionFound(bool bWasSuccessful)
//{
//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("FINISH SEARCH")));
//
//	if (!bWasSuccessful)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("CANT found sessions")));
//		return;
//	} 
//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("found count = %d"), OnlineSessionSearch->SearchResults.Num()));
//	for (const FOnlineSessionSearchResult& result : OnlineSessionSearch->SearchResults)
//	{
//		FString sessionId = result.GetSessionIdStr();
//		FString userName = result.Session.OwningUserName;
//
//		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("user name = %s, id = %s"), *userName, *sessionId));
//
//		FString nameGame;
//		result.Session.SessionSettings.Get(FName(TEXT("MyGameTestName")), nameGame);				//  добавлено в фильтре поиска
//
//		if (OnlineSessionPtr.IsValid() && nameGame== TEXT("VCellsWarMultiplayerTest"))
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("start connection")));
//			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
//
//			OnlineSessionPtr->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
//
//			OnlineSessionPtr->JoinSession(*LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, result);
//			return;
//		}
//	}
//}

void UMultiplayerSessionSubsystem::OnSessionJoin(const FName SessionName, EOnJoinSessionCompleteResult::Type ResultType)
{
	FString ResultString = LexToString(ResultType);
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("join session = %s, result = %s"), *SessionName.ToString(), *ResultString));
	if (ResultType == EOnJoinSessionCompleteResult::Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("JOIN 1")));
		//APlayerController* playerController = Cast<APlayerController>(GetController());

		// Используем более надежный способ получения контроллера для локального клиента
		APlayerController* playerController = GetWorld()->GetFirstPlayerController();


		if (playerController != nullptr && OnlineSessionPtr.IsValid())
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString(TEXT("JOIN 2")));
			//FString connectionInfo;
			//OnlineSessionPtr->GetResolvedConnectString(NAME_GameSession, connectionInfo);
			//playerController->ClientTravel(connectionInfo, ETravelType::TRAVEL_Absolute);

			

			FString connectionInfo;
			// Важно: GetResolvedConnectString возвращает bool, проверяем успех
			if (OnlineSessionPtr->GetResolvedConnectString(SessionName, connectionInfo))
			{
				FString IPAddress;
				connectionInfo.Split(TEXT("/"), &IPAddress, nullptr);
				FString FinalURL = IPAddress + TEXT("/Game/ThirdPerson/L_MMap");

				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, FString::Printf(TEXT("Traveling to: %s"), *connectionInfo));
				//playerController->ClientTravel(connectionInfo, ETravelType::TRAVEL_Absolute);

				playerController->ClientTravel(FinalURL, ETravelType::TRAVEL_Absolute);
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Failed to get Connect String!"));
			}
		}
	}
}

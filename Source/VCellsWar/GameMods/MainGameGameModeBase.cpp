// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameModeBase.h"

#include "MainGameGameState.h"
#include "MainGamePlayerController.h"
#include "MainGamePlayerState.h"
#include "VCellsWar/Systems/MatchStatisticsSubsystem.h"
#include "VCellsWar/Systems/NodesSubsystem.h"

void AMainGameGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, TEXT("GM: MainGame Started "));
	
	// Сервер берет сид из подсистемы и записывает в GameState
	UMatchStatisticsSubsystem* Stats = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (Stats && GS)
	{
		// 2. Сервер записывает сид в GameState (это запускает репликацию для клиентов)
		GS->MapSeed = Stats->MapSeed; 
		GS->MapSize = Stats->MapSize;
		
		GS->AllPlayerCount = Stats->AllPlayerCount;
		
		//GS->NodesPositions = Stats->NodesPositions;
		GS->AllNodesCountOnInit = Stats->NodesPerPlayer * Stats->AllPlayerCount + 1;
		

		// 3. КРИТИЧЕСКИ ВАЖНО: Вручную вызываем OnRep для Сервера/Хоста!
		// Это заставит делегат "выстрелить" локально на сервере, 
		// и серверный генератор Dynamic Mesh тоже построит горы.
		GS->OnRep_MapSeed();
		GS->OnRep_AllNodesCountOnInit();
	}
}


void AMainGameGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (!NewPlayer) return;

	AMainGamePlayerState* PS = NewPlayer->GetPlayerState<AMainGamePlayerState>();
	if (PS)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, (TEXT("GM: Игрок '%s' успешно вошел в лобби!"), *PS->GetPlayerName()));
	}
}

void AMainGameGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Если игрок вышел из лобби, его занятый цвет освобождается автоматически.
	// Здесь можно вызвать кастомное обновление UI у оставшихся игроков через GameState, 
	// чтобы заблокированные кнопки цветов снова стали активными.
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, TEXT("GM: Игрок покинул лобби. "));
	
}

void AMainGameGameModeBase::SpawnNodesFromSubsystem()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("GM: Call spawn towers "));
    // 1. Получаем доступ к подсистеме статистики
    UMatchStatisticsSubsystem* StatsSubsystem = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
    if (!StatsSubsystem || !StrategyNodeClass) return;
	
	if (StatsSubsystem->NodesPositions.Num() == 0)
	{
		UMatchStatisticsSubsystem* Stats = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
		UNodesSubsystem* NSB = GetGameInstance()->GetSubsystem<UNodesSubsystem>();
		
		if (!NSB || !Stats) return;
		
		StatsSubsystem->NodesPositions = NSB->GenNodesList(Stats->MapSeed, Stats->MapSize, Stats->NodesPerPlayer, Stats->AllPlayerCount);
	}

    // 2. Запускаем цикл по всем сохраненным позициям нод
    for (const FVector2D& NodePos2D : StatsSubsystem->NodesPositions)
    {
        // Настраиваем точки для лучевого выстрела (Line Trace)
        // Стреляем из высокой точки неба (Z = 10000) вертикально вниз (Z = -5000)
        FVector StartTrace(NodePos2D.X, NodePos2D.Y, 10000.0f);
        FVector EndTrace(NodePos2D.X, NodePos2D.Y, -5000.0f);

        FHitResult HitResult;
        FCollisionQueryParams TraceParams;
        // Игнорируем сам GameMode, если у него вдруг есть коллизия
        TraceParams.AddIgnoredActor(this); 

        // Выполняем выстрел лучом по каналу видимости (ECC_Visibility)
        // Важно: ваш Dynamic Mesh ландшафта должен иметь включенную коллизию в этом канале!
        bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams);

        FVector FinalSpawnLocation;

        if (bHit)
        {
            // Если луч нашел землю — берем точные координаты точки соприкосновения
            FinalSpawnLocation = HitResult.Location;
        }
        else
        {
            // Подстраховка на случай, если луч промазал мимо карты (ставим на нулевую высоту)
            FinalSpawnLocation = FVector(NodePos2D.X, NodePos2D.Y, 0.0f);
            UE_LOG(LogTemp, Warning, TEXT("GameMode: Луч промазал мимо ландшафта в координатах X:%f, Y:%f!"), NodePos2D.X, NodePos2D.Y);
        }

        // 3. Спавним саму ноду на сервере
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        // Нода спавнится с дефолтным вращением
        AActor* SpawnedNode = GetWorld()->SpawnActor<AActor>(StrategyNodeClass, FinalSpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (SpawnedNode)
        {
            UE_LOG(LogTemp, Log, TEXT("GameMode: Успешно заспавнена сетевая нода в точке X:%f, Y:%f, Z:%f"), 
                FinalSpawnLocation.X, FinalSpawnLocation.Y, FinalSpawnLocation.Z);
        }
    }
	
	
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	if (StatsSubsystem)
	{
		CenterX = StatsSubsystem->MapSize / 2.0f;
		CenterY = StatsSubsystem->MapSize / 2.0f;
	}
	FVector CenterLocation(CenterX, CenterY, 1500.0f);

	// Рассылаем Client RPC всем контроллерам на сервере
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(It->Get());
		if (PC)
		{
			// Сервер командует контроллеру: "Сдвинь свою локальную камеру в центр!"
			PC->TeleportLocalCameraToCenter(CenterLocation);
		}
	}
}
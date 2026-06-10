// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameModeBase.h"

#include "MainGameGameState.h"
#include "MainGamePlayerController.h"
#include "MainGamePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
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
	
	// Запускаем бесконечный серверный игровой цикл 
	GetWorldTimerManager().SetTimer(StrategyLogicTimerHandle, this, &AMainGameGameModeBase::ProcessStrategyLogicTick, 1.0f, true);
	
	// APlayerController* HostPC = GetWorld()->GetFirstPlayerController();
	// if (HostPC)
	// {
	// 	SpawnNewPortal(HostPC);
	// }
}


void AMainGameGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
	
	if (NewPlayer)
	{
		SpawnNewPortal(NewPlayer);
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
		
		//SpawnNewPortal(NewPlayer);
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


FVector AMainGameGameModeBase::GetPointOnMapInLocation(FVector2D Location2D) const
{	
	// Настраиваем точки для лучевого выстрела (Line Trace)
	// Стреляем из высокой точки неба (Z = 10000) вертикально вниз (Z = -5000)
	FVector StartTrace(Location2D.X, Location2D.Y, 10000.0f);
	FVector EndTrace(Location2D.X, Location2D.Y, -5000.0f);
	
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	// Игнорируем сам GameMode, если у него вдруг есть коллизия
	TraceParams.AddIgnoredActor(this); 

	// Выполняем выстрел лучом по каналу видимости (ECC_Visibility)
	// Важно: ваш Dynamic Mesh ландшафта должен иметь включенную коллизию в этом канале!
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, TraceParams);
	
	if (bHit)
	{
		return HitResult.Location;
	}
	else
	{
		return FVector(Location2D.X, Location2D.Y, 0.0f);
	}
}


AActor* AMainGameGameModeBase::SpawnActorInLocation(const TSubclassOf<AActor> ActorToSpawn, const FVector2D Location) const
{
	if (!ActorToSpawn) return nullptr;
	
	FVector FinalSpawnLocation = GetPointOnMapInLocation(Location);

	// 3. Спавним саму ноду на сервере
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
	// Нода спавнится с дефолтным вращением
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorToSpawn, FinalSpawnLocation, FRotator::ZeroRotator, SpawnParams);
	
	return SpawnedActor;
}

void AMainGameGameModeBase::SpawnNewPortal(AController* NewPlayer)
{
	UMatchStatisticsSubsystem* StatsSubsystem = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	if (!StatsSubsystem || !StrategyPortalClass) return;
	
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	if (StatsSubsystem)
	{
		CenterX = StatsSubsystem->MapSize / 2.0f;
		CenterY = StatsSubsystem->MapSize / 2.0f;
	}
	FVector2D CenterLocation2D(CenterX, CenterY);
	
	//int32 index = 0; //SplayerSpawnedPortalsCounter
	float anglePerPlayer = 360.f/StatsSubsystem->AllPlayerCount;
	
	AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(NewPlayer);
			
	if (PC)
	{
		AMainGamePlayerState* PS = PC->GetPlayerState<AMainGamePlayerState>();
		if (PS)
		{
			//FLinearColor teamColor = PS->GetTeamColor();
				
			FVector2D vector = FVector2D(0.9*StatsSubsystem->MapSize/2.f, 0.f);
				
			vector = vector.GetRotated(SplayerSpawnedPortalsCounter * anglePerPlayer) + CenterLocation2D;
				
				
			FVector FinalSpawnLocation = GetPointOnMapInLocation(vector);
			FTransform SpawnTransform(FRotator::ZeroRotator, FinalSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
				
			APortalBase* DeferredPortal = GetWorld()->SpawnActorDeferred<APortalBase>(StrategyPortalClass, 	SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				
			if (DeferredPortal)
			{
				DeferredPortal->SetEntityOwner(PS);
				UGameplayStatics::FinishSpawningActor(DeferredPortal, SpawnTransform);
			}
		}
	}
	
	SplayerSpawnedPortalsCounter++;
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
    	SpawnActorInLocation(StrategyNodeClass, NodePos2D);
    }
	
}

void AMainGameGameModeBase::SpawnUnitFromPortal(APortalBase* PortalActor)
{
	if (!PortalActor || !CharacterUnitClass) return;

	// Спавним человечка прямо в координатах портала
	FVector SpawnLocation = PortalActor->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f); // Чуть приподнимаем над порталом
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewActor = GetWorld()->SpawnActor<AActor>(CharacterUnitClass, SpawnLocation, PortalActor->GetActorRotation(), SpawnParams);
	
	AStrategyEntityCharacter* NewUnit = Cast<AStrategyEntityCharacter>(NewActor);
	if (NewUnit)
	{
		// 1. Задаем владельца (наш прошлый C++ шаг)
		NewUnit->SetEntityOwner(PortalActor->GetEntityOwnerState());

		// 2. Толкаем человечка: передаем направление, куда смотрит портал
		NewUnit->LaunchFromPortal(PortalActor->GetActorForwardVector());
	}
}


void AMainGameGameModeBase::RegisterPortal(AStrategyEntityBase* NewPortal)
{
	if (NewPortal && !ActivePortals.Contains(NewPortal))
	{
		ActivePortals.Add(NewPortal);
	}
}

void AMainGameGameModeBase::UnregisterPortal(AStrategyEntityBase* OldPortal)
{
	if (OldPortal && ActivePortals.Contains(OldPortal))
	{
		ActivePortals.Remove(OldPortal);
	}
}

void AMainGameGameModeBase::ProcessStrategyLogicTick()
{
	// ЦЕНТРАЛЬНЫЙ СЕРВЕРНЫЙ ТИК СТРАТЕГИИ
	// Здесь мы одной легкой итерацией обрабатываем логику ВСЕХ порталов сразу!
	for (AStrategyEntityBase* Portal : ActivePortals)
	{
		if (Portal)
		{
			// Пример логики: если у портала есть владелец, начисляем ему ресурсы
			// AMainGamePlayerState* Owner = Portal->GetEntityOwnerState();
			// if (Owner) { Owner->AddGold(10); }
			
			// Или командуем порталу запустить визуальный эффект:
			// Portal->Execute_YourCustomInterfaceMethod(Portal);
		}
	}
}



// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameModeBase.h"

#include "MainGameGameState.h"
#include "MainGamePlayerController.h"
#include "MainGamePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "VCellsWar/Actors/StrategyEntityCharacter.h"
#include "VCellsWar/Systems/MatchStatisticsSubsystem.h"
#include "VCellsWar/Systems/NodesSubsystem.h"
#include "VCellsWar/Systems/VoronoiSubsystem.h"
#include "GenericTeamAgentInterface.h"
#include "NavigationSystem.h"
#include "Components/BrushComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"

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
	
	ServerPool = GetWorld()->GetSubsystem<UServerNetworkPoolSubsystem>();
	
	// НАСТРОЙКА ПРАВИЛ FFA ДЛЯ ВСЕГО МАТЧА В UE5
	// Передаем лямбда-функцию в статический метод движка. 
	// Движок будет вызывать этот кусок кода автоматически каждый раз, когда зрение ИИ сравнивает команды двух любых объектов!
	FGenericTeamId::SetAttitudeSolver([](FGenericTeamId TeamA, FGenericTeamId TeamB) -> ETeamAttitude::Type
	{
		// 1. Если ID команд полностью совпадают — это СВОЙ (Friendly)
		if (TeamA == TeamB)
		{
			return ETeamAttitude::Friendly;
		}

		// 2. Если один из объектов вообще не имеет команды (NoTeam / 255) — это НЕЙТРАЛ
		if (TeamA == FGenericTeamId::NoTeam || TeamB == FGenericTeamId::NoTeam)
		{
			return ETeamAttitude::Neutral;
		}

		// 3. В режиме FFA: если ID команд разные (например, Игрок 0 и Игрок 4) — это 100% ВРАГ!
		// Используем константу ETeamAttitude::Hostile, которую вы нашли на скриншоте кода движка
		return ETeamAttitude::Hostile;
	});
}


void AMainGameGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
	
	if (NewPlayer)
	{
		AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
		AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(NewPlayer);
		if (PC && GS)
		{
			AMainGamePlayerState* PS = PC->GetPlayerState<AMainGamePlayerState>();
			if (PS)
			{
				GS->AddTeamIDColor(PS->GetGenericTeamId(),PS->GetTeamColor());
				PS->Override_GiveFactionDefaultAbilities();
			}
		}
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
			LinkedStructuresCounter++;
				
			FVector2D vector = FVector2D(0.9*StatsSubsystem->MapSize/2.f, 0.f);
				
			vector = vector.GetRotated(PlayerSpawnedPortalsCounter * anglePerPlayer) + CenterLocation2D;
				
				
			FVector FinalSpawnLocation = GetPointOnMapInLocation(vector);
			FTransform SpawnTransform(FRotator::ZeroRotator, FinalSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
				
			APortalBase* DeferredPortal = GetWorld()->SpawnActorDeferred<APortalBase>(StrategyPortalClass, 	SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				
			if (DeferredPortal)
			{
				DeferredPortal->SetEntityOwner(PS);
				DeferredPortal->Server_SetNextSpawnDelay(0.f);
				DeferredPortal->SetGenericTeamId(PS->GetGenericTeamId());
				IStructureNetIDInterface::Execute_Server_SetStructureNetID(DeferredPortal, LinkedStructuresCounter);
				UGameplayStatics::FinishSpawningActor(DeferredPortal, SpawnTransform);
			}
		}
	}
	
	PlayerSpawnedPortalsCounter++;
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
    	LinkedStructuresCounter++;
    	
    	//SpawnActorInLocation(StrategyNodeClass, NodePos2D);
    	
    	FVector FinalSpawnLocation = GetPointOnMapInLocation(NodePos2D);
    	
    	FTransform SpawnTransform(FRotator::ZeroRotator, FinalSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
    	
    	ATowerBase* NewActor = GetWorld()->SpawnActorDeferred<ATowerBase>(StrategyNodeClass, 	SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					
    	if (NewActor)
    	{
    		IStructureNetIDInterface::Execute_Server_SetStructureNetID(NewActor, LinkedStructuresCounter);
    		//NewActor->Server_SetStructureNetID(LinkedStructuresCounter);
			
    		UGameplayStatics::FinishSpawningActor(NewActor, SpawnTransform);
    	}
    }
	
}


void AMainGameGameModeBase::RegisterPortal(APortalBase* NewPortal)
{
	if (NewPortal && !ActivePortals.Contains(NewPortal))
	{
		ActivePortals.Add(NewPortal);
		
		AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
		if (ActiveTower.Num() == GS->AllNodesCountOnInit)
		{
			UpdateVoronoiAndLinks();
		}
	}
}

void AMainGameGameModeBase::UnregisterPortal(APortalBase* OldPortal)
{
	if (OldPortal && ActivePortals.Contains(OldPortal))
	{
		ActivePortals.Remove(OldPortal);
	}
}

void AMainGameGameModeBase::RegisterTower(ATowerBase* NewTower)
{
	if (NewTower && !ActiveTower.Contains(NewTower))
	{
		ActiveTower.Add(NewTower);
	}
	
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (ActiveTower.Num() == GS->AllNodesCountOnInit)
	{
		bAllTowersSpawned = true;
		UpdateVoronoiAndLinks(true);
	}
}

void AMainGameGameModeBase::UpdateVoronoiAndLinks(bool NeedToUpdateCellsMap)
{
	if (!bAllTowersSpawned) return;
	
	TArray<FVector2D> ActiveTowerLocation;
	for (const ATowerBase* Tower : ActiveTower)
	{
		if (IsValid(Tower))
		{
			FVector Location = Tower->GetActorLocation();
			ActiveTowerLocation.Add(FVector2D(Location.X, Location.Y));
		}
	}
		
	UVoronoiSubsystem* VoronoiSB = GetGameInstance()->GetSubsystem<UVoronoiSubsystem>();
	if (!VoronoiSB) return;
				
	TArray<FDeloneGraphEdge> CachedDeloneEdges = VoronoiSB->GetCachedDeloneEdges();
	if (NeedToUpdateCellsMap || CachedDeloneEdges.Num() == 0)
	{
		VoronoiSB->UpdateNodePositions(ActiveTowerLocation);
		VoronoiSB->ReconstructAndDraw();
		CachedDeloneEdges = VoronoiSB->GetCachedDeloneEdges();
	}
	
	TArray<FDeloneGraphEdge> CachedDeloneEdgesTowerID;
	CachedDeloneEdgesTowerID.Empty();	
	
	for (FDeloneGraphEdge& Edge : CachedDeloneEdges)
	{
		int32 TowerIdA = ActiveTower[Edge.Start]->GetStructureNetID_Implementation();
		int32 TowerIdB = ActiveTower[Edge.End]->GetStructureNetID_Implementation();
		
		if (TowerIdA && TowerIdB)
			CachedDeloneEdgesTowerID.Add(FDeloneGraphEdge(TowerIdA, TowerIdB));
	}
	
	//add portals
	for (APortalBase* Portal : ActivePortals)
	{
		if (Portal)
		{
			int32 TowerIdA = Portal->GetStructureNetID_Implementation();
			int32 TowerIdB = -1;
			float MinDis = BIG_NUMBER;
			
			for (ATowerBase* Tower : ActiveTower)
			{
				float CurrentDis = FVector::Dist(Portal->GetActorLocation(), Tower->GetActorLocation());
				if (CurrentDis < MinDis)
				{
					MinDis = CurrentDis;
					TowerIdB = Tower->GetStructureNetID_Implementation();
				}
			}
			
			if (TowerIdA>0 && TowerIdB>0)
				CachedDeloneEdgesTowerID.Add(FDeloneGraphEdge(TowerIdA, TowerIdB));
		}
	}
	
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (GS)
	{
		GS->UpdateCachedDeloneEdgesTowerID(CachedDeloneEdgesTowerID);
	}
}

void AMainGameGameModeBase::UnregisterTower(ATowerBase* OldTower)
{
	if (OldTower && ActiveTower.Contains(OldTower))
	{
		ActiveTower.Remove(OldTower);
	}
}

void AMainGameGameModeBase::ProcessStrategyLogicTick()
{
	// ЦЕНТРАЛЬНЫЙ СЕРВЕРНЫЙ ТИК СТРАТЕГИИ
	// Здесь мы одной легкой итерацией обрабатываем логику ВСЕХ порталов сразу!
	for (APortalBase* Portal : ActivePortals)
	{
		if (Portal)
		{
			// Пример логики: если у портала есть владелец, начисляем ему ресурсы
			// AMainGamePlayerState* Owner = Portal->GetEntityOwnerState();
			// if (Owner) { Owner->AddGold(10); }
			
			// Или командуем порталу запустить визуальный эффект:
			// Portal->Execute_YourCustomInterfaceMethod(Portal);
			if (GetWorld()->GetTimeSeconds() >= Portal->GetNextSpawnTime())
			{
				//UPDATE with GAS
				Portal->Server_SetNextSpawnDelay(30.f);
				
				
				//SpawnUnitFromPortal(Portal);
				
			}
		}
	}
}

void AMainGameGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Сбрасываем глобальный указатель на лямбду, возвращая дефолтные настройки движка
	FGenericTeamId::ResetAttitudeSolver();
	
	Super::EndPlay(EndPlayReason);
}

void AMainGameGameModeBase::ForceRuntimeNavMeshRebuild()
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		// Жесткий приказ навигатору: отсканировать все зарегистрированные коробки Volumes 
		// и немедленно начать фоновое запекание Dynamic NavMesh!
		NavSys->Build();
	}
}

void AMainGameGameModeBase::ResizeNavMeshBoundsVolume(ANavMeshBoundsVolume* VolumeToResize, FVector NewHalfExtents)
{
	// 1. Проверяем валидность переданного волюма
	if (!IsValid(VolumeToResize) || !VolumeToResize->GetBrushComponent()) return;

	// 2. ЖЕСТКО И ПРАВИЛЬНО ИЗМЕНЯЕМ ГЕОМЕТРИЮ БРАША (Аналог Brush Settings из редактора)
	// В отличие от Scale, этот метод не ломает внутренние навигационные тайлы движка!
	UBrushComponent* BrushComp = VolumeToResize->GetBrushComponent();
	if (BrushComp)
	{
		// Переписываем внутренние размеры невидимой коробки
		BrushComp->BrushBodySetup = nullptr; // Сбрасываем старый кэш физики Chaos
		
		// Задаем кубу волюма новые чистые геометрические размеры
		VolumeToResize->Brush->Bounds = FBox(-NewHalfExtents, NewHalfExtents);
		
		// Обновляем параметры трансформа и отрисовки компонента на сервере
		//BrushComp->BuildSimpleBrushGeometry();
		BrushComp->UpdateBounds();
		
		// 4. ОФИЦИАЛЬНЫЙ ТРИГГЕР ДЛЯ UE5:
		// Находим навигационную систему мира и скармливаем ей обновленный волюм.
		// Движок сам поднимет нужные флаги, перенарежет тайлы в памяти и сотрет надпись с экрана!
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			NavSys->OnNavigationBoundsUpdated(VolumeToResize);
			
			// Принудительно пинаем сборку, чтобы навигатор не спал
			//NavSys->Build();
		}
	}
}



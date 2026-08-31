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
#include "Engine/OverlapResult.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "VCellsWar/AI/AIOpponent/AIGeneralDirector.h"

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
		
		GS->SetAllPlayerCount(Stats->AllPlayerCount);
		GS->SetAIPortalsCount(Stats->AIPortalsCount);
		
		
		//GS->NodesPositions = Stats->NodesPositions;
		GS->AllNodesCountOnInit = Stats->NodesPerPlayer * GS->GetSumAllPortalsCount() + 1;
		

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
	
	APlayerController* HostController = GetWorld()->GetFirstPlayerController();
	
	if (HostController && HostController->IsLocalController())
	{		
		// Принудительно вызываем наш метод спавна портала для игрока-хоста
		SpawnNewPortal(HostController);
	}
	
	if (HasAuthority() && Stats->AIPortalsCount > 0)
	{
		// Инициализируем ИИ-врага
		Server_InitializeAiOpponent(216);
	}
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
		
		if (!NewPlayer->IsLocalController())
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

void AMainGameGameModeBase::Server_InitializeAiOpponent(int32 AiFactionID)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// ШАГ 1: Спавним Плеер-Контроллер для ИИ
	// Используем ваш оригинальный контроллер, чтобы бот обладал теми же возможностями группового движения, что и человек
	AMainGamePlayerController* AiController = World->SpawnActor<AMainGamePlayerController>(AMainGamePlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (!AiController) return;

	// ШАГ 2: Спавним Плеер-Стейт для ИИ
	AMainGamePlayerState* AiPlayerState = World->SpawnActor<AMainGamePlayerState>(AMainGamePlayerState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (AiPlayerState)
	{
		// Жестко связываем контроллер ИИ и его стейт
		AiController->PlayerState = AiPlayerState;
		AiPlayerState->SetOwner(AiController);
		
		// Настраиваем фракцию/команду бота (предполагаем, что у вас в PlayerState есть для этого метод)
		AiPlayerState->SetGenericTeamId(AiFactionID);
		
		// На старте закидываем ИИ немного виртуальных солдат в подпространственный буфер на развитие
		// AiPlayerState->VirtualTroopsReserve = 100; 
		
		AiPlayerState->SetTeamColor(FLinearColor::White);
	}

	// ШАГ 3: Спавним Мозг Генерала (AInfo)
	EnemyAiDirector = World->SpawnActor<AAIGeneralDirector>(AAIGeneralDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (EnemyAiDirector)
	{
		// КРИТИЧЕСКИ ВАЖНО ДЛЯ НАШЕЙ СЕТЕВОЙ ЦЕПОЧКИ:
		// Назначаем Плеер-Контроллер бота ВЛАДЕЛЬЦЕМ (Owner) для актора Генерала.
		// Теперь метод GetOwner() внутри генерала и его компонентов гарантированно вернет AiController!
		EnemyAiDirector->SetOwner(AiController);
		AiController->EnemyAiDirector = EnemyAiDirector;
		
		UMatchStatisticsSubsystem* Stats = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
		if (Stats)
		{
			for (int i = 0;i<Stats->AIPortalsCount;i++)
			{
				APortalBase* AIPortal = SpawnNewPortal(AiController);
				//AIPortal->EnemyAiDirector = EnemyAiDirector;
			}
		}
	}
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

APortalBase* AMainGameGameModeBase::SpawnNewPortal(AController* NewPlayer)
{
	UMatchStatisticsSubsystem* StatsSubsystem = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	if (!StatsSubsystem || !StrategyPortalClass) return nullptr;
	
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	if (StatsSubsystem)
	{
		CenterX = StatsSubsystem->MapSize / 2.0f;
		CenterY = StatsSubsystem->MapSize / 2.0f;
	}
	FVector2D CenterLocation2D(CenterX, CenterY);
	
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (!GS) return nullptr;
	
	float anglePerPlayer = 360.f / GS->GetSumAllPortalsCount();
	
	AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(NewPlayer);
			
	if (PC)
	{
		AMainGamePlayerState* PS = PC->GetPlayerState<AMainGamePlayerState>();
		if (PS)
		{
			LinkedStructuresCounter++;
				
			FVector2D vector = FVector2D(0.9f * StatsSubsystem->MapSize / 2.f, 0.f);
			vector = vector.GetRotated(PlayerSpawnedPortalsCounter * anglePerPlayer) + CenterLocation2D;
				
			FVector FinalSpawnLocation = GetPointOnMapInLocation(vector);

			// =========================================================================
			// АЛГОРИТМ СМЕЩЕНИЯ ОТ БЛИЖАЙШИХ БАШЕН (Push-Away Logic)
			// =========================================================================
			// Настраиваем минимальную дистанцию между Порталом и любой Башней (например, 600 единиц)
			const float MinDistanceToTower = 1200.0f; 
			const float MinDistanceToTowerSQ = FMath::Square(MinDistanceToTower);

			// Делаем быстрый поиск башен в этой области. 
			TArray<FOverlapResult> Overlaps;
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(MinDistanceToTower);
			FCollisionQueryParams QueryParams;
			QueryParams.bTraceComplex = false;

			// Сканируем мир по динамическому каналу
			if (GetWorld()->OverlapMultiByChannel(Overlaps, FinalSpawnLocation, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, SphereShape, QueryParams))
			{
				for (const FOverlapResult& Overlap : Overlaps)
				{
					// Если в радиусе оказалась башня
					if (ATowerBase* NearbyTower = Cast<ATowerBase>(Overlap.GetActor()))
					{
						FVector TowerLoc = NearbyTower->GetActorLocation();
						
						// Проверяем расстояние по плоскости XY (игнорируя разницу высот ландшафта Z)
						float DistToTowerSQ = FVector::DistSquaredXY(FinalSpawnLocation, TowerLoc);

						if (DistToTowerSQ < MinDistanceToTowerSQ)
						{
							// Вычисляем вектор направления "ОТ башни К нашему порталу"
							FVector PushDirection = (FinalSpawnLocation - TowerLoc);
							PushDirection.Z = 0.0f; // Оставляем смещение только на плоскости
							PushDirection.Normalize();

							// Если точки совпали идеально (редкий случай), толкаем в сторону центра карты
							if (PushDirection.IsNearlyZero())
							{
								PushDirection = FVector(CenterLocation2D.X, CenterLocation2D.Y, 0.0f) - FinalSpawnLocation;
								PushDirection.Normalize();
							}

							// Смещаем координату спавна портала ровно на то расстояние, которого не хватает до лимита
							float CurrentDist = FMath::Sqrt(DistToTowerSQ);
							float PushDistance = MinDistanceToTower - CurrentDist;

							// Применяем смещение
							FinalSpawnLocation += PushDirection * (PushDistance + 50.0f); // +50 единиц запаса

							// Корректируем высоту Z новой точки под рельеф вашей карты
							FinalSpawnLocation = GetPointOnMapInLocation(FVector2D(FinalSpawnLocation.X, FinalSpawnLocation.Y));
						}
					}
				}
			}
			// =========================================================================

			FTransform SpawnTransform(FRotator::ZeroRotator, FinalSpawnLocation, FVector(1.0f, 1.0f, 1.0f));
				
			APortalBase* DeferredPortal = GetWorld()->SpawnActorDeferred<APortalBase>(
				StrategyPortalClass, 	
				SpawnTransform, 
				nullptr, 
				nullptr, 
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);
				
			if (DeferredPortal)
			{
				DeferredPortal->NativeRTSInitialize(PS->GetGenericTeamId(), PS, SpawnTransform);
				//DeferredPortal->SetEntityOwner(PS);
				DeferredPortal->Server_SetNextSpawnDelay(0.f);
				//DeferredPortal->SetGenericTeamId(PS->GetGenericTeamId());
				IStructureNetIDInterface::Execute_Server_SetStructureNetID(DeferredPortal, LinkedStructuresCounter);
				UGameplayStatics::FinishSpawningActor(DeferredPortal, SpawnTransform);
				PlayerSpawnedPortalsCounter++;
				return DeferredPortal;
			}
		}
	}
	
	return nullptr;
}


/*APortalBase* AMainGameGameModeBase::SpawnNewPortal(AController* NewPlayer)
{
	UMatchStatisticsSubsystem* StatsSubsystem = GetGameInstance()->GetSubsystem<UMatchStatisticsSubsystem>();
	if (!StatsSubsystem || !StrategyPortalClass) return nullptr;
	
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	if (StatsSubsystem)
	{
		CenterX = StatsSubsystem->MapSize / 2.0f;
		CenterY = StatsSubsystem->MapSize / 2.0f;
	}
	FVector2D CenterLocation2D(CenterX, CenterY);
	
	//int32 index = 0; //SplayerSpawnedPortalsCounter
	AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
	if (!GS) return nullptr;
	
	float anglePerPlayer = 360.f/GS->GetSumAllPortalsCount();
	
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
				DeferredPortal->NativeRTSInitialize(PS->GetGenericTeamId(), PS, SpawnTransform);
				//DeferredPortal->SetEntityOwner(PS);
				DeferredPortal->Server_SetNextSpawnDelay(0.f);
				//DeferredPortal->SetGenericTeamId(PS->GetGenericTeamId());
				IStructureNetIDInterface::Execute_Server_SetStructureNetID(DeferredPortal, LinkedStructuresCounter);
				UGameplayStatics::FinishSpawningActor(DeferredPortal, SpawnTransform);
				PlayerSpawnedPortalsCounter++;
				return DeferredPortal;
			}
		}
	}
	
	return nullptr;
}*/

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
		
		AMainGameGameState* GS = Cast<AMainGameGameState>(GameState);
		if (!GS) return;
		
		StatsSubsystem->NodesPositions = NSB->GenNodesList(Stats->MapSeed, Stats->MapSize, Stats->NodesPerPlayer, GS->GetSumAllPortalsCount());
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
    		
    		NewActor->NativeRTSInitialize(254, nullptr, SpawnTransform);
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
	/*// ЦЕНТРАЛЬНЫЙ СЕРВЕРНЫЙ ТИК СТРАТЕГИИ
	// Здесь мы одной легкой итерацией обрабатываем логику ВСЕХ порталов сразу!
	for (APortalBase* Portal : ActivePortals)`
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
	}*/
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



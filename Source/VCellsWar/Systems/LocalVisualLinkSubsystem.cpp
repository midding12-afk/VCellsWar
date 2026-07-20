// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "LocalVisualLinkSubsystem.h"

#include "LocalGraphicsPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VCellsWar/Actors/Interface/StructureNetIDInterface.h"
#include "VCellsWar/GameMods/MainGameGameState.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Actors/PortalBase.h"

bool ULocalVisualLinkSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;

	// Отсекаем выделенный сервер, подсистема живет только там, где есть рендеринг графики
	if (FApp::CanEverRender() == false || IsRunningDedicatedServer())
	{
		return false;
	}
	return true;
}

void ULocalVisualLinkSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Мир полностью загрузился и игра началась. Пытаемся поймать GameState
	TryInitGameStateBridge();
	
	
}


void ULocalVisualLinkSubsystem::TryInitGameStateBridge()
{
	if (!GetWorld()) return;

	// Пытаемся достать GameState из текущего мира
	AMainGameGameState* MyGameState = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	
	if (MyGameState)
	{
		GetWorld()->GetTimerManager().ClearTimer(GameStateCheckTimerHandle);

		// --- ЖЕЛЕЗОБЕТОННАЯ ПОДПИСКА НА ДИНАМИЧЕСКИЙ ДЕЛЕГАТ ---
		// Метод AddUniqueDynamic гарантирует, что мы случайно не подпишемся дважды 
		// (например, при микро-рассинхронизациях сети во время загрузки).
		MyGameState->OnCachedDeloneEdgesTowerIDChangedBP.AddUniqueDynamic(this, &ULocalVisualLinkSubsystem::HandleDeloneEdgesChanged);

		// Сразу же вызываем обработчик один раз вручную, чтобы отрисовать ребра, 
		// если они успели прилететь по сети ДО того, как подсистема завершила подписку.
		HandleDeloneEdgesChanged(MyGameState->CachedDeloneEdgesTowerID);
	}
	else
	{
		// ЕСЛИ МЫ НА КЛИЕНТЕ И ИЗ-ЗА ПИНГА GAMESTATE ЕЩЕ НЕ СКАЧАЛСЯ:
		// Запускаем легкий периодический опрос (раз в 0.1 сек), пока объект не материализуется
		if (!GameStateCheckTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(
				GameStateCheckTimerHandle, 
				this, 
				&ULocalVisualLinkSubsystem::TryInitGameStateBridge, 
				0.1f, 
				true
			);
		}
	}
}

void ULocalVisualLinkSubsystem::UpdateTowerIDMap()
{
	// 1. Собираем ВСЕ акторы башен, существующие прямо сейчас на карте клиента
	TArray<AActor*> FoundActors1;
	TArray<AActor*> FoundActors2;
	FoundActors1.Empty();
	FoundActors2.Empty();
	TowerIDMap.Empty();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATowerBase::StaticClass(), FoundActors1);
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortalBase::StaticClass(), FoundActors2);

	
	TowerIDMap.Reserve(FoundActors1.Num() + FoundActors2.Num()); // Заранее резервируем память под размер массива для скорости

	for (AActor* Actor : FoundActors1)
	{
		int32 TowerNetworkID = IStructureNetIDInterface::Execute_GetStructureNetID(Actor);
		TowerIDMap.Add(TowerNetworkID, Actor);
	}
	
	for (AActor* Actor : FoundActors2)
	{
		int32 TowerNetworkID = IStructureNetIDInterface::Execute_GetStructureNetID(Actor);
		TowerIDMap.Add(TowerNetworkID, Actor);
	}
	
}

bool ULocalVisualLinkSubsystem::IsTowerInPlayerNetlink(AActor* Tower, const AMainGamePlayerState* PS) const
{
	int32 TowerID = IStructureNetIDInterface::Execute_GetStructureNetID(Tower);
	
	if (TowerIDMap.IsEmpty()) 
		return false;
	
	AMainGameGameState* MyGameState = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	if (!MyGameState) return false;
	
	TArray<FDeloneGraphEdge> DeloneEdgesTowerID = MyGameState->CachedDeloneEdgesTowerID;
	
	TArray<int32> ListOfPlayerTowers;
	int32 currentIndex = 0;
	
	auto UpdateNeighboursWithPS = [&](const int32 StructureNetID)
	{		
		for (FDeloneGraphEdge Edge : DeloneEdgesTowerID)
		{
			if (Edge.Start==StructureNetID && IStrategyEntityInterface::Execute_GetEntityOwnerState(TowerIDMap[Edge.End]) == PS && !ListOfPlayerTowers.Contains(Edge.End))
				ListOfPlayerTowers.Add(Edge.End);
		
			if (Edge.End==StructureNetID && IStrategyEntityInterface::Execute_GetEntityOwnerState(TowerIDMap[Edge.Start]) == PS && !ListOfPlayerTowers.Contains(Edge.Start))
				ListOfPlayerTowers.Add(Edge.Start);
		}
		
	};
	
	UpdateNeighboursWithPS(TowerID);
	
	
	while (currentIndex < ListOfPlayerTowers.Num())
	{		
		if (TowerIDMap[ListOfPlayerTowers[currentIndex]]->IsA(APortalBase::StaticClass()))
			return true;
		
		UpdateNeighboursWithPS(ListOfPlayerTowers[currentIndex]);
		
		currentIndex++;
	}
	
	return false;
}

TSet<int32> ULocalVisualLinkSubsystem::GetAllNeighbours(const int32 StructureNetID, const TArray<FDeloneGraphEdge>& DeloneEdgesTowerID) const
{
	TSet<int32> List;
	if (TowerIDMap.IsEmpty()) return List;
	
	for (FDeloneGraphEdge Edge : DeloneEdgesTowerID)
	{
		if (Edge.Start==StructureNetID)
			List.Add(Edge.End);
		
		if (Edge.End==StructureNetID)
			List.Add(Edge.Start);
	}
	
	return List;
}

void ULocalVisualLinkSubsystem::HandleDeloneEdgesChanged(TArray<FDeloneGraphEdge> DeloneEdgesTowerID)
{
	// 1. Получаем доступ к синглтону настроек проекта
	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;

	// 2. Загружаем и получаем чистый C++ класс Блупринта (TSubclassOf<AActor>)
	// Поскольку в настройках используется TSoftClassPtr (для оптимизации памяти), мы его синхронно загружаем:
	TSubclassOf<AActor> BeamClass = Settings->VisualLinkClass.LoadSynchronous();
	ULocalGraphicsPoolSubsystem* GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	
	//if (TowerIDMap.IsEmpty()) 
	//TODO сделать вызов опциональным по параметру
	UpdateTowerIDMap();
	
	if (!BeamClass || !GraphicsPool || TowerIDMap.IsEmpty()) return;
	
	// for (FLinkDataState& BeamData : BeamList)
	// {
	// 	BeamData.isActive = false;
	// }
	
	for (const FDeloneGraphEdge& Edge : DeloneEdgesTowerID)
	{
		ABeamBase* Beam = nullptr;
		
		if (FLinkDataState* FoundState = BeamList.FindByKey(Edge))
		{
			if (FoundState->BeamPtr)
			{
				FoundState->isActive = true;
				Beam=FoundState->BeamPtr;
			}
		}
		else
		{
			AActor* ABeam = GraphicsPool->GetActorFromPool(BeamClass);
			if (ABeam)
			{
				Beam = Cast<ABeamBase>(ABeam);	
				BeamList[BeamList.Emplace(Edge.Start, Edge.End, Beam)].isActive = true;
			}				
		}
		AActor* const* StartTowerPtr = TowerIDMap.Find(Edge.Start);
		AActor* const* EndTowerPtr = TowerIDMap.Find(Edge.End);

		// 3. Проверяем, что обе башни ребра УЖЕ заспавнились на клиенте и валидны
		if (Beam && StartTowerPtr && EndTowerPtr && IsValid(*StartTowerPtr) && IsValid(*EndTowerPtr))
			//if (Beam && TowerIDMap[Edge.Start] && TowerIDMap[Edge.End])
		{				
			// Достаем чистые указатели на акторов башен/порталов
			AActor* StartTower = *StartTowerPtr;
			AActor* EndTower = *EndTowerPtr;			
					
			Beam->Start = StartTower->GetActorLocation();
			Beam->End = EndTower->GetActorLocation();
			Beam->StartColor = IStrategyEntityInterface::Execute_GetTeamColor(StartTower);
			Beam->EndColor = IStrategyEntityInterface::Execute_GetTeamColor(EndTower);
			Beam->UpdateBeam();
		}
	}
	
	for (int32 Index = BeamList.Num() - 1; Index >= 0; --Index)
	{
		FLinkDataState& CurrentLink = BeamList[Index];

		if (!CurrentLink.isActive)
		{
			if (GraphicsPool && IsValid(CurrentLink.BeamPtr))
			{
				GraphicsPool->ReturnActorToPool(CurrentLink.BeamPtr);
			}
			BeamList.RemoveAt(Index);
		}
		else
		{
			CurrentLink.isActive = false;
		}
	}
	
}



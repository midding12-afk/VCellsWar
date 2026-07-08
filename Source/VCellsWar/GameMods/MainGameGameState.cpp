// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "MainGameGameState.h"

#include "Net/UnrealNetwork.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Actors/BlasterBase.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"

AMainGameGameState::AMainGameGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	// Выставляем частоту тика GameState на сервере под сетевой рейт (например, 20-30 обновлений пакетов в секунду)
	PrimaryActorTick.TickInterval = 0.05f; 
	
	AllTeamsColors.Init(FLinearColor::Black, 8); 
}

void AMainGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, MapSeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, MapSize, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, AllPlayerCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, AllNodesCountOnInit, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(AMainGameGameState, CachedDeloneEdgesTowerID, COND_None, REPNOTIFY_Always);
	
	
}

void AMainGameGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Раз в 0.05 секунд проверяем: настреляли ли что-то наши солдаты на сервере?
	if (HasAuthority() && AccumulatedShots.Num() > 0)
	{
		// Отправляем ВСЕ выстрелы ВСЕХ армий мира в одном компактном RPC-пакете!
		Multicast_BroadcastRTSShotsBatch(AccumulatedShots);

		// Очищаем буфер кадра для накопления следующей пачки выстрелов
		AccumulatedShots.Reset();
	}
	
}


void AMainGameGameState::OnRep_MapSeed()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_MapSize()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_AllPlayerCount()
{
	OnMapSeedReplicatedBP.Broadcast(MapSeed, MapSize, AllPlayerCount);
}

void AMainGameGameState::OnRep_AllNodesCountOnInit()
{
	ReplicatedAllNodesCountOnInit.Broadcast(AllNodesCountOnInit);
}

void AMainGameGameState::UpdateCachedDeloneEdgesTowerID(TArray<FDeloneGraphEdge> NewCachedDeloneEdgesTowerID)
{
	CachedDeloneEdgesTowerID = NewCachedDeloneEdgesTowerID;
	OnRep_CachedDeloneEdgesTowerID();
}

void AMainGameGameState::OnRep_CachedDeloneEdgesTowerID()
{
	OnCachedDeloneEdgesTowerIDChangedBP.Broadcast(CachedDeloneEdgesTowerID);	
}

void AMainGameGameState::Server_RegisterRTSShot(const FVector& Origin, const FVector& Direction, uint8 PlayerID)
{
	if (!HasAuthority()) return;

	FRTSShotData NewShot;
	NewShot.ShotOrigin = Origin;
	NewShot.ShotDirection = Direction;
	NewShot.PlayerID = PlayerID; // Запоминаем автора выстрела!

	AccumulatedShots.Add(NewShot);
}

void AMainGameGameState::AddTeamIDColor(int32 TeamId, FLinearColor TeamColor)
{
	// Проверяем, входит ли ID в диапазон нашего массива (0-7)
	if (AllTeamsColors.IsValidIndex(TeamId))
	{
		// Перезаписываем цвет строго в ячейку этого игрока!
		AllTeamsColors[TeamId] = TeamColor;
	}
}

void AMainGameGameState::Multicast_BroadcastRTSShotsBatch_Implementation(const TArray<FRTSShotData>& ShotsBatch)
{
	if (ShotsBatch.Num() == 0) return;
	
	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;
	
	TSubclassOf<AActor> BlasterClass = Settings->VisualBlasterClass.LoadSynchronous();
	ULocalGraphicsPoolSubsystem* GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
			
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());

	if (!BlasterClass || !GraphicsPool || !GS) return;
	
	for (const FRTSShotData& Shot : ShotsBatch)
	{
		
		FLinearColor LaserColor = FLinearColor::White; // Дефолтный цвет

		if (AllTeamsColors.IsValidIndex(Shot.PlayerID))
		{
			LaserColor = AllTeamsColors[Shot.PlayerID];
		}

		AActor* ABlast = GraphicsPool->GetActorFromPool(BlasterClass);
		if (ABlast)
		{
			ABlasterBase* Blast = Cast<ABlasterBase>(ABlast);	
			FVector ShotOrigin = Shot.ShotOrigin;
			FVector Direction = Shot.ShotDirection;
		
			Blast->InitBlasterShoot(Shot.ShotOrigin, Shot.ShotDirection, LaserColor, Shot.PlayerID);
		}			
	}
}


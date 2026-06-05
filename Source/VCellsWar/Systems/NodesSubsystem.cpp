// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "NodesSubsystem.h"

TArray<FVector2D> UNodesSubsystem::GenNodesList(int32 Seed, int32 MapSize, int32 NodesCount, int32 PlayersCount)
{
	float startAngleOffset = 0.f;
	
	TArray<FVector2D> list;
	list.Empty();
	
	FRandomStream rStream;
	rStream.Initialize(Seed);	
	
	FVector2D center = FVector2D(MapSize/2.f, MapSize/2.f);
	
	float anglePerPlayer = 360.f/PlayersCount;
	
	list.Add(center);
	
	for (int nod = 0; nod < NodesCount; nod++)
	{
		float angle = rStream.FRandRange(0.f, anglePerPlayer);
		float range = rStream.FRandRange(MapSize*0.1, 0.8f*MapSize/2.f);
		
		FVector2D vector = FVector2D(range, 0.f);
		vector = vector.GetRotated(angle+startAngleOffset);
		
		for (int pl = 0; pl < PlayersCount; pl++)
		{
			FVector2D vector_ = vector.GetRotated(pl * anglePerPlayer) + center;
			list.Add(vector_);
		}
	}	
	
	return list;
}

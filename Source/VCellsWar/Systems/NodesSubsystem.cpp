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
	
	float Radius = MapSize / 2.0f;
	int32 NumVertices = PlayersCount * 2;
	if (NumVertices < 4) NumVertices = 4;

	// Находим радиус вписанной окружности (минимальное расстояние от центра до ЛЮБОЙ грани карты)
	float Apothem = Radius * FMath::Cos(PI / static_cast<float>(NumVertices));

	// Задаем безопасный внутренний отступ от самой границы (например, на 150 единиц внутрь карты, чтобы башни не спавнились прямо на "красной линии")
	float MaxAllowedDistance = Apothem - 150.0f; 
	
	list.Add(center);
	
	for (int nod = 0; nod < NodesCount; nod++)
	{
		float angle = rStream.FRandRange(0.f, anglePerPlayer);
		float range = rStream.FRandRange(MapSize*0.1, MaxAllowedDistance);
		
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

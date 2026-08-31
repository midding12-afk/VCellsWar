// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "VoronoiSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "VCellsWar/GameMods/MainGameGameState.h"

void UVoronoiSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bIsActive = true;
}

void UVoronoiSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UVoronoiSubsystem::SetMapSize(float Width, float Height)
{
    MapWidth = Width;
    MapHeight = Height;
}

void UVoronoiSubsystem::UpdateNodePositions(const TArray<FVector2D>& NewPositions)
{
    ActiveNodes = NewPositions;
    
    InitializeRenderTarget(1024);
}

TArray<FVoronoiGraphEdge> UVoronoiSubsystem::GetVoronoiEdges()
{
    if (CachedVoronoiEdges.Num() == 0)
    {
        ReconstructDiagram();
    }
    
    return  CachedVoronoiEdges;
}

TArray<FVoronoiGraphEdge> UVoronoiSubsystem::GetVoronoiEdgesForMapSize()
{
	TArray<FVoronoiGraphEdge> Result;
	Result.Empty();
    
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	if (!GS) return Result;
	
	float CurrentWorldMapSize = GS->MapSize; 
        
	if (CachedVoronoiEdges.Num() == 0)
	{
		ReconstructDiagram();
	}
    
	// Получаем наш замкнутый многоугольник (первая точка == последней)
	TArray<FVector2D> MapPoly = GenerateMapBoundaryVertices();
	
	Result.Reserve(CachedVoronoiEdges.Num() + MapPoly.Num());
    
	for (FVoronoiGraphEdge& line : CachedVoronoiEdges)
	{
		FVector2D ValidStart = line.Start;
		FVector2D ValidEnd = line.End;

		bool bStartInside = IsPointInsideClosedPolygon(ValidStart, MapPoly);
		bool bEndInside = IsPointInsideClosedPolygon(ValidEnd, MapPoly);

		// Если одна из точек вылетела за контур карты — клипаем её!
		// Если обе точки внутри (bStartInside && bEndInside) — этот блок просто пропускается, 
		// и ребро монолитно улетает на отрисовку без лишней математики.
		if (!bStartInside || !bEndInside)
		{
			// Бежим по граням замкнутого массива (до Num() - 1)
			for (int32 i = 0; i < MapPoly.Num() - 1; ++i)
			{
				FVector2D HitPoint;
				if (FindLineIntersection(ValidStart, ValidEnd, MapPoly[i], MapPoly[i + 1], HitPoint))
				{
					// Сдвигаем именно ту точку, которая была снаружи
					if (!bStartInside) ValidStart = HitPoint;
					else ValidEnd = HitPoint;
					break; // Пересечение найдено, грани контура больше не перебираем
				}
			}
		}

		// ПРОЕКЦИИ НА ЭКРАН миникарты
		float StartScreenX = (ValidStart.Y / CurrentWorldMapSize) * 256.0f;
		float StartScreenY = (1.f - ValidStart.X / CurrentWorldMapSize) * 256.0f;
        
		float EndScreenX = (ValidEnd.Y / CurrentWorldMapSize) * 256.0f;
		float EndScreenY = (1.f - ValidEnd.X / CurrentWorldMapSize) * 256.0f;
        
		Result.Add(FVoronoiGraphEdge(FVector2D(StartScreenX, StartScreenY), FVector2D(EndScreenX, EndScreenY)));
	}
    
    for (int32 i = 0; i < MapPoly.Num() - 1; ++i)
    {
        FVector2D PolyStart = MapPoly[i];
        FVector2D PolyB   = MapPoly[i + 1];

        // Прогоняем вершины красной рамки через математику экрана:
        float PolyStartScreenX = (PolyStart.Y / CurrentWorldMapSize) * 256.0f;
        float PolyStartScreenY = (1.f - PolyStart.X / CurrentWorldMapSize) * 256.0f;
        
        float PolyEndScreenX = (PolyB.Y / CurrentWorldMapSize) * 256.0f;
        float PolyEndScreenY = (1.f - PolyB.X / CurrentWorldMapSize) * 256.0f;

        // Упаковываем грань многоугольника в общий возвращаемый буфер ребер
        Result.Add(FVoronoiGraphEdge(
            FVector2D(PolyStartScreenX, PolyStartScreenY), 
            FVector2D(PolyEndScreenX, PolyEndScreenY)
        ));
    }
    
	return Result;
}


void UVoronoiSubsystem::ToggleActive(bool bActive)
{
    bIsActive = bActive;
    
    UWorld* World = GetGameInstance()->GetWorld();
    if (!World) return;

    if (bIsActive)
    {
        if (!TimerHandle.IsValid())
        {
            World->GetTimerManager().SetTimer(TimerHandle, this, &UVoronoiSubsystem::ReconstructAndDraw, 0.5f, true);
        }
    }
    else
    {
        if (TimerHandle.IsValid())
        {
            // ИСПРАВЛЕНО: Убиваем таймер и очищаем хэндл
            World->GetTimerManager().ClearTimer(TimerHandle);
            TimerHandle.Invalidate(); // Сбрасываем внутреннее состояние структуры в дефолтное
        }
    }
}

void UVoronoiSubsystem::ReconstructAndDraw()
{
    if (ActiveNodes.Num() < 3) return;

    ReconstructDiagram();
    DrawEdgesToRenderTarget();
}



// void UVoronoiSubsystem::Tick(float DeltaTime)
// {
//     if (ActiveNodes.Num() < 3) return;
//
//     // Запускаем пересчет геометрии каждый кадр
//     ReconstructDiagram();
//
//     // Опционально: Визуализация отладочными линиями в 3D мире (на высоте Z = 100)
//     if (UWorld* World = GetGameInstance()->GetWorld())
//     {
//         for (const FVoronoiGraphEdge& Edge : CachedVoronoiEdges)
//         {
//             FVector Start3D(Edge.Start.X, Edge.Start.Y, 100.0f);
//             FVector End3D(Edge.End.X, Edge.End.Y, 100.0f);
//             DrawDebugLine(World, Start3D, End3D, FColor::Green, false, -1.0f, 0, 15.0f);
//         }
//     }
// }

TArray<FVoronoiTriangle> UVoronoiSubsystem::RunBowyerWatson(const TArray<FVector2D>& Points, const FVoronoiTriangle& SuperTriangle)
{
    TArray<FVoronoiTriangle> Triangulation;
    Triangulation.Add(SuperTriangle);

    for (const FVector2D& Point : Points)
    {
        TArray<FVoronoiTriangle> BadTriangles;

        // Шаг 1: Ищем треугольники, нарушающие критерий пустой окружности
        for (const FVoronoiTriangle& Tri : Triangulation)
        {
            if (Tri.CircumcircleContains(Point))
            {
                BadTriangles.Add(Tri);
            }
        }

        // Шаг 2: Выделяем ребра внешнего контура поврежденной зоны
        TArray<FVoronoiEdge> PolygonEdges;
        for (const FVoronoiTriangle& Tri : BadTriangles)
        {
            FVoronoiEdge Edges[3] = { FVoronoiEdge(Tri.P1, Tri.P2), FVoronoiEdge(Tri.P2, Tri.P3), FVoronoiEdge(Tri.P3, Tri.P1) };
            
            for (const FVoronoiEdge& Edge : Edges)
            {
                bool bIsShared = false;
                for (const FVoronoiTriangle& OtherTri : BadTriangles)
                {
                    if (&Tri == &OtherTri) continue;
                    if (OtherTri.HasEdge(Edge))
                    {
                        bIsShared = true;
                        break;
                    }
                }
                if (!bIsShared)
                {
                    PolygonEdges.Add(Edge);
                }
            }
        }

        // Шаг 3: Удаляем старые невалидные треугольники
        for (const FVoronoiTriangle& Tri : BadTriangles)
        {
            Triangulation.RemoveAll([&Tri](const FVoronoiTriangle& T) {
                return T.P1.Equals(Tri.P1) && T.P2.Equals(Tri.P2) && T.P3.Equals(Tri.P3);
            });
        }

        // Шаг 4: Строим новые треугольники от точек контура к новой вершине
        for (const FVoronoiEdge& Edge : PolygonEdges)
        {
            Triangulation.Add(FVoronoiTriangle(Edge.P1, Edge.P2, Point));
        }
    }

    // Шаг 5: Очищаем сетку от треугольников, связанных с супер-треугольником
    /*Triangulation.RemoveAll([&SuperTriangle](const FVoronoiTriangle& T) {
        return T.SharesVertexWith(SuperTriangle);
    });*/

    return Triangulation;
}

/*void UVoronoiSubsystem::ReconstructDiagram()
{
    CachedVoronoiEdges.Empty();

    // Создаем супер-треугольник вокруг всей игровой области
    float Margin = 5.0f * FMath::Max(MapWidth, MapHeight);
    FVoronoiTriangle SuperTriangle(
        FVector2D(MapWidth / 2.0f, -Margin),
            FVector2D(-Margin, MapHeight * 1.5f),
        FVector2D(MapWidth * 1.5f, MapHeight * 1.5f)
    );

    // Вычисляем Триангуляцию Делоне
    TArray<FVoronoiTriangle> Triangles = RunBowyerWatson(ActiveNodes, SuperTriangle);
    
    // 1. Стандартный проход: ищем внутренние ребра между смежными треугольниками
    for (int32 i = 0; i < Triangles.Num(); ++i)
    {
        // Считаем, сколько соседей мы нашли для треугольника i
        // Всего у треугольника может быть максимум 3 соседа
        int32 NeighborCount = 0; 
        bool bHasShared[3] = { false, false, false }; // Для граней 1-2, 2-3, 3-1

        for (int32 j = 0; j < Triangles.Num(); ++j)
        {
            if (i == j) continue;

            // Проверяем общие вершины для определения смежности граней
            bool bShareP1 = Triangles[i].P1.Equals(Triangles[j].P1) || Triangles[i].P1.Equals(Triangles[j].P2) || Triangles[i].P1.Equals(Triangles[j].P3);
            bool bShareP2 = Triangles[i].P2.Equals(Triangles[j].P1) || Triangles[i].P2.Equals(Triangles[j].P2) || Triangles[i].P2.Equals(Triangles[j].P3);
            bool bShareP3 = Triangles[i].P3.Equals(Triangles[j].P1) || Triangles[i].P3.Equals(Triangles[j].P2) || Triangles[i].P3.Equals(Triangles[j].P3);

            if (bShareP1 && bShareP2) bHasShared[0] = true;
            if (bShareP2 && bShareP3) bHasShared[1] = true;
            if (bShareP3 && bShareP1) bHasShared[2] = true;

            // Если это обычное внутреннее ребро, добавляем его (только один раз, когда i < j)
            if (i < j)
            {
                int32 SharedCount = (bShareP1 ? 1 : 0) + (bShareP2 ? 1 : 0) + (bShareP3 ? 1 : 0);
                if (SharedCount >= 2)
                {
                    FVoronoiGraphEdge Edge;
                    Edge.Start = Triangles[i].GetCircumcenter();
                    Edge.End = Triangles[j].GetCircumcenter();
                    
                    // Обрезаем внутреннее ребро, если оно вылетает за рамки текстуры
                    FVector2D ClampedStart, ClampedEnd;
                    if (ClampLineToMap(Edge.Start, Edge.End, ClampedStart, ClampedEnd))
                    {
                        Edge.Start = ClampedStart;
                        Edge.End = ClampedEnd;
                        CachedVoronoiEdges.Add(Edge);
                    }
                }
            }
        }

        // 2. Дополнительный проход: обрабатываем открытые внешние грани (границы поля)
        // Если у грани треугольника нет соседа, значит ребро Вороного идет перпендикулярно этой грани в бесконечность
        FVector2D Center = Triangles[i].GetCircumcenter();

        // Проверяем каждую из 3-х граней текущего треугольника
        for (int32 EdgeIdx = 0; EdgeIdx < 3; ++EdgeIdx)
        {
            if (!bHasShared[EdgeIdx]) // Соседа с этой стороны нет! Ребро открыто наружу.
            {
                FVector2D EdgeStartNode, EdgeEndNode;
                if (EdgeIdx == 0) { EdgeStartNode = Triangles[i].P1; EdgeEndNode = Triangles[i].P2; }
                else if (EdgeIdx == 1) { EdgeStartNode = Triangles[i].P2; EdgeEndNode = Triangles[i].P3; }
                else { EdgeStartNode = Triangles[i].P3; EdgeEndNode = Triangles[i].P1; }

                // Вычисляем направление перпендикуляра к этой грани Делоне (серединный перпендикуляр)
                FVector2D MidPoint = (EdgeStartNode + EdgeEndNode) * 0.5f;
                FVector2D EdgeDir = EdgeEndNode - EdgeStartNode;
                
                // Направление луча Вороного (поворот вектора грани на 90 градусов)
                FVector2D VoronoiDir(-EdgeDir.Y, EdgeDir.X);
                VoronoiDir.Normalize();

                // Проверяем правильность направления луча (он должен идти ОТ третьей точки треугольника наружу)
                FVector2D ThirdPoint = (EdgeIdx == 0) ? Triangles[i].P3 : ((EdgeIdx == 1) ? Triangles[i].P1 : Triangles[i].P2);
                FVector2D ToCenter = Center - ThirdPoint;
                if (FVector2D::DotProduct(VoronoiDir, ToCenter) < 0.0f)
                {
                    VoronoiDir = -VoronoiDir; // Разворачиваем луч наружу, если он пошел внутрь треугольника
                }

                // Пускаем луч из центра окружности в направлении границы
                FVector2D IntersectionPoint;
                // Задаем условную "далёкую" точку по направлению луча для алгоритма обрезки
                FVector2D FarPoint = Center + VoronoiDir * (MapWidth + MapHeight); 

                if (FindMapBoundaryIntersection(Center, FarPoint, IntersectionPoint))
                {
                    // Если сам центр треугольника внутри карты, рисуем от него до края.
                    // Если центр снаружи (такое бывает у тупоугольных), обрезаем всю линию
                    FVector2D ClampedStart, ClampedEnd;
                    if (ClampLineToMap(Center, IntersectionPoint, ClampedStart, ClampedEnd))
                    {
                        FVoronoiGraphEdge BorderEdge;
                        BorderEdge.Start = ClampedStart;
                        BorderEdge.End = ClampedEnd;
                        CachedVoronoiEdges.Add(BorderEdge);
                    }
                }
            }
        }
    }
}*/
void UVoronoiSubsystem::ReconstructDiagram()
{
    CachedVoronoiEdges.Empty();
    CachedDeloneEdges.Empty();

    if (ActiveNodes.Num() < 3) return;

    // 1. Создаем супер-треугольник, который СИЛЬНО больше игровой карты
    // Это гарантирует, что все бесконечные лучи крайних ячеек пересекутся за пределами поля
    float Margin = 10.0f * FMath::Max(MapWidth, MapHeight);
    FVoronoiTriangle SuperTriangle(
        FVector2D(MapWidth / 2.0f, -Margin),
        FVector2D(-Margin, MapHeight * 2.0f),
        FVector2D(MapWidth * 2.0f, MapHeight * 2.0f)
    );

    // 2. Получаем триангуляцию Делоне (вместе с супер-треугольником!)
    TArray<FVoronoiTriangle> Triangles = RunBowyerWatson(ActiveNodes, SuperTriangle);

    // 3. Строим ребра Вороного
    // Так как внешние точки теперь связаны с супер-треугольником, у КАЖДОЙ игровой ноды 
    // теперь есть замкнутый цикл из окружающих её треугольников.
    for (int32 i = 0; i < Triangles.Num(); ++i)
    {
        for (int32 j = i + 1; j < Triangles.Num(); ++j)
        {
            // Проверяем, являются ли треугольники i и j соседями (делят ли они ребро)
            int32 SharedVertices = 0;
            if (Triangles[i].P1.Equals(Triangles[j].P1) || Triangles[i].P1.Equals(Triangles[j].P2) || Triangles[i].P1.Equals(Triangles[j].P3)) SharedVertices++;
            if (Triangles[i].P2.Equals(Triangles[j].P1) || Triangles[i].P2.Equals(Triangles[j].P2) || Triangles[i].P2.Equals(Triangles[j].P3)) SharedVertices++;
            if (Triangles[i].P3.Equals(Triangles[j].P1) || Triangles[i].P3.Equals(Triangles[j].P2) || Triangles[i].P3.Equals(Triangles[j].P3)) SharedVertices++;

            if (SharedVertices >= 2)
            {
                // Центры описанных окружностей соседних треугольников дают ребро Вороного
                FVector2D SetupStart = Triangles[i].GetCircumcenter();
                FVector2D SetupEnd = Triangles[j].GetCircumcenter();

                // 4. Обрезаем ребро по рамке карты (0,0) -> (MapWidth, MapHeight)
                FVector2D ClampedStart, ClampedEnd;
                if (ClampLineToMap(SetupStart, SetupEnd, ClampedStart, ClampedEnd))
                {
                    FVoronoiGraphEdge Edge;
                    Edge.Start = ClampedStart;
                    Edge.End = ClampedEnd;
                    CachedVoronoiEdges.Add(Edge);
                }
            }
        }
    }
    
    for (const FVoronoiTriangle& Tri : Triangles)
    {
        int32 TowersID[3] = {
            GetTowerIDByPoint(Tri.P1),
            GetTowerIDByPoint(Tri.P2), 
            GetTowerIDByPoint(Tri.P3)
        };
        
        if (TowersID[0] == INDEX_NONE || TowersID[1] == INDEX_NONE || TowersID[2] == INDEX_NONE)
        {
            continue;
        }
        
        auto MakeSortedEdge = [](int32 ID_A, int32 ID_B) -> FDeloneGraphEdge
        {
            return (ID_A < ID_B) ? FDeloneGraphEdge(ID_A, ID_B) : FDeloneGraphEdge(ID_B, ID_A);
        };
        
        FDeloneGraphEdge CurrentEdges[3] = {
            MakeSortedEdge(TowersID[0], TowersID[1]),
            MakeSortedEdge(TowersID[1], TowersID[2]),
            MakeSortedEdge(TowersID[2], TowersID[0])
        };
        
        for (const FDeloneGraphEdge& DEdge : CurrentEdges)
        {
            if (!CachedDeloneEdges.Contains(DEdge))
            {
                CachedDeloneEdges.Add(DEdge);
            }
        }
    }
    
}


int32 UVoronoiSubsystem::GetTowerIDByPoint(const FVector2D& Point)
{
    int32 FoundIndex = INDEX_NONE; 
    
    if (ActiveNodes.Find(Point, FoundIndex))
    {
        return FoundIndex;
    }
    
    return INDEX_NONE;
}


// Функция обрезки отрезка под рамки карты (0,0) -> (MapWidth, MapHeight)
bool UVoronoiSubsystem::ClampLineToMap(FVector2D Start, FVector2D End, FVector2D& OutStart, FVector2D& OutEnd)
{
    float MinX = 0.0f; float MinY = 0.0f;
    float MaxX = MapWidth; float MaxY = MapHeight;

    // Используем ранее написанную нами функцию FindMapBoundaryIntersection для обрезки обеих сторон
    //FVector2D IntersectStart, IntersectEnd;
    
    bool bStartInside = (Start.X >= 0 && Start.X <= MapWidth && Start.Y >= 0 && Start.Y <= MapHeight);
    bool bEndInside = (End.X >= 0 && End.X <= MapWidth && End.Y >= 0 && End.Y <= MapHeight);

    if (bStartInside && bEndInside) 
    {
        OutStart = Start; OutEnd = End;
        return true;
    }

    if (bStartInside)
    {
        OutStart = Start;
        FindMapBoundaryIntersection(Start, End, OutEnd);
        return true;
    }
    
    if (bEndInside)
    {
        OutEnd = End;
        FindMapBoundaryIntersection(End, Start,  OutStart);
        return true;
    }

    // Если обе точки снаружи, проверяем, пересекает ли отрезок карту вообще
    return false; 
}


bool UVoronoiSubsystem::FindMapBoundaryIntersection(FVector2D StartNode, FVector2D EndNode, FVector2D& OutIntersection)
{
    // 1. Получаем вектор направления и нормализуем его
    FVector2D Direction = EndNode - StartNode;
    
    // Защита от нулевого вектора (если точки совпали)
    if (Direction.IsNearlyZero())
    {
        OutIntersection = StartNode;
        return false;
    }
    Direction.Normalize();

    // Границы нашей карты (Мин: 0,0, Макс: Width, Height)
    float BoxMinX = 0.0f;
    float BoxMinY = 0.0f;
    float BoxMaxX = MapWidth;
    float BoxMaxY = MapHeight;

    // Инициализируем tMax большим числом. t — это дистанция (множитель) вдоль луча.
    float tMax = FLT_MAX; 

    // 2. Проверка пересечения по оси X
    if (!FMath::IsNearlyZero(Direction.X))
    {
        // Вычисляем дистанции до левой и правой границ
        float t1 = (BoxMinX - StartNode.X) / Direction.X;
        float t2 = (BoxMaxX - StartNode.X) / Direction.X;

        // Нам нужно пересечение только ВПЕРЕДИ по направлению луча (t > 0)
        float tTargetX = FMath::Max(t1, t2);
        if (tTargetX > 0.0f)
        {
            tMax = FMath::Min(tMax, tTargetX);
        }
    }

    // 3. Проверка пересечения по оси Y
    if (!FMath::IsNearlyZero(Direction.Y))
    {
        // Вычисляем дистанции до нижней и верхней границ
        float t1 = (BoxMinY - StartNode.Y) / Direction.Y;
        float t2 = (BoxMaxY - StartNode.Y) / Direction.Y;

        // Берем пересечение впереди по лучу
        float tTargetY = FMath::Max(t1, t2);
        if (tTargetY > 0.0f)
        {
            tMax = FMath::Min(tMax, tTargetY);
        }
    }

    // 4. Если нашли валидный множитель tMax, восстанавливаем финальную координату
    if (tMax != FLT_MAX)
    {
        OutIntersection = StartNode + Direction * tMax;
        return true;
    }

    // На случай, если точка находится вне карты или луч направлен неверно
    OutIntersection = StartNode;
    return false;
}


void UVoronoiSubsystem::InitializeRenderTarget(int32 Resolution)
{
    if (VoronoiRT || Resolution == 0) return;

    // Создаем Render Target программно
    VoronoiRT = UKismetRenderingLibrary::CreateRenderTarget2D(
        GetGameInstance()->GetWorld(), 
        Resolution, 
        Resolution, 
        RTF_RGBA8
    );
    
    // ГАРАНТИЯ ОЧИСТКИ: Задаем дефолтный цвет фона (черный) и включаем автоочистку
    VoronoiRT->ClearColor = FLinearColor::Black;
    //VoronoiRT->bAutoClear = true; // Указывает движку стирать память перед BeginDraw
    
    // Отключаем мип-мапы для четкости линий
    VoronoiRT->UpdateResourceImmediate(false);
}

void UVoronoiSubsystem::DrawEdgesToRenderTarget()
{
    if (!VoronoiRT || CachedVoronoiEdges.Num() == 0) return;

    UWorld* World = GetGameInstance()->GetWorld();
    UCanvas* Canvas = nullptr;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    
    UKismetRenderingLibrary::ClearRenderTarget2D(World, VoronoiRT, FLinearColor::Black);

    // Начинаем отрисовку на Render Target
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, VoronoiRT, Canvas, Size, Context);
    
    if (Canvas)
    {
        // Очищаем текстуру черным цветом (фон)
        //Canvas->Clear(FLinearColor::Black);

        // Рисуем каждое ребро Вороного
        for (const FVoronoiGraphEdge& Edge : CachedVoronoiEdges)
        {
            // Переводим мировые координаты (0..MapWidth) в координаты текстуры (0..Size)
            float StartX = (Edge.Start.X / MapWidth) * Size.X;
            float StartY = (Edge.Start.Y / MapHeight) * Size.Y;
            float EndX = (Edge.End.X / MapWidth) * Size.X;
            float EndY = (Edge.End.Y / MapHeight) * Size.Y;

            // Рисуем белую линию. Толщину (Thickness) можно настроить
            FCanvasLineItem LineItem(FVector2D(StartX, StartY), FVector2D(EndX, EndY));
            LineItem.SetColor(FLinearColor::White);
            LineItem.LineThickness = 4.0f; 
            
            Canvas->DrawItem(LineItem);
        }
        
        
        //////////////
        // 2. Отрисовка центров нод (Одиночные красные точки)
        FVector2D PointSize(6.0f, 6.0f); // Размер точки в пикселях (ширина и высота)

        for (const FVector2D& NodePos : ActiveNodes)
        {
            if (MapWidth <= 0.0f || MapHeight <= 0.0f) continue;

            // Переводим мировые координаты ноды в координаты текстуры
            float NodeX = (NodePos.X / MapWidth) * Size.X;
            float NodeY = (NodePos.Y / MapHeight) * Size.Y;

            // Смещаем позицию на половину размера, чтобы точка рисовалась ровно по центру ноды
            FVector2D TopLeftPosition(NodeX - (PointSize.X / 2.0f), NodeY - (PointSize.Y / 2.0f));

            // Рисуем заполненный красный квадрат-точку
            Canvas->K2_DrawBox(
                TopLeftPosition, 
                PointSize, 
                PointSize.X,          // Толщина (в данном случае заполняет весь бокс)
                FLinearColor::Red     // Цвет точки
            );
        }
    }
    

    // Завершаем отрисовку и сохраняем результат в текстуру
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);
}

TArray<FVector2D> UVoronoiSubsystem::GenerateMapBoundaryVertices()
{
    TArray<FVector2D> Vertices;
    
    AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	
    if (!GS) return Vertices;
    
    float MapSize = GS->MapSize; 
    int32 NumPlayers = GS->GetSumAllPortalsCount();    
    
    int32 NumVertices = NumPlayers * 2;
    if (NumVertices < 4) NumVertices = 4;

    float Radius = MapSize / 2.0f;
    Vertices.Reserve(NumVertices+1);

    // Шаг угла в радианах для каждой вершины
    float AngleStep = (2.0f * PI) / NumVertices;
	
    // Корректируем начальный поворот (например, на PI/4 для ромба при 2 игроках)
    float StartAngleOffset = 0.f;//-PI / 2.0f;//(NumPlayers == 2) ? (PI / 4.0f) : 0.0f;

    for (int32 i = 0; i <= NumVertices; ++i)
    {
        float CurrentAngle = StartAngleOffset + (i * AngleStep);
        float X = Radius * FMath::Cos(CurrentAngle) + MapSize/2.0f;
        float Y = Radius * FMath::Sin(CurrentAngle) + MapSize/2.0f;
        Vertices.Add(FVector2D(X, Y));
    }

    return Vertices;
}

/** Находит точку пересечения двух 2D отрезков (A-B и C-D) */
bool UVoronoiSubsystem::FindLineIntersection(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D, FVector2D& OutIntersection)
{
    float Denominator = (B.X - A.X) * (D.Y - C.Y) - (B.Y - A.Y) * (D.X - C.X);
    if (FMath::IsNearlyZero(Denominator)) return false; // Линии параллельны

    float Numerator1 = (A.Y - C.Y) * (D.X - C.X) - (A.X - C.X) * (D.Y - C.Y);
    float Numerator2 = (A.Y - C.Y) * (B.X - A.X) - (A.X - C.X) * (B.Y - A.Y);

    float U = Numerator1 / Denominator;
    float V = Numerator2 / Denominator;

    if (U >= 0.0f && U <= 1.0f && V >= 0.0f && V <= 1.0f)
    {
        OutIntersection.X = A.X + U * (B.X - A.X);
        OutIntersection.Y = A.Y + U * (B.Y - A.Y);
        return true;
    }
    return false;
}

/** Проверяет, находится ли точка внутри выпуклого многоугольника контура */
bool UVoronoiSubsystem::IsPointInsideClosedPolygon(const FVector2D& P, const TArray<FVector2D>& ClosedPolygon)
{
    bool bInside = false;
	
    // Бежим строго до Num() - 1, так как грань образуется парами: [i] и [i + 1]
    for (int32 i = 0; i < ClosedPolygon.Num() - 1; ++i)
    {
        const FVector2D& PolyA = ClosedPolygon[i];
        const FVector2D& PolyB = ClosedPolygon[i + 1];

        // Проверка Коэна-Сазерленда без модульной арифметики
        if (((PolyA.Y > P.Y) != (PolyB.Y > P.Y)) &&
            (P.X < (PolyB.X - PolyA.X) * (P.Y - PolyA.Y) / (PolyB.Y - PolyA.Y) + PolyA.X))
        {
            bInside = !bInside;
        }
    }
    return bInside;
}
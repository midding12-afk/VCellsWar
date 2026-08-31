// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VoronoiSubsystem.generated.h"

// Структура ребра для поиска уникальных границ полигона
struct FVoronoiEdge
{
    FVector2D P1 = FVector2D::ZeroVector;
    FVector2D P2 = FVector2D::ZeroVector;

    FVoronoiEdge(FVector2D InP1, FVector2D InP2) : P1(InP1), P2(InP2) {}

    bool IsAlmostEqual(const FVoronoiEdge& Other) const
    {
        return (P1.Equals(Other.P1, 0.1f) && P2.Equals(Other.P2, 0.1f)) ||
               (P1.Equals(Other.P2, 0.1f) && P2.Equals(Other.P1, 0.1f));
    }
};

// Структура треугольника Делоне
struct FVoronoiTriangle
{
    FVector2D P1 = FVector2D::ZeroVector;
    FVector2D P2 = FVector2D::ZeroVector;
    FVector2D P3 = FVector2D::ZeroVector;

    FVoronoiTriangle(FVector2D InP1, FVector2D InP2, FVector2D InP3) : P1(InP1), P2(InP2), P3(InP3) {}

    // Проверка: находится ли точка внутри описанной окружности треугольника
    bool CircumcircleContains(const FVector2D& Point) const
    {
        float Abx = P1.X - Point.X;
        float Aby = P1.Y - Point.Y;
        float Bbx = P2.X - Point.X;
        float Bby = P2.Y - Point.Y;
        float Cbx = P3.X - Point.X;
        float Cby = P3.Y - Point.Y;

        float AbSq = Abx * Abx + Aby * Aby;
        float BbSq = Bbx * Bbx + Bby * Bby;
        float CcSq = Cbx * Cbx + Cby * Cby;

        // Вычисление детерминанта матрицы 3x3
        float Det = Abx * (Bby * CcSq - Cby * BbSq) -
                    Aby * (Bbx * CcSq - Cbx * BbSq) +
                    AbSq * (Bbx * Cby - Cbx * Bby);

        // Вершины должны быть ориентированы против часовой стрелки
        float CrossProduct = (P2.X - P1.X) * (P3.Y - P1.Y) - (P2.Y - P1.Y) * (P3.X - P1.X);
        if (CrossProduct < 0)
        {
            Det = -Det;
        }

        return Det > 0.0f;
    }

    // Вычисление центра описанной окружности (Узел Вороного)
    FVector2D GetCircumcenter() const
    {
        float D = 2.0f * (P1.X * (P2.Y - P3.Y) + P2.X * (P3.Y - P1.Y) + P3.X * (P1.Y - P2.Y));
        if (FMath::IsNearlyZero(D)) return P1;

        float P1Sq = P1.X * P1.X + P1.Y * P1.Y;
        float P2Sq = P2.X * P2.X + P2.Y * P2.Y;
        float P3Sq = P3.X * P3.X + P3.Y * P3.Y;

        float Ux = (P1Sq * (P2.Y - P3.Y) + P2Sq * (P3.Y - P1.Y) + P3Sq * (P1.Y - P2.Y)) / D;
        float Uy = (P1.X * (P2Sq - P3Sq) + P2.X * (P3Sq - P1Sq) + P3.X * (P1Sq - P2Sq)) / D;

        return FVector2D(Ux, Uy);
    }

    bool SharesVertexWith(const FVoronoiTriangle& Other) const
    {
        return P1.Equals(Other.P1) || P1.Equals(Other.P2) || P1.Equals(Other.P3) ||
               P2.Equals(Other.P1) || P2.Equals(Other.P2) || P2.Equals(Other.P3) ||
               P3.Equals(Other.P1) || P3.Equals(Other.P2) || P3.Equals(Other.P3);
    }

    bool HasEdge(const FVoronoiEdge& Edge) const
    {
        FVoronoiEdge E1(P1, P2), E2(P2, P3), E3(P3, P1);
        return E1.IsAlmostEqual(Edge) || E2.IsAlmostEqual(Edge) || E3.IsAlmostEqual(Edge);
    }
    
    bool IsCircumcenterInside() const
    {
        // Строим векторы сторон треугольника
        FVector2D AB = P2 - P1;
        FVector2D BC = P3 - P2;
        FVector2D CA = P1 - P3;

        // Для каждого угла берем два вектора, направленные ИЗ вершины
        // Угол A: векторы AB и -CA (что равно AC)
        FVector2D AC = -CA;
        float DotA = FVector2D::DotProduct(AB, AC);

        // Угол B: векторы BC и -AB (что равно BA)
        FVector2D BA = -AB;
        float DotB = FVector2D::DotProduct(BC, BA);

        // Угол C: векторы CA и -BC (что равно CB)
        FVector2D CB = -BC;
        float DotC = FVector2D::DotProduct(CA, CB);

        // Центр внутри, если ВСЕ три угла острые (скалярное произведение > 0)
        // Если нужно включить границы (прямоугольный треугольник), замените > на >=
        return (DotA > 0.0f) && (DotB > 0.0f) && (DotC > 0.0f);
    }
};

// Структура ребра Вороного для вывода в Blueprints
USTRUCT(BlueprintType)
struct FVoronoiGraphEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector2D Start = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector2D End = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FDeloneGraphEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Start = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 End = -1;
    
    FDeloneGraphEdge() {}
    FDeloneGraphEdge(int32 InA, int32 InB) : Start(InA), End(InB) {}
    
    bool operator==(const FDeloneGraphEdge& Other) const
    {
        return (Start == Other.Start && End == Other.End);
    }
};

UCLASS()
class VCELLSWAR_API UVoronoiSubsystem : public UGameInstanceSubsystem//, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Реализация интерфейса FTickableGameObject
    //virtual void Tick(float DeltaTime) override;
    //virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
    //virtual bool IsTickable() const override { return bIsActive; }
    //virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVoronoiSubsystem, STATCAT_Advanced); }

    // API для управления точками из игры (Blueprints/C++)
    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void SetMapSize(float Width, float Height);

    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void UpdateNodePositions(const TArray<FVector2D>& NewPositions);

    UFUNCTION(BlueprintPure, Category = "Voronoi")
    TArray<FVoronoiGraphEdge> GetVoronoiEdges();
    
    UFUNCTION(BlueprintPure, Category = "Voronoi")
    TArray<FVoronoiGraphEdge> GetVoronoiEdgesForMapSize();

    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void ToggleActive(bool bActive);
    
    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void ReconstructAndDraw();

    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    UTextureRenderTarget2D* GetVoronoiRenderTarget() const { return VoronoiRT; }

    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    void InitializeRenderTarget(int32 Resolution = 1024);
    
    TArray<FDeloneGraphEdge> GetCachedDeloneEdges() const { return CachedDeloneEdges; }
    
    UFUNCTION(BlueprintCallable, Category = "Voronoi")
    TArray<FVector2D> GenerateMapBoundaryVertices();
    
    bool FindLineIntersection(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D,
                          FVector2D& OutIntersection);
    bool IsPointInsideClosedPolygon(const FVector2D& P, const TArray<FVector2D>& ClosedPolygon);
    
    TArray<FVector2D> GetTovers() const { return ActiveNodes; }
private:
    void ReconstructDiagram();
    bool ClampLineToMap(FVector2D Start, FVector2D End, FVector2D& OutStart, FVector2D& OutEnd);
    bool FindMapBoundaryIntersection(FVector2D StartNode, FVector2D EndNode, FVector2D& OutIntersection);
    TArray<FVoronoiTriangle> RunBowyerWatson(const TArray<FVector2D>& Points, const FVoronoiTriangle& SuperTriangle);

    bool bIsActive = false;
    float MapWidth = 10000.0f;
    float MapHeight = 10000.0f;

    TArray<FVector2D> ActiveNodes;
    TArray<FVoronoiGraphEdge> CachedVoronoiEdges;
    TArray<FDeloneGraphEdge> CachedDeloneEdges;
    
    int32 GetTowerIDByPoint(const FVector2D& Point);
    
    UPROPERTY()
    UTextureRenderTarget2D* VoronoiRT = nullptr;

    void DrawEdgesToRenderTarget();
    
    

    FTimerHandle TimerHandle;
};

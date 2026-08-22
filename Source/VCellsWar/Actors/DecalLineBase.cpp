// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "DecalLineBase.h"

#include "Components/DecalComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "VCellsWar/Systems/LocalGraphicsPoolSubsystem.h"


ADecalLineBase::ADecalLineBase()
{
	// Полностью отключаем тик для производительности
	PrimaryActorTick.bCanEverTick = false;
	
	// Настройки оптимизации для сетевой RTS
	bReplicates = false;
	bAlwaysRelevant = false;
	bNetLoadOnClient = false;
	SetReplicatingMovement(false);
	SetActorEnableCollision(false);

	// Создаем компонент декали
	LineDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("LineDecalComponent"));
	RootComponent = LineDecalComponent;

	// Включаем легковесный просчет габаритов
	LineDecalComponent->bComputeFastLocalBounds = true;
	LineDecalComponent->FadeScreenSize = 0.01f;
	LineDecalComponent->SetSortOrder(0);

	// Базовая видимость
	LineDecalComponent->SetVisibility(true);
	LineDecalComponent->SetHiddenInGame(false);
}

void ADecalLineBase::InitLineSettings(UMaterialInterface* NewMaterial, 	float InThickness, 	float InLengthMultiplier, 	float InProjectionHeight)
{
	// Сохраняем настройки во внутренние переменные
	CachedThickness = InThickness;
	CachedLengthMultiplier = InLengthMultiplier;
	CachedProjectionHeight = InProjectionHeight;

	// Устанавливаем материал один раз
	if (NewMaterial && LineDecalComponent->GetDecalMaterial() != NewMaterial)
	{
		LineDecalComponent->SetDecalMaterial(NewMaterial);
	}
}

void ADecalLineBase::SetParametrs(const FVector& PointStart, const FVector& PointEnd)
{
	// Находим центр и разворот в сторону конечной точки
	const FVector NewLocation = PointStart + (PointEnd - PointStart) * 0.5 * CachedLengthMultiplier;
	const FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(PointStart, PointEnd);
	SetActorLocationAndRotation(NewLocation, NewRotation);

	// Считаем расстояние между точками
	const float Distance = FVector::Dist(PointStart, PointEnd);

	// Рассчитываем полудлину с учетом сохраненного коэффициента
	const float HalfLength = (Distance * 0.5f) * CachedLengthMultiplier;

	// Задаем итоговый размер, используя кэшированные параметры
	LineDecalComponent->DecalSize = FVector(HalfLength, CachedThickness, CachedProjectionHeight);

	// Принудительно обновляем рендер-стейт (необходимо, так как изменились размеры)
	LineDecalComponent->MarkRenderStateDirty(); 
}

void ADecalLineBase::RemoveDecal()
{
	ULocalGraphicsPoolSubsystem* GraphicsPool = nullptr;
	if (GetWorld())
	{
		GraphicsPool = GetWorld()->GetSubsystem<ULocalGraphicsPoolSubsystem>();
	}
	
	if (GraphicsPool)
	{
		GraphicsPool->ReturnActorToPool(this);
	}
	else
	{
		Destroy();
	}
}

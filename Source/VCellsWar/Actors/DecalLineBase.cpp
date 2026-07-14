// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "DecalLineBase.h"

#include "Components/DecalComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "VCellsWar/RTSVisualSettings.h"

// Sets default values
ADecalLineBase::ADecalLineBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = false;
	bAlwaysRelevant = false;
	bNetLoadOnClient = false;
	SetReplicatingMovement(false);

	// Создаем компонент декали выделения
	LineDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("LineDecalComponent"));
	RootComponent = LineDecalComponent;

	LineDecalComponent->DecalSize = FVector(1.0f, 3.0f, 100.0f);
	
	// 1. Включаем легковесный просчет габаритов. Огромные толпы маркеров назначения
	// больше не будут перегружать процессор калькуляциями тяжелых Bound Boxes!
	LineDecalComponent->bComputeFastLocalBounds = true;
	
	SetActorEnableCollision(false);
	
	LineDecalComponent->FadeScreenSize = 0.01f;
	
	// 3. Сортировка (она у вас белая, но для надежности оставляем как есть)
	LineDecalComponent->SetSortOrder(0);


	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;

	UMaterialInterface* DecalMaterial = Settings->MoveLineDecalMaterial.LoadSynchronous();
	
	// Если материал кольца задан — принудительно накатываем его на декаль
	if (DecalMaterial && LineDecalComponent->GetDecalMaterial() == nullptr)
	{
		LineDecalComponent->SetDecalMaterial(DecalMaterial);
	}

	// Будим и показываем зеленое кольцо под ногами!
	LineDecalComponent->SetVisibility(true);
	LineDecalComponent->SetHiddenInGame(false);
}

// Called when the game starts or when spawned
void ADecalLineBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADecalLineBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ADecalLineBase::SetParametrs(const FVector& PointStart, const FVector& PointEnd)
{
	SetActorLocationAndRotation((PointStart+PointEnd)/2.f, UKismetMathLibrary::FindLookAtRotation(PointStart, PointEnd));
	LineDecalComponent->DecalSize = FVector(FVector::Dist(PointStart, PointEnd)/2.f, 3.0f, 500.0f);
	LineDecalComponent->MarkRenderStateDirty(); 
}


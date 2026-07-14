// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "DecalMoveTargetBase.h"

#include "Components/DecalComponent.h"
#include "VCellsWar/RTSVisualSettings.h"

// Sets default values
ADecalMoveTargetBase::ADecalMoveTargetBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = false;
	bAlwaysRelevant = false;
	bNetLoadOnClient = false;
	SetReplicatingMovement(false);

	// Создаем компонент декали выделения
	SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
	RootComponent = SelectionDecalComponent;

	// Разворачиваем декаль строго вертикально вниз (лицом к земле)
	SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// Настраиваем 3D-размер проекционного куба декали (X - глубина луча, Y и Z - радиус круга)
	// Размер 64, 45, 45 идеально накроет землю под стандартной капсулой
	SelectionDecalComponent->DecalSize = FVector(64.0f, 90.0f, 90.0f);
	
	// 1. Включаем легковесный просчет габаритов. Огромные толпы маркеров назначения
	// больше не будут перегружать процессор калькуляциями тяжелых Bound Boxes!
	SelectionDecalComponent->bComputeFastLocalBounds = true;
	
	SetActorEnableCollision(false);
	
	SelectionDecalComponent->FadeScreenSize = 0.01f;
	
	// 3. Сортировка (она у вас белая, но для надежности оставляем как есть)
	SelectionDecalComponent->SetSortOrder(0);


	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;

	UMaterialInterface* DecalMaterial = Settings->SelectionDecalMaterial.LoadSynchronous();
	
	// Если материал кольца задан — принудительно накатываем его на декаль
	if (DecalMaterial && SelectionDecalComponent->GetDecalMaterial() == nullptr)
	{
		SelectionDecalComponent->SetDecalMaterial(DecalMaterial);
	}

	// Будим и показываем зеленое кольцо под ногами!
	SelectionDecalComponent->SetVisibility(true);
	SelectionDecalComponent->SetHiddenInGame(false);
	
}

// Called when the game starts or when spawned
void ADecalMoveTargetBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADecalMoveTargetBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


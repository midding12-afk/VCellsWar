// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "StrategyPlacementSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "VCellsWar/GameMods/MainGamePlayerController.h"
#include "CollisionQueryParams.h" 
#include "WorldCollision.h"  
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"
#include "VCellsWar/TacticalFlag/TacticalFlagBase.h"

UStrategyPlacementSubsystem::UStrategyPlacementSubsystem()
{
}

void UStrategyPlacementSubsystem::StartPlacementMode(FPlacementBuildingData BuildingData)
{
	if (CurrentLocalPreviewActor) CancelPlacement();

	if (!BuildingData.RealServerBuildingClass || !BuildingData.LocalClientPreviewClass) return;

	CurrentActiveBuildingData = BuildingData;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	// Спавним фантома строго на клиенте внутри текущего мира локального игрока!
	if (UWorld* World = GetWorld())
	{
		CurrentLocalPreviewActor = World->SpawnActor<AActor>(CurrentActiveBuildingData.LocalClientPreviewClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentLocalPreviewActor)
		{
			bIsPlacingActive = true;
			bCurrentLocationIsValid = false;
		}
	}
}

void UStrategyPlacementSubsystem::TickPlacement(float DeltaTime, APlayerController* PC)
{
	if (!bIsPlacingActive || !CurrentLocalPreviewActor || !PC) return;

	FHitResult HitResult;
	
	// Конвертируем С++ канал коллизии в системный тип ETraceTypeQuery, который теперь требует мышка!
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);

	if (PC->GetHitResultUnderCursorByChannel(TraceType, true, HitResult))
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, *FString::Printf(TEXT("PROJECTILE OVERLAP: %s"), *HitResult.Location.ToString()));
		
		// 1. Двигаем фантома за курсором
		CurrentLocalPreviewActor->SetActorLocation(HitResult.Location, false, nullptr, ETeleportType::TeleportPhysics);

		// 2. Сканируем мир на коллизии
		bool bNewLocationLegality = CheckPlacementLegality(HitResult.Location);
		
		if (bNewLocationLegality != bCurrentLocationIsValid)
		{
			bCurrentLocationIsValid = bNewLocationLegality;
			UpdatePreviewMaterials(bCurrentLocationIsValid);
		}
	}
}



bool UStrategyPlacementSubsystem::CheckPlacementLegality(const FVector& TestLocation)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	// Создаем невидимую сферу-щуп под физические габариты текущей постройки
	FCollisionShape OverlapSphere = FCollisionShape::MakeSphere(CurrentActiveBuildingData.PlacementCheckRadius);
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	
	// Базовые игноры фантома и камеры игрока
	QueryParams.AddIgnoredActor(CurrentLocalPreviewActor); 
	if (APlayerController* PC = Cast<APlayerController>(GetOuter()))
	{
		QueryParams.AddIgnoredActor(PC->GetPawn());
	}

	// Мы создаем жесткий поисковый фильтр по ТИПАМ ОБЪЕКТОВ (Object Types).
	// Мы насильно приказываем сфере-щупу Chaos затянуть в массив абсолютно всё, 
	// что является живым юнитом, статической башней или динамическим порталом!
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);         // Намертво ловим пехоту и солдат!
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);   // Намертво ловим башни, стены и домики!
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);  // Намертво ловим порталы и аномалии!

	// Запускаем специализированный метод OverlapMultiByObjectType.
	// Он опрашивает честные 3D-объемы тел в памяти ОЗУ, полностью игнорируя слепые лучевые каналы!
	World->OverlapMultiByObjectType(
		OverlapResults, 
		TestLocation, 
		FQuat::Identity, 
		ObjectQueryParams, // Наш бронебойный многоканальный С++ фильтр
		OverlapSphere, 
		QueryParams
	);
	// =========================================================================

	// Бежим по собранным результатам (теперь здесь гарантированно сидят и солдаты, и башни!)
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (AActor* OverlappedActor = Overlap.GetActor())
		{
			// Проверяем: реализует ли этот объект наш С++ интерфейс игровых сущностей?
			if (OverlappedActor->GetClass()->ImplementsInterface(UStrategyEntityInterface::StaticClass()))
			{
				// Мы наступили на легитимную башню, солдата или стену! СТРОИТЬ НЕЛЬЗЯ.
				return false;
			}
		}
	}

	// Если дошли сюда и никто с интерфейсом не заблокировал — место идеально свободно!
	return true;
}





void UStrategyPlacementSubsystem::UpdatePreviewMaterials(bool bIsLegallyValid)
{
	if (!CurrentLocalPreviewActor) return;

	// 1. Формируем HDR-неоновые цвета фракций (выкручиваем яркость на 5.0-10.0 для сочного свечения)
	FLinearColor LegalColor = FLinearColor(0.0f, 10.0f, 5.0f, 1.0f);   // Сочный неоновый бирюзово-зеленый
	FLinearColor IllegalColor = FLinearColor(10.0f, 0.0f, 0.0f, 1.0f); // Грозный плазменно-красный цвет запрета

	FLinearColor TargetColor = bIsLegallyValid ? LegalColor : IllegalColor;

	// 2. ПОИСК КОМПОНЕНТОВ NIAGARA НА КЛИЕНТЕ
	TArray<UNiagaraComponent*> NiagaraComponents;
	CurrentLocalPreviewActor->GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
	{
		if (NiagaraComp)
		{
			// Напрямую меняем переменную типа Linear Color внутри графа эмиттера.
			// Как только Slate/RHI конвейер зафиксирует изменение, Niagara-луч
			// мгновенно перекрасится прямо на ландшафте под мышкой игрока!
			NiagaraComp->SetNiagaraVariableLinearColor(TEXT("MainColor"), TargetColor);
		}
	}
}

// void UStrategyPlacementSubsystem::ConfirmPlacement(APlayerController* PC)
// {
// 	if (!bIsPlacingActive || !bCurrentLocationIsValid || !CurrentLocalPreviewActor || !PC) return;
//
// 	FVector FinalLocation = CurrentLocalPreviewActor->GetActorLocation();
//
// 	CurrentLocalPreviewActor->Destroy();
// 	CurrentLocalPreviewActor = nullptr;
// 	bIsPlacingActive = false;
//
// 	// Извлекаем кастомный контроллер и даем команду на RPC спавн!
// 	if (auto PC = Cast<AMainGamePlayerController>(PC))
// 	{
// 		PC->Server_SpawnRtsBuilding(CurrentActiveBuildingData.RealServerBuildingClass, FinalLocation);
// 	}
// }

void UStrategyPlacementSubsystem::ConfirmPlacement(APlayerController* PC)
{
	if (!bIsPlacingActive || !CurrentLocalPreviewActor || !PC) return;

	FVector FinalLocation = CurrentLocalPreviewActor->GetActorLocation();
	
	// Опрашиваем луч мыши, чтобы узнать, наступили ли мы на готовый флаг
	FHitResult ClickHitResult;
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	ATacticalFlagBase* ClickedFlag = nullptr;
	
	TSubclassOf<AActor> BuildingClassToSpawn = CurrentActiveBuildingData.RealServerBuildingClass;

	if (BuildingClassToSpawn->IsChildOf(ATacticalFlagBase::StaticClass()))
	{
		if (PC->GetHitResultUnderCursorByChannel(TraceType, true, ClickHitResult))
		{
			if (AActor* HitActor = ClickHitResult.GetActor())
			{
				if (HitActor != CurrentLocalPreviewActor && HitActor->IsA(ATacticalFlagBase::StaticClass()))
				{
					ClickedFlag = Cast<ATacticalFlagBase>(HitActor);
				}
			}
		}
	}
	
	if (!bCurrentLocationIsValid && !ClickedFlag) return;

	// Гасим текущего локального фантома на клиенте
	CurrentLocalPreviewActor->Destroy();
	CurrentLocalPreviewActor = nullptr;
	bIsPlacingActive = false;

	// ПЕРЕДАЕМ РУЛЕВОЕ УПРАВЛЕНИЕ БЕСКОНЕЧНОМУ КОНВЕЙЕРУ КОНТРОЛЛЕРА
	if (auto StrategyPC = Cast<AMainGamePlayerController>(PC))
	{
		StrategyPC->HandleUniversalPlacementClick(ClickedFlag, FinalLocation, CurrentActiveBuildingData);
	}
}

void UStrategyPlacementSubsystem::CancelPlacement()
{
	if (CurrentLocalPreviewActor)
	{
		CurrentLocalPreviewActor->Destroy();
		CurrentLocalPreviewActor = nullptr;
	}
	bIsPlacingActive = false;
}
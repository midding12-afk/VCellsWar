// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "RTSMinimapSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "VCellsWar/RTSVisualSettings.h"
#include "VCellsWar/Actors/Interface/StrategyEntityInterface.h"
#include "VCellsWar/GameMods/MainGameGameState.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h" 
#include "Kismet/KismetRenderingLibrary.h"
#include "VCellsWar/Actors/TowerBase.h"
#include "VCellsWar/GameMods/MainGamePlayerController.h"


void URTSMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Жесткий сетевой замок: миникарта полностью вырезана из памяти выделенного сервера! 
	if (GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	
	const URTSVisualSettings* Settings = GetDefault<URTSVisualSettings>();
	if (!Settings) return;

	MinimapNiagaraAsset = Settings->MinimapNiagaraAsset.LoadSynchronous();


	// 2. СПАВН ГРАФИЧЕСКОГО КОМПЬЮТ-КОМПОНЕНТА В МИРЕ ЗА 0 НАНОСЕКУНД
	if (MinimapNiagaraAsset)// && LoadedRenderTargetTexture)
	{
		MinimapNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), 
			MinimapNiagaraAsset, 
			FVector::ZeroVector, 
			FRotator::ZeroRotator, 
			FVector(1.0f), 
			false, // Не уничтожать автоматически
			false  // Не активировать до первой настройки переменных
		);

		if (MinimapNiagaraComponent)
		{
			MinimapNiagaraComponent->Activate();
		}
	}

	// Аллоцируем (резервируем) память на С++ стеке Кучи под 4096 юнитов,	
	// чтобы полностью исключить микро-фризы из-за перевыделения памяти во время битва 1к на 1к! 
	GPUEntityDataBuffer.Reserve(4096);
	
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(MinimapUpdateTimerHandle))
	{
		TimerManager.SetTimer(MinimapUpdateTimerHandle, this, &URTSMinimapSubsystem::UpdateMinimapGPU, 1.0f, true);
	}
}

void URTSMinimapSubsystem::Deinitialize()
{
	if (MinimapNiagaraComponent)
	{
		MinimapNiagaraComponent->DestroyComponent();
	}
	
	RegisteredEntities.Empty();
	GPUEntityDataBuffer.Empty();
	
	if (GetWorld()) 
		GetWorld()->GetTimerManager().ClearTimer(MinimapUpdateTimerHandle);
	
	Super::Deinitialize();
}

void URTSMinimapSubsystem::RegisterEntity(AActor* Entity)
{
	if (IsValid(Entity))
	{
		// Используем AddUnique, чтобы застраховать подсистему от случайных повторных вызовов репликации
		RegisteredEntities.AddUnique(Entity);
	}
}

void URTSMinimapSubsystem::UnregisterEntity(AActor* Entity)
{
	RegisteredEntities.Remove(Entity);
}

void URTSMinimapSubsystem::UpdateMinimapGPU()
{
	// Если компонент сломан или на карте пока нет никого — мгновенно выходим, освобождая CPU
	if (!MinimapNiagaraComponent || RegisteredEntities.Num() == 0) return;

	// Сбрасываем счетчик буфера кадра БЕЗ очистки зарезервированной емкости (Capacity)
	GPUEntityDataBuffer.Reset();

	
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	
	if (!GS) return;
	float CurrentWorldMapSize = GS->MapSize; 
	
	FVector CurrentWorldCenter = FVector(CurrentWorldMapSize/2.f,CurrentWorldMapSize/2.f,0.f); 
	
	// Фиксированный пиксельный размер текстуры Render Target миникарты (256x256 пикселей)
	float TargetTextureSize = 256.0f; 
	
	if (GPUEntityDataBuffer.Max() < RegisteredEntities.Num())
	{
		// Выделяем память с запасом +256 ячеек сверху, чтобы процессор не вызывал 
		// этот тяжелый системный метод на каждом следующем заспавненном юните!
		int32 NewSafeCapacity = RegisteredEntities.Num() + 256;
		GPUEntityDataBuffer.Reserve(NewSafeCapacity);
	}
	
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), MyDynamicMinimapRT, MyDynamicMinimapRT->ClearColor);

	// =========================================================================
	// ТРАНСЛЯЦИЯ ВЕКТОРОВ И ПАКЕТИРОВАНИЕ ДЛЯ ШИНЫ PCIe 
	// Переводим 3D-координаты мира в пиксели от 0 до 255 прямо в С++ за один проход O(N)!
	// =========================================================================
	for (const TWeakObjectPtr<AActor>& EntityPtr : RegisteredEntities)
	{
		AActor* Entity = EntityPtr.Get();
		if (!IsValid(Entity)) continue;

		IStrategyEntityInterface* Interface = Cast<IStrategyEntityInterface>(Entity);
		if (!Interface) continue;

		FVector Loc = Entity->GetActorLocation();
		int32 FactionID = Interface->Execute_GetEntityFactionID(Entity);
		
		// Нативно вычисляем тип: 1.0f - Башня (Pawn), 0.0f - Пехотинец (Character)
		float TypeFlag = Entity->IsA(ATowerBase::StaticClass()) ? 1.0f : 0.0f;

		// С++ Математика перевода координат мира в пиксельный холст
		FVector RelativeCoords = Loc - CurrentWorldCenter;
		float NormX = RelativeCoords.X / CurrentWorldMapSize;
		float NormY = RelativeCoords.Y / CurrentWorldMapSize;

		// Применяем инверсию осей RTS-экрана, чтобы верх миникарты четко смотрел на Север (+X мира)
		float MinimapX = (NormY + 0.5f) * TargetTextureSize;
		float MinimapY = (0.5f - NormX) * TargetTextureSize;

		// Бронированная защита памяти GPU: гарантируем, что пиксель не вылетит за диапазон 0..255! 
		float FinalPixelX = FMath::Clamp(MinimapX, 0.0f, TargetTextureSize - 1.0f);
		float FinalPixelY = FMath::Clamp(MinimapY, 0.0f, TargetTextureSize - 1.0f);
		
		
		// =========================================================================
		// ПОБИТОВАЯ УПАКОВКА RGB-ЦВЕТА В ОДИН FLOAT
		// Переводим дробные каналы 0.0-1.0 в байты 0-255 и сдвигаем по регистрам памяти:
		// =========================================================================
		FLinearColor FinalFactionColor = IStrategyEntityInterface::Execute_GetTeamColor(Entity);
		
		
		/*
		uint32 Red   = FMath::Clamp(FMath::RoundToInt(FinalFactionColor.R * 255.0f), 0, 255);
		uint32 Green = FMath::Clamp(FMath::RoundToInt(FinalFactionColor.G * 255.0f), 0, 255);
		uint32 Blue  = FMath::Clamp(FMath::RoundToInt(FinalFactionColor.B * 255.0f), 0, 255);

		// Сливаем 3 байта в одно 32-битное беззнаковое целое число (0x00RRGGBB)
		uint32 PackedInt = (Red << 16) | (Green << 8) | Blue;

		// ГЛАВНЫЙ С++ ХАК: Переносим битовую сетку в float БЕЗ изменения битов (Bit Cast)
		float PackedColorAsFloat = *reinterpret_cast<float*>(&PackedInt);
		*/
		
		float R_Part = FMath::Clamp(FinalFactionColor.R, 0.0f, 1.0f) * 1000000.0f; // Например: 1.0 -> 1000000
		float G_Part = FMath::Clamp(FinalFactionColor.G, 0.0f, 1.0f) * 1000.0f;    // Например: 0.5 -> 500
		float B_Part = FMath::Clamp(FinalFactionColor.B, 0.0f, 1.0f) * 1.0f;       // Например: 0.2 -> 0.2

		// Просто складываем их! На выходе получается абсолютно легитимный чистый float: R.RR + G.GG + B.BB >> RRRGGGB.BB
		float SafePackedColor = R_Part + G_Part + B_Part;


		// Упаковываем полностью готовую пиксельную дату в буфер кадра
		// X=ПиксельX, Y=ПиксельY, Z=Тип, W=Цвет
		GPUEntityDataBuffer.Add(FVector4f(FinalPixelX, FinalPixelY, TypeFlag, SafePackedColor));
	}

	
	if (MinimapNiagaraComponent && GPUEntityDataBuffer.Num() > 0)
	{
		// 1. Отправляем сам массив координат солдат на GPU (наш прошлый рабочий метод):
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(MinimapNiagaraComponent, TEXT("EntityDataArray"), GPUEntityDataBuffer);

		// 2. СВЯЩЕННЫЙ С++ ФИКС: Передаем точное количество живых солдат для ElementCount X!
		// Метод SetNiagaraVariableInt встроен прямо в класс компонента. 
		// Имя "UnitsCount" должно символ в символ (с учетом регистра) совпадать с левой панелью!
		MinimapNiagaraComponent->SetNiagaraVariableInt(TEXT("UnitsCount"), GPUEntityDataBuffer.Num());
	}
}

void URTSMinimapSubsystem::SetupMinimapTexture(UTextureRenderTarget2D* InRenderTarget)
{
	if (!IsValid(InRenderTarget) || !MinimapNiagaraAsset) return;

	// 1. Намертво запоминаем адрес готового ассета, переданного из виджета!
	MyDynamicMinimapRT = InRenderTarget;
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), MyDynamicMinimapRT, MyDynamicMinimapRT->ClearColor);

	// 2. Спавним живой Niagara-компонент в мире
	//MinimapNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MinimapNiagaraAsset, FVector::ZeroVector);

	/*if (MinimapNiagaraComponent)
	{
		// 3. Приводим наш готовый ассет к базовому UTexture* специально для метода Niagara [1.5]
		UTexture* BaseTexture = Cast<UTexture>(MyDynamicMinimapRT);
		
		// 4. Скармливаем этот же холст в Compute-шейдер видеокарты
		UNiagaraFunctionLibrary::SetTextureObject(MinimapNiagaraComponent, TEXT("MinimapRenderTarget"), BaseTexture);
		MinimapNiagaraComponent->Activate();
	}*/
}

FMinimapCameraFrustum URTSMinimapSubsystem::GetCameraFrustumPoints()
{
	FMinimapCameraFrustum Frustum;
	
	// 1. Извлекаем GameState и размер карты
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	if (!GS) return Frustum;
	float CurrentWorldMapSize = GS->MapSize;

	// 2. Извлекаем ЛОКАЛЬНОГО PlayerController игрока на этом клиенте
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController()) return Frustum;

	// 3. Получаем размеры экрана монитора
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	// Углы вьюпорта
	TArray<FVector2D> ScreenCorners = {
		FVector2D(0, 0),                           // Верхний левый
		FVector2D(ViewportSizeX, 0),               // Upper-Right
		FVector2D(0, ViewportSizeY),               // Lower-Left
		FVector2D(ViewportSizeX, ViewportSizeY)    // Lower-Right
	};

	TArray<FVector2D> WorldPoints;
	WorldPoints.Reserve(4);

	for (const FVector2D& Corner : ScreenCorners)
	{
		FVector WorldLocation, WorldDirection;
		
		if (PC->DeprojectScreenPositionToWorld(Corner.X, Corner.Y, WorldLocation, WorldDirection))
		{
			// Находим точку пересечения луча с плоскостью земли (Z = 0)
			if (!FMath::IsNearlyZero(WorldDirection.Z))
			{
				float t = -WorldLocation.Z / WorldDirection.Z;
				FVector Intersection = WorldLocation + (WorldDirection * t);
				WorldPoints.Add(FVector2D(Intersection.X, Intersection.Y));
			}
			else 
			{
				WorldPoints.Add(FVector2D(WorldLocation.X, WorldLocation.Y));
			}
		}
	}

	// Защита, если депроекция сбойнула
	if (WorldPoints.Num() < 4) return Frustum;

	// 4. Проецируем мировые координаты в 2D пиксели Slate-миникарты (Твоя формула!)
	auto ProjectToMinimap = [CurrentWorldMapSize](const FVector2D& WorldPos) -> FVector2D
	{
		float ScreenX = (WorldPos.Y / CurrentWorldMapSize) * 256.0f;
		float ScreenY = (1.f - WorldPos.X / CurrentWorldMapSize) * 256.0f;
		return FVector2D(ScreenX, ScreenY);
	};

	Frustum.TopLeft     = ProjectToMinimap(WorldPoints[0]);
	Frustum.TopRight    = ProjectToMinimap(WorldPoints[1]);
	Frustum.BottomLeft  = ProjectToMinimap(WorldPoints[2]);
	Frustum.BottomRight = ProjectToMinimap(WorldPoints[3]);

	return Frustum;
}

void URTSMinimapSubsystem::ProcessMinimapClick(FVector2D LocalClickPosition)
{
	AMainGameGameState* GS = Cast<AMainGameGameState>(GetWorld()->GetGameState());
	if (!GS) return;
	float CurrentWorldMapSize = GS->MapSize;

	AMainGamePlayerController* PC = Cast<AMainGamePlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC) return;

	// =========================================================================
	// 1. Находим чистый процент положения курсора на миникарте (от 0.0 до 1.0)
	// 2. Помним рокировку: Экранный X — это мировой Y, а экранный Y — мировой X!
	// =========================================================================
	float PercentX = LocalClickPosition.X / 256.f; // Экранный X (вправо)
	float PercentY = LocalClickPosition.Y / 256.f; // Экранный Y (вниз)

	// В мире: 
	// Мировой Y идет слева направо (совпадает с PercentX)
	// Мировой X идет снизу вверх (инвертирован относительно PercentY: 1.0f - PercentY)
	float WorldX = (1.0f - PercentY) * CurrentWorldMapSize;
	float WorldY = PercentX * CurrentWorldMapSize;

	// Безопасный Clamp, чтобы при клике на самый край рамки камера не вылетала за карту
	WorldX = FMath::Clamp(WorldX, 0.0f, CurrentWorldMapSize);
	WorldY = FMath::Clamp(WorldY, 0.0f, CurrentWorldMapSize);

	// Отправляем чистые С++ координаты в твой готовый метод контроллера
	PC->TeleportLocalCameraTo(FVector2D(WorldX, WorldY));
}
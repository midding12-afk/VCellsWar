// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RTSMinimapSubsystem.generated.h"

// Опережающие объявления типов ядра для ускорения времени компиляции
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class VCELLSWAR_API URTSMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Нативная С++ инициализация: подсистема рождается на клиентах автоматически при старте мира 
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void RegisterEntity(AActor* Entity);
	void UnregisterEntity(AActor* Entity);

	/** Главный воркер кадра: вызывается из HUD Tick или PlayerController Tick для слива памяти на GPU */
	void UpdateMinimapGPU();
	
	// Ссылка на Niagara-компонент, который выступает в роли нашего графического сопроцессора 
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> MinimapNiagaraComponent;
	
	UPROPERTY(BlueprintReadOnly)
	UTexture* LoadedRenderTargetTexture;
	

private:
	// Стерильный локальный список живых сущностей на этом клиенте (используем слабые указатели для безопасности памяти) 
	TArray<TWeakObjectPtr<AActor>> RegisteredEntities;

	

	// Ассет Niagara GPU Compute системы, загружаемый из настроек
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> MinimapNiagaraAsset;

	// Высокоскоростной плоский буфер памяти кадра для прямой трансляции на видеокарту
	TArray<FVector4f> GPUEntityDataBuffer;
	
	UPROPERTY()
	FTimerHandle MinimapUpdateTimerHandle;
	
public:
	/** 
	 * иджет сам вызывает этот метод при старте (Construct) 
	 * и скармливает подсистеме свой готовый ассет Render Target! 
	 */
	UFUNCTION(BlueprintCallable, Category = "RTS|Minimap")
	void SetupMinimapTexture(UTextureRenderTarget2D* InRenderTarget);

private:
	// Наш внутренний С++ указатель, который будет очищаться через ClearRenderTarget2D
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> MyDynamicMinimapRT;
};

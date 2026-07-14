// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RTSVisualSettings.generated.h"

/**
 * 
 */

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RTS Visual Settings"))
class VCELLSWAR_API URTSVisualSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Ссылка на Блупринт вашего лазера. Будет видна в настройках проекта!
	UPROPERTY(Config, EditAnywhere, Category = "BeamsBetweenTowers")
	TSoftClassPtr<AActor> VisualLinkClass;
	
	UPROPERTY(Config, EditAnywhere, Category = "BlasterShoots")
	TSoftClassPtr<AActor> VisualBlasterClass;
	
	UPROPERTY(Config, EditAnywhere, Category = "SelectDecal")
	TSoftObjectPtr<UMaterialInterface> SelectionDecalMaterial;
	
	UPROPERTY(Config, EditAnywhere, Category = "SelectDecal")
	TSoftObjectPtr<UMaterialInterface> MoveLineDecalMaterial;
	
};

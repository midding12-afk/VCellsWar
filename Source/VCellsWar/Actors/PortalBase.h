// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityBase.h"
#include "GameFramework/Actor.h"
#include "PortalBase.generated.h"

UCLASS()
class VCELLSWAR_API APortalBase : public AStrategyEntityBase
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Тег BlueprintNativeEvent означает: логика есть в C++, но Блупринт может её переопределить.
	// Тег BlueprintCallable оставляем ВНУТРИ макроса, чтобы функцию можно было вызывать нодой.
	// UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Visual")
	// void SetMainStructureColor(FLinearColor NewTeamColor);

	// КРИТИЧЕСКИ ВАЖНО: Для каждого BlueprintNativeEvent в C++ движок требует 
	// создать виртуальный метод со строгим суффиксом "_Implementation".
	// Именно в нем будет лежать базовый C++ код функции.
	//virtual void SetMainStructureColor_Implementation(FLinearColor NewTeamColor);
};

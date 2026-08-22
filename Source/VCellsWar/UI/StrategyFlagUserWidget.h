// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VCellsWar/TacticalFlag/StrategyFlagUiInterface.h"
#include "StrategyFlagUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class VCELLSWAR_API UStrategyFlagUserWidget : public UUserWidget, public IStrategyFlagUiInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	ATacticalFlagBase* OwnerFlag;
	virtual void SetSelfFlagActor(ATacticalFlagBase* Flag) override {OwnerFlag = Flag;};
	
	/** САМ ДЕЛЕГАТ: Его будет прекрасно видно и в Блупринт-слайдере, и в С++ коде флага! */
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiRadiusSliderChangedSignature OnRadiusSliderChanged;

	// Реализуем геттер интерфейса: просто отдаем ссылку на наш делегат в С++ флага!
	virtual FOnUiRadiusSliderChangedSignature& GetRadiusSliderChangedDelegate() override { return OnRadiusSliderChanged; }
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiRadiusSliderChangedSignatureLocal OnRadiusSliderChangedLocal;

	virtual FOnUiRadiusSliderChangedSignatureLocal& GetRadiusSliderChangedDelegateLocal() override { return OnRadiusSliderChangedLocal; }
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiButtonDeletePressedSignatureLocal OnButtonDeletePressed;

	virtual FOnUiButtonDeletePressedSignatureLocal& GetButtonDeletePressedDelegate() override { return OnButtonDeletePressed; }
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiCheckboxInFilterChangedSignatureLocal OnUiCheckboxInFilterChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiCheckboxOutFilterChangedSignatureLocal OnUiCheckboxOutFilterChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiCountInFilterChangedSignatureLocal OnUiCountInFilterChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiCountOutFilterChangedSignatureLocal OnUiCountOutFilterChanged;
	
	virtual FOnUiCheckboxInFilterChangedSignatureLocal& GetOnUiCheckboxInFilterChangedSignatureDelegate() override {return OnUiCheckboxInFilterChanged;}
	virtual FOnUiCheckboxOutFilterChangedSignatureLocal& GetOnUiCheckboxOutFilterChangedSignatureDelegate() override {return OnUiCheckboxOutFilterChanged;}
	virtual FOnUiCountInFilterChangedSignatureLocal& GetOnUiCountInFilterChangedSignatureDelegate() override {return OnUiCountInFilterChanged;}
	virtual FOnUiCountOutFilterChangedSignatureLocal& GetOnUiCountOutFilterChangedSignatureDelegate() override {return OnUiCountOutFilterChanged;}
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiButtonDeleteSourcePressedSignatureLocal OnButtonDeleteSourcePressed;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTS|UI|Events")
	FOnUiButtonDeleteDestinationPressedSignatureLocal OnButtonDeleteDestinationPressed;
	
	virtual FOnUiButtonDeleteSourcePressedSignatureLocal& GetButtonDeleteSourcePressedDelegate() override { return OnButtonDeleteSourcePressed; }
	virtual FOnUiButtonDeleteDestinationPressedSignatureLocal& GetButtonDeleteDestinationPressedDelegate() override { return OnButtonDeleteDestinationPressed; }
	
};
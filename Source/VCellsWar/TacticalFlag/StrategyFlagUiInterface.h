// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategyFlagUiInterface.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UStrategyFlagUiInterface : public UInterface
{
	GENERATED_BODY()
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiRadiusSliderChangedSignature, float, NewRadius);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiRadiusSliderChangedSignatureLocal, float, NewRadius);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUiButtonDeletePressedSignatureLocal);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiCheckboxInFilterChangedSignatureLocal, bool, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiCheckboxOutFilterChangedSignatureLocal, bool, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiCountInFilterChangedSignatureLocal, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUiCountOutFilterChangedSignatureLocal, int32, NewCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUiButtonDeleteSourcePressedSignatureLocal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUiButtonDeleteDestinationPressedSignatureLocal);


class VCELLSWAR_API IStrategyFlagUiInterface
{
	GENERATED_BODY()

public:
	
	virtual void SetSelfFlagActor(class ATacticalFlagBase* Flag) = 0;
	/** Вызывается из С++ на клиенте, чтобы обновить данные в UI */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|UI|Interface")
	void RefreshSoldierCount(int32 NewCount);

	/** Вызывается из Блупринт-UI кнопок, чтобы С++ код флага изменил тактический режим */
	// UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|UI|Interface")
	// void OnUiModeButtonClicked(uint8 NewModeIndex);

	/** Вызывается из Блупринт-ползунка (Slider), чтобы С++ изменил радиус сонара флага */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|UI|Interface")
	void OnUiRadiusSliderChanged(float NewRadiusValue);
	
	virtual FOnUiRadiusSliderChangedSignature& GetRadiusSliderChangedDelegate() = 0;
	virtual FOnUiRadiusSliderChangedSignatureLocal& GetRadiusSliderChangedDelegateLocal() = 0;
	virtual FOnUiButtonDeletePressedSignatureLocal& GetButtonDeletePressedDelegate() = 0;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|UI|Interface")
	void UpdateUiSizing(float NewScale, float NewOpacity);
	
	virtual FOnUiCheckboxInFilterChangedSignatureLocal& GetOnUiCheckboxInFilterChangedSignatureDelegate() = 0;
	virtual FOnUiCheckboxOutFilterChangedSignatureLocal& GetOnUiCheckboxOutFilterChangedSignatureDelegate() = 0;
	virtual FOnUiCountInFilterChangedSignatureLocal& GetOnUiCountInFilterChangedSignatureDelegate() = 0;
	virtual FOnUiCountOutFilterChangedSignatureLocal& GetOnUiCountOutFilterChangedSignatureDelegate() = 0;
	
	virtual FOnUiButtonDeleteSourcePressedSignatureLocal& GetButtonDeleteSourcePressedDelegate() = 0;
	virtual FOnUiButtonDeleteDestinationPressedSignatureLocal& GetButtonDeleteDestinationPressedDelegate() = 0;
	
};

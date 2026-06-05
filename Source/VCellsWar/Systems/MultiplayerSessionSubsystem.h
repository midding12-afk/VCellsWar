// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Logging/LogMacros.h"
#include "OnlineSubsystem.h"
#include "GameFramework/OnlineSession.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "VCellsWar/Menu/ServerDataObj.h"
#include "Engine/DataTable.h"


#include "MultiplayerSessionSubsystem.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FFactionColorRow : public FTableRowBase
{
    GENERATED_BODY()

    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy Colors")
    //FName ColorID;

    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy Colors")
    //FText ColorDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy Colors")
    FLinearColor ColorValue = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FMapSizeRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText SizeDisplayName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SizeValue = 0;
};

// Первый аргумент: имя нового типа делегата
// Второй аргумент: тип передаваемых данных (массив результатов)
// Третий аргумент: имя переменной, как она будет называться в Блупринте
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindSessionsCompleteBlueprint, const TArray<UServerDataObj*>&, FoundServers);

UCLASS()
class VCELLSWAR_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionSubsystem();

	
	// BlueprintAssignable — делает ее видимой для Блупринтов как Event Dispatcher
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer")
	FOnFindSessionsCompleteBlueprint OnFindSessionsCompleteBP;
	
protected:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	
public:	
	UFUNCTION(BlueprintCallable)
	void CreateSession(int MaxPlayerCount);
	
	UFUNCTION(BlueprintCallable)
	void FindSession();
	
	UFUNCTION(BlueprintCallable)
	void JoinSession(FString HostSteamID);
	
	UFUNCTION(BlueprintCallable)
	bool CheckSubsystem();
	
private:
	void OnSessionCreated(FName InSessionName, bool InWasCreated);
	void OnSessionFound(bool bWasSuccessful);
	void OnSessionJoin(const FName SessionName, EOnJoinSessionCompleteResult::Type ResultType);
	
	IOnlineSessionPtr OnlineSessionPtr;

	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;

	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;

	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	TSharedPtr<FOnlineSessionSearch> OnlineSessionSearch;
};

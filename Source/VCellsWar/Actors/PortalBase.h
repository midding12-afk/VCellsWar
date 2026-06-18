// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyEntityBase.h"
#include "GameFramework/Actor.h"
#include "Interface/StructureNetIDInterface.h"
#include "PortalBase.generated.h"

UCLASS()
class VCELLSWAR_API APortalBase : public AStrategyEntityBase, public IStructureNetIDInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_NextSpawnTime, BlueprintReadOnly, Category = "RTS | Logic")
	float NextSpawnTime = 0.0f;

	UFUNCTION()
	void OnRep_NextSpawnTime();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void Server_SetNextSpawnDelay(float DelaySeconds);
	
	float GetNextSpawnTime() { return NextSpawnTime; }

	// Тег BlueprintNativeEvent означает: логика есть в C++, но Блупринт может её переопределить.
	// Тег BlueprintCallable оставляем ВНУТРИ макроса, чтобы функцию можно было вызывать нодой.
	// UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Strategy | Visual")
	// void SetMainStructureColor(FLinearColor NewTeamColor);

	// КРИТИЧЕСКИ ВАЖНО: Для каждого BlueprintNativeEvent в C++ движок требует 
	// создать виртуальный метод со строгим суффиксом "_Implementation".
	// Именно в нем будет лежать базовый C++ код функции.
	//virtual void SetMainStructureColor_Implementation(FLinearColor NewTeamColor);
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS | Logic")
	int32 PortalId = -1;
	
	virtual int32 GetStructureNetID_Implementation() override {return PortalId;};
	virtual void Server_SetStructureNetID_Implementation(int32 NewID) override {if (HasAuthority()) PortalId =  NewID;};
	
	
};

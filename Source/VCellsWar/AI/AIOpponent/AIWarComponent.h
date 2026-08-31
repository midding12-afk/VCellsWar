// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AISquad.h" 
#include "AIWarComponent.generated.h"

UCLASS(ClassGroup=(RTS_AI), meta=(BlueprintSpawnableComponent))
class VCELLSWAR_API UAIWarComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAIWarComponent();
	virtual void BeginPlay() override;
protected:
	

	/** Настройки балансировки военного ИИ */
	// UPROPERTY(EditDefaultsOnly, Category = "RTS AI | Settings")
	// int32 AttackSquadSizeTarget = 40; // Сколько солдат копить в штурмовом отряде
	//
	// UPROPERTY(EditDefaultsOnly, Category = "RTS AI | Settings")
	// int32 DefenseSquadSizeTarget = 20; // Желаемый размер отряда защиты на фронтовой башне

	/** Активные отряды ИИ, находящиеся под управлением этого Генерала */
	UPROPERTY()
	TArray<class UAISquad*> ActiveSquads;

	/** Переиспользуемый буфер для пакетного переноса солдат между отрядами (без аллокаций памяти) */
	UPROPERTY()
	TArray<class ATroopBase*> SharedTransferBuffer;

	/** Ссылки на контроллер ИИ-игрока */
	UPROPERTY()
	class AMainGamePlayerController* MyAIController;

	int32 MyFactionID = 216; // ID фракции этого бота
	
	int32 MapSize;
	
	int32 GetEnemyDangerousByHeatMap(TArray<int32> &HeatMap, int32 Index, int32 Radius, int32 MapLength) const;	

public:	
	/** Главный цикл логики войны, вызываемый директором-родителем раз в 2 секунды */
	void TickWarLogics();
	void ExecuteSmartAttack(class ATowerBase* TargetTower, int32 TargetSquadSize);

	/** 
	 * Сверхбыстрый событийный обработчик кризиса. 
	 * Срабатывает мгновенно в ту же микросекунду, когда отряд несет тяжелые потери!
	 */
	UFUNCTION()
	void HandleSquadCrisis(class UAISquad* ReportingSquad);

	/** Метод для регистрации новоприбывших из портала солдат во временный буфер-отряд */
	void RegisterNewSpawnedTroops(const TArray<class ATroopBase*>& FreshTroops);

	/** Вспомогательный метод для создания нового отряда, настройки его параметров и подписки на его ивенты */
	class UAISquad* CreateNewSquad(EAISquadRole InitialRole, FVector TargetLocation);

	/** Метод безопасного переливания солдат из отряда-донора в отряд-приемник */
	void TransferTroopsBetweenSquads(class UAISquad* FromSquad, class UAISquad* ToSquad, int32 Count);

	/** Быстрая оценка графа Делоне для поиска фронтовых башен */
	bool IsTowerOnFrontline(class ATowerBase* Tower);
	
	bool IsEnemyTowerUnderMyAttack(class ATowerBase* Tower);
	
	
	
};

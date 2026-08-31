// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#pragma once

#include "CoreMinimal.h"
#include "AISquad.generated.h"

// Роли/Задачи, которые Генерал может поставить отряду
UENUM(BlueprintType)
enum class EAISquadRole : uint8
{
	PortalSquad,	  //Живет на портале и принимает в себя местный спавн
	IdleBuffer,       // Временный отряд накопления сил при портале
	TowerDefense,     // Оборона конкретной башни Делоне
	TowerAssault,     // Штурм вражеской/нейтральной башни
	TacticalRetreat   // Организованный отход на перегруппировку
};

// Объявляем динамический делегат для рапортов Генералу
// Передаем указатель на сам отряд, чтобы Генерал знал, кто именно просит о помощи
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSquadCrisisReport, class UAISquad*, ReportingSquad);

UCLASS(BlueprintType)
class VCELLSWAR_API UAISquad : public UObject
{
	GENERATED_BODY()

public:
	UAISquad();

	/** Инициализация отряда Генералом */
	void InitSquad(int32 InFactionID, EAISquadRole InInitialRole, FVector InTargetLocation);

	/** Событие критического рапорта (Генерал подпишется на него при создании отряда) */
	UPROPERTY(BlueprintAssignable, Category = "RTS AI | Squad | Events")
	FOnSquadCrisisReport OnSquadCrisisReport;

	/** Плотный массив жестких указателей на солдат отряда */
	UPROPERTY(VisibleAnywhere, Category = "RTS AI | Squad")
	TArray<class ATroopBase*> SquadMembers;
	
private:
	// Текущие тактические данные отряда
	EAISquadRole CurrentRole = EAISquadRole::IdleBuffer;
	FVector CurrentTargetLocation = FVector::ZeroVector;
	int32 FactionID = 0;

	// Пороги паники для полуглупой логики рапортов
	int32 InitialSquadSize = 0;
	float PanicThresholdPercent = 0.4f; // Рапортовать, если потеряно более 60% состава (осталось < 40%)
	
	
	
	void UnsubscribeFromCurrentTower();

public:
	UPROPERTY()
	TWeakObjectPtr<class ATowerBase> TargetTowerRef;
	
	/** 
	 * Вызывается из EndPlay солдата за O(1).
	 * Вычеркивает погибшего из массива методом Swap-and-Pop и проверяет лимиты паники.
	 */
	void Server_NotifyMemberDeath(int32 DeadIndex);

	/** Добавление солдата в отряд */
	void AddMember(class ATroopBase* NewTroop);

	/** Очистка отряда при расформировании */
	void ClearSquad();

	// Геттеры и Сеттеры для управления Генералом
	FORCEINLINE EAISquadRole GetSquadRole() const { return CurrentRole; }
	void SetSquadRole(EAISquadRole NewRole) { CurrentRole = NewRole; }

	FORCEINLINE FVector GetTargetLocation() const { return CurrentTargetLocation; }
	void SetTargetLocation(FVector NewLocation) { CurrentTargetLocation = NewLocation; }

	FORCEINLINE int32 GetSquadSize() const { return SquadMembers.Num(); }
	
	/**
	 * Пакетное добавление группы солдат (подкрепление).
	 * Перенастраивает "паспорта" всех вошедших бойцов.
	 */
	void AddMembersBatch(const TArray<class ATroopBase*>& NewTroops);

	/**
	 * Изъятие пачки солдат с хвоста массива для отправки в другой отряд.
	 * @param CountToExtract - Сколько солдат нужно забрать
	 * @param OutExtractedTroops - Массив, куда запишутся указатели на изъятых бойцов
	 */
	void ExtractMembersBatch(int32 CountToExtract, TArray<class ATroopBase*>& OutExtractedTroops);
	
	UFUNCTION()
	void OnTargetTowerFactionChanged(class ATowerBase* Tower, int32 NewFactionID);

	void SetTargetTower(ATowerBase* NewTower);

};

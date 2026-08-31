// Copyright (c) 2026, Dmitry Tur. All rights reserved.


#include "AIGeneralDirector.h"
#include "AIWarComponent.h" 

AAIGeneralDirector::AAIGeneralDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	bAlwaysRelevant = false;
	bNetLoadOnClient = false;

	// Инициализируем военный компонент. Теперь он жестко привязан к директору
	WarComponent = CreateDefaultSubobject<UAIWarComponent>(TEXT("AIWarComponent"));
}

void AAIGeneralDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		Destroy();
		return;
	}

	// Запускаем единый таймер на 2 секунды, который будет дергать логику военного компонента
	GetWorldTimerManager().SetTimer(MacroLogicTimerHandle, this, &AAIGeneralDirector::Server_AnalyzeBattlefield, 2.0f, true);
}

void AAIGeneralDirector::Server_AnalyzeBattlefield()
{
	if (WarComponent)
	{
		// Передаем управление военному модулю на этом такте
		WarComponent->TickWarLogics();
	}
}

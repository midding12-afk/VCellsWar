// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#include "LobbyPlayerDataObj.h"

#include "VCellsWar/GameMods/LobbyPlayerState.h"


ULobbyPlayerDataObj::ULobbyPlayerDataObj()
{
	// Инициализируем указатель безопасным нулевым значением
	LobbyPlayerStateRef = nullptr;
}

#pragma once

#include "../../common/game/gameSystemOrder.h"

// This enum defines known common game systems FOR R6 PROJECT.
// It allows to get a reference to a specific subsystem and DEFINES TICK ORDER.
// All of these systems will be processed after systems defined in commonGame project.
// Do not ever define explicit values in it, but change the order of the enum if you want to achieve some specific tick order
// All the systems must be created by the Init() method in CR6Game.
enum ER6GameSystems
{
	GSR6_DialogDisplay	 = GS_MAX	,
	GSR6_AI,
	GSR6_MAX
};

#define ASSIGN_R6_GAME_SYSTEM_ID( system ) public: static ER6GameSystems GetSystemId() { return system; }

#define DECLARE_R6_GAME_SYSTEM( className, systemId ) DECLARE_ENGINE_CLASS( className, IGameSystem, 0 ); ASSIGN_R6_GAME_SYSTEM_ID( systemId )

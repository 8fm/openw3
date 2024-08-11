/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/gameSystemOrder.h"

// This enum defines known common game systems FOR R4 PROJECT.
// It allows to get a reference to a specific subsystem and DEFINES TICK ORDER.
// All of these systems will be processed after systems defined in commonGame project.
// Do not ever define explicit values in it, but change the order of the enum if you want to achieve some specific tick order
// All the systems must be created by the Init() method in CR4Game.
enum ER4GameSystems
{
	GSR4_MapPinManager = GS_MAX,
	GSR4_FocusMode,
	GSR4_SurfacePost,
	GSR4_WorldMap,
	GSR4_CommonMapManager,
	GSR4_LootManager,
	GSR4_CityLightManager,
	GSR4_TutorialSystem,
	GSR4_MAX
};

#define ASSING_R4_GAME_SYSTEM_ID( system ) public: static ER4GameSystems GetSystemId() { return system; }

#define DECLARE_R4_GAME_SYSTEM( className, systemId ) DECLARE_ENGINE_CLASS( className, IGameSystem, 0 ); ASSING_R4_GAME_SYSTEM_ID( systemId )
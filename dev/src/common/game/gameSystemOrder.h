/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// This enum defines known common game systems.
// It allows to get a reference to a specific subsystem and DEFINES TICK ORDER.
// Do not ever define explicit values in it, but change the order of the enum if you want to achieve some specific tick order
// All the systems must be created by the Init() method in CCommonGame.
enum EGameSystems
{
	GS_InteractionManager,
	GS_FactsDB,
	GS_QuestsSystem,
	GS_StoryScene,	
	GS_Community,
	GS_Attitudes,
	GS_Journal,
	GS_InteractiveDialog,
	GS_CrowdManager,
	GS_VolumePathManager,
	GS_EntitiesDetector,
	GS_Telemetry,
	GS_OcclusionSystem,
	GS_SecondScreen,
	GS_Gwint,
	GS_RoadsManager,
	GS_FastForward,
	GS_StrayActorsManager,
	GS_MAX
};

#define ASSIGN_GAME_SYSTEM_ID( system ) public: static EGameSystems GetSystemId() { return system; }

#define DECLARE_GAME_SYSTEM( className, systemId ) DECLARE_ENGINE_CLASS( className, IGameSystem, 0 ); ASSIGN_GAME_SYSTEM_ID( systemId )

/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerFlight.h"

#include "behTreeNodeDynamicFlightIdle.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerIdleFlightAI )


///////////////////////////////////////////////////////////////////////////////
//	___________.__  .__       .__     __   
//	\_   _____/|  | |__| ____ |  |___/  |_ 
//	 |    __)  |  | |  |/ ___\|  |  \   __\
//	 |     \   |  |_|  / /_/  >   Y  \  |  
//	 \___  /   |____/__\___  /|___|  /__|  
//	     \/           /_____/      \/      
//	
///////////////////////////////////////////////////////////////////////////////


CName CSpawnTreeInitializerIdleFlightAI::GetDynamicNodeEventName() const
{
	return CBehTreeNodeFlightIdleDynamicRootDefinition::StaticEventName();
}

String CSpawnTreeInitializerIdleFlightAI::GetEditorFriendlyName() const
{
	return TXT("FreeFlight");
}
/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "../engine/debugServerManager.h"
#include "gameDebuggerPlugin.h"
#include "gameDebuggerCommands.h"


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// init
Bool CGameDebuggerPlugin::Init()
{
	// add game commands
	DBGSRV_REG_COMMAND( this, "getworld", CDebugServerCommandGetWorld );
	DBGSRV_REG_COMMAND( this, "getcamera", CDebugServerCommandGetCamera );
	DBGSRV_REG_COMMAND( this, "getlayers", CDebugServerCommandGetLayers );
	DBGSRV_REG_COMMAND( this, "getinventory", CDebugServerCommandGetInventory );
	DBGSRV_REG_COMMAND( this, "getcharactercontroller", CDebugServerCommandGetCharacterController );
	DBGSRV_REG_COMMAND( this, "getwaypoint", CDebugServerCommandGetWaypoint );
	DBGSRV_REG_COMMAND( this, "getwaypoints", CDebugServerCommandGetWaypoints );				// deprecated
	DBGSRV_REG_COMMAND( this, "getwaypointsList", CDebugServerCommandGetWaypointsList );
	DBGSRV_REG_COMMAND( this, "getstubs", CDebugServerCommandGetStubs );
	DBGSRV_REG_COMMAND( this, "getactionpoint", CDebugServerCommandGetActionPoint );
	DBGSRV_REG_COMMAND( this, "getactionpoints", CDebugServerCommandGetActionPoints );			// deprecated
	DBGSRV_REG_COMMAND( this, "getactionpointsList", CDebugServerCommandGetActionPointsList );
	DBGSRV_REG_COMMAND( this, "getencounter", CDebugServerCommandGetEncounter );
	DBGSRV_REG_COMMAND( this, "getencounters", CDebugServerCommandGetEncounters );
	DBGSRV_REG_COMMAND( this, "getdoors", CDebugServerCommandGetDoors );
	DBGSRV_REG_COMMAND( this, "getarea", CDebugServerCommandGetArea );

	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// cleaning up
Bool CGameDebuggerPlugin::ShutDown()
{
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// when game is started
void CGameDebuggerPlugin::GameStarted()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when game is stopped
void CGameDebuggerPlugin::GameStopped()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when world was loaded
void CGameDebuggerPlugin::AttachToWorld()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when world was unloaded
void CGameDebuggerPlugin::DetachFromWorld()
{
}
//////////////////////////////////////////////////////////////////////////
//
// on every game tick
void CGameDebuggerPlugin::Tick()
{
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#include "build.h"


#ifndef NO_DEBUG_SERVER

//////////////////////////////////////////////////////////////////////////
// headers
#include "debugServerPlugin.h"
#include "debugServerInternalPlugin.h"
#include "debugServerInternalCommands.h"
#include "debugServerManager.h"


//////////////////////////////////////////////////////////////////////////
// implementations

//////////////////////////////////////////////////////////////////////////
//
// init
Bool CDebugServerInternalPlugin::Init()
{
	// assign internal command handlers
	DBGSRV_REG_COMMAND( this, "areWeConnected?", CDebugServerCommandAreWeConnected );
	DBGSRV_REG_COMMAND( this, "disconnect", CDebugServerCommandDisconnect );
	DBGSRV_REG_COMMAND( this, "enableFrameTime", CDebugServerCommandEnableFrameTime );

	// remote console
	DBGSRV_REG_COMMAND( this, "consoleExec", CDebugServerCommandConsoleExec );

	// remote properties tracer
	DBGSRV_REG_COMMAND( this, "startPropsTrace", CDebugServerCommandStartPropsTrace );
	DBGSRV_REG_COMMAND( this, "finishPropsTrace", CDebugServerCommandFinishPropsTrace );

	// remote profiler - TODO: move to new PROFILER plugin
	DBGSRV_REG_COMMAND( this, "initProfile", CDebugServerCommandInitProfile );
	DBGSRV_REG_COMMAND( this, "startProfile", CDebugServerCommandStartProfile );
	DBGSRV_REG_COMMAND( this, "stopProfile", CDebugServerCommandStopProfile );
	DBGSRV_REG_COMMAND( this, "storeProfile", CDebugServerCommandStoreProfile );
	DBGSRV_REG_COMMAND( this, "sendProfile", CDebugServerCommandSendProfile );
	DBGSRV_REG_COMMAND( this, "statusProfile", CDebugServerCommandStatusProfile );
	DBGSRV_REG_COMMAND( this, "toggleScriptsProfile", CDebugServerCommandToggleScriptsProfile );

	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// cleaning up
Bool CDebugServerInternalPlugin::ShutDown()
{
	return true;
}
//////////////////////////////////////////////////////////////////////////
//
// when game is started
void CDebugServerInternalPlugin::GameStarted()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when game is stopped
void CDebugServerInternalPlugin::GameStopped()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when world was loaded
void CDebugServerInternalPlugin::AttachToWorld()
{
}
//////////////////////////////////////////////////////////////////////////
//
// when world was unloaded
void CDebugServerInternalPlugin::DetachFromWorld()
{
}
//////////////////////////////////////////////////////////////////////////
//
// on every game tick
void CDebugServerInternalPlugin::Tick()
{
}

#endif


//////////////////////////////////////////////////////////////////////////
// EOF
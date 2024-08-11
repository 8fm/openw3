/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#pragma once


//////////////////////////////////////////////////////////////////////////
// commands

//////////////////////////////////////////////////////////////////////////
//
// areWeConnected?
class CDebugServerCommandAreWeConnected
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// disconnect
class CDebugServerCommandDisconnect
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// enable frame time
class CDebugServerCommandEnableFrameTime
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// consoleExec
class CDebugServerCommandConsoleExec
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// startPropsTrace
class CDebugServerCommandStartPropsTrace
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// finishPropsTrace
class CDebugServerCommandFinishPropsTrace
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// initProfile
class CDebugServerCommandInitProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// startProfile
class CDebugServerCommandStartProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// stopProfile
class CDebugServerCommandStopProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// storeProfile
class CDebugServerCommandStoreProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// sendProfile
class CDebugServerCommandSendProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// statusProfile
class CDebugServerCommandStatusProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// toggleScriptsProfile
class CDebugServerCommandToggleScriptsProfile
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};


//////////////////////////////////////////////////////////////////////////
// EOF

/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#pragma once


//////////////////////////////////////////////////////////////////////////
// commands

//////////////////////////////////////////////////////////////////////////
//
// getworld
class CDebugServerCommandGetWorld
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = false;
};
//////////////////////////////////////////////////////////////////////////
//
// getcamera
class CDebugServerCommandGetCamera
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getlayers
class CDebugServerCommandGetLayers
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getinventory
class CDebugServerCommandGetInventory
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getwaypoint - get selected WP data
class CDebugServerCommandGetWaypoint
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getwaypoints - deprecated
class CDebugServerCommandGetWaypoints
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getwaypointsList - get all WP's stacked in packets
class CDebugServerCommandGetWaypointsList
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getstubs
class CDebugServerCommandGetStubs
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getactionpoint - get selected AP data
class CDebugServerCommandGetActionPoint
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getactionpoints - deprecated
class CDebugServerCommandGetActionPoints
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getactionpointsList - get all AP's stacked in packets
class CDebugServerCommandGetActionPointsList
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getencounters - get all encounters - one-by-one
class CDebugServerCommandGetEncounters
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getencounter - get selected encounter data
class CDebugServerCommandGetEncounter
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getcharactercontroller - get character controller data
class CDebugServerCommandGetCharacterController
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getdoors
class CDebugServerCommandGetDoors
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getarea
class CDebugServerCommandGetArea
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};


//////////////////////////////////////////////////////////////////////////
// EOF

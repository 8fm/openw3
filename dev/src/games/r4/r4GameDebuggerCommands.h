/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#pragma once


//////////////////////////////////////////////////////////////////////////
// commands

//////////////////////////////////////////////////////////////////////////
//
// getplayer
class CDebugServerCommandGetPlayer
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};
//////////////////////////////////////////////////////////////////////////
//
// getnpcs
class CDebugServerCommandGetNPCs
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};


//////////////////////////////////////////////////////////////////////////
// EOF
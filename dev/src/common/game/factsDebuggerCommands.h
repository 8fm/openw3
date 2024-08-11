/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CDebugServerPlugin;
class CDebugServerCommandGetFactIDs
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

class CDebugServerCommandGetFactData
{
public:
	static Uint32 ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data );
	static const Bool gameThread = true;
};

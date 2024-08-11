/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#pragma once


//////////////////////////////////////////////////////////////////////////
// declarations
class CDebugServerPlugin
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

public:
	CDebugServerPlugin() {}
	virtual ~CDebugServerPlugin() {}

public:
	// common
	virtual Bool Init() = 0;
	virtual Bool ShutDown() = 0;

	// life-time
	virtual void GameStarted() = 0;
	virtual void GameStopped() = 0;
	virtual void AttachToWorld() = 0;
	virtual void DetachFromWorld() = 0;
	virtual void Tick() = 0;
};


//////////////////////////////////////////////////////////////////////////
// EOF
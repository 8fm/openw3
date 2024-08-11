/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// World tick info
class CWorldTickInfo
{
public:
	CWorld*				m_world;			// World to tick
	Float				m_timeDelta;		// Time delta to advance the world with
	Bool				m_updatePhysics;	// True if we should update physics
	Bool				m_fromTickThread;	// True if Tick() is called from tick thread
	Bool				m_updateWater;
	Bool				m_updateFoliage;
	Bool				m_updateTerrain;
	Bool				m_updateGame;
	Bool				m_updateTriggers;
	Bool				m_updateCameraDirector;
	Bool				m_updateEffects;
	Bool				m_updatePathLib;
	Bool				m_updateScripts;

	EWorldUpdatePhase	m_phase;

public:
	RED_INLINE CWorldTickInfo( CWorld* world, Float timeDelta )
		: m_world( world )
		, m_timeDelta( timeDelta )
		, m_updatePhysics( false )
		, m_fromTickThread( false )
		, m_updateWater( true )
		, m_updateFoliage( true )
		, m_updateTerrain( true )
		, m_updateGame( true )
		, m_updateTriggers( true )
		, m_updateCameraDirector( true )
		, m_updateEffects( true )
		, m_updatePathLib( true )
		, m_updateScripts( true )
		, m_phase( WUP_Tick )
	{
		ASSERT( world );
	};
};

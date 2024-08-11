/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "entity.h"

class CSoundAmbientEmitter : public CEntity
{
	DECLARE_ENGINE_CLASS( CSoundAmbientEmitter, CEntity, 0 );

public:
	CSoundAmbientEmitter();
	virtual ~CSoundAmbientEmitter() {}

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

protected:
	StringAnsi				m_soundEvents;		//!< Event to invoke to play the sound
	Float					m_maxDistance;
	Bool					m_occlusionEnabled;
};

BEGIN_CLASS_RTTI( CSoundAmbientEmitter )
	PARENT_CLASS( CEntity )
	PROPERTY( m_soundEvents )
	PROPERTY( m_maxDistance )
	PROPERTY( m_occlusionEnabled )
END_CLASS_RTTI()

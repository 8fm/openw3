/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"

/// Initialize texture flip
class CParticleInitializerRandomFlip : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerRandomFlip, IParticleInitializer, 0 );

protected:
	Bool	m_randomFlipU;		//!< Randomize texture flip in U direction
	Bool	m_randomFlipV;		//!< Randomize texture flip in V direction

public:
	CParticleInitializerRandomFlip()
		: m_randomFlipU( true )
		, m_randomFlipV( true )
	{
		m_editorName = TXT("Initial random flip");
		m_editorGroup = TXT("Material");
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerRandomFlip );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_EDIT( m_randomFlipU, TXT("Enable random flip in U direction") );
	PROPERTY_EDIT( m_randomFlipV, TXT("Enable random flip in V direction") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerRandomFlip );
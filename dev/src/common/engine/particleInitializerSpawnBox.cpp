/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectDummyPoint.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Initialize spawn position of a particle inside a box
class CParticleInitializerSpawnBox : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerSpawnBox, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*	m_extents;			//!< Extents of the spawn box
	Bool				m_worldSpace;		//!< Spawn in the world space

public:
	CParticleInitializerSpawnBox()
		: m_worldSpace( true )
	{
		m_editorName = TXT("Spawn box");
		m_editorGroup = TXT("Location");
		m_extents = new CEvaluatorVectorConst( this, Vector( 0.1f, 0.1f, 0.1f, 1.0f ) );
		m_requiredParticleField = 0xffffffff;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		if ( m_extents )
		{
			data.m_spawn_BoxCircle_WorldSpace = m_worldSpace;
			// Fill direction samples
			TDynArray< Vector > samples;
			m_extents->GetApproximationSamples( samples );
			data.m_spawnBoxExtents.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_extents )
		{
			return 0;
		}
		return (Uint32)PIF_SpawnBox;
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerSpawnBox );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_extents, TXT("Extents of the spawn box") );
	PROPERTY_EDIT( m_worldSpace, TXT("Spawn area is defined in world space") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerSpawnBox );
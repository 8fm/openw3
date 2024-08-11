/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "particleInitializer.h"

/// Initialize alpha of a particle
class CParticleInitializerCollisionSpawn : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerCollisionSpawn, IParticleInitializer, 0 );

protected:
	Float	m_probability;
	CName	m_parentEmitterName;

public:
	CParticleInitializerCollisionSpawn()
		: m_probability( 1.0f )
		, m_parentEmitterName( RED_NAME( None ) )

	{
		m_editorName = TXT("Collision Spawn");
		m_editorGroup = TXT("Physics");
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const;

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const;
};

BEGIN_CLASS_RTTI( CParticleInitializerCollisionSpawn );
PARENT_CLASS( IParticleInitializer );
	PROPERTY_EDIT( m_probability, TXT("Spawn probability") );
	PROPERTY_EDIT( m_parentEmitterName, TXT("Spawn particle") );
END_CLASS_RTTI();

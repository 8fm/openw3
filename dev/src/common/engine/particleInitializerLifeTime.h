/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "particleInitializer.h"
#include "evaluatorFloat.h"

/// Initialize life time of a particle
class CParticleInitializerLifeTime : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerLifeTime, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_lifeTime;			//!< Life time of spawned particles

public:
	CParticleInitializerLifeTime()
	{
		m_editorName = TXT("Life time");
		m_editorGroup = TXT("Default");
		m_lifeTime = new CEvaluatorFloatConst( this, 1.0f );
		m_requiredParticleField = 0xffffffff;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const;

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const;
};

BEGIN_CLASS_RTTI( CParticleInitializerLifeTime );
PARENT_CLASS( IParticleInitializer );
PROPERTY_INLINED( m_lifeTime, TXT("Life time of the spawned particle") );
END_CLASS_RTTI();
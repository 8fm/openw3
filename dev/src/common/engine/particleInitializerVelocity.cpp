/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectDummyPoint.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Initialize velocity of a particle
class CParticleInitializerVelocity : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerVelocity, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*		m_velocity;			//!< Initial velocity of spawned particles
	Bool					m_worldSpace;		//!< Use world space velocity

public:
	CParticleInitializerVelocity()
		: m_worldSpace( true )
	{
		m_editorName = TXT("Initial velocity");
		m_editorGroup = TXT("Velocity");
		m_velocity = new CEvaluatorVectorConst( this, Vector::EZ );
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		if ( m_velocity )
		{
			data.m_velocityInitWorldSpace = m_worldSpace;
			// Fill direction samples
			TDynArray< Vector > samples;
			m_velocity->GetApproximationSamples( samples );
			data.m_velocityInit.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_velocity )
		{
			return 0;
		}
		if ( m_velocity->IsA< CEvaluatorVectorRandomUniform >() )
		{
			return (Uint32)PIF_VelocityRandom;
		}
		else
		{
			return (Uint32)PIF_Velocity;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerVelocity );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_velocity, TXT("Initial velocity of the particle") );
	PROPERTY_EDIT( m_worldSpace, TXT("Added velocity is in world space") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerVelocity );
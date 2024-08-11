/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Inherit base velocity for particles
class CParticleInitializerVelocityInherit : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerVelocityInherit, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_scale;			//!< Scale of velocity inheritance

public:
	CParticleInitializerVelocityInherit()
	{
		m_editorName = TXT("Inherit velocity");
		m_editorGroup = TXT("Velocity");
		m_scale = new CEvaluatorFloatConst( this, 1.0f );
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		// Fill direction samples
		if ( m_scale )
		{
			m_scale->GetApproximationSamples( data.m_velocityInheritInit.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_scale )
		{
			return 0;
		}
		if ( m_scale->IsA< CEvaluatorFloatRandomUniform >() )
		{
			return (Uint32)PIF_VelocityInheritRandom;
		}
		else
		{
			return (Uint32)PIF_VelocityInherit;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerVelocityInherit );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_scale, TXT("Scale of velocity inheritance") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerVelocityInherit );
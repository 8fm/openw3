/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Spread initial velocity of a particle
class CParticleInitializerVelocitySpread : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerVelocitySpread, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*		m_scale;				//!< Spread scale
	Bool					m_conserveMomentum;		//!< Conserve particle momentum

public:
	CParticleInitializerVelocitySpread()
		: m_conserveMomentum( true )
	{
		m_editorName = TXT("Spread velocity");
		m_editorGroup = TXT("Velocity");
		m_scale = new CEvaluatorFloatConst( this, 0.1f );
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		if ( m_scale )
		{
			data.m_conserveVelocitySpreadMomentum = m_conserveMomentum;
			// Fill direction samples
			m_scale->GetApproximationSamples( data.m_velocitySpreadInit.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_scale )
		{
			return 0;
		}
		return (Uint32)PIF_VelocitySpread;
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerVelocitySpread );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_scale, TXT("Spread scale") );
	PROPERTY_EDIT( m_conserveMomentum, TXT("Conserve particle momentum") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerVelocitySpread );
/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"
#include "evaluatorVector.h"

/// Modify velocity of a particle over the particle lifetime
class CParticleModificatorVelocityTurbulize : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorVelocityTurbulize, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_scale;			//!< Turbulization scale
	Float				m_noiseInterval;	//!< Frequency of turbulizations
	Float				m_duration;			//!< Time to full turbulence application
	IEvaluatorFloat*	m_timelifeLimit;

public:
	CParticleModificatorVelocityTurbulize()
		: m_noiseInterval( 1.0f )
		, m_duration( 1.0f )
	{
		m_editorName = TXT("Velocity turbulize");
		m_editorGroup = TXT("Velocity");
		m_scale = new CEvaluatorVectorConst( this, Vector::ZEROS );
		m_timelifeLimit = new CEvaluatorFloatConst( this, 1.0f );

		m_requiredParticleField = PFS_Turbulence;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		if ( m_scale )
		{
			Float duration = m_duration > 0 ? m_duration : 0.1f;

			data.m_turbulenceNoiseInterval = m_noiseInterval;
			data.m_turbulenceDuration = duration;

			TDynArray< Vector > samples;
			m_scale->GetApproximationSamples( samples );
			data.m_turbulenceScale.SetData( samples );

		}
		if ( m_timelifeLimit )
		{
			m_timelifeLimit->GetApproximationSamples( data.m_turbulenceTimeLifeLimit.m_samples );
		}
	}

	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_scale )
		{
			return 0;
		}
		return (Uint32)PMF_Turbulize;
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorVelocityTurbulize );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_scale, TXT("Velocity turbulizations strength") );
PROPERTY_INLINED( m_timelifeLimit, TXT("Velocity time life limit") );
PROPERTY_EDIT( m_noiseInterval, TXT("Velocity turbulizations interval") );
PROPERTY_EDIT( m_duration, TXT("Time for turbulence to be fully applied") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorVelocityTurbulize );
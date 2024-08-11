/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"
#include "evaluatorFloat.h"

/// Act with constant force on the particle
class CParticleModificatorAcceleration : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorAcceleration, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_direction;		//!< Acceleration direction
	IEvaluatorFloat*	m_scale;			//!< Acceleration strength
	Bool				m_worldSpace;		//!< Acceleration is given in the world space

public:
	CParticleModificatorAcceleration()
		: m_worldSpace( true )
	{
		m_editorName = TXT("Acceleration");
		m_editorGroup = TXT("Velocity");
		m_scale = new CEvaluatorFloatConst( this, 1.0f );
		m_direction = new CEvaluatorVectorConst( this, -Vector::EZ );
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_direction )
		{
			TDynArray< Vector > dirSamples;
			m_direction->GetApproximationSamples( dirSamples );
			data.m_accelerationDirEval.SetData( dirSamples );
		}
		
		// Fill scale samples
		if ( m_scale )
		{
			m_scale->GetApproximationSamples( data.m_accelerationScaleEval.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const
	{
		if ( !m_direction || !m_scale )
		{
			return 0;
		}
		if ( m_worldSpace )
		{
			return (Uint32)PMF_AccelerationWorld;
		}
		else
		{
			return (Uint32)PMF_AccelerationLocal;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorAcceleration );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_direction, TXT("Acceleration direction") );
PROPERTY_INLINED( m_scale, TXT("Acceleration strength") );
PROPERTY_EDIT( m_worldSpace, TXT("If true the value is modulated with the base alpha") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorAcceleration );
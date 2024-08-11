/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Modify rotation rate of a particle over the particle lifetime
class CParticleModificatorRotationRateOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorRotationRateOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorFloat*	m_rotationRate;		//!< Rotation rate over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base alpha

public:
	CParticleModificatorRotationRateOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("Rotation rate over life");
		m_editorGroup = TXT("Rotation");
		m_rotationRate = new CEvaluatorFloatConst( this, 0.0f );
		m_requiredParticleField = PFS_Rotation2D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_rotationRate )
		{
			m_rotationRate->GetApproximationSamples( data.m_RotRateEval.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_rotationRate )
		{
			return 0;
		}
		if ( m_modulate )
		{
			return (Uint32)PMF_1ARotRateOverLifeModulate;
		}
		else
		{
			return (Uint32)PMF_1ARotRateOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorRotationRateOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_rotationRate, TXT("Rotation rate over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base rotation rate") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorRotationRateOverLife );
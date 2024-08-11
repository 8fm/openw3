/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Modify velocity of a particle over the particle lifetime
class CParticleModificatorVelocityOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorVelocityOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_velocity;			//!< Velocity over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base alpha
	Bool				m_absolute;			//!<

public:
	CParticleModificatorVelocityOverLife()
		: m_modulate( true )
		, m_absolute( false )
	{
		m_editorName = TXT("Velocity over life");
		m_editorGroup = TXT("Velocity");
		m_velocity = new CEvaluatorVectorConst( this, Vector::ONES );
		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill size samples
		if ( m_velocity )
		{
			TDynArray< Vector > velocitySamples;

			m_velocity->GetApproximationSamples( velocitySamples );
			data.m_velocityEval.SetData( velocitySamples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const
	{
		if ( !m_velocity )
		{
			return 0;
		}
		if ( m_modulate )
		{
			return (Uint32)PMF_VelocityOverLifeModulate;
		}
		else
		{
			return (Uint32)PMF_VelocityOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorVelocityOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_velocity, TXT("Velocity over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base size") );
PROPERTY_EDIT( m_absolute, TXT("If true the value is modulated with the base size") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorVelocityOverLife );
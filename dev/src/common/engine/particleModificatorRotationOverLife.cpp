/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Modify rotation rate of a particle over the particle lifetime
class CParticleModificatorRotationOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorRotationOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorFloat*	m_rotation;			//!< Rotation over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base alpha

public:
	CParticleModificatorRotationOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("Rotation over life");
		m_editorGroup = TXT("Rotation");
		m_rotation = new CEvaluatorFloatConst( this, 0.0f );
		m_requiredParticleField = PFS_Rotation2D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_rotation )
		{
			m_rotation->GetApproximationSamples( data.m_RotEval.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_rotation )
		{
			return 0;
		}
		if ( m_modulate )
		{
			return (Uint32)PMF_1ARotationOverLifeModulate;
		}
		else
		{
			return (Uint32)PMF_1ARotationOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorRotationOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_rotation, TXT("Rotation rate over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base rotation") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorRotationOverLife );
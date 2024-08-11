/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"


/// Modify rotation rate of a particle over the particle lifetime
class CParticleModificatorRotationRate3DOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorRotationRate3DOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_rotationRate;		//!< Rotation rate over particle's life
	//Bool				m_modulate;			//!< If true the value is modulated with the base rotation rate

public:
	CParticleModificatorRotationRate3DOverLife()
		//	: m_modulate( true )
	{
		m_editorName = TXT("3D rotation rate over life");
		m_editorGroup = TXT("Rotation");
		m_rotationRate = new CEvaluatorVectorConst( this, Vector::ZEROS );
		m_requiredParticleField = PFS_Rotation3D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_rotationRate )
		{
			TDynArray< Vector > samples;
			m_rotationRate->GetApproximationSamples( samples );
			data.m_rotRateEval3D.SetData( samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_rotationRate )
		{
			return 0;
		}
		return (Uint32)PMF_3ARotRateOverLifeAbsolute;
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorRotationRate3DOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_rotationRate, TXT("Rotation rate over particle's life") );
//PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base rotation rate") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorRotationRate3DOverLife );
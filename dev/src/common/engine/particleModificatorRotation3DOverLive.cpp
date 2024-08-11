/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Modify rotation rate of a particle over the particle lifetime
class CParticleModificatorRotation3DOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorRotation3DOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_rotation;			//!< 3 axis rotation over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base rotation

public:
	CParticleModificatorRotation3DOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("3D rotation over life");
		m_editorGroup = TXT("Rotation");
		m_rotation = new CEvaluatorVectorConst( this, Vector::ZEROS );
		m_requiredParticleField = PFS_Rotation3D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_rotation )
		{
			TDynArray< Vector > samples;
			m_rotation->GetApproximationSamples( samples );
			data.m_rotEval3D.SetData( samples );
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
			return (Uint32)PMF_3ARotationOverLifeModulate;
		}
		else
		{
			return (Uint32)PMF_3ARotationOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorRotation3DOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_rotation, TXT("Rotation rate over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base rotation") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorRotation3DOverLife );
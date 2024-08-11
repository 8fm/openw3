/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Initialize rotation rate of a particle
class CParticleInitializerRotationRate : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerRotationRate, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_rotationRate;			//!< Rotation of spawned particles

public:
	CParticleInitializerRotationRate()
	{
		m_editorName = TXT("Inital rotation rate");
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
			m_rotationRate->GetApproximationSamples( data.m_rotationRateInit.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_rotationRate )
		{
			return 0;
		}
		if ( m_rotationRate->IsA< CEvaluatorFloatRandomUniform >() )
		{
			return (Uint32)PIF_RotationRateRandom;
		}
		else
		{
			return (Uint32)PIF_RotationRate;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerRotationRate );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_rotationRate, TXT("Rotation of the spawned particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerRotationRate );
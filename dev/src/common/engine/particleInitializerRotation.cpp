/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "evaluatorFloat.h"
#include "particleEmitter.h"

/// Initialize rotation of a particle
class CParticleInitializerRotation : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerRotation, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_rotation;			//!< Rotation of spawned particles

public:
	CParticleInitializerRotation()
	{
		m_editorName = TXT("Inital rotation");
		m_editorGroup = TXT("Rotation");
		m_rotation = new CEvaluatorFloatConst( this, 0.0f );
		m_requiredParticleField = PFS_Rotation2D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		// Fill samples
		if ( m_rotation )
		{
			m_rotation->GetApproximationSamples( data.m_rotationInit.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_rotation )
		{
			return 0;
		}
		if ( m_rotation->IsA< CEvaluatorFloatRandomUniform >() )
		{
			return (Uint32)PIF_RotationRandom;
		}
		else
		{
			return (Uint32)PIF_Rotation;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerRotation );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_rotation, TXT("Rotation of the spawned particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerRotation );
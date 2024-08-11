/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Initialize rotation rate of a particle
class CParticleInitializerRotationRate3D : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerRotationRate3D, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*	m_rotationRate;			//!< Rotation of spawned particles

public:
	CParticleInitializerRotationRate3D()
	{
		m_editorName = TXT("Inital rotation rate 3D");
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
			data.m_rotationRate3DInit.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_rotationRate )
		{
			return 0;
		}
		if ( m_rotationRate->IsA< CEvaluatorVectorRandomUniform >() )
		{
			return (Uint32)PIF_RotationRate3DRandom;
		}
		else
		{
			return (Uint32)PIF_RotationRate3D;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerRotationRate3D );
PARENT_CLASS( IParticleInitializer );
PROPERTY_INLINED( m_rotationRate, TXT("Rotation of the spawned particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerRotationRate3D );
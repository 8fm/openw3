/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Initialize rotation of a particle
class CParticleInitializerRotation3D : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerRotation3D, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*	m_rotation;			//!< Rotation of spawned particles

public:
	CParticleInitializerRotation3D()
	{
		m_editorName = TXT("Inital rotation 3D");
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
			data.m_rotation3DInit.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_rotation )
		{
			return 0;
		}
		if ( m_rotation->IsA< CEvaluatorVectorRandomUniform >() )
		{
			return (Uint32)PIF_Rotation3DRandom;
		}
		else
		{
			return (Uint32)PIF_Rotation3D;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerRotation3D );
PARENT_CLASS( IParticleInitializer );
PROPERTY_INLINED( m_rotation, TXT("Rotation of the spawned particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerRotation3D );
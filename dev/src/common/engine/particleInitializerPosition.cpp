/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorVector.h"

/// Initialize position of a particle
class CParticleInitializerPosition : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerPosition, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*	m_position;			//!< Initial position of spawned particles
	Float				m_offset;			//!< offset on vector between particle and camera

public:
	CParticleInitializerPosition()
		: m_offset ( 0.0f )
	{
		m_editorName = TXT("Initial position");
		m_editorGroup = TXT("Location");
		m_position = new CEvaluatorVectorConst( this, Vector(0.0f, 0.0f, 0.0f, 0.0f) );
		m_requiredParticleField = 0xffffffff;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		if ( m_position )
		{
			data.m_positionOffset = m_offset;
			// Fill direction samples
			TDynArray< Vector > samples;
			m_position->GetApproximationSamples( samples );
			data.m_positionInit.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_position )
		{
			return 0;
		}
		if ( m_position->IsA< CEvaluatorVectorRandomUniform >() )
		{
			return (Uint32)PIF_PositionRandom;
		}
		else
		{
			return (Uint32)PIF_Position;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerPosition );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_EDIT( m_offset, TXT("Offset between camera and particle") );
	PROPERTY_INLINED( m_position, TXT("Initial position of the particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerPosition );
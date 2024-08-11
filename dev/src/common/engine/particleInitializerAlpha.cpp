/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Initialize alpha of a particle
class CParticleInitializerAlpha : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerAlpha, IParticleInitializer, 0 );

protected:
	IEvaluatorFloat*	m_alpha;			//!< Initial alpha of spawned particles

public:
	CParticleInitializerAlpha()
	{
		m_editorName = TXT("Initial alpha");
		m_editorGroup = TXT("Material");
		m_alpha = new CEvaluatorFloatConst( this, 1.0f );
		m_requiredParticleField = PFS_Color;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		// Fill samples
		if ( m_alpha )
		{
			m_alpha->GetApproximationSamples( data.m_alphaInit.m_samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_alpha )
		{
			return 0;
		}
		if ( m_alpha->IsA< CEvaluatorFloatRandomUniform >() )
		{
			return (Uint32)PIF_AlphaRandom;
		}
		else
		{
			return (Uint32)PIF_Alpha;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerAlpha );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_alpha, TXT("Initial alpha of the particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerAlpha );
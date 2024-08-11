/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Modify alpha of a particle over the particle lifetime
class CParticleModificatorAlphaOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorAlphaOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorFloat*	m_alpha;			//!< Alpha over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base alpha

public:
	CParticleModificatorAlphaOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("Alpha over life");
		m_editorGroup = TXT("Material");
		m_alpha = new CEvaluatorFloatConst( this, 1.0f );
		m_requiredParticleField = PFS_Color;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill scale samples
		if ( m_alpha )
		{
			m_alpha->GetApproximationSamples( data.m_alphaEval.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_alpha )
		{
			return 0;
		}
		if ( m_modulate )
		{
			return (Uint32)PMF_AlphaOverLifeModulate;
		}
		else 
		{
			return (Uint32)PMF_AlphaOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorAlphaOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_alpha, TXT("Alpha over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base alpha") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorAlphaOverLife );
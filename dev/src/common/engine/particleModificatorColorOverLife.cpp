/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorColor.h"

/// Modify color of a particle over the particle lifetime
class CParticleModificatorColorOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorColorOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorColor*	m_color;			//!< Color over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base color

public:
	CParticleModificatorColorOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("Color over life");
		m_editorGroup = TXT("Material");
		m_color = new CEvaluatorColorConst( this, Color::WHITE );
		m_requiredParticleField = PFS_Color;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_color )
		{
			TDynArray< Vector > colorSamples;
			m_color->GetApproximationSamples( colorSamples );
			data.m_colorEval.SetData( colorSamples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_color )
		{
			return 0;
		}
		if ( m_modulate )
		{
			return (Uint32)PMF_ColorOverLifeModulate;
		}
		else
		{
			return (Uint32)PMF_ColorOverLifeAbsolute;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorColorOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_color, TXT("Color over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base color") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorColorOverLife );
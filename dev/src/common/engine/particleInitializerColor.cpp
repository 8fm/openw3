/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "evaluatorColor.h"

/// Initialize color and alpha of a particle
class CParticleInitializerColor : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerColor, IParticleInitializer, 0 );

protected:
	IEvaluatorColor*	m_color;			//!< Initial color of a particle

public:
	CParticleInitializerColor()
	{
		m_editorName = TXT("Initial color");
		m_editorGroup = TXT("Material");
		m_color = new CEvaluatorColorConst( this, Color::RED );
		m_requiredParticleField = PFS_Color;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		// Fill direction samples
		if ( m_color )
		{
			TDynArray< Vector > samples;
			m_color->GetApproximationSamples( samples );
			data.m_colorInit.SetData( samples );
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_color )
		{
			return 0;
		}
		if( m_color->IsA<CEvaluatorColorRandom>() )
		{
			return (Uint32)PIF_ColorRandom;
		}
		else
		{
			return (Uint32)PIF_Color;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerColor );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_color, TXT("Initial color of the particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerColor );
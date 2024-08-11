/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Texture animation mode
enum ETextureAnimationMode
{
	TAM_Speed,
	TAM_LifeTime,
};

BEGIN_ENUM_RTTI( ETextureAnimationMode );
ENUM_OPTION( TAM_Speed );
ENUM_OPTION( TAM_LifeTime );
END_ENUM_RTTI();

IMPLEMENT_RTTI_ENUM( ETextureAnimationMode );

/// Texture animation
class CParticleModificatorTextureAnimation : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorTextureAnimation, IParticleModificator, 0 );

protected:
	IEvaluatorFloat*		m_initialFrame;			//!< Particle initial frame
	IEvaluatorFloat*		m_animationSpeed;		//!< Animation speed ( frames per second )
	ETextureAnimationMode	m_animationMode;		//!< Animation mode

public:
	CParticleModificatorTextureAnimation()
	{
		m_editorName = TXT("Texture animation");
		m_editorGroup = TXT("Material");
		m_initialFrame = new CEvaluatorFloatConst( this, 0.0f );
		m_animationSpeed = new CEvaluatorFloatConst( this, 30.0f );
		m_animationMode = TAM_Speed;
		m_requiredParticleField = PFS_Frame;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Fill direction samples
		if ( m_animationSpeed )
		{
			m_animationSpeed->GetApproximationSamples( data.m_animFrameEval.m_samples );
		}
		if ( m_initialFrame )
		{
			m_initialFrame->GetApproximationSamples( data.m_animFrameInit.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_animationSpeed )
		{
			return 0;
		}
		if ( m_animationMode == TAM_Speed )
		{
			return (Uint32)PMF_AnimationFrameBySpeed;
		}
		else
		{
			return (Uint32)PMF_AnimationFrameByLife;
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_initialFrame )
		{
			return 0;
		}
		if ( m_initialFrame->IsA< CEvaluatorFloatRandomUniform >() )
		{
			return (Uint32)PIF_AnimationFrameRandom;
		}
		else
		{
			return (Uint32)PIF_AnimationFrame;
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorTextureAnimation );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_initialFrame, TXT("Particle initial frame") );
PROPERTY_INLINED( m_animationSpeed, TXT("Animation speed ( frames per second )") );
PROPERTY_EDIT( m_animationMode, TXT("Type of particle animation") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorTextureAnimation );
/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "particleDrawer.h"

/// Modify size of a particle over the particle lifetime
class CParticleModificatorSize3DOverLife : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorSize3DOverLife, IParticleModificator, 0 );

protected:
	IEvaluatorVector*	m_size;				//!< Size over particle's life
	Bool				m_modulate;			//!< If true the value is modulated with the base alpha

public:
	CParticleModificatorSize3DOverLife()
		: m_modulate( true )
	{
		m_editorName = TXT("Size3D over life DEPRECATED");
		m_editorGroup = TXT("Deprecated");
		m_size = new CEvaluatorVectorConst( this, Vector::ONES );
		m_requiredParticleField = PFS_Size3D | PFS_Size2D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		// Due to compatibility against old particle system stuff,  split setup into two versions, one for 2d particles and one for 3d particles
		// Fill direction samples
		if ( m_size )
		{
			TDynArray< Vector > samples;
			m_size->GetApproximationSamples( samples );

			if ( particleType == PT_Mesh )
			{
				data.m_sizeEval3D.SetData( samples );
			}
			else
			{
				data.m_sizeEval.SetData( samples );
			}
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_size )
		{
			return 0;
		}
		if ( m_modulate )
		{
			if ( GetEmitter()->GetParticleType() == PT_Mesh )
			{
				return (Uint32)PMF_3DSizeOverLifeModulate;
			}
			else
			{
				return (Uint32)PMF_2DSizeOverLifeModulate;
			}
		}
		else
		{
			if ( GetEmitter()->GetParticleType() == PT_Mesh )
			{
				return (Uint32)PMF_3DSizeOverLifeAbsolute;
			}
			else
			{
				return (Uint32)PMF_2DSizeOverLifeAbsolute;
			}
		}
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorSize3DOverLife );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_size, TXT("Size over particle's life") );
PROPERTY_EDIT( m_modulate, TXT("If true the value is modulated with the base size") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorSize3DOverLife );
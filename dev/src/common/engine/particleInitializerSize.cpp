/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializer.h"
#include "particleEmitter.h"
#include "particleDrawer.h"
#include "evaluatorVector.h"

/// Initialize size of a particle
class CParticleInitializerSize : public IParticleInitializer
{
	DECLARE_ENGINE_CLASS( CParticleInitializerSize, IParticleInitializer, 0 );

protected:
	IEvaluatorVector*	m_size;

public:
	CParticleInitializerSize()
	{
		m_editorName = TXT("Initial size");
		m_editorGroup = TXT("Size");
		m_size = new CEvaluatorVectorConst( this, Vector( 0.1f, 0.1f, 0.1f ) );
		m_requiredParticleField = PFS_Size3D | PFS_Size2D;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
	{
		// Fill direction samples
		if ( m_size )
		{
			TDynArray< Vector > samples;
			m_size->GetApproximationSamples( samples );	

			data.m_sizeSpillFirstAxis = m_size->IsSpilled() && m_size->GetFreeAxes() == FVA_One;
			
			if ( particleType == PT_Mesh )
			{
				data.m_size3DInit.SetData( samples );
				
			}
			else
			{	
				data.m_sizeInit.SetData( samples );
			}
		}
	}

	//! Get mask flag of this initializer
	virtual Uint32 GetInitializerId() const
	{
		if ( !m_size )
		{
			return 0;
		}
		if ( m_size->IsA< CEvaluatorVectorRandomUniform >() )
		{
			if ( GetEmitter()->GetParticleType() == PT_Mesh )
			{
				return (Uint32)PIF_Size3DRandom;
			}
			else
			{
				return (Uint32)PIF_SizeRandom;
			}
		}
		else
		{
			if ( GetEmitter()->GetParticleType() == PT_Mesh )
			{
				return (Uint32)PIF_Size3D;
			}
			else
			{
				return (Uint32)PIF_Size;
			}
		}
	}
};

BEGIN_CLASS_RTTI( CParticleInitializerSize );
	PARENT_CLASS( IParticleInitializer );
	PROPERTY_INLINED( m_size, TXT("Initial size of the particle") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleInitializerSize );
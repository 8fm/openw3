/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleModificator.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

/// Accelerate particles toward a target
class CParticleModificatorTargetNode : public IParticleModificator
{
	DECLARE_ENGINE_CLASS( CParticleModificatorTargetNode, IParticleModificator, 0 );

protected:
	IEvaluatorFloat*	m_forceScale;		//!< Acceleration strength
	IEvaluatorFloat*	m_killRadius;		//!< Kill radius
	Float				m_maxForce;			//!< Maximum force value

public:
	CParticleModificatorTargetNode()
		: m_maxForce( 100.0f )
	{
		m_editorName = TXT("Target node");
		m_editorGroup = TXT("Velocity");
		m_forceScale = new CEvaluatorFloatConst( this, 1.0f );
		m_killRadius = new CEvaluatorFloatConst( this, 0.0f );

		m_requiredParticleField = PFS_Velocity;
	}

	//! Set particle updater data based on this modifier's evaluators
	virtual void SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const
	{
		data.m_targetMaxForce = m_maxForce;
		
		if ( m_forceScale )
		{
			m_forceScale->GetApproximationSamples( data.m_targetForceScale.m_samples );
		}
		if ( m_killRadius ) 
		{
			m_killRadius->GetApproximationSamples( data.m_targetKillRadius.m_samples );
		}
	}

	//! Get mask flag of this modifier
	virtual Uint32 GetModificatorId() const 
	{
		if ( !m_forceScale || !m_killRadius )
		{
			return 0;
		}
		return (Uint32)PMF_TargetNode;
	}
};

BEGIN_CLASS_RTTI( CParticleModificatorTargetNode );
PARENT_CLASS( IParticleModificator );
PROPERTY_INLINED( m_forceScale, TXT("Acceleration strength") );
PROPERTY_INLINED( m_killRadius, TXT("Kill radius") );
PROPERTY_EDIT( m_maxForce, TXT("Maximum force to apply to particles") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CParticleModificatorTargetNode );
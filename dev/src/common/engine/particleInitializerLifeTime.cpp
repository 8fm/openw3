/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleInitializerLifeTime.h"
#include "particleEmitter.h"
#include "evaluatorFloat.h"

IMPLEMENT_ENGINE_CLASS( CParticleInitializerLifeTime );

void CParticleInitializerLifeTime::SetupParticleUpdateData( SParticleUpdaterData& data, EParticleType particleType ) const 
{
	if ( m_lifeTime )
	{
		m_lifeTime->GetApproximationSamples( data.m_lifeTimeInit.m_samples );
	}
}

//! Get mask flag of this initializer
Uint32 CParticleInitializerLifeTime::GetInitializerId() const
{
	if ( !m_lifeTime )
	{
		return 0;
	}
	if ( m_lifeTime->IsA< CEvaluatorFloatRandomUniform >() )
	{
		return (Uint32)PIF_LifeTimeRandom;
	}
	else
	{
		return (Uint32)PIF_LifeTime;
	}
}
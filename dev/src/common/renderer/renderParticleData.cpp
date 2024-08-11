/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderParticleData.h"

Uint32 IParticleData::s_frameIndex = 0;

struct SSeedKeyValue
{
	Uint32 m_key;
	Uint32 m_val;
};

IParticleData::IParticleData( )
: m_spawnCounter( 0.0f )
, m_cycleTime( 0.0f )
, m_cycleCount( 0 )
, m_lastBurstTime( 999.0f )
, m_randomBurstTime( 0.0f )
, m_randomDelayTime( 0.0f )
, m_bursted( false )
, m_delayedParticles( false )
, m_lastFrameIndex( s_frameIndex-1 ) // not yet updated but also not a total fosil
{
}

void IParticleData::SetGenerators( Uint32 numInitializers, const TDynArray< SSeedKeyValue >& seeds )
{
	Generator< CStandardRand > gen;
	m_randGenerators.Grow( numInitializers );
	for( Uint32 i = 0; i < numInitializers; ++i )
	{
		if( !seeds.Empty() && seeds[i].m_val > 0 )
		{
			m_randGenerators[i].Seed( seeds[i].m_val );
		}
		else
		{
			m_randGenerators[i].Seed( gen.Get< Uint32 >() );
		}
	}
}

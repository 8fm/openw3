/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderParticleBuffer.h"

// Drey TODO: introduce particle buffer factory, and base particle buffer code generation on masking, not direct types

Vector3 GCameraPosition;
static int CompareParticlesBackToFront( const void *a , const void *b ) 
{
	const Vector3* positionA = ( Vector3* )( (char*)a + PARTICLE_POSITION );
	const Vector3* positionB = ( Vector3* )( (char*)b + PARTICLE_POSITION );

	const Float particleADistance = ( GCameraPosition - *positionA ).SquareMag();
	const Float particleBDistance = ( GCameraPosition - *positionB ).SquareMag();

	if ( particleADistance < particleBDistance )
	{
		return 1;
	}
	else if ( particleADistance > particleBDistance )
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

IParticleBuffer::IParticleBuffer( Uint32 maxParticles )
: m_maxParticles( maxParticles )
, m_activeParticles( 0 )
{

}

void IParticleBuffer::SortBackToFront( void* buf, Int32 numElements, Int32 elemSize, const Vector& cameraPosition )
{
	GCameraPosition = cameraPosition;
	qsort( buf, numElements, elemSize, &CompareParticlesBackToFront );
}

//////////////////////////////////////////////////////////////////////////
// Fixed orientation TRAIL PARTICLES BUFFER
//////////////////////////////////////////////////////////////////////////

CParticleBuffer< TrailParticle >::CParticleBuffer( Uint32 maxParticles )
: IParticleBuffer( maxParticles )
, m_particles( NULL )
, m_usedParticles( NULL )
, m_freeParticles( NULL )
{
	// Stats
	g_numBuffers++;

	// Check if global particle data limit is not exceeded
	if ( g_numParticlesAllocated + m_maxParticles <= GMaxParticlesGlobal )
	{
		// Allocate particle buffer
		const Uint32 dataSize = sizeof( TrailParticle ) * maxParticles;
		m_particles = static_cast< TrailParticle* >( GParticlePool->Alloc( dataSize ) );
		g_numParticlesAllocated += maxParticles;
		m_usedParticles = static_cast< TrailParticle** >( GParticlePool->Alloc( maxParticles * sizeof(TrailParticle*) ) );
		m_freeParticles = static_cast< TrailParticle** >( GParticlePool->Alloc( maxParticles * sizeof(TrailParticle*) ) );
		for( Uint32 i = 0; i < maxParticles; i++ )
		{
			m_usedParticles[ i ] = NULL;
			m_freeParticles[ i ] = &m_particles[ i ];
		}
	}
	else
	{
		ERR_RENDERER( TXT("Failed to allocate particles due to particle allocs limit. Check maxParticles value in current particle systems") );
	}
}

CParticleBuffer< TrailParticle >::~CParticleBuffer()
{
	// Stats
	g_numBuffers--;

	// Free particle buffer
	if ( m_particles )
	{
		GParticlePool->Free( m_particles );
		m_particles = NULL;
		g_numParticlesAllocated -= m_maxParticles;
	}

	if ( m_usedParticles )
	{
		GParticlePool->Free( m_usedParticles );
		m_usedParticles = NULL;
	}

	if ( m_freeParticles )
	{
		GParticlePool->Free( m_freeParticles );
		m_freeParticles = NULL;
	}
}

Uint32 CParticleBuffer< TrailParticle >::Alloc( Uint32 count )
{
	// No particles to allocate
	if ( !count )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	if ( !m_particles )
	{
		// No data allocated
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	// No free particles
	if ( m_activeParticles + count > m_maxParticles )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	Uint32 firstAllocatedParticle = m_activeParticles;

	for( Uint32 i = 0; i < count; i++ )
	{
		Uint32 indexToFreeParticlePointer = m_maxParticles - m_activeParticles - i - 1;
		TrailParticle* particle = m_freeParticles[ indexToFreeParticlePointer ];
		InitParticle( particle );
		m_usedParticles[ m_activeParticles + i ] = particle;
	}

	m_activeParticles += count;
	g_numActiveParticles += count;

	return firstAllocatedParticle;
}

void CParticleBuffer< TrailParticle >::Dealloc( Uint32 index )
{
	ASSERT( index < m_activeParticles );

	TrailParticle* freedParticle = m_usedParticles[ index ];

	// do we need to remove from middle ?
	if( index != (m_activeParticles - 1) )
	{
		// Trail particles require proper ordering, so we can't just move them around in the buffer
		Uint32 particlesToEndOfList = m_activeParticles - index - 1;
		Red::System::MemoryCopy( &m_usedParticles[ index ], &m_usedParticles[ index + 1 ], sizeof( TrailParticle* ) * particlesToEndOfList );
	}
	m_activeParticles -= 1;
	g_numActiveParticles -= 1;
	m_freeParticles[ m_maxParticles - m_activeParticles - 1 ] = freedParticle;
}

void CParticleBuffer< TrailParticle >::SortBackToFront( const Vector& cameraPosition )
{
	// No sorting for trails
}

//////////////////////////////////////////////////////////////////////////
// Camera facing orientation TRAIL PARTICLES BUFFER
//////////////////////////////////////////////////////////////////////////

CParticleBuffer< FacingTrailParticle >::CParticleBuffer( Uint32 maxParticles )
	: IParticleBuffer( maxParticles )
	, m_particles( NULL )
	, m_usedParticles( NULL )
	, m_freeParticles( NULL )
{
	// Stats
	g_numBuffers++;

	// Check if global particle data limit is not exceeded
	if ( g_numParticlesAllocated + m_maxParticles <= GMaxParticlesGlobal )
	{
		// Allocate particle buffer
		const Uint32 dataSize = sizeof( FacingTrailParticle ) * maxParticles;
		m_particles = static_cast< FacingTrailParticle* >( GParticlePool->Alloc( dataSize ) );
		g_numParticlesAllocated += maxParticles;
		m_usedParticles = static_cast< FacingTrailParticle** >( GParticlePool->Alloc( maxParticles * sizeof(FacingTrailParticle*) ) );
		m_freeParticles = static_cast< FacingTrailParticle** >( GParticlePool->Alloc( maxParticles * sizeof(FacingTrailParticle*) ) );
		for( Uint32 i = 0; i < maxParticles; i++ )
		{
			m_usedParticles[ i ] = NULL;
			m_freeParticles[ i ] = &m_particles[ i ];
		}
	}
	else
	{
		ERR_RENDERER( TXT("Failed to allocate particles due to particle allocs limit. Check maxParticles value in current particle systems") );
	}
}

CParticleBuffer< FacingTrailParticle >::~CParticleBuffer()
{
	// Stats
	g_numBuffers--;

	// Free particle buffer
	if ( m_particles )
	{
		GParticlePool->Free( m_particles );
		m_particles = NULL;
		g_numParticlesAllocated -= m_maxParticles;
	}

	if ( m_usedParticles )
	{
		GParticlePool->Free( m_usedParticles );
		m_usedParticles = NULL;
	}

	if ( m_freeParticles )
	{
		GParticlePool->Free( m_freeParticles );
		m_freeParticles = NULL;
	}
}

Uint32 CParticleBuffer< FacingTrailParticle >::Alloc( Uint32 count )
{
	// No particles to allocate
	if ( !count )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	if ( !m_particles )
	{
		// No data allocated
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	// No free particles
	if ( m_activeParticles + count > m_maxParticles )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	Uint32 firstAllocatedParticle = m_activeParticles;

	for( Uint32 i = 0; i < count; i++ )
	{
		Uint32 indexToFreeParticlePointer = m_maxParticles - m_activeParticles - i - 1;
		FacingTrailParticle* particle = m_freeParticles[ indexToFreeParticlePointer ];
		InitParticle( particle );
		m_usedParticles[ m_activeParticles + i ] = particle;
	}

	m_activeParticles += count;
	g_numActiveParticles += count;

	return firstAllocatedParticle;
}

void CParticleBuffer< FacingTrailParticle >::Dealloc( Uint32 index )
{
	ASSERT( index < m_activeParticles );

	FacingTrailParticle* freedParticle = m_usedParticles[ index ];

	// do we need to remove from middle ?
	if( index != (m_activeParticles - 1) )
	{
		// in theory, we could replace removed particle with last active, but this would shuffle particles
		// and theoritically this would mess up trail particles which require to be sorted by LifeTime
		//m_usedParticles[ index ] = m_usedParticles[ m_activeParticles - 1 ];
		Uint32 particlesToEndOfList = m_activeParticles - index - 1;
		Red::System::MemoryCopy( &m_usedParticles[ index ], &m_usedParticles[ index + 1 ], sizeof( FacingTrailParticle* ) * particlesToEndOfList );
	}
	m_activeParticles -= 1;
	g_numActiveParticles -= 1;
	m_freeParticles[ m_maxParticles - m_activeParticles - 1 ] = freedParticle;
}

void CParticleBuffer< FacingTrailParticle >::SortBackToFront( const Vector& cameraPosition )
{
	// No sorting for trails
}

//////////////////////////////////////////////////////////////////////////
// BEAM PARTICLES BUFFER
//////////////////////////////////////////////////////////////////////////

CParticleBuffer< BeamParticle >::CParticleBuffer( Uint32 maxParticles )
: IParticleBuffer( maxParticles )
, m_particles( NULL )
, m_usedParticles( NULL )
, m_freeParticles( NULL )
{
	// Stats
	g_numBuffers++;

	// Check if global particle data limit is not exceeded
	if ( g_numParticlesAllocated + m_maxParticles <= GMaxParticlesGlobal )
	{
		// Allocate particle buffer
		const Uint32 dataSize = sizeof( BeamParticle ) * maxParticles;
		m_particles = static_cast< BeamParticle* >( GParticlePool->Alloc( dataSize ) );
		g_numParticlesAllocated += maxParticles;
		m_usedParticles = static_cast< BeamParticle** >( GParticlePool->Alloc( maxParticles * sizeof(BeamParticle*) ) );
		m_freeParticles = static_cast< BeamParticle** >( GParticlePool->Alloc( maxParticles * sizeof(BeamParticle*) ) );
		for( Uint32 i = 0; i < maxParticles; i++ )
		{
			m_usedParticles[ i ] = NULL;
			m_freeParticles[ i ] = &m_particles[ i ];
		}
	}
	else
	{
		ERR_RENDERER( TXT("Failed to allocate particles due to particle allocs limit. Check maxParticles value in current particle systems") );
	}
}

CParticleBuffer< BeamParticle >::~CParticleBuffer()
{
	// Stats
	g_numBuffers--;

	// Free particle buffer
	if ( m_particles )
	{
		GParticlePool->Free( m_particles );
		m_particles = NULL;
		g_numParticlesAllocated -= m_maxParticles;
	}

	if ( m_usedParticles )
	{
		GParticlePool->Free( m_usedParticles );
		m_usedParticles = NULL;
	}

	if ( m_freeParticles )
	{
		GParticlePool->Free( m_freeParticles );
		m_freeParticles = NULL;
	}
}

Uint32 CParticleBuffer< BeamParticle >::Alloc( Uint32 count )
{
	// No particles to allocate
	if ( !count )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	if ( !m_particles )
	{
		// No data allocated
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	// No free particles
	if ( m_activeParticles + count > m_maxParticles )
	{
		return IParticleBuffer::INVALID_PARTICLE_INDEX;
	}

	Uint32 firstAllocatedParticle = m_activeParticles;

	for( Uint32 i = 0; i < count; i++ )
	{
		Uint32 indexToFreeParticlePointer = m_maxParticles - m_activeParticles - i - 1;
		BeamParticle* particle = m_freeParticles[ indexToFreeParticlePointer ];
		InitParticle( particle );
		m_usedParticles[ m_activeParticles + i ] = particle;
	}

	m_activeParticles += count;
	g_numActiveParticles += count;

	return firstAllocatedParticle;
}

void CParticleBuffer< BeamParticle >::Dealloc( Uint32 index )
{
	ASSERT( index < m_activeParticles );

	BeamParticle* freedParticle = m_usedParticles[ index ];

	// do we need to remove from middle ?
	if( index != (m_activeParticles - 1) )
	{
		Uint32 particlesToEndOfList = m_activeParticles - index - 1;
		Red::System::MemoryCopy( &m_usedParticles[ index ], &m_usedParticles[ index + 1 ], sizeof( BeamParticle* ) * particlesToEndOfList );
	}
	m_activeParticles -= 1;
	g_numActiveParticles -= 1;
	m_freeParticles[ m_maxParticles - m_activeParticles - 1 ] = freedParticle;
}

void CParticleBuffer< BeamParticle >::SortBackToFront( const Vector& cameraPosition )
{
	// No sorting for beams
}
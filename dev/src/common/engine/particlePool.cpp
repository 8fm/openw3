/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "particlePool.h"

CParticlePool* GParticlePool = NULL;

CParticlePool::CParticlePool()
{
}

CParticlePool::~CParticlePool()
{
}

void* CParticlePool::Alloc( Uint32 size )
{
	// Just allocate from main memory pool right now
	void* rawParticleData = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Particles, size, DEFAULT_PARTICLE_POOL_ALIGNMENT );
	return rawParticleData;
}

void CParticlePool::Free( void* rawParticleData )
{
	ASSERT( rawParticleData );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Particles, rawParticleData );
}

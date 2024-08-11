/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#define DEFAULT_PARTICLE_POOL_ALIGNMENT 16

/// Special memory pool for particles
class CParticlePool
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Particles );
public:
	CParticlePool();
	~CParticlePool();

	//! Allocate particle buffer memory
	void *Alloc( Uint32 size );

	//! Free particle memory
	void Free( void* rawParticleData );
};

/// Particle pool
extern CParticlePool* GParticlePool;

/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/particlePool.h"
#include "particleLayouts.h"

static Uint32 g_numBuffers = 0;				//!< Number of active particle buffers
static Uint32 g_numActiveParticles = 0;		//!< Active particles
static Uint32 g_numParticlesAllocated = 0;	//!< Particle data structures allocated

class IParticleBuffer
{
public:
	static const Uint32 INVALID_PARTICLE_INDEX = 0xFFFFFFFF;
	static const Uint32 GMaxParticlesGlobal = 200000;

protected:
	Uint32	m_maxParticles;			//!< Buffer capacity
	Uint32	m_activeParticles;		//!< Number of active particles. Index of the last element in m_usedParticles

protected:
	//! Sort particles (internal)
	static void SortBackToFront( void* buf, Int32 numElements, Int32 elemSize, const Vector& cameraPosition );

public:
	RED_INLINE virtual void* GetParticleDataAt( Uint32 index ) = 0;

	//! Get max particles this buffer can hold
	RED_INLINE Uint32 GetMaxParticles() const { return m_maxParticles; };

	//! Get current number of active particles
	RED_INLINE Uint32 GetNumParticles() const { return m_activeParticles; }

	//! Allocate particles from the buffer, returns index to first allocated particle in m_usedParticles array
	virtual Uint32 Alloc( Uint32 count ) = 0;

	//! Deallocate particle at given index (index points to m_usedParticles array)
	virtual void Dealloc( Uint32 index ) = 0;

	IParticleBuffer( Uint32 maxParticles );
	IParticleBuffer() {}
	virtual ~IParticleBuffer() {}
};

//////////////////////////////////////////////////////////////////////////
// Particle type - independent initializer function, used for all types
namespace
{
	template< typename PARTICLE_TYPE >
	RED_INLINE static void InitParticle( PARTICLE_TYPE* particle )
	{
		char* particleRaw = ( char* )particle;
		Float* life = ( Float* )( particleRaw + PARTICLE_LIFE );
		*life = 0.0f;

		Float* lifeSpanInv = ( Float* )( particleRaw + PARTICLE_LIFE_SPAN_INV );
		*lifeSpanInv = 1.0f;

		Vector3* position = ( Vector3* )( particleRaw + PARTICLE_POSITION );
		*position = Vector3( 0.0f, 0.0f, 0.0f );

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Velocity )		
		{
			Vector3* baseVelocity = ( Vector3* )( particleRaw + PARTICLE_BASE_VELOCITY );
			*baseVelocity = Vector3( 0.0f, 0.0f, 0.0f );
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Frame )		
		{
			Float* baseFrame = ( Float* )( particleRaw + PARTICLE_BASE_FRAME );
			*baseFrame = 0.0f;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Size2D )
		{
			Vector2* baseSize = ( Vector2* )( particleRaw + PARTICLE_BASE_SIZE );
			*baseSize = Vector2( 0.1f, 0.1f );
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Size3D )
		{
			Vector3* baseSize3D = ( Vector3* )( particleRaw + PARTICLE_BASE_SIZE3D );
			*baseSize3D = Vector3( 0.1f, 0.1f, 0.1f );
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Color )
		{
			Vector3* baseColor = ( Vector3* )( particleRaw + PARTICLE_BASE_COLOR );
			*baseColor = Vector3( 1.0f, 1.0f, 1.0f );

			Float* baseAlpha = ( Float* )( particleRaw + PARTICLE_BASE_ALPHA );
			*baseAlpha = 1.0f;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation2D )
		{
			Float* rotation = ( Float* )( particleRaw + PARTICLE_ROTATION );
			*rotation = 0.0f;
			Float* baseRotationRate = ( Float* )( particleRaw + PARTICLE_BASE_ROTATION_RATE );
			*baseRotationRate = 0.0f;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Rotation3D )
		{
			Vector3* rotation3D = ( Vector3* )( particleRaw + PARTICLE_ROTATION3D );
			*rotation3D = Vector3( 0.0f, 0.0f, 0.0f );
			Vector3* baseRotationRate3D = ( Vector3* )( particleRaw + PARTICLE_BASE_ROTATION_RATE3D );
			*baseRotationRate3D = 0.0f;
		}

		if ( PARTICLE_TYPE::m_fieldSet & PFS_Turbulence )
		{
			Vector3* turbulence = ( Vector3* )( particleRaw + PARTICLE_TURBULENCE );
			*turbulence = Vector3( 0.0f, 0.0f, 0.0f );
			Float* turbulenceCounter = ( Float* )( particleRaw + PARTICLE_TURBULENCE_COUNTER );
			*turbulenceCounter = 999.0f;
		}
	}
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// PARTICLE BUFFER TEMPLATE
//////////////////////////////////////////////////////////////////////////

template < typename T >
class CParticleBuffer : public IParticleBuffer
{
protected:
	T*		m_particles;			//!< Particles	

public:
	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE T* GetParticleAt( Uint32 index ) { return &m_particles[ index ]; }

	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE const T* GetParticleAt( Uint32 index ) const { return &m_particles[ index ]; }

	RED_INLINE virtual void* GetParticleDataAt( Uint32 index ) { return ( void* )&m_particles[ index ]; }

public:
	CParticleBuffer( Uint32 maxParticles );
	virtual ~CParticleBuffer();

	//! Allocate particles from the buffer, returns index to first allocated particle in m_usedParticles array
	virtual Uint32 Alloc( Uint32 count );

	//! Deallocate particle at given index (index points to m_usedParticles array)
	virtual void Dealloc( Uint32 index );

	//! Sort particles 
	void SortBackToFront( const Vector& cameraPosition );

private:
	RED_INLINE CParticleBuffer( const CParticleBuffer& other ) {};
	RED_INLINE CParticleBuffer& operator==( const CParticleBuffer& other ) { return *this; };
};

template < typename T >
CParticleBuffer<T>::CParticleBuffer( Uint32 maxParticles )
: IParticleBuffer( maxParticles )
, m_particles( NULL )
{
	// Stats
	g_numBuffers++;

	// Check if global particle data limit is not exceeded
	if ( g_numParticlesAllocated + m_maxParticles <= GMaxParticlesGlobal )
	{
		// Allocate particle buffer
		const Uint32 dataSize = sizeof( T ) * maxParticles;
		m_particles = static_cast< T* >( GParticlePool->Alloc( dataSize ) );
		g_numParticlesAllocated += maxParticles;
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( Particles ), TXT("Failed to allocate particles due to particle allocs limit. Check maxParticles value in current particle systems.") );
	}
}

template < typename T >
CParticleBuffer<T>::~CParticleBuffer()
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
}

template < typename T >
Uint32 CParticleBuffer<T>::Alloc( Uint32 count )
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
		InitParticle( &m_particles[ m_activeParticles + i ] );
	}

	m_activeParticles += count;
	g_numActiveParticles += count;

	return firstAllocatedParticle;
}

template < typename T >
void CParticleBuffer<T>::Dealloc( Uint32 index )
{
	ASSERT( index < m_activeParticles );

	Red::System::MemoryCopy( &m_particles[ index ], &m_particles[ m_activeParticles - 1 ], sizeof( T ) );

	m_activeParticles -= 1;
	g_numActiveParticles -= 1;
}

template < typename T >
void CParticleBuffer<T>::SortBackToFront( const Vector& cameraPosition )
{
	IParticleBuffer::SortBackToFront( m_particles, m_activeParticles, sizeof( T ), cameraPosition );
}

//////////////////////////////////////////////////////////////////////////
// PARTICLE BUFFER TEMPLATE EXPLICIT SPECIALIZATION FOR TRAIL PARTICLES
// Required due to order bound character of trail particles
//////////////////////////////////////////////////////////////////////////

template<> class CParticleBuffer< TrailParticle > : public IParticleBuffer
{
protected:
	TrailParticle*		m_particles;			//!< Particles	
	TrailParticle**		m_usedParticles;
	TrailParticle**		m_freeParticles;

public:
	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE TrailParticle* GetParticleAt( Uint32 index ) { return m_usedParticles[ index ]; }

	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE const TrailParticle* GetParticleAt( Uint32 index ) const { return m_usedParticles[ index ]; }

	RED_INLINE virtual void* GetParticleDataAt( Uint32 index ) { return ( void* )m_usedParticles[ index ]; }

public:
	CParticleBuffer( Uint32 maxParticles );
	virtual ~CParticleBuffer();

	//! Allocate particles from the buffer, returns index to first allocated particle in m_usedParticles array
	virtual Uint32 Alloc( Uint32 count );

	//! Deallocate particle at given index (index points to m_usedParticles array)
	virtual void Dealloc( Uint32 index );

	//! Sort particles 
	void SortBackToFront( const Vector& cameraPosition );

private:
	RED_INLINE CParticleBuffer( const CParticleBuffer& other ) {};
	RED_INLINE CParticleBuffer& operator==( const CParticleBuffer& other ) { return *this; };
};

template<> class CParticleBuffer< FacingTrailParticle > : public IParticleBuffer
{
protected:
	FacingTrailParticle*		m_particles;			//!< Particles	
	FacingTrailParticle**		m_usedParticles;
	FacingTrailParticle**		m_freeParticles;

public:
	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE FacingTrailParticle* GetParticleAt( Uint32 index ) { return m_usedParticles[ index ]; }

	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE const FacingTrailParticle* GetParticleAt( Uint32 index ) const { return m_usedParticles[ index ]; }

	RED_INLINE virtual void* GetParticleDataAt( Uint32 index ) { return ( void* )m_usedParticles[ index ]; }

public:
	CParticleBuffer( Uint32 maxParticles );
	virtual ~CParticleBuffer();

	//! Allocate particles from the buffer, returns index to first allocated particle in m_usedParticles array
	virtual Uint32 Alloc( Uint32 count );

	//! Deallocate particle at given index (index points to m_usedParticles array)
	virtual void Dealloc( Uint32 index );

	//! Sort particles 
	void SortBackToFront( const Vector& cameraPosition );

private:
	RED_INLINE CParticleBuffer( const CParticleBuffer& other ) {};
	RED_INLINE CParticleBuffer& operator==( const CParticleBuffer& other ) { return *this; };
};

//////////////////////////////////////////////////////////////////////////
// PARTICLE BUFFER TEMPLATE EXPLICIT SPECIALIZATION FOR BEAM PARTICLES
// Required due to beam particles being handled in batches ( per beam )
//////////////////////////////////////////////////////////////////////////

template<> class CParticleBuffer< BeamParticle > : public IParticleBuffer
{
protected:
	BeamParticle*		m_particles;			//!< Particles	
	BeamParticle**		m_usedParticles;
	BeamParticle**		m_freeParticles;

public:
	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE BeamParticle* GetParticleAt( Uint32 index ) { return m_usedParticles[ index ]; }

	//! Return alive/used particle at given index: 0...m_activeParticles - 1
	RED_INLINE const BeamParticle* GetParticleAt( Uint32 index ) const { return m_usedParticles[ index ]; }

	RED_INLINE virtual void* GetParticleDataAt( Uint32 index ) { return ( void* )m_usedParticles[ index ]; }

public:
	CParticleBuffer( Uint32 maxParticles );
	virtual ~CParticleBuffer();

	virtual Uint32 Alloc( Uint32 count );

	virtual void Dealloc( Uint32 index );

	//! Sort particles 
	void SortBackToFront( const Vector& cameraPosition );

private:
	RED_INLINE CParticleBuffer( const CParticleBuffer& other ) {};
	RED_INLINE CParticleBuffer& operator==( const CParticleBuffer& other ) { return *this; };
};
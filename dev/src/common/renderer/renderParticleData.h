/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "renderParticleBuffer.h"
#include "particleVertexBuffer.h"
#include "../../common/redMath/random/fastRand.h"

//////////////////////////////////////////////////////////////////

class CRenderProxy_Particles;
using namespace Red::Math::Random;

class IParticleData
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Particles );

public:
	/// Per emitter particle drawer cached data, parameters can be used in different ways depending on the drawer
	struct ParticleDrawerCache
	{
		Vector		m_lastPosition;
		Bool		m_isSet;
		ParticleDrawerCache() : m_isSet(false) {}
		RED_INLINE void SetLastPosition( const Vector& lastPos ){ m_lastPosition = lastPos; MarkSet(); }
		RED_INLINE void MarkSet(){ m_isSet = true; }
		RED_INLINE Bool IsSet(){ return m_isSet; }
	};

public:
	Float									 m_spawnCounter;			//!< Internal spawn counter to accumulate for fractional spawning
	Float									 m_cycleTime;				//!< Time in current cycle
	Int32									 m_cycleCount;				//!< Number of cycles that we have made
	Float									 m_lastBurstTime;			//!< Time since last burst
	Float									 m_randomBurstTime;			//!< Random time for burst
	Float									 m_randomDelayTime;			//!< Random delay time for burst
	Float									 m_delayTimer;				//!< Time to remember the time for the delay
	Bool									 m_delayedParticles;		//!< Time to remember the time for the delay
	Bool									 m_bursted;					//!< If it has bursted or not
	TDynArray< Generator< FastRand >>		 m_randGenerators;			//!< Rand generators array for seeds
	Box										 m_box;						//!< Bounding box
	Uint32									 m_numVertices;				//!< Number of vertices to draw
	Uint32									 m_byteOffset;				//!< Offset in bytes, to the location in vertex buffer

protected:
	ParticleDrawerCache						 m_drawerCache;				//!< Cached particle drawer data

	Uint32									 m_lastFrameIndex;			//!< Index of the last update frame; used to prevent multiple simulations per frame; TODO: remove this once particle update gets separated from rendering
	static Uint32							 s_frameIndex;				//!< Global frame index

public:
	//! Get drawer cache
	RED_FORCE_INLINE ParticleDrawerCache& GetDrawerCache() { return m_drawerCache; }

	RED_FORCE_INLINE Bool WasUpdatedThisFrame() const { return m_lastFrameIndex == s_frameIndex; }
	RED_FORCE_INLINE void MarkAsUpdatedThisFrame() { m_lastFrameIndex = s_frameIndex; }
	RED_FORCE_INLINE void MarkAsNotUpdatedThisFrame() { m_lastFrameIndex = s_frameIndex - 1; }
	RED_FORCE_INLINE Uint32 GetFramesSinceUpdate() const { return s_frameIndex - m_lastFrameIndex; }
	
	void SetGenerators( Uint32 numInitializers, const TDynArray< SSeedKeyValue >& seeds );

	static void OnNewFrame() { s_frameIndex++; }

public:
	IParticleData();
	virtual ~IParticleData() {}

	//! Get access to particle buffer holding particle data
	virtual IParticleBuffer* GetParticleBufferInterface() = 0;

	//! Get read only access to particle buffer holding particle data
	virtual const IParticleBuffer*	GetParticleBufferInterface() const = 0;
};

//////////////////////////////////////////////////////////////////

/// Specialized particle data for given particle type
template< typename PARTICLE_TYPE >
class CParticleData : public IParticleData
{
protected:
	CParticleBuffer< PARTICLE_TYPE >	m_buffer;			//!< Buffer with particle data
	CParticleVertexBuffer				m_vertexBuffer;

public:
	//! Get particle buffer
	RED_FORCE_INLINE CParticleBuffer< PARTICLE_TYPE >& GetBuffer() { return m_buffer; }

	//! Get particle buffer ( read only )
	RED_FORCE_INLINE const CParticleBuffer< PARTICLE_TYPE >& GetBuffer() const { return m_buffer; }

public:
	CParticleData( Uint32 maxParticles );
	virtual ~CParticleData() {}

	//! Get access to particle buffer holding particle data
	virtual IParticleBuffer* GetParticleBufferInterface() { return &m_buffer; }

	//! Get access to particle buffer holding particle data
	virtual const IParticleBuffer* GetParticleBufferInterface() const { return &m_buffer; };
};

template< typename PARTICLE_TYPE >
CParticleData<PARTICLE_TYPE>::CParticleData( Uint32 maxParticles )
	: m_buffer( maxParticles )
{
}

//////////////////////////////////////////////////////////////////

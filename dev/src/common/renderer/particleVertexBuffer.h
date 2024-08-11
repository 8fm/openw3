/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Particles vertex types
typedef GpuApi::SystemVertex_ParticleStandard		ParticleSimpleVertex;
typedef GpuApi::SystemVertex_ParticleMotionBlur		ParticleMotionVertex;
typedef GpuApi::SystemVertex_ParticleTrail			ParticleTrailVertex;
typedef GpuApi::SystemVertex_ParticleFacingTrail_Beam	ParticleFacingTrail_Beam_Vertex;

//#define USE_INTERMEDIATE_VERTEX_BUFFER
#define PARTICLE_VERTEX_BUFFER_SIZE 2 * 1024 * 1024 // This is very big, but for the demo weather system imitation it can decrease the required amount of discard locks
#define MAX_PARTICLES_IN_BATCH		1000

class CParticleVertexBuffer
{
public:
	struct SBindInfo
	{
		Uint32					m_numVertices;			//!< Number of vertices to draw
		Uint32					m_byteOffset;			//!< Offset in bytes, to the location in vertex buffer

		SBindInfo() 
			: m_numVertices(0)
			, m_byteOffset(0)
		{}
	};

private:
	static GpuApi::BufferRef	s_vertexBufferRef;				//!< Double buffering to avoid Discards

	static const Int32			sc_bufferSize		= PARTICLE_VERTEX_BUFFER_SIZE;
	static const Int32			sc_maxParticleBatch	= MAX_PARTICLES_IN_BATCH;

	static Int32				s_currentVBOffset;				//!< Vertex buffer write offset
	static Int8* RESTRICT		s_bufferPtr;					//!< Pointer to the begnning of the [intermediate] vertex data (depending on USE_INTERMEDIATE_VERTEX_BUFFER)
	static Int8* RESTRICT		s_currentWritePtr;				//!< Current [intermediate] vertex buffer write position (depending on USE_INTERMEDIATE_VERTEX_BUFFER)
	
	static Box					s_currentBoundingBox;			//!< Bounding box from the last map-unmap session.
	static SBindInfo			s_lastBindInfo;					//!< Info for the subsequent bind command

public:
	// Allocate vertex buffer
	static RED_INLINE void AllocateBuffer()
	{
		ASSERT( s_vertexBufferRef.isNull() );

		// Create circular gpu vertex buffer for the first time.
		s_vertexBufferRef = GpuApi::CreateBuffer( sc_bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( s_vertexBufferRef, "ParticleVB" );

		// Zero vertex buffer memory ptr.
		s_currentWritePtr = NULL;

#ifdef USE_INTERMEDIATE_VERTEX_BUFFER
		// This buffer only needs to accomodate as much vertex data as one emitter can generate.
		s_bufferPtr = (Int8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ParticlesRendering , sizeof( GpuApi::SystemVertex_ParticleMotionBlur ) * sc_maxParticleBatch * 4);
		// Initialize write position.
		s_currentWritePtr = s_bufferPtr;
#endif
	}

	// Deallocate vertex buffer
	static RED_INLINE void DeallocateBuffer()
	{
		// Release vertex buffer
		GpuApi::SafeRelease( s_vertexBufferRef );

#ifdef USE_INTERMEDIATE_VERTEX_BUFFER
		RED_MEMORY_FREE( MemoryPool_Default, MC_ParticlesRendering, s_bufferPtr );
		s_bufferPtr = NULL;
#endif
		s_currentVBOffset = 0;
		s_currentWritePtr = NULL;
	}

	// Check if vertex buffer is ready
	static Bool IsAllocated()
	{ 
#ifdef USE_INTERMEDIATE_VERTEX_BUFFER
		return !s_vertexBufferRef.isNull() && s_bufferPtr;
#else
		return !s_vertexBufferRef.isNull();
#endif
	}
	
	static RED_INLINE void Map();

	static RED_INLINE void Unmap();

	template < typename PARTICLE_VERTEX_TYPE >
	static RED_INLINE void MoveOffset();

	// Get setup info for bind buffer command
	static RED_INLINE const SBindInfo& GetBindInfo() { return s_lastBindInfo; }

	static RED_INLINE GpuApi::BufferRef GetCurrentBufferRef() { return s_vertexBufferRef; }

	static RED_INLINE Box& BoundingBox() { return s_currentBoundingBox; }

	static RED_INLINE void ResetBoundingBox() { s_currentBoundingBox.Clear(); }

	template < typename PARTICLE_VERTEX_TYPE >
	static RED_INLINE Bool ValidateAvailableSize( Uint32 numParticles )
	{
		return ( s_currentVBOffset +  4 * numParticles * sizeof( PARTICLE_VERTEX_TYPE ) < sc_bufferSize );
	}

	template < typename PARTICLE_VERTEX_TYPE >
	static RED_INLINE void GetNextVertexPtrIncrement( PARTICLE_VERTEX_TYPE* RESTRICT & vertexPtr )
	{
		// VertexData is always generated in 4-th
		vertexPtr = ( PARTICLE_VERTEX_TYPE* )s_currentWritePtr;
		s_currentWritePtr += sizeof( PARTICLE_VERTEX_TYPE );
	}

	static RED_INLINE void OnNewFrame()
	{
	}
};

RED_INLINE void CParticleVertexBuffer::Map()
{
#ifndef USE_INTERMEDIATE_VERTEX_BUFFER
	s_currentVBOffset = 0;

	// Write data to vertex buffer
	s_bufferPtr = (Int8*)GpuApi::LockBuffer( GetCurrentBufferRef(), GpuApi::BLF_Discard, 0, sc_bufferSize );
	

#endif
	s_currentWritePtr = s_bufferPtr;
}

RED_INLINE void CParticleVertexBuffer::Unmap()
{
	// Unlock vertex buffer
	GpuApi::UnlockBuffer( GetCurrentBufferRef() );
}

template < typename PARTICLE_VERTEX_TYPE >
RED_INLINE void CParticleVertexBuffer::MoveOffset()
{
	// Compute number of used bytes
	Int32 dataSize = (Int32)( s_currentWritePtr - s_bufferPtr );
	s_bufferPtr += dataSize;

	ASSERT( dataSize % sizeof( PARTICLE_VERTEX_TYPE ) == 0 );

	// Prepare info for subsequent Bind operation
	s_lastBindInfo.m_numVertices = dataSize / sizeof( PARTICLE_VERTEX_TYPE );
	s_lastBindInfo.m_byteOffset = s_currentVBOffset;

	// Advance the circular buffer pointer
	s_currentVBOffset += dataSize;
}

template <> void CParticleVertexBuffer::MoveOffset< void >();
template <> Bool CParticleVertexBuffer::ValidateAvailableSize< void >( Uint32 numParticles );
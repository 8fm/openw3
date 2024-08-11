/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "particleVertexBuffer.h"

GpuApi::BufferRef					CParticleVertexBuffer::s_vertexBufferRef;
Int8* RESTRICT						CParticleVertexBuffer::s_currentWritePtr			= NULL;	
Int8* RESTRICT						CParticleVertexBuffer::s_bufferPtr					= NULL;	
Int32								CParticleVertexBuffer::s_currentVBOffset			= 0;

CParticleVertexBuffer::SBindInfo	CParticleVertexBuffer::s_lastBindInfo				= CParticleVertexBuffer::SBindInfo();
Box									CParticleVertexBuffer::s_currentBoundingBox			= Box();

template <> void CParticleVertexBuffer::MoveOffset< void >()
{
}

template <> Bool CParticleVertexBuffer::ValidateAvailableSize< void >( Uint32 numParticles )
{
	return true;
}
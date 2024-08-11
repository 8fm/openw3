/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderDestructionMesh.h"
#include  "../engine/physicsDestructionResource.h"

#ifndef RED_FINAL_BUILD
#include "renderResourceIterator.h"
#endif
#include "../core/ioTags.h"
#include "../redSystem/error.h"

namespace Config
{
	extern TConfigVar< Bool >		cvForceSyncMeshLoading;	
	extern TConfigVar< Float >		cvAutoRefreshTime;
	extern TConfigVar< Int32 >		cvLoadingDelay;
}

//------------------------

IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( CRenderDestructionMesh );

//------------------------

CRenderDestructionMesh::CRenderDestructionMesh( CRenderMesh* toCopy )
	: m_originalRenderMesh( toCopy )
{
	RED_ASSERT( toCopy != nullptr, TXT("Trying to initialize a CRenderDestructionMesh with a nullptr ") );

	if( !toCopy )
	{
		return;
	}

	m_batchList =  nullptr;
	m_batchNext =  nullptr;
	m_quantizationScale = toCopy->m_quantizationScale;
	m_quantizationOffset = toCopy->m_quantizationOffset;
	m_chunks = toCopy->m_chunks; 
	m_lods = toCopy->m_lods;
	m_bonePositions = toCopy->m_bonePositions;
	m_clusterBuilderTask = nullptr;

	m_buffersPendingLoad.SetValue( 2 );

	m_originalRenderMesh->AddRef();

	GpuApi::AddRefIfValid( m_bonePositions );

}

CRenderDestructionMesh::CRenderDestructionMesh()
	: CRenderMesh()
{

}

void CRenderDestructionMesh::FinalizeLoading( )
{
	RED_ASSERT( !IsFullyLoaded(), TXT("We already copied the index buffer.") );

	if( m_originalRenderMesh && m_originalRenderMesh->IsFullyLoaded() && !IsFullyLoaded() )
	{
		const auto& bufferDesc = GpuApi::GetBufferDesc( m_originalRenderMesh->m_indices );
		GpuApi::BufferRef ib = GpuApi::CreateBuffer( bufferDesc.size, bufferDesc.category, GpuApi::BUT_Dynamic, bufferDesc.accessFlags, 0 );
		if ( !ib.isNull() )
		{
			GpuApi::CopyBuffer(ib, 0, m_originalRenderMesh->m_indices, 0 , bufferDesc.size );

			// setup bindings
			m_indices = ib;
			
			m_vertices = m_originalRenderMesh->m_vertices;
			GpuApi::AddRef( m_vertices );

			m_buffersPendingLoad.SetValue( 0 );

			// We no longer need the original renderMesh, release it
			SAFE_RELEASE( m_originalRenderMesh)
		}
	}
}

CRenderDestructionMesh::~CRenderDestructionMesh()
{
}

Bool CRenderDestructionMesh::UpdateActiveIndices( const TDynArray<Uint16>& activeIndices, const TDynArray< Uint32 >& chunkOffsets, const TDynArray< Uint32 >& chunkNumIndices  )
{
	if( chunkOffsets.Size() != m_chunks.Size() )
	{
		return false;
	}

	if( !IsFullyLoaded() )
	{
		return false;
	}

	if( UpdateBuffer( activeIndices ) )
	{
		for( Uint32 i = 0; i < m_chunks.Size(); ++i )
		{
			m_chunks[i].m_chunkIndices.byteOffset = chunkOffsets[i];
			m_chunks[i].m_numIndices = chunkNumIndices[i];
		}
		return true;

	}
	
	return false;
	

}

Bool CRenderDestructionMesh::UpdateBuffer( const TDynArray< Uint16 >&	activeIndices )
{
	if( m_indices.isNull() )
	{
		return false;
	}

	Uint32 srcStride = 2;
	GpuApi::eBufferChunkType chunkType = m_chunks[0].m_chunkIndices.type;
	switch ( chunkType )
	{
	case GpuApi::BCT_IndexUShort:		srcStride = 2;	break;
	default:
		RED_HALT(  "Stride not supported: %u", srcStride );
		return false;
	}

	Uint32 offset		= 0;
	Uint32 lockedSize	= srcStride * activeIndices.Size();

	RED_ASSERT( lockedSize <= GetBufferDesc( m_indices ).size, TXT( "Locked data size should never exceed the index buffer size!") );

	void* dstData = GpuApi::LockBuffer( m_indices, GpuApi::BLF_Discard, offset, lockedSize );
	RED_ASSERT( dstData, TXT("Failed to lock the index buffer!") );

	if ( dstData )
	{	
		Red::MemoryCopy( dstData, &activeIndices[ 0 ], lockedSize );
	}
	GpuApi::UnlockBuffer( m_indices );

	return true;
}


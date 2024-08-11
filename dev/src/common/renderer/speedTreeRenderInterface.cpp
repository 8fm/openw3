/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeRenderInterface.h"

#ifdef USE_SPEED_TREE

using namespace SpeedTree;

Int32 SSpeedTreeRenderStats::ReadIndex()
{
	if( s_bufferIndex == 0 )
	{
		return c_bufferCount - 1;
	}
	else
	{
		return s_bufferIndex - 1;
	}
}

void SSpeedTreeRenderStats::NextFrame()
{
	s_bufferIndex++;
	if( s_bufferIndex >= c_bufferCount )
	{
		s_bufferIndex = 0;
	}

	SSpeedTreeRenderStats::s_grassLayerCount[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassCellCount[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassCellArrayCapacity[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassCellArraySize[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassInstanceCount[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassInstanceArrayCapacity[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleGrassInstanceArraySize[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeCellCount[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeCellArrayCapacity[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeCellArraySize[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeInstanceCount[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeInstanceArrayCapacity[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_visibleTreeInstanceArraySize[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_maxGrassLayerCullDistance[ s_bufferIndex ] = 0.0f;
	SSpeedTreeRenderStats::s_minGrassCellSize[ s_bufferIndex ] = 0.0f;
	SSpeedTreeRenderStats::s_maxGrassCellSize[ s_bufferIndex ] = 0.0f;

	SSpeedTreeRenderStats::s_treesRendered[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_billboardsRendered[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_grassRendered[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_treeDrawcalls[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_billboardDrawcalls[ s_bufferIndex ] = 0;
	SSpeedTreeRenderStats::s_grassDrawcalls[ s_bufferIndex ] = 0;
}

Int32 SSpeedTreeRenderStats::s_bufferIndex = 0;
Uint64 SSpeedTreeRenderStats::s_grassLayerCount[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassCellCount[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassCellArrayCapacity[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassCellArraySize[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassInstanceCount[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassInstanceArrayCapacity[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleGrassInstanceArraySize[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeCellCount[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeCellArrayCapacity[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeCellArraySize[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeInstanceCount[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeInstanceArrayCapacity[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_visibleTreeInstanceArraySize[ SSpeedTreeRenderStats::c_bufferCount ];
Float SSpeedTreeRenderStats::s_maxGrassLayerCullDistance[ SSpeedTreeRenderStats::c_bufferCount ];
Float SSpeedTreeRenderStats::s_minGrassCellSize[ SSpeedTreeRenderStats::c_bufferCount ];
Float SSpeedTreeRenderStats::s_maxGrassCellSize[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_treesRendered[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_billboardsRendered[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_grassRendered[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_treeDrawcalls[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_billboardDrawcalls[ SSpeedTreeRenderStats::c_bufferCount ];
Uint64 SSpeedTreeRenderStats::s_grassDrawcalls[ SSpeedTreeRenderStats::c_bufferCount ];

template<> CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache = NULL;
template<> CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache = NULL;
template<> CTexture::CTextureCache* CTexture::m_pCache = NULL;
template<> CStateBlock::CStateBlockCache* CStateBlock::m_pCache = NULL;
template<> st_bool CRenderState::m_bFallbackTexturesInited = false;

template<> CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> st_int32 CRenderState::m_nFallbackTextureRefCount = 0;
//template<> CShaderConstantBuffer CForestRender::m_cFrameConstantBuffer = CShaderConstantBuffer( ); MADE THIS NONSTATIC TO SUPPORT MULTIPLE FOREST IN EDITOR
//template<> SFrameCBLayout CForestRender::m_sFrameConstantBufferLayout = SFrameCBLayout( ); MADE THIS NONSTATIC TO SUPPORT MULTIPLE FOREST IN EDITOR


#define MAX_SPEEDTREE_RES_BUFFER_SIZE 1024 * 1024 * 5

bool SpeedTree::CLavaFileSystem::FileExists( const st_char* pFilename )
{
	return GFileManager->FileExist( ANSI_TO_UNICODE( pFilename ) );
}

size_t SpeedTree::CLavaFileSystem::FileSize( const st_char* pFilename )
{
	return (size_t)GFileManager->GetFileSize( ANSI_TO_UNICODE( pFilename ) );
}

st_byte* SpeedTree::CLavaFileSystem::LoadFile( const char* pFilename, ETermHint eTermHint )
{
	IFile* reader = GFileManager->CreateFileReader( ANSI_TO_UNICODE( pFilename ), FOF_AbsolutePath, MAX_SPEEDTREE_RES_BUFFER_SIZE );
	if ( !reader )
	{
		ASSERT( TXT("Couldn't create file reader for speed tree resource!") );
		return nullptr;
	}

	// Serialize resource
	size_t size = static_cast< size_t >( reader->GetSize() );
	RED_ASSERT( (Uint64)size == reader->GetSize(), TXT("Unexpectedly large file '%ls'"), reader->GetFileNameForDebug() );
	void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_SpeedTreeRender, size );
	reader->Serialize( buffer, size );

	// Reader's work is done
	delete reader;
	return (st_byte*)buffer;
}

void SpeedTree::CLavaFileSystem::Release( st_byte* pBuffer )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_SpeedTreeRender, pBuffer );
}

void SpeedTree::GetSpeedTreeGeneralStats( CTreeRender* treeRender, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& statistic )
{
	for( Uint32 i=0; i<RENDER_PASS_COUNT; ++i )
	{
	 	const CArray< CRenderState >& states = treeRender->Get3dRenderStates( (ERenderPass)i );
	 	const Uint32 stateCount = (Uint32)states.size();
	 	for( Uint32 j=0; j<stateCount; ++j )
	 	{
			GatherInformationAboutTextures( states[j], statistic );
	 	}

		// get information about billboards
		const CRenderState& bilboardState = treeRender->GetBillboardRenderState( (ERenderPass)i );
		GatherInformationAboutTextures( bilboardState, statistic );
	}
}

void SpeedTree::GatherInformationAboutTextures( const CRenderState& state, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& statistic )
{
	for( Uint32 i=0; i<TL_NUM_TEX_LAYERS; ++i )
	{
		const CTexture& textureClass = state.GetTextureClass( i );
		const CTextureGPUAPI& textureGPUAPI = textureClass.m_tTexturePolicy;

		const CRenderTexture* texture = textureGPUAPI.GetRenderTexture();
		if( texture == nullptr )
		{
			continue;
		}

		const GpuApi::TextureDesc& textureDesc = GpuApi::GetTextureDesc( texture->GetTextureRef() );

		Bool foundTheSameTexture = false;
		const Uint32 statisticCount = statistic.Size();
		for( Uint32 j=0; j<statisticCount; ++j )
		{
			if( statistic[j].m_textureDesc == textureDesc )
			{
				foundTheSameTexture = true;
				break;
			}
		}
		if( foundTheSameTexture == true )
		{
			continue;
		}

		SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats singleStat( textureDesc );
		singleStat.m_fileName = ANSI_TO_UNICODE( textureClass.GetFilename() );
		statistic.PushBack( singleStat );
	}
}

#endif

void CSpeedTreeInstanceRingBuffer::SetRingBufferSize( Uint32 size )
{
	m_size = size;
}

Bool CSpeedTreeInstanceRingBuffer::AppendData( Uint8* data, size_t size )
{
	RED_FATAL_ASSERT( m_isLocked, "The SpeedTree instance buffer wasn't locked and we want to write it now. BOOM!" );

	if ( m_currentOffset + size >= m_size )
	{
		RED_HALT( "Exceeded the size of the SpeedTree instancing ring buffer. Some foliage won't render." );
		return false;
	}
	
	Red::System::MemoryCopy( m_lockedData + m_currentOffset, data, size );
	m_currentOffset += (Int32)size;

	return true;
}

Bool CSpeedTreeInstanceRingBuffer::GetCurrentInstancePtr( Uint8*& outData, size_t size )
{
	RED_FATAL_ASSERT( m_isLocked, "The SpeedTree instance buffer wasn't locked and we want to write it now. BOOM!" );

	outData = ( Uint8* )(m_lockedData + m_currentOffset); 
	m_currentOffset += (Int32)size;

	return true;
}

Bool CSpeedTreeInstanceRingBuffer::Lock()
{
	RED_FATAL_ASSERT( !m_isLocked, "Locking a SpeedTree instance buffer that was already locked." );

	if ( !m_instanceBufferRef )
	{
		m_instanceBufferRef = GpuApi::CreateBuffer( m_size, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_instanceBufferRef, "SPT inst ring" );
		RED_FATAL_ASSERT( m_instanceBufferRef, "Failed to create SpeedTree instancing ring buffer!" );
	}
	
	if ( m_instanceBufferRef )
	{
		m_lockedData = static_cast< Uint8* >( GpuApi::LockBuffer( m_instanceBufferRef, GpuApi::BLF_Discard, 0, m_size ) );
		m_isLocked = true;
		m_currentOffset = 0;
	}

	return true;
}

void CSpeedTreeInstanceRingBuffer::Unlock()
{
	RED_FATAL_ASSERT( m_isLocked, "Unlocking a SpeedTree instance buffer that wasn't locked." );
	if ( m_instanceBufferRef )
	{
		GpuApi::UnlockBuffer( m_instanceBufferRef );
		m_isLocked = false;
		m_lockedData = nullptr;
	}
}

void CSpeedTreeInstanceRingBuffer::Bind( Uint32 reg, Uint32 offset, Uint32 stride )
{
	GpuApi::BindVertexBuffers( reg, 1, &m_instanceBufferRef, &stride, &offset );
}

void CSpeedTreeInstanceRingBuffer::ReleaseBuffer()
{
	GpuApi::SafeRelease( m_instanceBufferRef );
}

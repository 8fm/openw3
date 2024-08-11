/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderBinkVideo.h"
#include "../redSystem/cpuid.h"

#include "renderShaderPair.h"

#if defined(USE_BINK_VIDEO)

#include "../engine/videoPlayerBink.h"

const Float CRenderBinkVideo::REALTIME_BUFFER_FILL_PERCENTAGE = .8f;

CRenderBinkVideo::CRenderBinkVideo( const SVideoParamsBink& videoParams )
	: m_fileName( videoParams.m_fileName )
	, m_playLooped( videoParams.m_playLooped )
	, m_playRealtime( videoParams.m_playRealtime )
	, m_yPlaneDecodeThreadIndex( INVALID_BINK_THREAD_INDEX )
	, m_otherDecodeThreadIndex( INVALID_BINK_THREAD_INDEX )
	, m_hasValidFrame( false )
	, m_decodingFrame( false )
	, m_finishedVideo( false )
	, m_binkLoadSuspended( false )
	, m_gameLoadSuspended( false )
{
	Init();
}

CRenderBinkVideo::~CRenderBinkVideo()
{
	Cleanup();
}

void CRenderBinkVideo::Cleanup()
{
	UnsuspendLoadAll();
	WaitForDecode();
	StopDecodingThreads();
	ReleaseBinkResources();
	if ( m_bink )
	{
		BinkClose( m_bink );
	}
	m_bink = 0;
	m_hasValidFrame = false;
}

Bool CRenderBinkVideo::IsValid() const 
{ 
	return m_bink != 0 && ! m_finishedVideo;
}

void CRenderBinkVideo::StartNextFrame()
{
	LockMovieFrame();
	BinkDoFrameAsync( m_bink, m_yPlaneDecodeThreadIndex, m_otherDecodeThreadIndex );
	m_decodingFrame = true;
}

Bool CRenderBinkVideo::UpdateFrame()
{
	if ( m_finishedVideo )
	{
		return false;
	}

	Bool hasNewFrame = false;

	if ( ! BinkWait( m_bink ) )
	{
		if ( BinkDoFrameAsyncWait( m_bink, 1000 ) )
		{
			while ( BinkShouldSkip( m_bink ) )
			{
				BinkNextFrame( m_bink );
				BinkDoFrameAsync( m_bink, m_yPlaneDecodeThreadIndex, m_otherDecodeThreadIndex );
				BinkDoFrameAsyncWait( m_bink, -1 );
			}
			UnlockMovieFrame();
			hasNewFrame = true;
			m_decodingFrame = false;

			if ( m_playLooped ||  m_bink->FrameNum < m_bink->Frames )
			{
				// move to the next frame  
				BinkNextFrame( m_bink );
				StartNextFrame();
			}
			else
			{
				m_finishedVideo = true;
			}			
		}
	}
	return hasNewFrame;
}

void CRenderBinkVideo::Init()
{
	//TODO: Set sound track...

	StartDecodingThreads();

	String absolutePath;
	GDepot->GetAbsolutePath( absolutePath );
	absolutePath += TXT("movies\\");
	absolutePath += m_fileName;

	// E3 5.1 sound. Just hardcoding track numbers for now
	// since we need a more useful way to specify language tracks.
	//////////////////////////////////////////////////////////////////////////
	U32 TrackIDsToPlay[] =  { 0, 1, 2, 3 };
	BinkSetSoundTrack( ARRAYSIZE( TrackIDsToPlay ), TrackIDsToPlay );
	//////////////////////////////////////////////////////////////////////////

	m_bink = BinkOpen( UNICODE_TO_ANSI( absolutePath.AsChar() ), BINKNOFRAMEBUFFERS | BINKALPHA | BINKSNDTRACK );
	if ( ! m_bink )
	{
		VIDEO_ERROR( TXT("Failed to open bink video file '%ls'"), absolutePath.AsChar() );
		return;
	}

	// E3 5.1 sound.
	//////////////////////////////////////////////////////////////////////////
	U32 bins[ 2 ];
	bins[ 0 ] = 0;  // 0 is front left on XAudio
	bins[ 1 ] = 1;  // 1 is front right on XAudio
	BinkSetSpeakerVolumes( m_bink, 0, bins, 0, 2 );
	bins[ 0 ] = 2; // 2 is center on XAudio
	BinkSetSpeakerVolumes( m_bink, 1, bins, 0, 1 );
	bins[ 0 ] = 3; // 3 is sub on XAudio
	BinkSetSpeakerVolumes( m_bink, 2, bins, 0, 1 );
	bins[ 0 ] = 4;  // 4 is back left on XAudio
	bins[ 1 ] = 5;  // 5 is back right on XAudio
	BinkSetSpeakerVolumes( m_bink, 3, bins, 0, 2 );
	//////////////////////////////////////////////////////////////////////////


	BinkGetFrameBuffersInfo( m_bink, &m_binkBuffers );

	if ( ! CreateBinkResources() )
	{
		VIDEO_ERROR( TXT("Internal error. Failed to allocate rendering resources for bink video") );
		Cleanup();
	}

	StartNextFrame();
}

void CRenderBinkVideo::Render( CRenderFrame* frame )
{
	if ( m_bink == 0 )	// Thanks to that bink will be still rendering last frame after finishing movie until stopped from main thread
	{
		return;
	}
	
	m_hasValidFrame |= UpdateFrame();

	if ( m_hasValidFrame )
	{
		DrawMovieFrame( frame );
	}

	BalanceRealtimeBufferFill();
}

Bool CRenderBinkVideo::CreateBinkDrawTexture( TBinkSize width, TBinkSize height, GpuApi::TextureRef& outTex /*[out]*/ )
{
	GpuApi::TextureDesc texDesc;
	texDesc.width = static_cast< GpuApi::Uint32 >( width );
	texDesc.height = static_cast< GpuApi::Uint32 >( height );
	texDesc.initLevels = 1;
	texDesc.type = GpuApi::TEXTYPE_2D;
	texDesc.format = GpuApi::TEXFMT_A8;
	texDesc.usage = GpuApi::TEXUSAGE_Samplable;

	outTex = GpuApi::CreateTexture( texDesc, GpuApi::TEXG_System );
	ASSERT( outTex );
	GpuApi::SetTextureDebugPath( outTex, "Bink video plane" );
	return ! outTex.isNull();
}

Bool CRenderBinkVideo::CreateBinkResources()
{
	const size_t kTexMemAlign = 16;

	for ( Int32 i = 0; i < m_binkBuffers.TotalFrames; ++i )
	{
		SMappedBinkFrameBuffer& mappedBinkBuffer = m_mappedBinkBuffer[ i ];
		mappedBinkBuffer.m_yPlane = nullptr;
		mappedBinkBuffer.m_cRPlane = nullptr;
		mappedBinkBuffer.m_cBPlane = nullptr;
		mappedBinkBuffer.m_aPlane = nullptr;

		BINKFRAMEPLANESET& binkBuffer = m_binkBuffers.Frames[ i ];

		// Create Y plane
		if ( binkBuffer.YPlane.Allocate )
		{
			binkBuffer.YPlane.BufferPitch = ( m_binkBuffers.YABufferWidth + (kTexMemAlign - 1) ) & ~(kTexMemAlign - 1);
			mappedBinkBuffer.m_ySize = binkBuffer.YPlane.BufferPitch * m_binkBuffers.YABufferHeight;
			mappedBinkBuffer.m_yPlane = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_ySize, kTexMemAlign ) );
			ASSERT( mappedBinkBuffer.m_yPlane );
			ASSERT( ( reinterpret_cast< uintptr_t >( mappedBinkBuffer.m_yPlane ) & (kTexMemAlign - 1) ) == 0 );
			if ( ! mappedBinkBuffer.m_yPlane )
			{
				return false;
			}
		}

		// Create cR plane
		if ( binkBuffer.cRPlane.Allocate )
		{
			binkBuffer.cRPlane.BufferPitch = ( m_binkBuffers.cRcBBufferWidth + (kTexMemAlign - 1) ) & ~(kTexMemAlign - 1);
			mappedBinkBuffer.m_cRSize = binkBuffer.cRPlane.BufferPitch * m_binkBuffers.cRcBBufferHeight;
			mappedBinkBuffer.m_cRPlane = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_cRSize, kTexMemAlign ) );
			ASSERT( mappedBinkBuffer.m_cRPlane );
			ASSERT( ( reinterpret_cast< uintptr_t >( mappedBinkBuffer.m_cRPlane ) & (kTexMemAlign - 1) ) == 0 );
			if ( ! mappedBinkBuffer.m_cRPlane )
			{
				return false;
			}
		}

		// Create cB plane
		if ( binkBuffer.cBPlane.Allocate )
		{
			binkBuffer.cBPlane.BufferPitch = ( m_binkBuffers.cRcBBufferWidth + (kTexMemAlign - 1) ) & ~(kTexMemAlign - 1);
			mappedBinkBuffer.m_cBSize = binkBuffer.cBPlane.BufferPitch * m_binkBuffers.cRcBBufferHeight;
			mappedBinkBuffer.m_cBPlane = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_cBSize, kTexMemAlign ) );
			ASSERT( mappedBinkBuffer.m_cBPlane );
			ASSERT( ( reinterpret_cast< uintptr_t >( mappedBinkBuffer.m_cBPlane ) & (kTexMemAlign - 1) ) == 0 );
			if ( ! mappedBinkBuffer.m_cBPlane )
			{
				return false;
			}
		}

		// Create alpha plane
		if ( binkBuffer.APlane.Allocate )
		{
			binkBuffer.APlane.BufferPitch = ( m_binkBuffers.YABufferWidth + (kTexMemAlign - 1) ) & ~(kTexMemAlign - 1);
			mappedBinkBuffer.m_aSize = binkBuffer.APlane.BufferPitch * m_binkBuffers.YABufferHeight;
			mappedBinkBuffer.m_aPlane = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_aSize, kTexMemAlign ) );
			ASSERT( mappedBinkBuffer.m_aPlane );
			ASSERT( ( reinterpret_cast< uintptr_t >( mappedBinkBuffer.m_aPlane ) & (kTexMemAlign - 1) ) == 0 );
			if ( ! mappedBinkBuffer.m_aPlane )
			{
				return false;
			}
		}
	}

	// Create the draw textures
	if ( m_binkBuffers.Frames[ 0 ].YPlane.Allocate && ! CreateBinkDrawTexture( m_binkBuffers.YABufferWidth, m_binkBuffers.YABufferHeight, m_drawTexture.m_yTexture ) )
	{
		return false;
	}
	if ( m_binkBuffers.Frames[ 0 ].cRPlane.Allocate && ! CreateBinkDrawTexture( m_binkBuffers.cRcBBufferWidth, m_binkBuffers.cRcBBufferHeight, m_drawTexture.m_cRTexture ) )
	{
		return false;
	}
	if ( m_binkBuffers.Frames[ 0 ].cBPlane.Allocate && ! CreateBinkDrawTexture( m_binkBuffers.cRcBBufferWidth, m_binkBuffers.cRcBBufferHeight, m_drawTexture.m_cBTexture ) )
	{
		return false;
	}
	if ( m_binkBuffers.Frames[ 0 ].APlane.Allocate && ! CreateBinkDrawTexture( m_binkBuffers.YABufferWidth, m_binkBuffers.YABufferHeight, m_drawTexture.m_aTexture ) )
	{
		return false;
	}

	// Create constant buffer
	m_cbuffer = GpuApi::CreateBuffer( BINK_CBUFFER_SIZE * sizeof(Float), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	ASSERT( m_cbuffer );
	if ( m_cbuffer.isNull() )
	{
		return false;
	}
	GpuApi::SetBufferDebugPath( m_cbuffer, "Bink cbuffer" );

	// Setup up the vertices for a full screen quad.
	GpuApi::SystemVertex_Pos2UVColor vertices[4];
	vertices[ 0 ].m_position[0]	= -1.f;
	vertices[ 0 ].m_position[1]	= -1.f;
	vertices[ 0 ].m_uv[0]		= 0.f;
	vertices[ 0 ].m_uv[1]		= 1.f;
	vertices[ 1 ].m_position[0]	= -1.f;
	vertices[ 1 ].m_position[1]	= 1.f;
	vertices[ 1 ].m_uv[0]		= 0.f;
	vertices[ 1 ].m_uv[1]		= 0.f;
	vertices[ 2 ].m_position[0]	= 1.f;
	vertices[ 2 ].m_position[1]	= -1.f;
	vertices[ 2 ].m_uv[0]		= 1.f;
	vertices[ 2 ].m_uv[1]		= 1.f;
	vertices[ 3 ].m_position[0]	= 1.f;
	vertices[ 3 ].m_position[1]	= 1.f;
	vertices[ 3 ].m_uv[0]		= 1.f;
	vertices[ 3 ].m_uv[1]		= 0.f;
	m_vertexBuffer = GpuApi::CreateBuffer( ARRAYSIZE( vertices ) * sizeof( GpuApi::SystemVertex_Pos2UVColor ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, vertices );
	ASSERT( m_vertexBuffer );
	if ( m_vertexBuffer.isNull() )
	{
		return false;
	}
	GpuApi::SetBufferDebugPath( m_vertexBuffer, "Bink vertex buffer" );

	GpuApi::Uint16 indexes[] = { 0, 1, 2, 2, 1, 3 };
	m_indexBuffer = GpuApi::CreateBuffer( ARRAYSIZE( indexes ) * 2, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, indexes );
	ASSERT( m_indexBuffer );
	if ( m_indexBuffer.isNull() )
	{
		return false;
	}
	GpuApi::SetBufferDebugPath( m_indexBuffer, "Bink index buffer" );

	return true;
}

void CRenderBinkVideo::ReleaseBinkResources()
{
	GpuApi::SafeRelease( m_cbuffer );
	GpuApi::SafeRelease( m_vertexBuffer );
	GpuApi::SafeRelease( m_indexBuffer );
	
	for (Int32 i = 0; i < BINKMAXFRAMEBUFFERS; ++i )
	{
		SMappedBinkFrameBuffer& mappedBinkBuffer = m_mappedBinkBuffer[ i ];
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_yPlane );
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_cRPlane );
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_cBPlane );
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBink, mappedBinkBuffer.m_aPlane );
		mappedBinkBuffer.m_yPlane = nullptr;
		mappedBinkBuffer.m_cRPlane = nullptr;
		mappedBinkBuffer.m_cBPlane = nullptr;
		mappedBinkBuffer.m_aPlane = nullptr;
	}

	GpuApi::SafeRelease( m_drawTexture.m_yTexture );
	GpuApi::SafeRelease( m_drawTexture.m_cRTexture );
	GpuApi::SafeRelease( m_drawTexture.m_cBTexture );
	GpuApi::SafeRelease( m_drawTexture.m_aTexture );
}

void CRenderBinkVideo::LockMovieFrame()
{
	BINKFRAMEPLANESET* binkBufferFrames = m_binkBuffers.Frames;

	for ( Int32 i = 0; i < m_binkBuffers.TotalFrames; ++i )
	{
		binkBufferFrames[ i ].YPlane.Buffer = m_mappedBinkBuffer[ i ].m_yPlane;
		binkBufferFrames[ i ].cRPlane.Buffer = m_mappedBinkBuffer[ i ].m_cRPlane;
		binkBufferFrames[ i ].cBPlane.Buffer = m_mappedBinkBuffer[ i ].m_cBPlane;
		binkBufferFrames[ i ].APlane.Buffer = m_mappedBinkBuffer[ i ].m_aPlane;
	}
	BinkRegisterFrameBuffers( m_bink, &m_binkBuffers );
}

void CRenderBinkVideo::UnlockMovieFrame()
{
	const BINKFRAMEPLANESET& srcFrame = m_binkBuffers.Frames[ m_binkBuffers.FrameNum ];
	Int32 numRects = BinkGetRects( m_bink, BINKSURFACEFAST );
	if ( numRects > 0 )
	{
		for ( Int32 i = 0; i < numRects; ++i )
		{
			const BINKRECT& br = m_bink->FrameRects[ i ];
			GpuApi::Rect rc( br.Left, br.Top, br.Left + br.Width, br.Top + br.Height );
			
			GpuApi::LoadTextureData2D( m_drawTexture.m_yTexture, 0, 0, &rc,
				static_cast< Uint8* >( srcFrame.YPlane.Buffer ) + rc.top * srcFrame.YPlane.BufferPitch + rc.left,
				srcFrame.YPlane.BufferPitch );
			if ( ! m_drawTexture.m_aTexture.isNull() )
			{
				GpuApi::LoadTextureData2D( m_drawTexture.m_aTexture, 0, 0, &rc,
					static_cast< Uint8* >( srcFrame.APlane.Buffer ) + rc.top * srcFrame.APlane.BufferPitch + rc.left,
					srcFrame.APlane.BufferPitch );
			}

			// Other planes are half the size
			rc.left >>= 1;
			rc.top >>= 1;
			rc.right >>= 1;
			rc.bottom >>= 1;

			GpuApi::LoadTextureData2D( m_drawTexture.m_cRTexture, 0, 0, &rc,
				static_cast< Uint8* >( srcFrame.cRPlane.Buffer ) + rc.top * srcFrame.cRPlane.BufferPitch + rc.left,
				srcFrame.cRPlane.BufferPitch );
			GpuApi::LoadTextureData2D( m_drawTexture.m_cBTexture, 0, 0, &rc,
				static_cast< Uint8* >( srcFrame.cBPlane.Buffer ) + rc.top * srcFrame.cBPlane.BufferPitch + rc.left,
				srcFrame.cBPlane.BufferPitch );
		}
	}
}

void CRenderBinkVideo::DrawMovieFrame( CRenderFrame* frame )
{
	if ( m_bink == 0 )	// Thanks to that bink will be still rendering last frame after finishing movie until stopped from main thread
	{
		return;
	}

	GpuApi::ViewportDesc vp;
	vp.minZ     = 0.0f;
	vp.maxZ     = 0.0f;
	vp.x		= 0;
	vp.y		= 0;
	vp.width    = frame->GetFrameInfo().GetWidth();
	vp.height   = frame->GetFrameInfo().GetHeight();
	GpuApi::SetViewport( vp );

	// Turn on texture filtering and texture clamping
	GpuApi::eSamplerStatePreset preset = GpuApi::SAMPSTATEPRESET_ClampLinearNoMip;
	GpuApi::SetSamplerStatePreset( 0, preset );
	GpuApi::SetSamplerStatePreset( 1, preset );
	GpuApi::SetSamplerStatePreset( 2, preset );
	GpuApi::SetSamplerStatePreset( 3, preset );

	// are we using an alpha plane? if so, turn on the 4th texture and set our pixel shader
	if ( ! m_drawTexture.m_aTexture.isNull() )
	{
		// Set the textures.
		GpuApi::TextureRef refs[4] = { m_drawTexture.m_yTexture, m_drawTexture.m_cRTexture, m_drawTexture.m_cBTexture, m_drawTexture.m_aTexture };
		GpuApi::BindTextures( 0, 4, &(refs[0]), GpuApi::PixelShader );
		// turn on our pixel shader
		GetRenderer()->m_binkRGBA->Bind();
	}
	else
	{
		// Set the textures.
		GpuApi::TextureRef refs[3] = { m_drawTexture.m_yTexture, m_drawTexture.m_cRTexture, m_drawTexture.m_cBTexture };
		GpuApi::BindTextures( 0, 3, &(refs[0]), GpuApi::PixelShader );
		// turn on our pixel shader
		GetRenderer()->m_binkRGB->Bind();
	}

	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_2D, m_drawTexture.m_aTexture.isNull() ? 1 : -1 );

	SBinkShaderParams shaderParams = {
		m_bink->Width,	// Movie width
		m_bink->Height,	// Movie height
		1.0f,			// Alpha level
		1.0f,			// X scale
		1.0f,			// Y scale
		0.0f,			// X offset
		0.0f,			// Y offset
		false,			// Premultiplied alpha
	};

	SetShaderParams( shaderParams );

	const Int32 quadVertexCount = 6;
	const Int32 quadPrimCount = 2;
	GpuApi::BindIndexBuffer( m_indexBuffer );

	Uint32 offset = 0;
	Uint32 stride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPos2UVColor );

	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos2UVColor, true );
	GpuApi::BindVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, quadVertexCount, 0, quadPrimCount );

	// clear the textures
	GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );
}

void CRenderBinkVideo::SetShaderParams( const SBinkShaderParams& shaderParams )
{
	ASSERT( m_cbuffer );
	ASSERT( m_bink );

	GpuApi::BufferDesc cbDesc;
	GpuApi::GetBufferDesc( m_cbuffer, cbDesc );
	void* mappedPtr = GpuApi::LockBuffer( m_cbuffer, GpuApi::BLF_Discard, 0, cbDesc.size );
	ASSERT( mappedPtr );
	Float* cbufferData = static_cast< Float* >( mappedPtr );

	// PS alpha consts
	Int32 cbIdx = 0;
	cbufferData[cbIdx++] = shaderParams.m_isPremultAlpha ? shaderParams.m_alphaLevel : 1.0f;
	cbufferData[cbIdx++] = cbufferData[0];
	cbufferData[cbIdx++] = cbufferData[1];
	cbufferData[cbIdx++] = shaderParams.m_alphaLevel;

	// VS consts
	cbufferData[cbIdx++] = shaderParams.m_xScale * 2.0f;
	cbufferData[cbIdx++] = -shaderParams.m_yScale * 2.0f;
	cbufferData[cbIdx++] = shaderParams.m_xOffset * 2.0f / (Float)(Int32) shaderParams.m_width - 1.0f;
	cbufferData[cbIdx++] = 1.0f - shaderParams.m_yOffset * 2.0f / (Float)(Int32)shaderParams.m_height;

	GpuApi::UnlockBuffer( m_cbuffer );

	GpuApi::BindConstantBuffer( 0, m_cbuffer, GpuApi::PixelShader );
	GpuApi::BindConstantBuffer( 0, m_cbuffer, GpuApi::VertexShader );
}

void CRenderBinkVideo::WaitForDecode()
{
	// Need to wait for the frame to finish or else BinkClose can hang, even though you stopped and "waited" for
	// the async threads to end.
	if ( m_decodingFrame )
	{
		BinkDoFrameAsyncWait( m_bink, -1 );
	}
}

void CRenderBinkVideo::StartDecodingThreads()
{
	// TOOD: Choose appropriate thread index depending on the platform (e.g., PC vs console).
	if ( m_yPlaneDecodeThreadIndex == INVALID_BINK_THREAD_INDEX )
	{
		BinkStartAsyncThread( 0, nullptr );
		m_yPlaneDecodeThreadIndex = 0;
	}

	if ( m_otherDecodeThreadIndex == INVALID_BINK_THREAD_INDEX )
	{
		if ( Red::System::CpuId::GetInstance().GetNumberOfLogicalCores() > 1 )
		{
			BinkStartAsyncThread( 1, nullptr );
			m_otherDecodeThreadIndex = 1;
		}
		else
		{
			m_otherDecodeThreadIndex = m_yPlaneDecodeThreadIndex;
		}
	}
}

void CRenderBinkVideo::StopDecodingThreads()
{
	if ( m_yPlaneDecodeThreadIndex != INVALID_BINK_THREAD_INDEX )
	{
		BinkRequestStopAsyncThread( m_yPlaneDecodeThreadIndex );
	}
	if ( m_otherDecodeThreadIndex != INVALID_BINK_THREAD_INDEX && m_otherDecodeThreadIndex != m_yPlaneDecodeThreadIndex )
	{
		BinkRequestStopAsyncThread( m_otherDecodeThreadIndex );
	}
	if ( m_yPlaneDecodeThreadIndex != INVALID_BINK_THREAD_INDEX )
	{
		BinkWaitStopAsyncThread( m_yPlaneDecodeThreadIndex );
	}
	if ( m_otherDecodeThreadIndex != INVALID_BINK_THREAD_INDEX && m_otherDecodeThreadIndex != m_yPlaneDecodeThreadIndex )
	{
		BinkWaitStopAsyncThread( m_otherDecodeThreadIndex );
	}
	m_yPlaneDecodeThreadIndex = INVALID_BINK_THREAD_INDEX;
	m_otherDecodeThreadIndex = INVALID_BINK_THREAD_INDEX;
}

void CRenderBinkVideo::BalanceRealtimeBufferFill()
{
	if ( ! m_playRealtime || ! IsValid() )
	{
		return;
	}

	BINKREALTIME rt;
	BinkGetRealtime( m_bink, &rt, 0 );
	
	const Float fillRatio = (Float)rt.ReadBufferUsed / rt.ReadBufferSize;
	if ( fillRatio > REALTIME_BUFFER_FILL_PERCENTAGE )
	{		
		SuspendBinkLoad();
	}
	else
	{
		ResumeBinkLoad();
	}
}

void CRenderBinkVideo::SuspendBinkLoad()
{
	TrySuspendBinkLoad();
	TryResumeGameLoad();
}

void CRenderBinkVideo::TrySuspendBinkLoad()
{
	if ( ! m_binkLoadSuspended )
	{
		m_binkLoadSuspended = true;
		BinkControlBackgroundIO( m_bink, BINKBGIOSUSPEND );
	}
}

void CRenderBinkVideo::ResumeBinkLoad()
{
	TrySuspendGameLoad();
	TryResumeBinkLoad();	
}

void CRenderBinkVideo::TrySuspendGameLoad()
{
	if ( ! m_gameLoadSuspended )
	{
		m_gameLoadSuspended = true;
		SJobManager::GetInstance().Lock();
		//CSystem::SystemFileAccessLock();
	}
}

void CRenderBinkVideo::TryResumeBinkLoad()
{
	if ( m_binkLoadSuspended )
	{
		m_binkLoadSuspended = false;
		BinkControlBackgroundIO( m_bink, BINKBGIORESUME );
	}
}

void CRenderBinkVideo::TryResumeGameLoad()
{
	if ( m_gameLoadSuspended )
	{
		m_gameLoadSuspended = false;
		//CSystem::SystemFileAccessUnlock();
		SJobManager::GetInstance().Unlock();
	}	
}

void CRenderBinkVideo::UnsuspendLoadAll()
{
	TryResumeGameLoad();
	TryResumeBinkLoad();
}

#endif //! USE_BINK_VIDEO
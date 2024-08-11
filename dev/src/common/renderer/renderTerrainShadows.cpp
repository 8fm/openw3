/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTerrainShadows.h"
#include "renderProxyTerrain.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderShaderPair.h"
#include "../core/taskManager.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSettings.h"

//-----------------------------------------------------------------------------

// How many segments each patch is divided into. For best quality, should be PATCH_COVERAGE, but for better performance, lower.
// TODO : Dropping PATCH_SIZE to half drops the GPU time significantly, has some graphical artifacts that need to be resolved first.
static const Uint32 PATCH_SIZE		= 64;
// How many terrain texels each patch should cover. Determines how many patches are used to cover the terrain.
static const Uint32 PATCH_COVERAGE	= 64;
// The maximum number of instances that the patch drawer may need to draw at once.
static const Uint32 MAX_INSTANCES	= 4096;

//-----------------------------------------------------------------------------

namespace Config
{
	TConfigVar<Float> cvTerrainShadowsDegreesLimit		( "Rendering", "TerrainShadowsDegreesLimit",		15 );
}

//-----------------------------------------------------------------------------

Vector CalculateTerrainLimitedLightDirection( const Vector &lightDir )
{
	Vector CalculateLimitedShadowDirection( const Vector &shadowDir, Float degreesLimit );

	Vector limitedDir = CalculateLimitedShadowDirection( -lightDir, Config::cvTerrainShadowsDegreesLimit.Get() );
	return Vector ( -limitedDir.X, -limitedDir.Y, -limitedDir.Z, 1 );
}

//-----------------------------------------------------------------------------

#if USE_TERRAIN_SHADOW_CULLING

CRenderTerrainShadows::CullingBuffer::CullingBuffer( Uint32 size )
	: m_size( size )
	, m_data( NULL )
	, m_mem( NULL )
	, m_age( 1 )
	, m_updating( false )
{
	// allocate memory, make sure that the data pointer will be aligned to 16 bytes
	m_mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderVisibility , 15 + m_size*m_size* sizeof(Float));
	m_data = AlignPtr( (const Float*) m_mem, 16 );

	// initialize empty window
	m_rect = Box();

	// Offset and scale to help convert world space position into pixels
	m_offsetX = m_offsetY = 0.0f;
	m_scaleX = m_scaleY = 0.0f;
}

CRenderTerrainShadows::CullingBuffer::~CullingBuffer()
{
	ASSERT( !m_updating );

	// free temporary buffer
	if ( m_mem != NULL )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_RenderVisibility, (void*)m_mem );
		m_mem = NULL;
		m_data = NULL;
	}
}

void CRenderTerrainShadows::CullingBuffer::GrabData( const CTask& jobToken, const void* srcData, Uint32 srcPitch, const Box& worldRect )
{
	// Lock
	ASSERT( !m_updating );
	m_updating = true;

	// Invalidate all the previously calculated results
	m_age.Increment();

	// Update window
	m_rect = worldRect;

	// Calculate the mapping params
	m_offsetX = -worldRect.Min.X;
	m_offsetY = -worldRect.Min.Y;
	m_scaleX = (1.0f / ( worldRect.Max.X - worldRect.Min.X )) * m_size;
	m_scaleY = (1.0f / ( worldRect.Max.Y - worldRect.Min.Y )) * m_size;

	// We shift the height values form terrain to account for shadow smoothing
	const Float heightEpsilon = 10.0f;
	__m128 eps = _mm_set_ps1( heightEpsilon ); 

	// Convert the values from float 16 to float 32, take maximum of two values as we go
	const Float* readPtr = (const Float*) srcData;
	Float* writePtr = (Float*) m_data;
	for ( Uint32 y=0; y<m_size; ++y )
	{
		const Float* saveReadPtr = readPtr;
		for ( Uint32 x=0; x<(m_size/4); ++x, readPtr += 4, writePtr += 4 )
		{
			// TODO: THIS shall be rewriten for other platforms that Windows

			// Load and bias four at a time
			__m128 p0 = _mm_load_ps( readPtr );
			__m128 m = _mm_sub_ps( p0, eps);

			// write without poluting cache
			_mm_stream_ps( writePtr, m ); 
		}

		// move to next row
		readPtr = (const Float*)saveReadPtr + (srcPitch/4);
	}

	// End update
	m_updating = false;
}

CRenderTerrainShadows::CullingBuffer::QueryResult CRenderTerrainShadows::CullingBuffer::TestBox( const CTask& jobToken, Uint32 age, const Box& box ) const
{
	// Already outdated
	if ( (Int32)age < m_age.GetValue() )
	{
		return Outdated;
	}

	// Still updating
	if ( m_updating ) 
	{
		return NotReady;
	}

	// Calculate pixel area
	const Int32 minX = Clamp<Int32>( (Int32)floorf(( box.Min.X + m_offsetX ) * m_scaleX), 0, m_size-1 );
	const Int32 maxY = (m_size-1) - Clamp<Int32>( (Int32)floorf(( box.Min.Y + m_offsetY ) * m_scaleY), 0, m_size-1 );
	const Int32 maxX = Clamp<Int32>( (Int32)ceilf(( box.Max.X + m_offsetX ) * m_scaleX), 0, m_size-1 );
	const Int32 minY = (m_size-1) - Clamp<Int32>( (Int32)ceilf(( box.Max.Y + m_offsetY ) * m_scaleY), 0, m_size-1 );

	// Outside the buffer
	if ( maxX <= minX || maxY <= minY )
	{
		return NotCulled;
	}

	// Get the test height
	const Float testHeight = box.Max.Z;
	const Float* testPtr = (const Float*) m_data + minX + minY*m_size;
	for ( Int32 y=minY; y<=maxY; ++y )
	{
		const Float* savePtr = testPtr;

		// test pixels
		for ( Int32 x=minX; x<=maxX; ++x, ++testPtr )
		{
			if ( testHeight > testPtr[0] )
			{
				return NotCulled;
			}
		}

		// next row
		testPtr = savePtr + m_size;
	}

	// all pixels of the box are below the shadow line, box is occluded by shadow
	return Culled;
}

#endif

//-----------------------------------------------------------------------------


class CRenderTerrainShadowsPatchDrawer
{
	// Single vertex in the patch mesh.
	struct Vertex
	{
		Float x, y;
		Float u, v;

		Vertex ()
		{}

		Vertex ( Float newX, Float newY, Float newU, Float newV )
			: x( newX ), y( newY )
			, u( newU ), v( newV )
		{}
	};

	// Per-instance data, describing the particular patch
	struct Instance
	{
		Vector Data0;
		Vector Data1;
		Vector Data2;
	};


	GpuApi::BufferRef			m_terrainPatchVertices;
	GpuApi::BufferRef			m_terrainPatchIndices;
	GpuApi::BufferRef			m_terrainPatchInstances;
	GpuApi::VertexLayoutRef		m_terrainPatchLayout;

	Instance*					m_lockedInstanceData;

	Uint32						m_instanceStart;
	Uint32						m_numInstances;

	Uint32						m_patchSize;
	Uint32						m_maxInstances;

public:
	Bool Init( Uint32 patchSize, Uint32 maxInstances )
	{
		m_numInstances = 0;

		if ( m_patchSize != patchSize || !m_terrainPatchVertices || !m_terrainPatchIndices )
		{
			SafeRelease( m_terrainPatchVertices );
			SafeRelease( m_terrainPatchIndices );
			m_patchSize = patchSize;

			// Create patch vertices
			TDynArray< Vertex > vertices;
			vertices.Resize( (m_patchSize+1)*(m_patchSize+1) );
			for ( Uint32 y=0; y<=m_patchSize; ++y )
			{
				for ( Uint32 x=0; x<=m_patchSize; ++x )
				{
					Vertex& v = vertices[x + (y*(m_patchSize+1) )];
					v.u = ( 0.5f + (Float)x ) / (Float) m_patchSize;
					v.v = ( 0.5f + (Float)y ) / (Float) m_patchSize;
					v.x = (Float)x / (Float)m_patchSize;
					v.y = (Float)y / (Float)m_patchSize;
				}
			}

			// Create patch indices
			TDynArray< Uint16 > indices;
			indices.Resize( m_patchSize*m_patchSize*6 );
			Uint16* writeIndexPtr = &indices[0];
			for ( Uint32 y=0; y<m_patchSize; ++y )
			{
				for ( Uint32 x=0; x<m_patchSize; ++x, writeIndexPtr += 6 )
				{
					Uint16 a = (Uint16)( ((x+0) + (y+0)*(m_patchSize+1)) );
					Uint16 b = (Uint16)( ((x+1) + (y+0)*(m_patchSize+1)) );
					Uint16 c = (Uint16)( ((x+1) + (y+1)*(m_patchSize+1)) );
					Uint16 d = (Uint16)( ((x+0) + (y+1)*(m_patchSize+1)) );

					writeIndexPtr[0] = c;
					writeIndexPtr[1] = b;
					writeIndexPtr[2] = a;

					writeIndexPtr[3] = d;
					writeIndexPtr[4] = c;
					writeIndexPtr[5] = a;
				}
			}

			// Create vertex buffer
			{
				const Uint32 vertexDataSize = vertices.Size() * sizeof(Vertex);
				GpuApi::BufferInitData bufInitData;
				bufInitData.m_buffer = &vertices[0];
				m_terrainPatchVertices = GpuApi::CreateBuffer( vertexDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
				GpuApi::SetBufferDebugPath( m_terrainPatchVertices, "terrainPatchVB" );
			}
			
			// Create index buffer
			{
				const Uint32 indexDataSize = indices.Size() * sizeof(Uint16);
				GpuApi::BufferInitData bufInitData;
				bufInitData.m_buffer = &indices[0];
				m_terrainPatchIndices = GpuApi::CreateBuffer( indexDataSize, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
				GpuApi::SetBufferDebugPath( m_terrainPatchIndices, "terrainPatchIB" );
			}
		}

		if ( !m_terrainPatchLayout )
		{
			using namespace GpuApi::VertexPacking;

			PackingElement LayoutElements[] =
			{
				{	PT_Float2,		PS_Position,	0,			0,	ST_PerVertex	},
				{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},

				{	PT_Float4,		PS_ExtraData,	0,			1, ST_PerInstance	},
				{	PT_Float4,		PS_ExtraData,	1,			1, ST_PerInstance	},
				{	PT_Float4,		PS_ExtraData,	2,			1, ST_PerInstance	},

				PackingElement::END_OF_ELEMENTS
			};

			GpuApi::VertexLayoutDesc desc;
			desc.AddElements( LayoutElements );
			m_terrainPatchLayout = GpuApi::CreateVertexLayout( desc );
		}

		if ( m_maxInstances != maxInstances || !m_terrainPatchInstances )
		{
			SafeRelease( m_terrainPatchInstances );

			m_maxInstances = maxInstances;

			const Uint32 instanceDataSize = m_maxInstances * sizeof( Instance );
			m_terrainPatchInstances = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, nullptr );
			GpuApi::SetBufferDebugPath( m_terrainPatchInstances, "terrainPatchInst" );

			m_instanceStart = 0;
		}

		return m_terrainPatchVertices && m_terrainPatchIndices && m_terrainPatchLayout && m_terrainPatchInstances;
	}

	~CRenderTerrainShadowsPatchDrawer()
	{
		GpuApi::SafeRelease(m_terrainPatchVertices);
		GpuApi::SafeRelease(m_terrainPatchIndices);
		GpuApi::SafeRelease(m_terrainPatchInstances);
		GpuApi::SafeRelease(m_terrainPatchLayout);
	}

	Bool BeginInstances( Uint32 maxInstances )
	{
		if ( !m_terrainPatchInstances )
		{
			RED_HALT( "Patch Drawer was not initialized before using" );
			return false;
		}

		if ( maxInstances > m_maxInstances )
		{
			RED_HALT( "maxInstances is too big (%u > %u)! Might need to raise the limit", maxInstances, m_maxInstances );
			return false;
		}

		RED_ASSERT( m_lockedInstanceData == nullptr );

		Uint32 lockFlag = GpuApi::BLF_NoOverwrite;
		if ( m_instanceStart + maxInstances > m_maxInstances )
		{
			lockFlag = GpuApi::BLF_Discard;
			m_instanceStart = 0;
		}

		Uint32 startBytes = m_instanceStart * sizeof( Instance );
		Uint32 sizeBytes = maxInstances * sizeof( Instance );
		m_lockedInstanceData = (Instance*)GpuApi::LockBuffer( m_terrainPatchInstances, lockFlag, startBytes, sizeBytes);
		m_numInstances = 0;

		return m_lockedInstanceData != nullptr;
	}

	void EndInstances()
	{
		RED_ASSERT( m_lockedInstanceData != nullptr, TXT("EndInstances called when buffer was not successfully locked.") );

		GpuApi::UnlockBuffer( m_terrainPatchInstances );
		m_lockedInstanceData = nullptr;
	}

	void AddInstance( const Vector& offset, const Vector& scale, const Vector2& uvOffset, const Vector2& uvScale, Uint32 level )
	{
		RED_ASSERT( m_lockedInstanceData != nullptr, TXT("AddInstance called when buffer was not successfully locked.") );

		m_lockedInstanceData->Data0 = offset;
		m_lockedInstanceData->Data1 = Vector( scale.X, scale.Y, scale.Z, (Float)level );
		m_lockedInstanceData->Data2 = Vector( uvOffset.X, uvOffset.Y, uvScale.X, uvScale.Y );

		++m_lockedInstanceData;
		++m_numInstances;
	}

	void DrawTerrainPatches( GpuApi::TextureRef clipmap )
	{
		if ( !m_terrainPatchIndices || !m_terrainPatchVertices || !m_terrainPatchInstances || !m_terrainPatchLayout )
		{
			RED_HALT( "Patch Drawer was not initialized before drawing" );
			return;
		}

		if ( m_numInstances == 0 )
		{
			return;
		}

		
		GpuApi::BindTextures( 0, 1, &clipmap, GpuApi::VertexShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::VertexShader );


		GetRenderer()->m_shaderTerrainPatchDraw->Bind();
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef(), RST_PixelShader );


		const Uint32 instSize = GpuApi::GetVertexLayoutStride( m_terrainPatchLayout, 1 );
		Uint32 offsets[2] = { 0, m_instanceStart * instSize };
		Uint32 strides[2] = { GpuApi::GetVertexLayoutStride( m_terrainPatchLayout, 0 ), instSize };
		GpuApi::BufferRef buffers[2] = { m_terrainPatchVertices, m_terrainPatchInstances };
		GpuApi::SetVertexFormatRaw( m_terrainPatchLayout );
		GpuApi::BindVertexBuffers(0, 2, buffers, strides, offsets);

		GpuApi::BindIndexBuffer( m_terrainPatchIndices );

		GpuApi::DrawInstancedIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, (m_patchSize+1)*(m_patchSize+1), 0, m_patchSize*m_patchSize*2, m_numInstances );

		m_instanceStart += m_numInstances;
	}



	void DrawTerrain2DPatch( GpuApi::TextureRef shadowmap, const CRenderCamera& shadowCamera, GpuApi::TextureRef clipmap, Int32 level, const Vector& offset, const Vector& scale, Float u0, Float v0, Float u1, Float v1 )
	{
		// Set texture
		GpuApi::TextureRef pixelRefs[2] = { shadowmap, clipmap };
		GpuApi::BindTextures( 0, 2, &(pixelRefs[0]), GpuApi::PixelShader );

		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Bind shader
		GetRenderer()->m_shaderTerrainShadowCalc->Bind();

		// Set the drawing data
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, offset );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, scale );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector( (Float)level, 0.0f, 0.0f, 0.0f ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector( u0, v0, 1.0f / (u1-u0), 1.0f / (v1-v0) ) );

		// Extract sun camera projection matrix ( to calculate shadowmap coordinates )
		const Matrix worldToSunShadowmap = shadowCamera.GetWorldToScreen();
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_Matrix, worldToSunShadowmap );	

		// Assemble vertices
		DebugVertexUV points[6];
		Color color(1,1,1,1);
		points[2].Set( Vector( 0, 0, 0.50f ), color, u0, v0 );
		points[1].Set( Vector( 1, 0, 0.50f ), color, u1, v0 );
		points[0].Set( Vector( 1, 1, 0.50f ), color, u1, v1 );
		points[5].Set( Vector( 0, 0, 0.50f ), color, u0, v0 );
		points[4].Set( Vector( 1, 1, 0.50f ), color, u1, v1 );
		points[3].Set( Vector( 0, 1, 0.50f ), color, u0, v1 );

		// Draw
		GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );

		// unbind textures
		GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
	}

};


//-----------------------------------------------------------------------------

CRenderTerrainShadows::CRenderTerrainShadows()
	: m_patchDrawer( nullptr )
	, m_terrainProxy( nullptr )
	, m_terrainScene( nullptr )
	, m_terrainTextureRange( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_fullUpdateRequest( false )
	, m_isVisible( true )
#if USE_TERRAIN_SHADOW_CULLING
	, m_cullingBuffer( nullptr )
	, m_cullingBufferUpdateRequested( false )
	, m_dataCopiedToStagingBuffer( false )
	, m_cullingBufferUpdateJob( nullptr )
#endif
{
	// Use standard shadowmap resolution
	m_terrainShadowResolution = 1024;

	// Create the shadowmap depth texture
	GpuApi::TextureDesc shadowmapDesc;
	shadowmapDesc.type = GpuApi::TEXTYPE_2D;
	shadowmapDesc.format = GpuApi::TEXFMT_D16U;
	shadowmapDesc.width = m_terrainShadowResolution * 2; // larger than terrain shadow map to prevent aliasing
	shadowmapDesc.height = m_terrainShadowResolution * 2;
	shadowmapDesc.initLevels = 1;
	shadowmapDesc.sliceNum = 1;
	shadowmapDesc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_DepthStencil;
	m_shadowDepthBuffer = GpuApi::CreateTexture( shadowmapDesc, GpuApi::TEXG_TerrainShadow );
	GpuApi::AddDynamicTexture( m_shadowDepthBuffer, "TerrainDepthMap" );
	GpuApi::SetTextureDebugPath( m_shadowDepthBuffer, "TerrainDepthMap" );

#ifdef RED_PLATFORM_DURANGO
	// Create a second texture, in esram. This will not cause higher memory usage (besides any texture overhead).
	shadowmapDesc.esramOffset = 0;
	shadowmapDesc.usage |= GpuApi::TEXUSAGE_ESRAMResident;
	m_shadowDepthBufferEsram = GpuApi::CreateTexture( shadowmapDesc, GpuApi::TEXG_System );
	GpuApi::AddDynamicTexture( m_shadowDepthBufferEsram, "TerrainDepthMap (ESRAM)" );
#else
	GpuApi::SafeRefCountAssign( m_shadowDepthBufferEsram, m_shadowDepthBuffer );
#endif

	// Create the terrain shadow texture
	// Use 16-bit unorm here. Only the terrain itself goes into the shadows texture, so we don't have to worry about
	// actual surface points outside of the terrain height range. It does assume that there won't be objects above the
	// terrain's maximum height (or below minimum height) that need terrain shadows...
	GpuApi::TextureDesc terrainShadowDesc;
	terrainShadowDesc.type = GpuApi::TEXTYPE_ARRAY;
	terrainShadowDesc.format = GpuApi::TEXFMT_Uint_16_norm;
	terrainShadowDesc.width = m_terrainShadowResolution;
	terrainShadowDesc.height = m_terrainShadowResolution;
	terrainShadowDesc.initLevels = 1;
	terrainShadowDesc.sliceNum = 5; // TODO: why do we have 5 levels of clipmap ?
	terrainShadowDesc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	m_shadowTexture = GpuApi::CreateTexture( terrainShadowDesc, GpuApi::TEXG_TerrainShadow );
	GpuApi::AddDynamicTexture( m_shadowTexture, "TerrainShadowMap" );
	GpuApi::SetTextureDebugPath( m_shadowTexture, "TerrainShadowMap" );

#if USE_TERRAIN_SHADOW_CULLING
	// Create staging texture for coping data from GPU to CPU
	GpuApi::TextureDesc copyBufferDesc;
	copyBufferDesc.type = GpuApi::TEXTYPE_2D;
	copyBufferDesc.format = GpuApi::TEXFMT_Float_R32; // expanded version
	copyBufferDesc.width = terrainShadowDesc.width;
	copyBufferDesc.height = terrainShadowDesc.height;
	copyBufferDesc.initLevels = 1;
	copyBufferDesc.sliceNum = 1;
	copyBufferDesc.usage = GpuApi::TEXUSAGE_Staging;
	m_copyBuffer = GpuApi::CreateTexture( copyBufferDesc, GpuApi::TEXG_TerrainShadow );
	GpuApi::SetTextureDebugPath( m_copyBuffer, "TerrainShadowCopy" );

	// Create proxy culling buffer
	m_cullingBuffer = new CullingBuffer( m_terrainShadowResolution );
#endif
}

CRenderTerrainShadows::~CRenderTerrainShadows()
{
	BindToTerrainProxy( nullptr, nullptr );

	GpuApi::SafeRelease( m_shadowDepthBuffer );
	GpuApi::SafeRelease( m_shadowDepthBufferEsram );
	GpuApi::SafeRelease( m_shadowTexture );

#if USE_TERRAIN_SHADOW_CULLING
	GpuApi::SafeRelease( m_copyBuffer );

	if ( m_cullingBufferUpdateJob )
	{
		m_cullingBufferUpdateJob->TryCancel();
		m_cullingBufferUpdateJob->Release();
		m_cullingBufferUpdateJob = NULL;
	}

	delete m_cullingBuffer;
#endif

	delete m_patchDrawer;
}

void CRenderTerrainShadows::RequestFullUpdate()
{
	m_fullUpdateRequest = true;
}

bool CRenderTerrainShadows::IsValidForScene( const CRenderSceneEx* scene ) const
{
	return (m_terrainProxy != NULL) && (scene == m_terrainScene);
}

void CRenderTerrainShadows::BindToTerrainProxy( CRenderProxy_Terrain* proxy, CRenderSceneEx* scene )
{
	// New proxy
	if ( proxy != m_terrainProxy )
	{
		// Release previous proxy
		if ( m_terrainProxy )
		{
			GpuApi::RemoveDynamicTexture( m_terrainProxy->GetHeightMapArray() );
			m_terrainProxy->Release();
		}

		// Release previous clipmap texture
		if ( m_terrainClipmapTexture )
		{
			GpuApi::SafeRelease( m_terrainClipmapTexture );
			m_terrainClipmapTexture = GpuApi::TextureRef::Null();
		}

		// Set new proxy
		m_terrainProxy = proxy;
		m_terrainScene = scene;

		// Attach to new proxy
		if ( m_terrainProxy )
		{
			// Keep extra reference to terrain
			m_terrainProxy->AddRef();

			// Register terrain clipmap in the dynamic texture preview
			GpuApi::AddDynamicTexture( m_terrainProxy->GetHeightMapArray(), "ClipMap" );

			// Read the height resolution
			GpuApi::TextureDesc heightMapDesc = GpuApi::GetTextureDesc( m_terrainProxy->GetHeightMapArray() );
			m_terrainHeightResolution = heightMapDesc.width;

			// Limit the number of levels evaluated to the number of heightmap levels
			m_clipmapLevels = Min<Uint32>( heightMapDesc.sliceNum, 5 );

			// Reset the clipmap windows
			m_shadowTextureWindows.Resize( m_clipmapLevels );
			for ( Uint32 i=0; i<m_clipmapLevels; ++i )
			{
				m_shadowTextureWindows[i].m_isValid = false;
				m_shadowTextureWindows[i].m_lightDirection = Vector::ZEROS;
				m_shadowTextureWindows[i].m_worldRect = Box();
			}

			// Get clipmap texture
			m_terrainClipmapTexture = m_terrainProxy->GetHeightMapArray();
			GpuApi::AddRef( m_terrainClipmapTexture );

			// Extract original windows
			const Uint32 numWindows = m_terrainProxy->GetClipmapWindows().Size();
			m_terrainTextureWindows.Resize( numWindows );
			for ( Uint32 i=0; i<numWindows; ++i )
			{
				const SClipmapWindow& wnd = m_terrainProxy->GetClipmapWindows()[i];

				// initialize
				m_terrainTextureWindows[i].m_isValid = true;
				m_terrainTextureWindows[i].m_lightDirection = Vector::ZEROS;
				m_terrainTextureWindows[i].m_worldRect.Min.X = wnd.m_worldRect.Min.X;
				m_terrainTextureWindows[i].m_worldRect.Min.Y = wnd.m_worldRect.Min.Y;
				m_terrainTextureWindows[i].m_worldRect.Max.X = wnd.m_worldRect.Max.X;
				m_terrainTextureWindows[i].m_worldRect.Max.Y = wnd.m_worldRect.Max.Y;
				m_terrainTextureWindows[i].m_validTexels = wnd.m_validTexels;
			}

			// Get terrain range
			const Float minHeight = m_terrainProxy->GetLowestElevation();
			const Float maxHeight = m_terrainProxy->GetHighestElevation();
			m_terrainTextureRange.Set4( maxHeight - minHeight, minHeight, 0.0f, 0.0f ); 
		}

		if ( m_patchDrawer == nullptr )
		{
			m_patchDrawer = new CRenderTerrainShadowsPatchDrawer();
		}

	}
}

static bool MatchWorldRect( const Box& a, const Box& b )
{
	if ( Abs( a.Min.X - b.Min.X ) > 0.01f ) return false;
	if ( Abs( a.Min.Y - b.Min.Y ) > 0.01f ) return false;
	if ( Abs( a.Max.X - b.Max.X ) > 0.01f ) return false;
	if ( Abs( a.Max.Y - b.Max.Y ) > 0.01f ) return false;
	return true;
}

void CRenderTerrainShadows::PrepareShadows( const CRenderCollector& collector )
{
	PC_SCOPE_RENDER_LVL1( TerrainShadowsPrepare );

	// No proxy to process
	if ( !m_terrainProxy )
	{
		return;
	}

	RED_ASSERT( m_patchDrawer != nullptr );
	m_patchDrawer->Init( PATCH_SIZE, MAX_INSTANCES );

	// Get frame info
	const CRenderFrameInfo& frameInfo = collector.GetRenderFrameInfo();

#if USE_TERRAIN_SHADOW_CULLING
	// Job update
	if ( m_cullingBufferUpdateJob )
	{
		// Current job has ended
		if ( m_cullingBufferUpdateJob->IsFinished() )
		{
			// Realase job pointer
			m_cullingBufferUpdateJob->Release();
			m_cullingBufferUpdateJob = NULL;

			// Unmap the stating buffer
			GpuApi::UnlockLevel( m_copyBuffer, 0, 0 );
		}
	}

	// We request an new update of the culling buffer
	if ( !m_cullingBufferUpdateJob && m_cullingBufferUpdateRequested )
	{
		// Copy the shadow data to staging texture
		if ( !m_dataCopiedToStagingBuffer )
		{
			GetRenderer()->CopyTextureData( m_copyBuffer, 0, 0, m_shadowTexture, 0, 0 );
			m_dataCopiedToStagingBuffer = true;
		}

		// If we have valid data try to lock it and copy
		if ( m_dataCopiedToStagingBuffer )
		{
			// Try to lock(map) the staging buffer
			Uint32 srcDataPitch = 0;
			void* srcDataPtr = GpuApi::LockLevel( m_copyBuffer, 0, 0, GpuApi::BLF_Read | GpuApi::BLF_DoNotWait, srcDataPitch );
			if ( srcDataPtr != NULL )
			{
				// Create update job
				const Box& worldRect = m_shadowTextureWindows[0].m_worldRect;
				m_cullingBufferUpdateJob = new ( CTask::Root ) CJobCopyTerrainShadowBuffer( srcDataPtr, srcDataPitch, worldRect, m_cullingBuffer );		
				GTaskManager->Issue( *m_cullingBufferUpdateJob );

				// Reset update flag ( we may accept new updates )
				m_cullingBufferUpdateRequested = false;
			}
		}
	}
#endif

	// Determine the number of clipmap level we CAN update
	const Uint32 allowedClipmapLevels = Config::cvMaxTerrainShadowAtlasCount.Get();
	const Uint32 clipmapLevelsToUpdate = Min< Uint32 >( allowedClipmapLevels, m_clipmapLevels );

	// Check each terrain region for update
	const Vector curLightDir = CalculateTerrainLimitedLightDirection( frameInfo.m_baseLightingParameters.m_lightDirection );
	const Float lightDirSimilarity = cosf( DEG2RAD( 1.0f ) );
	Bool distanceRequirementSatisfied = false;
	for ( Uint16 i=0; i<clipmapLevelsToUpdate; ++i )
	{
		// Distance requirement was satisfied, we do not need to update any more levels
		if ( distanceRequirementSatisfied ) 
		{
			// Mark all skipped levels as invalid
			while ( i<clipmapLevelsToUpdate )
			{
				m_shadowTextureWindows[i++].m_isValid = false;
			}

			break;
		}
		 
		// Get the source clipmap size
		const SClipmapWindow& srcWnd = m_terrainProxy->GetClipmapWindows()[i];
		const Float wndSize = ( srcWnd.m_worldRect.Max.X - srcWnd.m_worldRect.Min.X );
		if ( wndSize > ( 2.0f * frameInfo.m_terrainShadowsDistance ) )
		{
			distanceRequirementSatisfied = true;
		}

 		// Refresh the level only if not valid or the light direction or clipmap window changed
		const Window& wnd = m_shadowTextureWindows[i];
		if ( m_fullUpdateRequest ||  // forced update
			!wnd.m_isValid || // shadowmap level not valid
			( Vector::Dot3( wnd.m_lightDirection, curLightDir ) < lightDirSimilarity ) ||  // light direction changed
			!MatchWorldRect( wnd.m_worldRect, srcWnd.m_worldRect ) ) // world region changed
		{
			RefreshLevel( collector, frameInfo, i );
		}
	}

	// Update terrain clipmap windows
	{
		// Extract original windows
		for ( Uint32 i=0; i<m_terrainTextureWindows.Size(); ++i )
		{
			const SClipmapWindow& wnd = m_terrainProxy->GetClipmapWindows()[i];

			// copy bounds
			m_terrainTextureWindows[i].m_worldRect.Min.X = wnd.m_worldRect.Min.X;
			m_terrainTextureWindows[i].m_worldRect.Min.Y = wnd.m_worldRect.Min.Y;
			m_terrainTextureWindows[i].m_worldRect.Max.X = wnd.m_worldRect.Max.X;
			m_terrainTextureWindows[i].m_worldRect.Max.Y = wnd.m_worldRect.Max.Y;
			m_terrainTextureWindows[i].m_validTexels = wnd.m_validTexels;
		}

		// Get terrain range
		const Float minHeight = m_terrainProxy->GetLowestElevation();
		const Float maxHeight = m_terrainProxy->GetHighestElevation();
		m_terrainTextureRange.Set4( maxHeight - minHeight, minHeight, 0.0f, 0.0f ); 
	}

	// Reset forced update flag
	m_fullUpdateRequest = false;
}


void CRenderTerrainShadows::DrawTerrainLevel( const CRenderCamera& camera, Uint32 level, const Box* excludeBox ) const
{
	// Calculate texel resolution
	const Int32 texelResolution = m_terrainHeightResolution;
	const Int32 chunkResolution = (texelResolution + (PATCH_COVERAGE-1)) / PATCH_COVERAGE;

	// Chunk size, get from the proxy ( original data not the cached one )
	const Box& window = m_terrainProxy->GetClipmapWindows()[level].m_worldRect;
	const Float xsize = (Float)( window.Max.X - window.Min.X ) / (Float)chunkResolution;
	const Float ysize = (Float)( window.Max.Y - window.Min.Y ) / (Float)chunkResolution;

	const Vector& validTexels = m_terrainProxy->GetClipmapWindows()[level].m_validTexels;
	// UV step
	const Float usize = ( validTexels.Z - validTexels.X ) / (Float)chunkResolution;
	const Float vsize = ( validTexels.W - validTexels.Y ) / (Float)chunkResolution;

	// Terrain scale
	const Float zOffset = m_terrainProxy->GetLowestElevation();
	const Float zScale = ( m_terrainProxy->GetHighestElevation() - m_terrainProxy->GetLowestElevation() );


	if ( !m_patchDrawer->BeginInstances( chunkResolution*chunkResolution ) )
	{
		ERR_RENDERER( TXT("BeginInstances failed. Cannot generate terrain patch instances.") );
		return;
	}

	CFrustum frust;
	frust.InitFromCamera( camera.GetWorldToScreen() );

	const Vector scale( xsize, ysize, zScale, 0.0f );

	// draw terrain slice
	Float yofs = window.Min.Y;
	Float vofs = validTexels.Y;
	for ( Int32 y=0; y<chunkResolution; ++y, yofs += ysize, vofs += vsize )
	{
		Float xofs = window.Min.X;
		Float uofs = validTexels.X;
		for ( Int32 x=0; x<chunkResolution; ++x, xofs += xsize, uofs += usize )
		{
			const Vector offset( xofs, yofs, zOffset, 1.0f );

			// test simple exclusion
			if ( excludeBox )
			{
				if ( offset.X >= excludeBox->Min.X && offset.Y >= excludeBox->Min.Y &&
					(offset.X + scale.X) <= excludeBox->Max.X && (offset.Y + scale.Y) <= excludeBox->Max.Y )
				{
					continue;
				}
			}

			Float minHeight, maxHeight;
			m_terrainProxy->GetHeightRangeInArea( offset.AsVector2(), ( offset + scale ).AsVector2(), minHeight, maxHeight );

			Box patchBox( offset, offset + scale );
			patchBox.Min.Z = minHeight;
			patchBox.Max.Z = maxHeight;

			if ( frust.TestBox( patchBox ) == 0 )
			{
				continue;
			}

			m_patchDrawer->AddInstance( offset, scale, Vector2( uofs, vofs ), Vector2( usize, vsize ), level );
		}
	}

	m_patchDrawer->EndInstances();

	m_patchDrawer->DrawTerrainPatches( m_terrainProxy->GetHeightMapArray() );
}

void CRenderTerrainShadows::CalculateSunCamera( const CRenderFrameInfo& frameInfo, Uint32 level, CRenderCamera& outSunCamera ) const
{
	// Always center camera on the center of the given clipmap level
	const Box& clipmapLevelSize = m_shadowTextureWindows[ level ].m_worldRect;
	const Float centerX = ( clipmapLevelSize.Min.X + clipmapLevelSize.Max.X ) / 2.0f;
	const Float centerY = ( clipmapLevelSize.Min.Y + clipmapLevelSize.Max.Y ) / 2.0f;

	// Center the camera in Z direction so it can cover the whole height range
	const Float minHeight = m_terrainProxy->GetLowestElevation();
	const Float maxHeight = m_terrainProxy->GetHighestElevation();
	const Float centerZ = ( minHeight + maxHeight ) / 2.0f;

	// Calculate camera direction based on the lighting direction
	EulerAngles cameraDirection = CalculateTerrainLimitedLightDirection( frameInfo.m_baseLightingParameters.m_lightDirection ).ToEulerAngles();
	cameraDirection.Yaw += 180.0f; // WHY DO WE NEED THIS ????

	// Camera zoom - should be large enough to contain the whole level
	Float cameraZoom = 1.f;
	{
		Vector corners[8];
		const Box clipmapLevelBox ( Vector ( clipmapLevelSize.Min.X, clipmapLevelSize.Min.Y, minHeight, 1.f ), Vector ( clipmapLevelSize.Max.X, clipmapLevelSize.Max.Y, maxHeight, 1.f ) );
		clipmapLevelBox.CalcCorners( corners );

		Vector dirForward, dirRight, dirUp;
		CRenderCamera::CalcCameraVectors( cameraDirection, dirForward, dirRight, dirUp );
		
		Float rightMin	= dirRight.Dot3( corners[0] );
		Float rightMax	= rightMin;
		Float upMin		= dirUp.Dot3( corners[0] );
		Float upMax		= upMin;
		for ( Uint32 corner_i=1; corner_i<ARRAY_COUNT(corners); ++corner_i )
		{
			Float currRightValue = dirRight.Dot3( corners[corner_i] );
			rightMin = Min( rightMin, currRightValue );
			rightMax = Max( rightMax, currRightValue );
			
			Float currUpValue = dirUp.Dot3( corners[corner_i] );
			upMin = Min( upMin, currUpValue );
			upMax = Max( upMax, currUpValue );
		}

		cameraZoom = Max( rightMax - rightMin, upMax - upMin );
		
		// TODO, OPTIMIZE: rectangle shaped projection would be more efficient.		
	}

	// Prepare sun camera
	const Vector cameraCenter( centerX, centerY, centerZ );

	// Calculate final sun camera
	CRenderCamera sunCamera( 
		cameraCenter, 
		cameraDirection, 
		0.0f,  // FOV=0 means ortho projection
		1.0f,  // square aspect ratio
		-20000.0f,  // TOTO: fake near plane for now
		20000.0f,	// TOTO: fake far plane for now
		cameraZoom );

	// Return the calculated camera
	outSunCamera = sunCamera;
}

void CRenderTerrainShadows::PrepareShadowMap( const CRenderCollector& collector, const CRenderFrameInfo& frameInfo, Uint32 level, CRenderCamera& outSunCamera ) const
{
	PC_SCOPE_RENDER_LVL1( CRenderTerrainShadows_PrepareShadowMap );
	// Set draw context for drawing a shadowmap
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_ShadowMapGenCSM_Terrain );

	// Calculate sun camera
	CalculateSunCamera( frameInfo, level, outSunCamera );

	// Bind render target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetNullColorTarget();
	rtSetup.SetDepthStencilTarget( m_shadowDepthBufferEsram );
	rtSetup.SetViewportFromTarget( m_shadowDepthBufferEsram );
	GpuApi::SetupRenderTargets( rtSetup );

	// Clear the depth target
	ASSERT( !GpuApi::IsReversedProjectionState() );
	GetRenderer()->ClearDepthTarget( m_shadowDepthBufferEsram, 1.0f );

	// Setup global camera parameters
	GetRenderer()->GetStateManager().SetLocalToWorld( NULL );
	GetRenderer()->GetStateManager().SetCamera( outSunCamera );

	// Draw the regions that contribute to the shadowmap
	// Draw also all of the lower LOD regions
	Box excludedAreaRect;
	for ( Uint32 drawLevel=level; drawLevel<m_clipmapLevels; ++drawLevel )
	{	
		// when drawing make sure that the are already covered by higher quality slicdes
		// are not being redrawn
		DrawTerrainLevel( outSunCamera, drawLevel, (drawLevel > level) ? &excludedAreaRect : NULL );
		excludedAreaRect = m_shadowTextureWindows[ level ].m_worldRect;
	}
}

void CRenderTerrainShadows::RefreshLevel( const CRenderCollector& collector, const CRenderFrameInfo& frameInfo, Uint16 level )
{
	PC_SCOPE_RENDER_LVL1( CRenderTerrainShadows_RefreshLevel );
	// Update shadowmap data
	const TDynArray< SClipmapWindow >& windows = m_terrainProxy->GetClipmapWindows();
	const SClipmapWindow& clipmapWindow = windows[ level ];

	// Update the shadow clipmap window
	m_shadowTextureWindows[ level ].m_isValid = true;
	m_shadowTextureWindows[ level ].m_worldRect = clipmapWindow.m_worldRect;
	m_shadowTextureWindows[ level ].m_validTexels = clipmapWindow.m_validTexels;
	m_shadowTextureWindows[ level ].m_lightDirection = CalculateTerrainLimitedLightDirection( frameInfo.m_baseLightingParameters.m_lightDirection );

	// Prepare shadowmap
	CRenderCamera sunCamera;
	CalculateSunCamera( frameInfo, level, sunCamera );

	// Calculate terrain drawing range
	const Vector terrainOffset( clipmapWindow.m_worldRect.Min.X, clipmapWindow.m_worldRect.Min.Y, m_terrainProxy->GetLowestElevation() );
	const Vector terrainScale( 
		clipmapWindow.m_worldRect.Max.X - clipmapWindow.m_worldRect.Min.X,
		clipmapWindow.m_worldRect.Max.Y - clipmapWindow.m_worldRect.Min.Y,
		m_terrainProxy->GetHighestElevation() - m_terrainProxy->GetLowestElevation() );

	// UV range - update the whole slice ( that's so gooooood! ), where there are valid clipmap texels.
	const Float u0 = m_shadowTextureWindows[ level ].m_validTexels.X;
	const Float v0 = m_shadowTextureWindows[ level ].m_validTexels.Y;
	const Float u1 = m_shadowTextureWindows[ level ].m_validTexels.Z;
	const Float v1 = m_shadowTextureWindows[ level ].m_validTexels.W;

	// Render the terrain only shadowmap
	PrepareShadowMap( collector, frameInfo, level, sunCamera );
	
	// Prepare shadowmap buffer for given tile
	{
		PC_SCOPE_RENDER_LVL1( CRenderTerrainShadows_RefreshLevel_Terrain );
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Bind render targets
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, m_shadowTexture, level ); // SLICE is the same as the level we are updating
		rtSetup.SetViewportFromTarget( m_shadowTexture );
		GpuApi::SetupRenderTargets( rtSetup );

		// Calculate the highest occluded height
		m_patchDrawer->DrawTerrain2DPatch( 
			m_shadowDepthBufferEsram,
			sunCamera,
			m_terrainProxy->GetHeightMapArray(),
			level,
			terrainOffset,
			terrainScale,
			u0, v0, u1, v1 );
	}

	// Unbind textures
	GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );

#if USE_TERRAIN_SHADOW_CULLING
	// Initialize the copying of the texture to the CPU ( for shadow culling )
	if ( level == 0 )
	{
		// Start copying data as soon as possible
		if ( !m_cullingBufferUpdateJob )
		{
			GetRenderer()->CopyTextureData( m_copyBuffer, 0, 0, m_shadowTexture, 0, 0 );
			m_dataCopiedToStagingBuffer = true;
		}
		else
		{
			// cancel current job!
			m_dataCopiedToStagingBuffer = false; // we still need to copy data after the job is canceled
			m_cullingBufferUpdateJob->TryCancel();
		}

		// Request an update job to be scheduled once we map the buffer
		m_cullingBufferUpdateRequested = true;
	}
#endif
}

//-----------------------------------------------------------------------------

CRenderProxyShadowCulling::CRenderProxyShadowCulling( CRenderTerrainShadows* terrainShadows, const Box& initialBox )
	: m_age( 0 )
	, m_validResult( 0 )
	, m_shadowCulledByTerrain( 0 )
	, m_worldBox( initialBox )	
	, m_terrainShadows( terrainShadows )
	, m_job( NULL )
{
}

CRenderProxyShadowCulling::~CRenderProxyShadowCulling()
{
}

void CRenderProxyShadowCulling::UpdateBoundingBox( const Box& newBox )
{
#if USE_TERRAIN_SHADOW_CULLING
	// Cancel current job
	if ( m_job )
	{
		m_job->TryCancel();
		m_job->Release();
		m_job = NULL;
	}

	// Invalidate result
	m_validResult = 0;
	m_worldBox = newBox;
#endif
}

bool CRenderProxyShadowCulling::IsShadowCulledByTerrain()
{
#if USE_TERRAIN_SHADOW_CULLING

	// Process the job
	if ( m_job && m_job->IsFinished() )
	{
		// Valid result ?
		if ( ! m_job->IsCancelled() )
		{
			// Valid age of the culling buffer ? ( nothing has changed while we were waiting )
			if ( m_job->GetAge() == m_terrainShadows->GetCullingBuffer().GetAge() )
			{
				// Get the result
				m_validResult = 1;
				m_age = m_job->GetAge();
				m_shadowCulledByTerrain = m_job->GetResult();
			}
		}

		// Release the job
		m_job->Release();
		m_job = NULL;
	}

	// Invalidate when the height buffer changed
	if ( m_age != m_terrainShadows->GetCullingBuffer().GetAge() )
	{
		// invalidate
		m_validResult = 0;
	}

	// Valid result, use
	if ( m_validResult )
	{
		return m_shadowCulledByTerrain;
	}

	// We don't have a valid result and we do not have a processing job
	if ( !m_job )
	{
		// Create a new job that will be used to evaluate the culling
		const Uint32 age = m_terrainShadows->GetCullingBuffer().GetAge();
		m_job = new ( CTask::Root ) CJobTestShadowOcclusion( &m_terrainShadows->GetCullingBuffer(), m_worldBox );
		GTaskManager->Issue( *m_job );
	}

	// While the culling job is not completed draw the proxy as if it is casting shadows
	return false;

#else
	return false;
#endif
}

CRenderProxyShadowCulling* CRenderTerrainShadows::CreateShadowCullingProxy( const Box& initialBoundingBox )
{
#if USE_TERRAIN_SHADOW_CULLING
	return new CRenderProxyShadowCulling( this, initialBoundingBox );
#else
	return nullptr;
#endif
}


//-----------------------------------------------------------------------------

#if USE_TERRAIN_SHADOW_CULLING

CJobCopyTerrainShadowBuffer::CJobCopyTerrainShadowBuffer( const void* srcData, Uint32 srcPitch, const Box& worldRect, CRenderTerrainShadows::CullingBuffer* target )
	: m_srcData( srcData )
	, m_srcPitch( srcPitch )
	, m_target( target )
	, m_worldRect( worldRect )
{
}

CJobCopyTerrainShadowBuffer::~CJobCopyTerrainShadowBuffer()
{
}

void CJobCopyTerrainShadowBuffer::Run()
{
	PC_SCOPE_PIX( CJobCopyTerrainShadowBuffer );

	m_target->GrabData( *this, m_srcData, m_srcPitch, m_worldRect );
}
		
//-----------------------------------------------------------------------------

CJobTestShadowOcclusion::CJobTestShadowOcclusion( const CRenderTerrainShadows::CullingBuffer* target, const Box& box )
	: m_target( target )
	, m_age( target->GetAge() )
	, m_box( box )
	, m_result( false )
{
}

CJobTestShadowOcclusion::~CJobTestShadowOcclusion()
{
}

void CJobTestShadowOcclusion::Run()
{
	PC_SCOPE_PIX( CJobTestShadowOcclusion );

	for (;;)
	{
		// Test the box culling
		CRenderTerrainShadows::CullingBuffer::QueryResult ret = m_target->TestBox( *this, m_age, m_box );
		if ( ret == CRenderTerrainShadows::CullingBuffer::NotReady )
		{
			// Still waiting for result
			Red::Threads::SleepOnCurrentThread(1);
		}
		else if ( ret == CRenderTerrainShadows::CullingBuffer::Outdated )
		{
			// Outdated ( new buffer in on the way )
			m_result = false;
			return;// JR_Failed;
		}
		else if ( ret == CRenderTerrainShadows::CullingBuffer::Culled )
		{
			// Culled
			m_result = true;
			return;// JR_Finished;
		}
		else if ( ret == CRenderTerrainShadows::CullingBuffer::NotCulled )
		{
			// Not culled
			m_result = false;
			return;// JR_Finished;
		}
	}
}

#endif
//-----------------------------------------------------------------------------


/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderProxyTerrain.h"
#include "..\..\common\engine\clipMap.h"

#include "renderShaderPair.h"
#include "renderTerrainShadows.h"
#include "renderTerrainUpdateData.h"
#include "renderGrassUpdateData.h"
#include "renderTextureArray.h"
#include "renderMaterial.h"
#include "renderQuery.h"
#include "renderScene.h"
#include "renderElementMap.h"
#include "..\engine\baseEngine.h"
#include "..\engine\renderFragment.h"
#include "..\engine\materialDefinition.h"
#include "..\engine\materialGraph.h"
#include "..\engine\renderSettings.h"
#include "..\engine\umbraScene.h"
#include "..\engine\terrainUtils.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"

#define ENABLE_GRASS_MAP_DEBUGGING

#define NUM_TESS_BLOCKS_PER_PATCH ( TERRAIN_PATCH_RES / TERRAIN_TESS_BLOCK_RES )

#define TERRAIN_PATCH_INSTANCES_BUFFER_SIZE 8192

#define MAX_QUADTREE_STACK_NODES 32


// Maximum depth underwater for terrain to be rendered.
// TODO : This could come from the environment or something. In that case, just look for where it's used (BuildQuadTree).
// CRenderFrameInfo is available there, which has some environment stuff and such.
#define MAXIMUM_RENDER_DEPTH 40.0f

namespace Config
{
	extern TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvTerrainLateAllocVSLimit;
}

CTerrainTemplateMesh::CTerrainTemplateMesh( Uint32 res, Float size )
{
	ASSERT( size > 0.0f );

	// Gpu tesselation generates all quad vertices from one, corner vertex. This way we only prepare corner vertices for the template mesh.
	m_vertexResolution = res;
	Uint32 numVerts = res * res;
	m_numFaces = res * res;
	
	// No indices needed

	// Reserve memory
	m_vertices.Resize( numVerts );

	// Compute corner of the terrain in world space
	const Float cornerX = -size / 2.0f;
	const Float cornerY = -size / 2.0f;

	// Compute x/y distance between vertices
	Float stepWorldSpace = size / (Float)( res + 1 );
	Float stepNorm = 1.0f / (Float)m_vertexResolution;

	// Fill vertex buffer
	Float currentY = cornerY;
	Float v = 0.0f;
	for ( Uint32 y=0; y<m_vertexResolution; ++y )
	{
		Float u = 0.0f;
		Float currentX = cornerX;
		for ( Uint32 x=0; x<m_vertexResolution; ++x )
		{
			GpuApi::SystemVertex_Terrain& vert = m_vertices[ y * m_vertexResolution + x ];

			vert.m_patchCorner[0] = u;
			vert.m_patchCorner[1] = v;

			currentX += stepWorldSpace;
			u += stepNorm;
		}

		v += stepNorm;
		currentY += stepWorldSpace;
	}

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = m_vertices.Data();
	m_vertexBuffer = GpuApi::CreateBuffer( numVerts * sizeof(GpuApi::SystemVertex_Terrain), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	GpuApi::SetBufferDebugPath( m_vertexBuffer, "terrainTemplate" );
}

CTerrainTemplateMesh::~CTerrainTemplateMesh()
{
	GpuApi::SafeRelease( m_vertexBuffer );
};

//////////////////////////////////////////////////////////////////////////

CSkirtTemplateMesh::CSkirtTemplateMesh( )
{
	Uint16 indices[30] = {0,3,1,0,2,3,2,5,3,2,4,5,4,7,5,4,6,7,6,9,7,6,8,9,8,11,9,8,10,11};

	m_vertices.Resize( 12 );
	for( Uint32 i = 0; i < 6; ++i )
	{
		/* Skirt instance:
				0---2---4---6---8--10
				| \	| \	| \	| \ | \ |
				1---3---5---7---9--11
		*/

		//m_patchCorner[0] - upper (0) / lower (1); m_patchCorner[1] - index (0,1,2,3,4)
		m_vertices[2*i].m_patchCorner[0] = 0.0f;
		m_vertices[2*i].m_patchCorner[1] = (Float)i;

		m_vertices[(i<<1)+1].m_patchCorner[0] = 1.0f; 
		m_vertices[(i<<1)+1].m_patchCorner[1] = (Float)i;
	}

	{
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = m_vertices.Data();
		m_vertexBuffer = GpuApi::CreateBuffer( m_vertices.Size() * sizeof(GpuApi::SystemVertex_Terrain), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	}
	
	{
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = &indices;
		m_indexBuffer = GpuApi::CreateBuffer( 30 * sizeof(GpuApi::Uint16), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
	}
}

CSkirtTemplateMesh::~CSkirtTemplateMesh()
{
	GpuApi::SafeRelease( m_vertexBuffer );
	GpuApi::SafeRelease( m_indexBuffer );
};

//////////////////////////////////////////////////////////////////////////

struct CompareTerrainPatchDistance
{
	Vector2 m_cameraPosition2d;

	CompareTerrainPatchDistance( const Vector2& cameraPosition2d )
		: m_cameraPosition2d( cameraPosition2d )
	{}

	RED_FORCE_INLINE Bool operator() ( const STerrainPatchInstanceData& patchA, const STerrainPatchInstanceData& patchB ) const
	{
		// Sort by clip level first.
		const Uint32 patchALevel = ( Uint32 )patchA.m_patchBias.Z;
		const Uint32 patchBLevel = ( Uint32 )patchB.m_patchBias.Z;
		if ( patchALevel != patchBLevel )
		{
			return patchALevel < patchBLevel;
		}
	
		// Then by distance, front to back.
		const Float patchADistanceSquare = ( m_cameraPosition2d - patchA.m_patchBias.AsVector2() ).SquareMag();
		const Float patchBDistanceSquare = ( m_cameraPosition2d - patchB.m_patchBias.AsVector2() ).SquareMag();

		return patchADistanceSquare < patchBDistanceSquare;
	}
};

//////////////////////////////////////////////////////////////////////////

static CRenderTextureArray* ExtractTerrainDiffuseTextureArray( CRenderMaterial *material )
{
	if ( !material )
	{
		return NULL;
	}

	Int32 diffuseTexParamOffset = -1;
	for ( Uint32 i=0; i<material->m_pixelParameters.Size(); ++i )
	{
		const IMaterialDefinition::Parameter &param = material->m_pixelParameters[i];
		if ( CMaterialGraph::PT_TextureArray == param.m_type && CNAME( diffuse ) == param.m_name )
		{
			diffuseTexParamOffset = (Int32)param.m_offset;
			break;
		}	 
	}

	CRenderTextureArray *diffuseArray = nullptr;
	if ( -1 != diffuseTexParamOffset && diffuseTexParamOffset < (Int32)material->m_pixelData.Size() )
	{
		IRenderResource *res = * (IRenderResource**) &material->m_pixelData[diffuseTexParamOffset];
		diffuseArray = static_cast<CRenderTextureArray*>( res );
	}

	return diffuseArray;
}

//////////////////////////////////////////////////////////////////////////


void SClipmapWindow::SetValidTexels( const Rect& rect, Uint32 clipSize )
{
	Float clipSizeF = ( Float )( clipSize );
	m_validTexels.X = rect.m_left == 0 ? 0.0f : (Float)rect.m_left / clipSizeF;
	m_validTexels.Y = rect.m_top == 0 ? 0.0f : (Float)rect.m_top / clipSizeF;
	m_validTexels.Z = rect.m_right == (Int32)clipSize ? 1.0f : (Float)(rect.m_right-1) / clipSizeF;
	m_validTexels.W = rect.m_bottom == (Int32)clipSize ? 1.0f : (Float)(rect.m_bottom-1) / clipSizeF;
}


Bool SClipmapWindow::IsAllValid() const
{
	return m_validTexels.X == 0 && m_validTexels.Y == 0 && m_validTexels.Z == 1.0f && m_validTexels.W == 1.0f;
}


//////////////////////////////////////////////////////////////////////////

STerrainPigmentData::STerrainPigmentData ()
{}

STerrainPigmentData::~STerrainPigmentData ()
{
	Release();
}

Bool STerrainPigmentData::IsInit()
{
	return !m_texture.isNull();
}

void STerrainPigmentData::Init( Uint32 resolution )
{
	ASSERT ( resolution > 0 );

	Release();

	// Create heightmap stamp texture
	GpuApi::TextureDesc texDesc;
	texDesc.type		= GpuApi::TEXTYPE_2D;
	texDesc.format		= GpuApi::TEXFMT_R8G8B8A8;
	texDesc.initLevels	= 1;
	texDesc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	texDesc.width		= resolution;
	texDesc.height		= resolution;
	texDesc.sliceNum	= 1;
	m_texture = GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
	ASSERT( m_texture );

	GpuApi::SetTextureDebugPath( m_texture, "terrainPigment" );

	// Register pigmentMap in the dynamic texture preview
	GpuApi::AddDynamicTexture( m_texture, "TerrainPigment" );
}

void STerrainPigmentData::Release()
{
	if ( m_texture )
	{
		GpuApi::RemoveDynamicTexture( m_texture );
		GpuApi::SafeRelease( m_texture );
	}

	

	m_state.Reset();
}

void STerrainPigmentData::UpdateConstant()
{
	if ( m_state.m_areaSize.X > 0 && m_state.m_areaSize.Y > 0 )
	{
		const Float scaleX = 1.f / Max( 0.001f, m_state.m_areaSize.X );
		const Float scaleY = 1.f / Max( 0.001f, m_state.m_areaSize.Y );
		m_constants.pigmentWorldAreaScaleBias.Set4( scaleX, scaleY, -(m_state.m_areaCenter.X - 0.5f * m_state.m_areaSize.X) * scaleX, -(m_state.m_areaCenter.Y - 0.5f * m_state.m_areaSize.Y) * scaleY );
	}
	else
	{
		m_constants.pigmentWorldAreaScaleBias.Set4( 0, 0, -1.f, -1.f );	//< outside the texture
	}
}

//////////////////////////////////////////////////////////////////////////

CClipMapShaderData::CClipMapShaderData()
	: m_paramsChanged( false )
	, m_clipWindowschanged( false )
	, m_textureParamsChanged( false ) 
{

}

CClipMapShaderData::~CClipMapShaderData()
{
	Shutdown();
}

void CClipMapShaderData::Init( Uint32 numClipWindows )
{
	if ( !m_paramsCB )
	{
		m_paramsCB = GpuApi::CreateBuffer( sizeof( CClipMapShaderData::STerrainParams ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		RED_ASSERT( m_paramsCB );
		GpuApi::SetBufferDebugPath( m_paramsCB, "terrain params cbuffer" );
	}

	if ( !m_clipWindowsTB )
	{
		GpuApi::TextureDesc desc;
		desc.width		= numClipWindows;
		desc.height		= 2;
		desc.format		= GpuApi::TEXFMT_Float_R32G32B32A32;
		desc.initLevels	= 1;
		desc.sliceNum	= 1;
		desc.type		= GpuApi::TEXTYPE_2D;
		desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Dynamic;
		m_clipWindowsTB = GpuApi::CreateTexture( desc, GpuApi::TEXG_Terrain );
		GpuApi::SetTextureDebugPath( m_clipWindowsTB, "terrain clip windows" );
	}

	if ( !m_textureParamsCB )
	{
		// Create constant buffer for the texture params
		m_textureParamsCB = GpuApi::CreateBuffer( s_textureParamsCBSize, GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		RED_ASSERT( m_textureParamsCB );
		GpuApi::SetBufferDebugPath( m_textureParamsCB, "terrain tex-params cbuffer" );
	}
}

Bool CClipMapShaderData::WasInit() const
{
	return m_paramsCB && m_clipWindowsTB && m_textureParamsCB;
}

void CClipMapShaderData::Shutdown()
{
	GpuApi::SafeRelease( m_clipWindowsTB );
	GpuApi::SafeRelease( m_textureParamsCB );
	GpuApi::SafeRelease( m_paramsCB );
}

void CClipMapShaderData::UpdateTerrainParams(const CRenderProxy_Terrain* terrain)
{
	// Set some constants
	m_params.m_lowestElevation = terrain->GetLowestElevation();
	m_params.m_highestElevation = terrain->GetHighestElevation();
	m_params.m_mipmapLevelRes = (Float)terrain->GetMipmapLevelRes();
	m_params.m_numLevels = (Float)terrain->GetNumLevels();

	m_params.m_terrainEdgeLength = terrain->GetTerrainEdgeLength();
	m_params.m_fullResolution = (Float)terrain->GetFullResolution();
	m_params.m_interVertexSpace = terrain->GetInterVertexSpace();
	m_params.m_colormapStartingIndex = (Float)terrain->GetColormapStartingIndex();

	m_params.m_screenSpaceErrorThresholdNear = terrain->GetScreenspaceErrorThresholdNear();
	m_params.m_screenSpaceErrorThresholdFar = terrain->GetScreenspaceErrorThresholdFar();
	m_params.m_quadTreeRootNodeSize = terrain->GetQuadTreeRootNodeSize();

	m_paramsChanged = true;
}

void CClipMapShaderData::UpdateClipWindows( const CRenderProxy_Terrain* terrain )
{
	// Set some texture buffers
	const TDynArray< SClipmapWindow >& clipmapWindows = terrain->GetClipmapWindows();
	for ( Uint32 i=0; i<clipmapWindows.Size(); ++i )
	{
		const SClipmapWindow& window = clipmapWindows[ i ];
		GetRectangle( i ) = Vector( window.m_worldRect.Min.X, window.m_worldRect.Min.Y, window.m_worldRect.Max.X, window.m_worldRect.Max.Y );
		GetValidTexels( i ) = window.m_validTexels;
	}

	// Flag them as changed
	m_clipWindowschanged = true;
}

void CClipMapShaderData::UpdateTextureParams( const Vector* params )
{
	// Update texture parameters (TODO: Do it only when something changed)
	if ( params )
	{
		for ( Uint32 p=0; p<NUM_TERRAIN_TEXTURES_AVAILABLE; ++p )
		{
			Vector& packedParams0 = GetTextureParams( p, 0 );
			// First vector
			packedParams0.X = params[p*2].X; // blend sharpness
			packedParams0.Y = params[p*2].Y; // slope based damp
			packedParams0.Z = params[p*2].Z * 0.5f + 0.5f; //Make normal damp 0.0 appear as 0.5f, and 1.0f as 0.5f
			packedParams0.W = params[p*2].W; // Falloff

			// Second vector
			Vector& packedParams1 = GetTextureParams( p, 1 );
			packedParams1.X = params[p*2 + 1].X; // specularity
			packedParams1.Y = params[p*2 + 1].Y; // RSpec_Base
			packedParams1.Z = params[p*2].W; // Optimization: Copy Falloff from the first vector, to the second vector. Less lerp instructions this way.
			packedParams1.W = params[p*2 + 1].Z; // RSpec_Scale
		}
		m_textureParamsChanged = true;
	}
}

void CClipMapShaderData::BindTerrainParams( Int32 cbReg ) const
{
	UploadTerrainParams();
	GpuApi::BindConstantBuffer( cbReg, m_paramsCB, GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( cbReg, m_paramsCB, GpuApi::HullShader );
	GpuApi::BindConstantBuffer( cbReg, m_paramsCB, GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( cbReg, m_paramsCB, GpuApi::PixelShader );
}

void CClipMapShaderData::BindTextureParams( Int32 cbReg ) const
{
	UploadTextureParams();
	GpuApi::BindConstantBuffer( cbReg, m_textureParamsCB, GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( cbReg, m_textureParamsCB, GpuApi::HullShader );
	GpuApi::BindConstantBuffer( cbReg, m_textureParamsCB, GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( cbReg, m_textureParamsCB, GpuApi::PixelShader );
}

void CClipMapShaderData::BindClipWindows( Int32 texReg, GpuApi::eShaderType shaderType ) const
{
	UploadClipWindows();
	GpuApi::BindTextures( texReg, 1, &m_clipWindowsTB, shaderType );
}

void CClipMapShaderData::UploadTerrainParams() const
{
	// Upload main terrain parameter
	if ( m_paramsChanged )
	{
		void* constantData = GpuApi::LockBuffer( m_paramsCB, GpuApi::BLF_Discard, 0, sizeof( STerrainParams ) );
		if ( constantData )
		{
			STerrainParams* lockedTerrainParams = static_cast< STerrainParams* >( constantData );
			*lockedTerrainParams = m_params;
			GpuApi::UnlockBuffer( m_paramsCB );

			// Changes uploaded
			m_paramsChanged = false;
		}
	}
}

void CClipMapShaderData::UploadTextureParams() const
{
	// Upload texture parameters
	if ( m_textureParamsChanged )
	{
		void* constantData = GpuApi::LockBuffer( m_textureParamsCB, GpuApi::BLF_Discard, 0, s_textureParamsCBSize );
		if ( constantData )
		{
			Red::System::MemoryCopy( constantData, &m_textureParams[0][0], s_textureParamsCBSize );
			GpuApi::UnlockBuffer( m_textureParamsCB );

			// Changes uploaded
			m_textureParamsChanged = false;
		}
	}
}

void CClipMapShaderData::UploadClipWindows() const
{
	// Upload clipwindows rectangles and valid texels data
	if ( m_clipWindowschanged )
	{
		Uint32 pitch;
		void* data = GpuApi::LockLevel( m_clipWindowsTB, 0, 0, GpuApi::BLF_Discard, pitch );
		if ( data )
		{
			Vector* levelRect = static_cast< Vector* >( data );
			Vector* validTexels = static_cast< Vector* >( OffsetPtr( data, pitch ) );
			for ( Uint32 i=0; i<m_params.m_numLevels; ++i )
			{
				levelRect[i] = m_rectangles[i];
				validTexels[i] = m_validTexels[i];
			}

			GpuApi::UnlockLevel( m_clipWindowsTB, 0, 0 );

			// Changes uploaded
			m_clipWindowschanged = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Uint32 GNum0LevelInstances = 0;
const Bool CRenderProxy_Terrain::s_pregenNormals = true;

const Int64 g_stagingCopyFramesDelay = 3;

CRenderProxy_Terrain::CRenderProxy_Terrain( const CRenderTerrainUpdateData* initData, const SClipmapParameters& clipmapParams )
	: IRenderProxy( RPT_Terrain )
	, m_templateMesh( NULL )
	, m_skirtTemplateMesh( NULL )
	, m_material( NULL )
#ifndef NO_HEIGHTMAP_EDIT
	, m_renderStampVisualization( false )
	, m_heightmapStamp( NULL )
	, m_colorStamp( NULL )
	, m_controlStamp( NULL )
#endif
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	, m_grassVisualiserEnable( false )
#endif
	, m_heightmapArray( NULL )
	, m_controlMapArray( NULL )
	, m_colorMapArray( NULL )
	, m_instanceData( NULL )
	, m_screenSpaceErrorThresholdOverride( -1.0f )
	, m_level0Heightmap( NULL )
	, m_level0ControlMap( NULL )
	, m_grassMask( nullptr )
	, m_grassMaskRes( 0 )
	, m_requestIntermediateMapsUpdate( false )
	, m_requestGrassMapUpdate( false )
	, m_requestPigmentMapUpdate( false )
	, m_requestGrassMapUpdateWhenDataExists( false )
	, m_requestPigmentMapUpdateWhenDataExists( false )
	, m_pigmentMapUpdateClipmapIndex( 0 )
	, m_pigmentMapUpdateColorClipmapIndex( 0 )
	, m_pigmentMapNormalsClipmapIndex( 0 )
	, m_pigmentMapNormalsWindow( Box::EMPTY )
	, m_grassMapData( nullptr )
	, m_lastGrassMapStagingCopyIssuanceFrame( -1 )
	, m_grassMapStagingCopyInProgress( false )
	, m_visible( true )
	, m_customOverlay( nullptr )
	, m_buildQuadTreeTask( nullptr )
#ifndef NO_EDITOR
	, m_isEditing( false )
#endif
{
	ASSERT( initData );

	m_instancedPtr = NULL;

	m_isColorMapCompressed = clipmapParams.useCompressedColorMap;

	m_lowestElevation = clipmapParams.lowestElevation;
	m_highestElevation = clipmapParams.highestElevation;
	m_elevationRange = m_highestElevation - m_lowestElevation;
	m_terrainEdgeLength = clipmapParams.terrainSize;
	m_fullResolution = clipmapParams.clipmapSize;
	m_tilesPerEdge = clipmapParams.clipmapSize / clipmapParams.tileRes;

	m_terrainCorner = Vector( -m_terrainEdgeLength/2.0f, -m_terrainEdgeLength/2.0f, 0.0f );

	m_interVertexSpace = m_terrainEdgeLength / m_fullResolution;

	m_windowResolution = initData->GetUpdates()[0]->m_resolution;
	ASSERT( m_windowResolution > 0 );

	m_numLevels = static_cast<Uint16>( initData->GetUpdates().Size() );
	ASSERT( m_numLevels > 0 );

	// Store resolution of the mipmap stack level texture
	m_mipmapLevelRes = initData->GetUpdates().Back()->m_resolution;

	m_clipmapWindows.Resize( m_numLevels );

	// Compute the maximum level of quad tree
	m_maxQuadTreeLevel = 0;
	Int32 tempRes = m_fullResolution;
	while ( tempRes > TERRAIN_PATCH_RES )
	{
		++m_maxQuadTreeLevel;
		tempRes /= 2;
	}

	// If the data set (full data resolution) is not power-of-two, then round it up to the closest pow2 size. That's necessary for the quadtree to fit the texels properly.
	m_terrainEdgeLengthPowerOf2 = m_terrainEdgeLength;
	if ( !Red::Math::IsPow2( m_fullResolution ) )
	{
		const Uint32 pow2Res = Red::Math::RoundUpToPow2( m_fullResolution );
		const Float pow2Ratio = (Float)pow2Res / (Float)m_fullResolution;
		m_terrainEdgeLengthPowerOf2 *= pow2Ratio;
	}

	// Scales of a quad tree nodes for different clipmap levels
	Float scales[MAX_CLIPMAP_COUNT];
	scales[m_maxQuadTreeLevel] = m_terrainEdgeLengthPowerOf2;
	for( Int32 i = m_maxQuadTreeLevel-1; i >= 0; i-- )
	{
		scales[i] = scales[i+1] / 2.0f;
	}
	m_quadTreeRootNodeSize = scales[0];

	m_quadTreeStack.Grow( MAX_QUADTREE_STACK_NODES );

#ifndef RED_FINAL_BUILD
	m_pipelineStatsQuery = new CRenderQuery( GpuApi::QT_PipelineStats, false );
	RED_ASSERT( m_pipelineStatsQuery->IsValid() );
#endif

	m_tileHeightRange.Resize( m_tilesPerEdge * m_tilesPerEdge );
}

CRenderProxy_Terrain::~CRenderProxy_Terrain()
{
	FinishBuildingQuadTree();

	m_instancedPtr = nullptr;

	m_pigmentData.Release();

	GpuApi::SafeRelease( m_heightmapArray );
	GpuApi::SafeRelease( m_controlMapArray );

#ifndef NO_HEIGHTMAP_EDIT
	GpuApi::SafeRelease( m_heightmapStamp );
	GpuApi::SafeRelease( m_colorStamp );
	GpuApi::SafeRelease( m_controlStamp );
	GpuApi::SafeRelease( m_grassMaskPreview );
#endif

	GpuApi::RemoveDynamicTexture( m_colorMapArray );
	GpuApi::SafeRelease( m_colorMapArray );

	if ( m_templateMesh )
	{
		delete m_templateMesh;
	}

	if( m_skirtTemplateMesh )
	{
		delete m_skirtTemplateMesh;
	}

	if ( m_material )
	{
		m_material->Release();
	}

	GpuApi::SafeRelease( m_instanceData );

	// Unregister terrain clipmap in the dynamic texture preview
	GpuApi::RemoveDynamicTexture( m_tesselationBlockErrorsArray );
	GpuApi::SafeRelease( m_tesselationBlockErrorsArray );

	GpuApi::RemoveDynamicTexture( m_normalMapsArray );
	GpuApi::SafeRelease( m_normalMapsArray );

	GpuApi::RemoveDynamicTexture( m_grassMap );
	GpuApi::SafeRelease( m_grassMap );
	GpuApi::SafeRelease( m_grassMap_Staging );

	if ( m_grassMask )
	{
		RED_MEMORY_FREE( MemoryPool_FoliageData, MC_FoliageGrassMask, m_grassMask );
	}

	if ( m_grassMapData )
	{
		RED_MEMORY_FREE( MemoryPool_FoliageData, MC_FoliageGrassMask, m_grassMapData );
	}

#ifndef RED_FINAL_BUILD
	if ( m_pipelineStatsQuery )
	{
		m_pipelineStatsQuery->Release();
	}
#endif

	if ( nullptr != m_level0Heightmap )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TerrainClipmap, m_level0Heightmap );
	}
	if ( nullptr != m_level0ControlMap )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TerrainClipmap, m_level0ControlMap );
	}
}

void CRenderProxy_Terrain::Init()
{
	if ( !m_clipMapShaderData.WasInit() )
	{
		m_clipMapShaderData.Init( m_numLevels );
	}

	if ( !m_heightmapArray )
	{
		// Create heightmap texture array
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_ARRAY;
		texDesc.format		= GpuApi::TEXFMT_Uint_16_norm;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.width		= m_windowResolution;
		texDesc.height		= m_windowResolution;
		texDesc.sliceNum	= m_numLevels;
		m_heightmapArray	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_heightmapArray );
		GpuApi::SetTextureDebugPath(m_heightmapArray, "heightMapArray");

		GpuApi::AddDynamicTexture(m_heightmapArray, "HeightMapTexture");
	}

	if ( !m_controlMapArray )
	{
		// Create control map texture array
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_ARRAY;
		texDesc.format		= GpuApi::TEXFMT_Uint_16;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.width		= m_windowResolution;
		texDesc.height		= m_windowResolution;
		texDesc.sliceNum	= m_numLevels;
		m_controlMapArray	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_controlMapArray );
		GpuApi::SetTextureDebugPath(m_controlMapArray, "controlMapArray");
	}

	if ( !m_colorMapArray )
	{
		// Create color map (RGB - color, Alpha - brightness)
		GpuApi::TextureDesc	texDesc;
		texDesc.type		= GpuApi::TEXTYPE_ARRAY;
		texDesc.initLevels	= 1;
		texDesc.width		= m_windowResolution;
		texDesc.height		= m_windowResolution;
		texDesc.format		= m_isColorMapCompressed ? GpuApi::TEXFMT_BC1 : GpuApi::TEXFMT_R8G8B8A8;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.sliceNum	= m_numLevels;
		m_colorMapArray		= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_colorMapArray );
		GpuApi::SetTextureDebugPath( m_colorMapArray, "colorMapArray");

		GpuApi::AddDynamicTexture(m_colorMapArray, "TerrainColorMap");
	}

	if ( !m_tesselationBlockErrorsArray )
	{
		// Create tesselation blocks errors array
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_ARRAY;
		texDesc.format		= GpuApi::TEXFMT_Float_R32G32B32A32;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDesc.width		= m_windowResolution / TERRAIN_TESS_BLOCK_RES;
		texDesc.height		= m_windowResolution / TERRAIN_TESS_BLOCK_RES;
		texDesc.sliceNum	= m_numLevels;
		m_tesselationBlockErrorsArray	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_tesselationBlockErrorsArray );
		GpuApi::SetTextureDebugPath(m_tesselationBlockErrorsArray, "tesselationBlockErrorsArray");

		// Register terrain clipmap in the dynamic texture preview
		GpuApi::AddDynamicTexture( m_tesselationBlockErrorsArray, "TessBlocksErrors" );
	}

	if ( s_pregenNormals && !m_normalMapsArray )
	{
		// Create normal maps array
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_ARRAY;
		texDesc.format		= GpuApi::TEXFMT_Float_R16G16;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDesc.width		= m_windowResolution;
		texDesc.height		= m_windowResolution;
		texDesc.sliceNum	= m_numLevels;
		m_normalMapsArray	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_normalMapsArray );
		GpuApi::SetTextureDebugPath(m_normalMapsArray, "terrainNormalsArray");

		// Register normal map array in the dynamic texture preview
		GpuApi::AddDynamicTexture( m_normalMapsArray, "TerrainNormals" );
	}

	if ( !m_grassMap )
	{
		// Create grass fade map render target
		{
			GpuApi::TextureDesc texDesc;
			texDesc.type		= GpuApi::TEXTYPE_2D;
			texDesc.format		= GpuApi::TEXFMT_L8;
			texDesc.initLevels	= 1;
			texDesc.usage		= GpuApi::TEXUSAGE_RenderTarget;
#ifdef ENABLE_GRASS_MAP_DEBUGGING
			texDesc.usage		|= GpuApi::TEXUSAGE_Samplable;
#endif
			texDesc.width		= m_windowResolution;
			texDesc.height		= m_windowResolution;
			texDesc.sliceNum	= 1;
			m_grassMap	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
			ASSERT( m_grassMap );
			GpuApi::SetTextureDebugPath( m_grassMap, "grassMap" );

			// Register normal map array in the dynamic texture preview
			GpuApi::AddDynamicTexture( m_grassMap, "Generic Grass Map" );
		}
		
		// Create staging texture for grabbing the grass map
		// TODO: Skip this step on consoles!
		{
			GpuApi::TextureDesc texDesc;
			texDesc.type		= GpuApi::TEXTYPE_2D;
			texDesc.format		= GpuApi::TEXFMT_L8;
			texDesc.initLevels	= 1;
			texDesc.usage		= GpuApi::TEXUSAGE_Staging;
			texDesc.width		= m_windowResolution;
			texDesc.height		= m_windowResolution;
			texDesc.sliceNum	= 1;
			m_grassMap_Staging	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
			ASSERT( m_grassMap_Staging );
			GpuApi::SetTextureDebugPath( m_grassMap_Staging, "grassMapStaging" );
		}
	}

	if ( !m_pigmentData.IsInit() )
	{
		m_pigmentData.Init( m_windowResolution );
		ASSERT( m_pigmentData.IsInit() );
	}

#ifndef NO_HEIGHTMAP_EDIT
	if ( !m_heightmapStamp )
	{
		// Create heightmap stamp texture
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_2D;
		texDesc.format		= GpuApi::TEXFMT_Uint_16_norm;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.width		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.height		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.sliceNum	= 1;
		m_heightmapStamp	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_heightmapStamp );
		GpuApi::SetTextureDebugPath(m_heightmapStamp, "heightMapStamp");
	}

	if ( !m_colorStamp )
	{
		// Create heightmap stamp texture
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_2D;
		texDesc.format		= GpuApi::TEXFMT_R8G8B8A8;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.width		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.height		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.sliceNum	= 1;
		m_colorStamp	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_colorStamp );
		GpuApi::SetTextureDebugPath(m_colorStamp, "colorStamp");
	}

	if ( !m_controlStamp )
	{
		// Create heightmap stamp texture
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_2D;
		texDesc.format		= GpuApi::TEXFMT_Uint_16;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;
		texDesc.width		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.height		= TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
		texDesc.sliceNum	= 1;
		m_controlStamp	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_Terrain );
		ASSERT( m_controlStamp );
		GpuApi::SetTextureDebugPath(m_controlStamp, "controlStamp");
	}
#endif

	if ( !m_templateMesh )
	{
		m_templateMesh = new CTerrainTemplateMesh( NUM_TESS_BLOCKS_PER_PATCH, m_terrainEdgeLength );
		ASSERT( m_templateMesh );
	}

	if ( !m_skirtTemplateMesh )
	{
		m_skirtTemplateMesh = new CSkirtTemplateMesh( );
		ASSERT( m_skirtTemplateMesh );
	}

	if ( nullptr == m_level0Heightmap )
	{
		m_level0Heightmap = (Uint16*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TerrainClipmap, m_windowResolution * m_windowResolution * sizeof( Uint16 ) );
	}
	if ( nullptr == m_level0ControlMap )
	{
		m_level0ControlMap = (TControlMapType*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TerrainClipmap, m_windowResolution * m_windowResolution * sizeof( TControlMapType ) );
	}
}


Float CRenderProxy_Terrain::GetScreenspaceErrorThresholdNear() const
{
	if ( m_screenSpaceErrorThresholdOverride >= 0.0f )
	{
		return m_screenSpaceErrorThresholdOverride;
	}
	return Config::cvTerrainScreenSpaceErrorThresholdNear.Get();
}

Float CRenderProxy_Terrain::GetScreenspaceErrorThresholdFar() const
{
	if ( m_screenSpaceErrorThresholdOverride >= 0.0f )
	{
		return m_screenSpaceErrorThresholdOverride;
	}
	return Config::cvTerrainScreenSpaceErrorThresholdFar.Get();
}

void CRenderProxy_Terrain::Update( CRenderTerrainUpdateData* updateData )
{
	CPerformanceIssueCheckerScope perfIssue( 0.005f, TXT("TerrainUpdate"), TXT("%d updates"), updateData->GetUpdates().Size());

	// Make sure all DX objects are created
	Init();

#ifndef NO_EDITOR
	m_isEditing = updateData->IsEditing();
#endif

	if ( !m_updateDataPending.Empty() )
	{
		RED_ASSERT( m_updateDataPending.Size() == 1 );
		RED_LOG( RED_LOG_CHANNEL( Terrain ), TXT("Pushing a GPU terrain update during render command to avoid stacking up.") );

		GpuApi::BeginRender();
		FlushTerrainUpdates();
		GpuApi::EndRender();
	}
	RED_FATAL_ASSERT( m_updateDataPending.Empty(), "Terrain updates stacking!" );

	m_updateDataPending.Push( updateData );
	updateData->AddRef();
}

void CRenderProxy_Terrain::Update( const CRenderGrassUpdateData* updateData )
{
	for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE; ++i )
	{
		const TDynArray< SAutomaticGrassDesc >& descs = updateData->GetDescriptors()[i];
		m_grassSetup[i].Reserve( descs.Size() );

		m_grassSetup[i].ClearFast();

		for ( Uint32 j=0; j<descs.Size(); ++j )
		{
			m_grassSetup[i].PushBack( updateData->GetDescriptors()[i][j] );
		}
	}

	// Make a frequent-access friendly version of the data that we just received
	OptimizeGrassSetup();
}

void CRenderProxy_Terrain::UpdateMinimumWaterLevels( const TDynArray< Float >& minWaterLevels )
{
	RED_ASSERT( m_minWaterLevels.Empty() || minWaterLevels.Size() == m_minWaterLevels.Size() );
	m_minWaterLevels.Resize( m_tilesPerEdge * m_tilesPerEdge );
	Red::System::MemoryCopy( m_minWaterLevels.Data(), minWaterLevels.Data(), m_minWaterLevels.DataSize() );
}

void CRenderProxy_Terrain::UpdateTileHeightRanges( const TDynArray< Vector2 >& tileHeightRanges )
{
	RED_ASSERT( m_tileHeightRange.Size() == tileHeightRanges.Size() );
	Red::System::MemoryCopy( m_tileHeightRange.Data(), tileHeightRanges.Data(), m_tileHeightRange.DataSize() );
}

void CRenderProxy_Terrain::Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene, MeshDrawingStats& stats )
{
	GpuApi::SetVsWaveLimits( 0, Config::cvTerrainLateAllocVSLimit.Get() );

	// Terrain doesn't have light channels, so it's as though they were 0
	if ( !context.CheckLightChannels( 0 ) || !IsVisible() )
	{
		return;
	}

	// Make sure all DX objects are created
	Init();

	RED_WARNING( m_updateDataPending.Empty(), "Still have pending terrain updates during Render. Won't break anything, but shouldn't happen" );

	if ( !m_templateMesh )
	{
		// Nooo! whyyy?
		return;
	}
	if ( !m_material )
	{
		return;
	}

	const Int64 frameIndex = scene->GetLastAllocatedFrame();

	if ( m_grassMapStagingCopyInProgress && ( frameIndex - m_lastGrassMapStagingCopyIssuanceFrame ) >= g_stagingCopyFramesDelay )
	{
		// A staging copy of the grass map was requested and it should be safe to get the data now
		ReadBackGrassMapMemoryFromStaging();
	}

	static Bool DEBUGAlwaysRenderIntermediate = false;
	if ( m_requestIntermediateMapsUpdate || DEBUGAlwaysRenderIntermediate )
	{
		RenderIntermediateMaps( scene );
		m_requestIntermediateMapsUpdate = false;
	}

	if ( m_requestGrassMapUpdate || DEBUGAlwaysRenderIntermediate )
	{
		RenderGrassMap( scene );
		m_requestGrassMapUpdate = false;
		m_requestGrassMapUpdateWhenDataExists = false;
	}

	if ( m_requestPigmentMapUpdate || DEBUGAlwaysRenderIntermediate )
	{
		// TODO: handle partial update to speed things up
		RenderPigmentMap( m_pigmentMapUpdateClipmapIndex, m_pigmentMapUpdateColorClipmapIndex );
		m_requestPigmentMapUpdate = false;
		m_requestPigmentMapUpdateWhenDataExists = false;
	}

	// Rendering to GBuffer, mask by lighting channel, enable stencil
	const Bool deferredLighting = (context.m_pass == RP_GBuffer);
	const CGpuApiScopedDrawContext scopedDrawContext;
	if ( deferredLighting )
	{
		GpuApi::eDrawContext curContext = scopedDrawContext.GetOrigDrawContext();
		GpuApi::eDrawContext newContext = GpuApi::GetStencilLightChannelsContext( curContext );
		if ( newContext != curContext )
		{
			GpuApi::SetDrawContext( newContext, LC_Default );
		}
	}

	scene->GetTerrainShadows()->BindToTerrainProxy( this, scene );

	// Calculate stream stride
	const Uint32 streamStride = sizeof( STerrainPatchInstanceData );
	
	FinishBuildingQuadTree();
	Uint32 numInstances = m_instances.Size();

	ASSERT( GNum0LevelInstances <= numInstances );

	if ( numInstances > 0 && numInstances + m_skirtInstances.Size() <= TERRAIN_PATCH_INSTANCES_BUFFER_SIZE )
	{
		BindConstantsAndSamplers( context );

#ifndef RED_FINAL_BUILD
		if ( frameInfo.IsShowFlagOn( SHOW_TerrainStats ) )
		{
			m_pipelineStatsQuery->BeginQuery();
		}
#endif

		Bool buffersBound = false;
		// Actual terrain always uses HQ shader. If this changes, make sure to update CMaterialCooker::CollectTechniques() to include LQ.
		if ( BindMaterialForHolesSkippingPass( context, frameInfo, MVF_TesselatedTerrain, false ) )
		{
			// Bind combined terrain buffer and instance buffer
			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexTerrain );

			GpuApi::BufferRef buffers[2]	= { m_templateMesh->m_vertexBuffer,			m_instanceData								};
			Uint32 strides[2]				= { sizeof( GpuApi::SystemVertex_Terrain ),	streamStride								};
			Uint32 offsets[2]				= { 0,										0	};
			GpuApi::BindVertexBuffers( 0, 2, buffers, strides, offsets );
			buffersBound = true;

			// Draw using instance buffer
			static Uint32 DEBUGMaxInstances = 8192;
			numInstances = Min< Uint32 >( numInstances, DEBUGMaxInstances );

			GpuApi::DrawInstancedPrimitive( GpuApi::PRIMTYPE_1CP_PATCHLIST, 0, m_templateMesh->m_numFaces, m_templateMesh->m_numFaces, numInstances );
		}

		if ( GNum0LevelInstances > 0 && BindMaterialForHolesOnlyPass( context, frameInfo ) )
		{
			if ( !buffersBound )
			{
				// Bind combined terrain buffer and instance buffer
				GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexTerrain );

				GpuApi::BufferRef buffers[2]	= { m_templateMesh->m_vertexBuffer,			m_instanceData								};
				Uint32 strides[2]				= { sizeof( GpuApi::SystemVertex_Terrain ),	streamStride								};
				Uint32 offsets[2]				= { 0,										0	};
				GpuApi::BindVertexBuffers( 0, 2, buffers, strides, offsets );
			}
			GpuApi::DrawInstancedPrimitive( GpuApi::PRIMTYPE_1CP_PATCHLIST, 0, m_templateMesh->m_numFaces, m_templateMesh->m_numFaces, GNum0LevelInstances );
		}

		// Draw skirt
		if( frameInfo.IsShowFlagOn( SHOW_Skirt ) && !m_skirtInstances.Empty() )
		{
			// Make sure we don't have any extra shaders.
			GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
			GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
			GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

			// Skirts always use LQ shader. If this changes, make sure to update CMaterialCooker::CollectTechniques() to include HQ.
			if( BindMaterialForHolesSkippingPass( context, frameInfo, MVF_TerrainSkirt, true ) )
			{
				// Set textures to vertex shade
				GpuApi::BindTextures( 0, 1, &m_heightmapArray, GpuApi::VertexShader );
				GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::VertexShader );

				m_clipMapShaderData.BindClipWindows( 1, GpuApi::VertexShader );
				
				GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexTerrainSkirt );

				GpuApi::BufferRef buffers[2]	= { m_skirtTemplateMesh->m_vertexBuffer,	m_instanceData				};
				Uint32 strides[2]				= { sizeof( GpuApi::SystemVertex_Terrain ),	streamStride				};
				Uint32 offsets[2]				= { 0,										numInstances * streamStride	};
				GpuApi::BindVertexBuffers( 0, 2, buffers, strides, offsets );
				GpuApi::BindIndexBuffer( m_skirtTemplateMesh->m_indexBuffer );

				GpuApi::DrawInstancedIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_skirtTemplateMesh->m_vertices.Size(), 0, 10, m_skirtInstances.Size() );

				// Unbind
				GpuApi::BindTextures( 0, 2, nullptr, GpuApi::VertexShader );
			}
		}

#ifndef RED_FINAL_BUILD
		if ( frameInfo.IsShowFlagOn( SHOW_TerrainStats ) )
		{
			m_pipelineStatsQuery->EndQuery();
			GpuApi::SPipelineStatistics pipelineStats;
			if ( m_pipelineStatsQuery->GetQueryResult( pipelineStats, false ) == EQR_Success )
			{
				extern SceneRenderingStats GRenderingStats;
				GRenderingStats.m_terrainVerticesRead += pipelineStats.VerticesRead;
				GRenderingStats.m_terrainPrimitivesRead += pipelineStats.PrimitivesRead;
				GRenderingStats.m_terrainPrimitivesRendered += pipelineStats.PrimitivesRendered;
				GRenderingStats.m_terrainPrimitivesSentToRasterizer += pipelineStats.PrimitivesSentToRasterizer;
				GRenderingStats.m_terrainVertexShaderInvocations += pipelineStats.VertexShaderInvocations;
				GRenderingStats.m_terrainPixelShaderInvocations += pipelineStats.PixelShaderInvocations;
				GRenderingStats.m_terrainHullShaderInvocations += pipelineStats.HullShaderInvocations;
				GRenderingStats.m_terrainDomainShaderInvocations += pipelineStats.DomainShaderInvocations;
			}
		}
#endif
	}

	m_instances.ClearFast();
	m_skirtInstances.ClearFast();

	//HACK
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

	GpuApi::ResetVsWaveLimits();
}

void CRenderProxy_Terrain::GetHeightRangeInArea( const Vector2& min, const Vector2& max, Float& outMinHeight, Float& outMaxHeight )
{
	const Box wholeClipRect = m_clipmapWindows.Back().m_worldRect;
	const Float worldToTileScale = m_tilesPerEdge / ( wholeClipRect.Max.X - wholeClipRect.Min.X );

	Float minimumHeight = m_highestElevation;
	Float maximumHeight = m_lowestElevation;


	Int32 umin = (Int32)MFloor( ( min.X - wholeClipRect.Min.X ) * worldToTileScale );
	Int32 vmin = (Int32)MFloor( ( min.Y - wholeClipRect.Min.Y ) * worldToTileScale );
	Int32 umax = (Int32)MCeil(  ( max.X - wholeClipRect.Min.X ) * worldToTileScale );
	Int32 vmax = (Int32)MCeil(  ( max.Y - wholeClipRect.Min.Y ) * worldToTileScale );

	umin = Clamp( umin, 0, (Int32)m_tilesPerEdge - 1 );
	vmin = Clamp( vmin, 0, (Int32)m_tilesPerEdge - 1 );
	umax = Clamp( umax, 0, (Int32)m_tilesPerEdge );
	vmax = Clamp( vmax, 0, (Int32)m_tilesPerEdge );

	for ( Int32 v = vmin; v < vmax; ++v )
	{
		for ( Int32 u = umin; u < umax; ++u )
		{
			const Vector2& heightRange = m_tileHeightRange[u + v * m_tilesPerEdge];
			minimumHeight = Min( minimumHeight, heightRange.X );
			maximumHeight = Max( maximumHeight, heightRange.Y );
		}
	}

	outMinHeight = minimumHeight;
	outMaxHeight = maximumHeight;
}

void CRenderProxy_Terrain::StartBuildingQuadTree( const CRenderSceneEx* scene, const CRenderFrameInfo& frameInfo )
{
	if ( !m_instanceData )
	{
		// Create the instance buffer
		const Uint32 chunkSize = sizeof( STerrainPatchInstanceData );
		const Uint32 instanceDataSize = chunkSize * TERRAIN_PATCH_INSTANCES_BUFFER_SIZE;
		m_instanceData = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_instanceData, "terrainInstance" );
		ASSERT( m_instanceData );
	}

	FinishBuildingQuadTree();

	m_instances.ClearFast();
	m_skirtInstances.ClearFast();

	if( !m_buildQuadTreeTask )
	{
		// Compose quad tree contents
		GNum0LevelInstances = 0;

		m_instancedPtr = (Int8*)GpuApi::LockBuffer( m_instanceData, GpuApi::BLF_Discard, 0, TERRAIN_PATCH_INSTANCES_BUFFER_SIZE );

		m_buildQuadTreeTask = new ( CTask::Root ) CBuildQuadTreeTask( this, 
#ifdef USE_UMBRA
			scene->GetOcclusionData(),
#endif // USE_UMBRA
			frameInfo, scene->ShouldCollectWithUmbra() );
		GTaskManager->Issue( *m_buildQuadTreeTask, TSP_Normal );
	}
}

void CRenderProxy_Terrain::FinishBuildingQuadTree()
{
	PC_SCOPE( FinishBuildingQuadTree );

	if ( m_buildQuadTreeTask )
	{
		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *m_buildQuadTreeTask, taskDispatcher );

		while( !m_buildQuadTreeTask->IsFinished() ){RED_BUSY_WAIT();}

		m_buildQuadTreeTask->Release();
		m_buildQuadTreeTask = nullptr;

		// Return the buffer
		GpuApi::UnlockBuffer( m_instanceData );
		m_instancedPtr = nullptr;
	}
}

void CRenderProxy_Terrain::BindConstantsAndSamplers( const RenderingContext& context )
{
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLowAnisoMip, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_WrapLowAnisoMip, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_WrapLowAnisoMip, GpuApi::PixelShader );

	// Set matrices	
	const Matrix localToWorld = Matrix::IDENTITY;
	GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );

	// Set textures to hull shader
	GpuApi::TextureRef hullRefs[3] = { m_heightmapArray, m_controlMapArray, m_tesselationBlockErrorsArray };
	GpuApi::BindTextures( 0, 3, &(hullRefs[0]), GpuApi::HullShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::HullShader );

	// Set textures to domain shader
	GpuApi::BindTextures( 0, 1, &m_heightmapArray, GpuApi::DomainShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::DomainShader );

#ifndef NO_HEIGHTMAP_EDIT
	GpuApi::BindTextures( 4, 1, &m_heightmapStamp, GpuApi::DomainShader );
	GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::DomainShader );
#endif

	// Set textures to pixel shader
	GpuApi::BindTextures( 3, 1, &m_colorMapArray, GpuApi::PixelShader );

	if ( s_pregenNormals )
	{
		GpuApi::BindTextures( 4, 1, &m_normalMapsArray, GpuApi::PixelShader );
	}

	// Bind control map texture
	// Color and control by PS
	GpuApi::BindTextures( 5, 1, &m_controlMapArray, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 5, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

	m_clipMapShaderData.BindTerrainParams( 5 );
	m_clipMapShaderData.BindTextureParams( 6 );
	m_clipMapShaderData.BindClipWindows( 3, GpuApi::HullShader );
	m_clipMapShaderData.BindClipWindows( 3, GpuApi::DomainShader );

#ifndef NO_HEIGHTMAP_EDIT
	GpuApi::TextureRef refs[2] = { m_colorStamp, m_controlStamp };
	GpuApi::BindTextures( 6, 2, &(refs[0]), GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 6, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

	// push stamp rect to gpu
	if ( m_renderStampVisualization )
	{
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_4, m_stampCenterAxis );
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_5, m_stampVertexSettings );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, m_stampPixelSettings );
	}
	else
	{
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_4, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_5, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector::ZEROS );
	}

	if ( context.m_grassMaskPaintMode )
	{
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector( (Float)m_grassMaskRes, (Float)m_grassMaskRes, 0.0f, 0.0f ) );
		GpuApi::BindTextures( 8, 1, &m_grassMaskPreview, GpuApi::PixelShader );
	}
#endif

	if ( m_customOverlay )
	{
		Vector baseWindowRect;
		baseWindowRect.X = m_clipmapWindows.Back().m_worldRect.Min.X;
		baseWindowRect.Y = m_clipmapWindows.Back().m_worldRect.Min.Y;
		baseWindowRect.Z = m_clipmapWindows.Back().m_worldRect.Max.X;
		baseWindowRect.W = m_clipmapWindows.Back().m_worldRect.Max.Y;
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, baseWindowRect );

		GpuApi::BindTextures( 9, 1, &m_customOverlay->ovarlayBitmap, GpuApi::PixelShader );
	}
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	else if ( m_grassVisualiserEnable )
	{
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, m_grassDensityRect );
		if ( m_grassDensityDebug )
		{
			GpuApi::BindTextures( 9, 1, &m_grassDensityDebug, GpuApi::PixelShader );
		}
	}
#endif

	if( context.m_materialDebugMode == MDM_Heightmap )
	{
		GpuApi::BindTextures( 10, 1, &m_heightmapArray, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 10, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
	}
}

Bool CRenderProxy_Terrain::BindMaterialForHolesSkippingPass( const RenderingContext& context, const CRenderFrameInfo& frameInfo, EMaterialVertexFactory vertexFactory, Bool lowQuality )
{
	MaterialRenderingContext materialContext( context );
	materialContext.m_vertexFactory = vertexFactory;
	materialContext.m_pregeneratedMaps = s_pregenNormals;
	materialContext.m_useInstancing = true;
	
	if ( context.m_grassMaskPaintMode )
	{
		// Need to see grass mask during painting it?
		materialContext.m_materialDebugMode = MDM_Mask;
	}
	else if ( frameInfo.IsShowFlagOn( SHOW_TerrainHoles ) )
	{
		// Terrain holes showing is respected if grass mask painting is not in progress
		materialContext.m_materialDebugMode = MDM_Holes;
	}
	else if ( m_customOverlay
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
		|| m_grassVisualiserEnable
#endif
		)
	{
		materialContext.m_materialDebugMode = MDM_Overlay;
	}
	else if( frameInfo.m_materialDebugMode == MDM_Heightmap )
	{
		materialContext.m_materialDebugMode = MDM_Heightmap;
	}	
	else if( frameInfo.m_materialDebugMode == MDM_WaterMode )
	{
		materialContext.m_materialDebugMode = MDM_WaterMode;
	}

	materialContext.m_discardingPass = false;

	materialContext.m_lowQuality = lowQuality;

	// Binding the terrain with 0 as distance, because the textures used are global, so there is no reason to unstream them
	return m_material->Bind( materialContext, static_cast< CRenderMaterialParameters* >( m_material ), 0.f );
}

Bool CRenderProxy_Terrain::BindMaterialForHolesOnlyPass( const RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	MaterialRenderingContext materialContext( context );
	materialContext.m_vertexFactory = MVF_TesselatedTerrain;
	materialContext.m_pregeneratedMaps = s_pregenNormals;
	materialContext.m_useInstancing = true;

	if ( context.m_grassMaskPaintMode )
	{
		// Need to see grass mask during painting it?
		materialContext.m_materialDebugMode = MDM_Mask;
	}
	else if ( frameInfo.IsShowFlagOn( SHOW_TerrainHoles ) )
	{
		// Terrain holes showing is respected if grass mask painting is not in progress
		materialContext.m_materialDebugMode = MDM_Holes;
	}
	else if ( m_customOverlay
#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
		|| m_grassVisualiserEnable
#endif
		)
	{
		materialContext.m_materialDebugMode = MDM_Overlay;
	}
	else if( frameInfo.m_materialDebugMode == MDM_Heightmap )
	{
		materialContext.m_materialDebugMode = MDM_Heightmap;
	}
	else if( frameInfo.m_materialDebugMode == MDM_WaterMode )
	{
		materialContext.m_materialDebugMode = MDM_WaterMode;
	}

	materialContext.m_discardingPass = true;

	// Binding the terrain with 0 as distance, because the textures used are global, so there is no reason to unstream them
	return m_material->Bind( materialContext, static_cast< CRenderMaterialParameters* >( m_material ), 0.f );
}

void CRenderProxy_Terrain::RenderIntermediateMaps(CRenderSceneEx* scene)
{
	// Store previous render target setup, as we will use two custom ones for rendering tessellation errors and normal maps
	GpuApi::RenderTargetSetup previousSetup = GpuApi::GetRenderTargetSetup();

	// Save wireframe info
	const Bool origWireframe = GpuApi::GetRenderSettings().wireframe;

	// Disable wireframe state
	GpuApi::SetRenderSettingsWireframe( false );

	// Bind simple context, with no blending, culling, etc.
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

	m_clipMapShaderData.BindTerrainParams( 5 );
	m_clipMapShaderData.BindTextureParams( 6 );

	GpuApi::BindTextures( 0, 1, &m_heightmapArray, GpuApi::VertexShader );

	GpuApi::TextureRef pixelRefs[2] = { m_heightmapArray, m_controlMapArray };
	GpuApi::BindTextures( 0, 2, &(pixelRefs[0]), GpuApi::PixelShader );

	//unbind the texture from the hull shader the hull shader 
	GpuApi::TextureRef nullRef = GpuApi::TextureRef::Null();
	GpuApi::BindTextures( 2, 1, &nullRef, GpuApi::HullShader );

	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::VertexShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

	
	// Do error maps
	{
		// Bind errors generation shader
		GetRenderer()->m_shaderTessBlocksErrorsGenerate->Bind();
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetViewportFromTarget( m_tesselationBlockErrorsArray );
		for ( Uint32 i=0; i<m_clipmapWindows.Size(); ++i )
		{
			SClipmapWindow& window = m_clipmapWindows[i];
			if ( window.NeedsIntermediateMapsRegeneration() )
			{
				// Bind proper slice of the tesselation block errors texarray
				rtSetup.SetColorTarget( 0, m_tesselationBlockErrorsArray, i );
				GpuApi::SetupRenderTargets( rtSetup );

				// Set the clipmap level we want to render
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector( (Float)i, 0.0f, 0.0f, 0.0f ) );

				// Calculate errors now
				GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );
			}
		}
	}
	
	// Do normal maps
	if ( s_pregenNormals )
	{
		// Bind normals generation shader
		GetRenderer()->m_shaderTerrainNormalsCalc->Bind();

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetViewportFromTarget( m_normalMapsArray );
		for ( Uint32 i=0; i<m_clipmapWindows.Size(); ++i )
		{
			SClipmapWindow& window = m_clipmapWindows[i];
			if ( window.NeedsIntermediateMapsRegeneration() )
			{
				// Bind normal maps array slice as a render target
				rtSetup.SetColorTarget( 0, m_normalMapsArray, i );
				GpuApi::SetupRenderTargets( rtSetup );

				// Set the clipmap level we want to render
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector( (Float)i, 0.0f, 0.0f, 0.0f ) );

				// Calculate normals now
				GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );
			}
		}

		// Update pigment clipmap level normals window
		m_pigmentMapNormalsClipmapIndex = m_pigmentMapUpdateClipmapIndex;
		m_pigmentMapNormalsWindow = m_pigmentMapNormalsClipmapIndex < m_clipmapWindows.Size() ? m_clipmapWindows[m_pigmentMapNormalsClipmapIndex].m_worldRect : Box::EMPTY;
	}

	// Clear all regeneration requests
	for ( Uint32 i=0; i<m_clipmapWindows.Size(); ++i )
	{
		m_clipmapWindows[i].MarkIntermediateMapsRegeneration( false );
	}

	// Restore previous render target setup
	GpuApi::SetupRenderTargets( previousSetup );

	// Restore wireframe setting
	GpuApi::SetRenderSettingsWireframe( origWireframe );
}

void CRenderProxy_Terrain::RenderPigmentMap( Uint32 clipmapIndex, Uint32 colorClipmapIndex )
{
	if ( !m_pigmentData.IsInit() )
	{
		return;
	}
	
	// Grab rendertarget setup
	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
	
	// Grab (by hand) wireframe info and force non_wireframe
	const Bool origWireframe = GpuApi::GetRenderSettings().wireframe;
	GpuApi::SetRenderSettingsWireframe( false );

	m_clipMapShaderData.BindTerrainParams( 5 );
	m_clipMapShaderData.BindTextureParams( 6 );

	// Bind render target
	GpuApi::RenderTargetSetup rtSetup;
	{
		rtSetup.SetColorTarget( 0, m_pigmentData.m_texture );
		rtSetup.SetViewportFromTarget( m_pigmentData.m_texture );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	// Extract diffuse array
	CRenderTextureArray *diffuseArray = ExtractTerrainDiffuseTextureArray( m_material );

	// Render stuff
	if ( s_pregenNormals && diffuseArray )
	{
		GetRenderer()->m_terrainPigmentGenerate->Bind();

		const Box clipmapWindow = m_clipmapWindows[clipmapIndex].m_worldRect;
		const Box colorClipmapWindow = m_clipmapWindows[colorClipmapIndex].m_worldRect;

		// Bind params
		GpuApi::TextureRef pixelRefs[] = { m_controlMapArray, m_normalMapsArray, m_colorMapArray };
		GpuApi::BindTextures( 0, 3, pixelRefs, GpuApi::PixelShader );
		diffuseArray->Bind( 3, RST_PixelShader ); //< needs to be bound explicitly to force submitting the deferred context

		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_WrapPointMip, GpuApi::PixelShader );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( clipmapWindow.Min.X, clipmapWindow.Min.Y, clipmapWindow.Max.X, clipmapWindow.Max.Y ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( colorClipmapWindow.Min.X, colorClipmapWindow.Min.Y, colorClipmapWindow.Max.X, colorClipmapWindow.Max.Y ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)clipmapIndex, (Float)colorClipmapIndex, 0, 0 ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)rtSetup.viewport.x, (Float)rtSetup.viewport.y, (Float)rtSetup.viewport.width, (Float)rtSetup.viewport.height ) );

		// Draw pigment map
		{
			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
		}

		// Unbind shit
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );

		// Update global pigment state
		m_pigmentData.m_state.m_areaCenter = m_clipmapWindows[clipmapIndex].m_worldRect.CalcCenter();
		m_pigmentData.m_state.m_areaSize = m_clipmapWindows[clipmapIndex].m_worldRect.CalcSize();
		m_pigmentData.UpdateConstant();
	}
	else
	{
		ASSERT( s_pregenNormals && "Pregen normals needed for vegetation coloring - falling back to no coloring" );

		// Clear to default value
		GetRenderer()->ClearColorTarget( Vector( 1, 1, 1, 0 ) );

		// Update global pigment state
		m_pigmentData.m_state.m_areaCenter = Vector::ZEROS;
		m_pigmentData.m_state.m_areaSize = Vector::ZEROS;
		m_pigmentData.UpdateConstant();
	}

	// Update cached clipmapIndices
	m_pigmentData.m_state.m_clipmapIndex = clipmapIndex;
	m_pigmentData.m_state.m_colorClipmapIndex = colorClipmapIndex;

	// Restore wireframe setting
	GpuApi::SetRenderSettingsWireframe( origWireframe );
}

void CRenderProxy_Terrain::RenderGrassMap( CRenderSceneEx* scene )
{
	// Store previous render target setup, as we will use two custom ones for rendering tesselation erros and normal maps
	GpuApi::RenderTargetSetup previousSetup = GpuApi::GetRenderTargetSetup();

	// Save wireframe info
	const Bool origWireframe = GpuApi::GetRenderSettings().wireframe;

	// Disable wireframe state
	GpuApi::SetRenderSettingsWireframe( false );

	m_clipMapShaderData.BindTerrainParams( 5 );
	m_clipMapShaderData.BindTextureParams( 6 );

	GetRenderer()->m_grassMapGenerate->Bind();

	// Draw grass map
	{
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
		
		// Bind grass map as target
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, m_grassMap );
			rtSetup.SetViewportFromTarget( m_grassMap );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		// Bind params
		GpuApi::TextureRef pixelRefs[] = { m_controlMapArray, m_normalMapsArray };
		GpuApi::BindTextures( 0, 2, pixelRefs, GpuApi::PixelShader );

		// Calculate
		GpuApi::DrawInstancedPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 4, m_numLevels );
	}

	// Unbind shit
	GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );

	// Now - schedule a copy to the staging texture
	GetRenderer()->CopyTextureData( m_grassMap_Staging, 0, 0, m_grassMap, 0, 0 );
	m_grassMapStagingCopyInProgress = true;
	m_lastGrassMapStagingCopyIssuanceFrame = scene->GetLastAllocatedFrame();


	// Restore previous render target setup
	GpuApi::SetupRenderTargets( previousSetup );

	// Restore wireframe setting
	GpuApi::SetRenderSettingsWireframe( origWireframe );
}

void CRenderProxy_Terrain::ReadBackGrassMapMemoryFromStaging()
{
	RED_ASSERT( m_grassMapStagingCopyInProgress == true, TXT("DEBUG NOW!! Serious problem with auto-grass code!") );

	// Try to lock(map) the staging buffer
	Uint32 srcDataPitch = 0;
	void* srcDataPtr = GpuApi::LockLevel( m_grassMap_Staging, 0, 0, GpuApi::BLF_Read | GpuApi::BLF_DoNotWait, srcDataPitch );
	if ( srcDataPtr != NULL )
	{
		const Uint32 grassMapMemorySize = m_windowResolution * m_windowResolution * sizeof( Uint8 );
		if ( !m_grassMapData )
		{
			m_grassMapData = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_FoliageData, MC_FoliageGrassMask, grassMapMemorySize );
		}
		Red::System::MemoryCopy( m_grassMapData, srcDataPtr, grassMapMemorySize );
		GpuApi::UnlockLevel( m_grassMap_Staging, 0, 0 );

		m_grassMapStagingCopyInProgress = false;
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Tried to readback staging copy of the grass map. Couldn't do it. Either 2 frames delay was too short(!?) or the code needs fixing.") );
	}
}

//////////////////////////////////////////////////////////////////////////

void CRenderProxy_Terrain::GetClipmap0Params( Box& worldRect, Uint32& windowResolution, Float& minElevation, Float& maxElevation, Float& interVertexSpace )
{
	windowResolution = m_windowResolution;
	minElevation = m_lowestElevation;
	maxElevation = m_highestElevation;
	interVertexSpace = m_interVertexSpace;
	if ( m_clipmapWindows.Size() > 0 )
	{
		worldRect = m_clipmapWindows[0].m_worldRect;
	}
}

Float CRenderProxy_Terrain::GetUnfilteredHeightAtPos( const Vector2& uv )
{
	// Compute clipmap window texture coords
	Float xCoordF = uv.X * ( m_windowResolution - 1 );
	Float yCoordF = uv.Y * ( m_windowResolution - 1 );

	Int32 xCoord = (Int32)xCoordF;
	Int32 yCoord = (Int32)yCoordF;

	// Sample point height
	Uint16 heightMapVal = m_level0Heightmap[ yCoord * m_windowResolution + xCoord ];

	Float heightNorm = heightMapVal / 65536.0f;
	return m_lowestElevation + heightNorm * m_elevationRange;
}

Bool CRenderProxy_Terrain::IsDataReadyForGrassGeneration( Int64 frameIndex )
{
	if ( m_grassMapData )
	{
		if ( m_requestGrassMapUpdate || m_requestGrassMapUpdateWhenDataExists || m_grassMapStagingCopyInProgress || m_lastGrassMapStagingCopyIssuanceFrame == -1 )
		{
			// Data not ready. Log what happened. It will be postponed until next frame and it is perfectly fine as long as you don't see this message spaming the log.
			// Don't log if we're waiting for missing clipmap data. Otherwise we get spam until those tiles are loaded.
			if ( !m_requestGrassMapUpdateWhenDataExists )
			{
				//RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Postponing grass generation until data is ready (not spamming? it's fine then!)") );
			}
			return false;
		}
		if ( m_updateDataPending.Size() >= 2 )
		{
			// If we have stacking update pending, we don't even want to try it
			return false;
		}
		if ( m_updateDataPending.Size() == 1 && !m_updateDataPending.Front()->GetUpdates().Empty() )
		{
			// If we have just one update, we need to check if it contanins update rectangles - otherwise we may be failing unnecessarily
			return false;
		}

		// Data should be ready
		RED_ASSERT( frameIndex >= m_lastGrassMapStagingCopyIssuanceFrame );
		RED_ASSERT( frameIndex - m_lastGrassMapStagingCopyIssuanceFrame >= g_stagingCopyFramesDelay );
		return true;
	}
	
	// No grass map allocated yet
	return false;
}

const SAutomaticGrassDesc* CRenderProxy_Terrain::GetGrassDescriptor( Int32 texIndex, IRenderObject* object ) const
{
	const TDynArray< SAutomaticGrassDesc >& descriptors = m_grassSetup[texIndex];
	TDynArray< SAutomaticGrassDesc >::const_iterator it = descriptors.Begin();
	while ( it != descriptors.End() )
	{
		if ( it->GetResource() == object )
		{
			return &(*it);
		}
		++it;
	}

	return NULL;
}

void CRenderProxy_Terrain::UpdateGrassMask( Uint8* grassMaskUpdate, Uint32 grassMaskResUpdate )
{
	if ( m_grassMask )
	{
		RED_MEMORY_FREE( MemoryPool_FoliageData, MC_FoliageGrassMask, m_grassMask );
		m_grassMask = nullptr;
	}

	m_grassMask = grassMaskUpdate;
	m_grassMaskRes = grassMaskResUpdate;

#ifndef NO_HEIGHTMAP_EDIT
	if ( m_grassMask && !m_grassMaskPreview )
	{
		// Create heightmap stamp texture
		GpuApi::TextureDesc texDesc;
		texDesc.type		= GpuApi::TEXTYPE_2D;
		texDesc.format		= GpuApi::TEXFMT_R8_Uint;
		texDesc.initLevels	= 1;
		texDesc.usage		= GpuApi::TEXUSAGE_Samplable;

		// We have bits compressed inside bytes, so account for that by specifying a resolution divided by eight (2*4)
		texDesc.width		= m_grassMaskRes / 2;
		texDesc.height		= m_grassMaskRes / 4;
		texDesc.sliceNum	= 1;
		m_grassMaskPreview	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_System );
		ASSERT( m_grassMaskPreview );
		GpuApi::SetTextureDebugPath( m_grassMaskPreview, "grassMaskPreview" );
	}

	if ( m_grassMaskPreview )
	{
		GpuApi::LoadTextureData2D( m_grassMaskPreview, 0, 0, nullptr, m_grassMask, m_grassMaskRes * sizeof( Uint8 ) / 2 );
	}
#endif
}

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
void CRenderProxy_Terrain::SetGrassDensityVisualisationTexture( GpuApi::TextureRef tex, const Vector& rect, Bool visualisationEnabled )
{
	m_grassDensityDebug = tex;
	m_grassDensityRect = rect;
	m_grassVisualiserEnable = visualisationEnabled;
}

#endif

void CRenderProxy_Terrain::SetCustomBitmaskOverlay( Uint32 width, Uint32 height, Uint32* data )
{
	if ( data == nullptr )
	{
		if ( m_customOverlay != nullptr )
		{
			GpuApi::SafeRelease( m_customOverlay->ovarlayBitmap );
			delete m_customOverlay;
			m_customOverlay = nullptr;
		}
		return;
	}

	if ( m_customOverlay == nullptr )
	{
		m_customOverlay = new SCustomDebugOverlay();
	}

	if ( !m_customOverlay->ovarlayBitmap || m_customOverlay->bitmaskWidth != width || m_customOverlay->bitmaskHeight != height )
	{
		GpuApi::SafeRelease( m_customOverlay->ovarlayBitmap );

		m_customOverlay->bitmaskWidth	= width;
		m_customOverlay->bitmaskHeight	= height;

		GpuApi::TextureDesc texDesc;
		texDesc.type					= GpuApi::TEXTYPE_2D;
		texDesc.initLevels				= 1;
		texDesc.usage					= GpuApi::TEXUSAGE_Samplable;
		texDesc.width					= width;
		texDesc.height					= height;
		texDesc.sliceNum				= 1;
		texDesc.format					= GpuApi::TEXFMT_R8G8B8A8;

		m_customOverlay->ovarlayBitmap	= GpuApi::CreateTexture( texDesc, GpuApi::TEXG_System );
		if ( m_customOverlay->ovarlayBitmap )
		{
			GpuApi::SetTextureDebugPath( m_customOverlay->ovarlayBitmap, "customBitmaskOverlay" );
			GpuApi::AddDynamicTexture( m_customOverlay->ovarlayBitmap, "customBitmaskOverlay" );
		}
	}

	if ( m_customOverlay->ovarlayBitmap )
	{
		GpuApi::LoadTextureData2D( m_customOverlay->ovarlayBitmap, 0, 0, nullptr, data, width * 4 );
	}
	else
	{
		if ( m_customOverlay != nullptr )
		{
			delete m_customOverlay;
			m_customOverlay = nullptr;
		}
	}
}

void CRenderProxy_Terrain::OptimizeGrassSetup()
{
	// Reset all settings
	for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE * MAX_AUTOMATIC_GRASS_TYPES; ++i )
	{
		m_lwGrassSetups[i] = SLWGrassSettings();
	}

	for ( Uint32 i=0; i<MAX_AUTOMATIC_GRASS_TYPES; ++i )
	{
		m_indexToGrassAssignments[i] = SGrassIndexPair();
	}

	// Make new settings
	for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE; ++i )
	{
		TDynArray< SAutomaticGrassDesc >& brush = m_grassSetup[i];
		for ( Uint32 j=0; j<brush.Size(); ++j )
		{
			SAutomaticGrassDesc& grassDesc = brush[j];
			Int32 grassIndex = AssignIndexToGrassType( grassDesc.GetResource() );

			SLWGrassSettings& settings = m_lwGrassSetups[ grassIndex * NUM_TERRAIN_TEXTURES_AVAILABLE + i ];
			settings.m_isOn = 1.0f;
			settings.m_radiusScale = grassDesc.GetRadiusScale();
			settings.m_radiusScaleSquare = grassDesc.GetRadiusScaleSqr();
		}
	}
}

Int32 CRenderProxy_Terrain::AssignIndexToGrassType( IRenderObject* grassType )
{
	// NOTE:  It assumes there are no holes in the m_indexToGrassAssignments!
	// Note2: Also currently the index assigned is just the index in the array itself.
	for ( Uint32 i=0; i<MAX_AUTOMATIC_GRASS_TYPES; ++i )
	{
		if ( m_indexToGrassAssignments[i].m_grassResourceHandle.Get() == grassType )
		{
			// Already assigned
			RED_ASSERT( m_indexToGrassAssignments[i].m_indexAssigned == i );
			return m_indexToGrassAssignments[i].m_indexAssigned;
		}

		if ( m_indexToGrassAssignments[i].m_indexAssigned == -1 )
		{
			// Empty slot, assign to it
			m_indexToGrassAssignments[i].m_indexAssigned = i;
			m_indexToGrassAssignments[i].m_grassResourceHandle.ResetFromExternal( grassType );
			return i;
		}
	}

	RED_HALT( "Too many automatic grass types!!!" );
	return -1;
}

Int32 CRenderProxy_Terrain::GetIndexForGrassType( IRenderObject* object )
{
	for ( Uint32 i=0; i<MAX_AUTOMATIC_GRASS_TYPES; ++i )
	{
		if ( m_indexToGrassAssignments[i].m_grassResourceHandle.Get() == object )
		{
			// Found
			return m_indexToGrassAssignments[i].m_indexAssigned;
		}
	}

	// Not found
	return -1;
}

class CTerrainTextureData
{
private:
	const GpuApi::TextureRef& m_destTex;
	const GpuApi::TextureDesc* m_destDesc;
	void* m_stagingMem;
	GpuApi::TextureDesc m_desc;
	GpuApi::TextureRef m_ref;

	Uint32 m_stagingPitch;

public:
	CTerrainTextureData( const GpuApi::TextureRef& destTex )
	:	m_destTex( destTex )
	,	m_stagingMem( nullptr )
	,	m_stagingPitch( 0 )
	{
	}

	RED_INLINE Bool IsValid() const { return !!m_stagingMem; }

	void Create( SClipmapLevelUpdate* update )
	{
		// Allocate some temporary memory via a Staging texture
		// Copy srcMemory into Staging texture
		// Use Compute to copy the Staging data to the correct mip/slice in the destTex texture
		// Add Staging texture to a queue to be deleted once finished with
		m_destDesc = &( GpuApi::GetTextureDesc( m_destTex ) );

		// Create staging texture to hold this slice/mip
		m_desc.type				= GpuApi::TEXTYPE_2D;
		m_desc.width			= update->m_resolution;
		m_desc.height			= update->m_resolution;
		m_desc.initLevels		= 1;
		m_desc.sliceNum			= 1;
		m_desc.usage			= GpuApi::TEXUSAGE_StagingWrite;
		m_desc.format			= m_destDesc->format;
		m_desc.msaaLevel		= m_destDesc->msaaLevel;
		m_desc.inPlaceType		= GpuApi::INPLACE_None;

		m_ref					= GpuApi::CreateTexture( m_desc, GpuApi::TEXG_Generic );

		if( m_ref )
		{
			m_stagingMem		= GpuApi::LockLevel( m_ref, 0, 0, GpuApi::BLF_Write | GpuApi::BLF_DoNotWait, m_stagingPitch );
			RED_ASSERT( m_stagingMem );
		}
	}


	static void FillExternal( SClipmapLevelUpdate* update, SClipRegionUpdateRectangle& updateRect, Uint16* heightmap, TControlMapType* controlMap, Uint32 resolution )
	{
		RED_FATAL_ASSERT( heightmap != nullptr, "Can't fill into null heightmap" );
		RED_FATAL_ASSERT( controlMap != nullptr, "Can't fill into null controlmap" );

		SClipRegionRectangleData& commonRectData = update->m_commonRectData[ updateRect.m_commonRectDataIndex ];

		const Uint16* heightMapData = static_cast< const Uint16* >( updateRect.m_heightMapBuffer->GetData() );
		const TControlMapType* controlMapData = static_cast< const TControlMapType* >( updateRect.m_controlMapBuffer->GetData() );

		for ( Int32 i = 0; i < commonRectData.m_texelsToCopyFromTile_Vertical; ++i )
		{
			Uint32 srcIndex = ( commonRectData.m_offsetInTileV + i ) * updateRect.m_resolution + commonRectData.m_offsetInTileU;
			Uint32 dstIndex = ( commonRectData.m_clipRegionYWriteOffset + i ) * update->m_resolution + commonRectData.m_clipRegionXWriteOffset;

			const Uint16* sourceTexels = &heightMapData[ srcIndex ];
			const TControlMapType* sourceCMTexels = &controlMapData[ srcIndex ];

			Uint16* targetTexels = &heightmap[ dstIndex ];
			TControlMapType* targetCMTexels = &controlMap[ dstIndex ];

			RED_FATAL_ASSERT( ( targetTexels - heightmap ) + ( commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( Uint16 ) ) <= ( resolution * resolution * sizeof( Uint16 ) ), "Buffer overrun" );
			RED_FATAL_ASSERT( ( targetCMTexels - controlMap ) + ( commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( TControlMapType ) ) <= ( resolution * resolution * sizeof( TControlMapType ) ), "Buffer overrun" );

			Red::System::MemoryCopy( targetTexels, sourceTexels, commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( Uint16 ) );
			Red::System::MemoryCopy( targetCMTexels, sourceCMTexels, commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( TControlMapType ) );
		}
	}

	static void Fill( SClipmapLevelUpdate* update, SClipRegionUpdateRectangle& updateRect, CTerrainTextureData& heightmap, CTerrainTextureData& controlMap )
	{
		RED_FATAL_ASSERT( heightmap.IsValid(), "Check before calling Fill()" );
		RED_FATAL_ASSERT( controlMap.IsValid(), "Check before calling Fill()" );

		Uint16* fullRegionTexels = static_cast< Uint16* >( heightmap.m_stagingMem );
		TControlMapType* fullRegionCMTexels = static_cast< TControlMapType* >( controlMap.m_stagingMem );
		SClipRegionRectangleData& commonRectData = update->m_commonRectData[ updateRect.m_commonRectDataIndex ];

		const Uint16* heightMapData = static_cast< const Uint16* >( updateRect.m_heightMapBuffer->GetData() );
		const TControlMapType* controlMapData = static_cast< const TControlMapType* >( updateRect.m_controlMapBuffer->GetData() );

		for ( Int32 i = 0; i < commonRectData.m_texelsToCopyFromTile_Vertical; ++i )
		{
			Uint32 srcIndex = ( commonRectData.m_offsetInTileV + i ) * updateRect.m_resolution + commonRectData.m_offsetInTileU;
			Uint32 dstIndex = ( commonRectData.m_clipRegionYWriteOffset + i ) * heightmap.m_stagingPitch / 2 + commonRectData.m_clipRegionXWriteOffset;

			const Uint16* sourceTexels = &heightMapData[ srcIndex ];
			const TControlMapType* sourceCMTexels = &controlMapData[ srcIndex ];

			Uint16* targetTexels = &fullRegionTexels[ dstIndex ];
			TControlMapType* targetCMTexels = &fullRegionCMTexels[ dstIndex ];

			RED_FATAL_ASSERT( ( targetTexels - fullRegionTexels ) + ( commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( Uint16 ) ) <= ( heightmap.m_stagingPitch * heightmap.m_desc.height ), "Buffer overrun" );
			RED_FATAL_ASSERT( ( targetCMTexels - fullRegionCMTexels ) + ( commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( TControlMapType ) ) <= ( controlMap.m_stagingPitch * controlMap.m_desc.height ), "Buffer overrun" );

			Red::System::MemoryCopy( targetTexels, sourceTexels, commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( Uint16 ) );
			Red::System::MemoryCopy( targetCMTexels, sourceCMTexels, commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( TControlMapType ) );
		}
	}

	RED_INLINE static void Fill( SClipmapLevelUpdate* update, SColorMapRegionUpdateRectangle& updateRect, CTerrainTextureData& colorMap )
	{
		RED_FATAL_ASSERT( colorMap.IsValid(), "Check before calling Fill()" );

		if( updateRect.m_cooked )
		{
			FillCooked( update, updateRect, colorMap );
		}
		else
		{
			FillUncooked( update, updateRect, colorMap );
		}
	}

private:
	static void FillCooked( SClipmapLevelUpdate* update, SColorMapRegionUpdateRectangle& updateRect, CTerrainTextureData& colorMap )
	{
		SClipRegionRectangleData& commonRectData = update->m_commonRectData[ updateRect.m_commonRectDataIndex ];

		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_offsetInTileU ) );
		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_offsetInTileV ) );
		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_clipRegionYWriteOffset ) );
		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_clipRegionXWriteOffset ) );
		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_texelsToCopyFromTile_Horizontal ) );
		RED_ASSERT( TerrainUtils::IsExactlyColorMapBlock( commonRectData.m_texelsToCopyFromTile_Vertical ) );

		const Uint32 blockStartInTileU = TerrainUtils::ColorMapTexelToBlock( commonRectData.m_offsetInTileU );
		const Uint32 blockStartInTileV = TerrainUtils::ColorMapTexelToBlock( commonRectData.m_offsetInTileV );

		const Uint32 blockStartInClipU = TerrainUtils::ColorMapTexelToBlock( commonRectData.m_clipRegionXWriteOffset );
		const Uint32 blockStartInClipV = TerrainUtils::ColorMapTexelToBlock( commonRectData.m_clipRegionYWriteOffset );

		const Uint32 blocksToCopy_Vertical = TerrainUtils::ColorMapTexelToBlock( commonRectData.m_texelsToCopyFromTile_Vertical );
		const Uint32 copyPitch = TerrainUtils::CalcColorMapPitch( commonRectData.m_texelsToCopyFromTile_Horizontal );

		const TColorMapRawType* colorData = static_cast< const TColorMapRawType* >( updateRect.m_buffer->GetData() );

		for ( Uint32 i = 0; i < blocksToCopy_Vertical; ++i )
		{
			// pitch, in blocks
			const Uint32 tilePitch = TerrainUtils::ColorMapTexelToBlock( updateRect.m_resolution );
			const Uint32 clipPitch = TerrainUtils::ColorMapTexelToBlock( colorMap.m_stagingPitch / 2 );

			Uint32 csrcIndex = ( blockStartInTileV + i ) * tilePitch + blockStartInTileU;
			Uint32 cdstIndex = ( blockStartInClipV + i ) * clipPitch + blockStartInClipU;

			const TColorMapRawType* sourceColorMapTexels = colorData + csrcIndex;
			TColorMapRawType* targetColorMapTexels = static_cast< TColorMapRawType* >( colorMap.m_stagingMem ) + cdstIndex;

			Red::System::MemoryCopy( targetColorMapTexels, sourceColorMapTexels, copyPitch );
		}
	}

	static void FillUncooked( SClipmapLevelUpdate* update, SColorMapRegionUpdateRectangle& updateRect, CTerrainTextureData& colorMap )
	{
		SClipRegionRectangleData& commonRectData = update->m_commonRectData[ updateRect.m_commonRectDataIndex ];
		const TColorMapType* colorData = static_cast< const TColorMapType* >( updateRect.m_buffer->GetData() );

		for ( Int32 i = 0; i < commonRectData.m_texelsToCopyFromTile_Vertical; ++i )
		{
			Uint32 srcIndex = ( commonRectData.m_offsetInTileV + i ) * updateRect.m_resolution + commonRectData.m_offsetInTileU;
			Uint32 dstIndex = ( commonRectData.m_clipRegionYWriteOffset + i ) * update->m_resolution + commonRectData.m_clipRegionXWriteOffset;

			const TColorMapType* sourceColorMapTexels = colorData + srcIndex;
			TColorMapType* targetColorMapTexels = static_cast< TColorMapType* >( colorMap.m_stagingMem ) + dstIndex;

			Red::System::MemoryCopy( targetColorMapTexels, sourceColorMapTexels, commonRectData.m_texelsToCopyFromTile_Horizontal * sizeof( TColorMapType ) );
		}
	}

public:
	RED_INLINE void Copy( void* destination, Uint32 size ) const
	{
		RED_FATAL_ASSERT( size <= ( m_stagingPitch * m_desc.height ), "Buffer overrun" );
		Red::System::MemoryCopy( destination, m_stagingMem, size );
	}

	void Release( SClipmapLevelUpdate* update )
	{
		if( m_stagingMem )
		{
			m_stagingMem = nullptr;
			GpuApi::UnlockLevel( m_ref, 0, 0 );

			const Uint32 mipLevel = 0;
			const Uint32 arraySlice = update->m_level;

			const Uint32 width = update->m_resolution;
			const Uint32 height = update->m_resolution;

			GpuApi::Rect srcRect(0, 0, width, height);
			GpuApi::Rect dstRect( 0, 0, width, height);

			// ok now initiate the copy on GPU
			GetRenderer()->CopyTextureData( m_destTex, mipLevel, arraySlice, dstRect, m_ref, 0, 0, srcRect );
		}

		// finally release the temporary texture which will be destroyed at the end of the next frame
		GpuApi::SafeRelease( m_ref );
	}
};

// Whole texture is already there (created by first frame operation)
void CRenderProxy_Terrain::PerformTerrainUpdateTileComplete( SClipmapLevelUpdate* update )
{
	RED_FATAL_ASSERT( update->m_complete, "Wrong function called" );

	const Uint32 numUpdateRects = update->m_updateRects.Size();
	RED_FATAL_ASSERT( numUpdateRects == 1, "Invalid assumption made about terrain update" );

	for ( Uint32 r = 0; r < numUpdateRects; ++r )
	{
		SClipRegionUpdateRectangle& updateRect = update->m_updateRects[ r ];

		GpuApi::Rect destRect;
		destRect.left = 0;
		destRect.top = 0;
		destRect.right = updateRect.m_resolution;
		destRect.bottom = updateRect.m_resolution;

		GetRenderer()->LoadTextureData2D( m_heightmapArray, 0, update->m_level, &destRect, updateRect.m_heightMapBuffer->GetData(), ( updateRect.m_resolution ) * sizeof( Uint16 ) );
		GetRenderer()->LoadTextureData2D( m_controlMapArray, 0, update->m_level, &destRect, updateRect.m_controlMapBuffer->GetData(), ( updateRect.m_resolution ) * sizeof( Uint16 ) );
	}
}

// Temporary staging texture needs to be constructed
void CRenderProxy_Terrain::PerformTerrainUpdateTileIncomplete( SClipmapLevelUpdate* update )
{
	RED_FATAL_ASSERT( !update->m_complete, "Wrong function called" );


	ASSERT( update->m_level < m_numLevels );

	CTerrainTextureData heightMap( m_heightmapArray );
	CTerrainTextureData controlMap( m_controlMapArray );

	heightMap.Create( update );
	controlMap.Create( update );

	if( heightMap.IsValid() && controlMap.IsValid() )
	{
		const Uint32 numUpdateRects = update->m_updateRects.Size();
		for ( Uint32 r = 0; r < numUpdateRects; ++r )
		{
			SClipRegionUpdateRectangle& updateRect = update->m_updateRects[ r ];

			CTerrainTextureData::Fill( update, updateRect, heightMap, controlMap );
		}

		//////////////////////////////////////////////////////////////////////////
		// Store highest clipmap level for vegetation generation
		if ( update->m_level == 0 )
		{
			for ( Uint32 r = 0; r < numUpdateRects; ++r )
			{
				SClipRegionUpdateRectangle& updateRect = update->m_updateRects[ r ];
				CTerrainTextureData::FillExternal( update, updateRect, m_level0Heightmap, m_level0ControlMap, m_windowResolution );
			}
		}
	}


	heightMap.Release( update );
	controlMap.Release( update );
}

void CRenderProxy_Terrain::PerformTerrainUpdateColorTileComplete( SClipmapLevelUpdate* update )
{
	RED_FATAL_ASSERT( update->m_complete, "Wrong function called" );

	const Uint32 numColorUpdateRects = update->m_colormapUpdateRects.Size();
	RED_FATAL_ASSERT( numColorUpdateRects == 1, "Invalid assumption made about terrain update" );

	for ( Uint32 r = 0; r < numColorUpdateRects; ++r )
	{
		SColorMapRegionUpdateRectangle& updateRect = update->m_colormapUpdateRects[ r ];

		//ASSERT( updateRect.m_targetLevel == update->m_level );
		ASSERT( updateRect.m_resolution <= m_windowResolution );

		GpuApi::Rect destRect;
		destRect.left = 0;
		destRect.top = 0;
		destRect.right = updateRect.m_resolution;
		destRect.bottom = updateRect.m_resolution;

		Uint32 pitch;
		if ( m_isColorMapCompressed )
		{
			pitch = TerrainUtils::CalcColorMapPitch( updateRect.m_resolution );
		}
		else
		{
			pitch = updateRect.m_resolution * sizeof( TColorMapType );
		}


		GetRenderer()->LoadTextureData2D( m_colorMapArray, 0, update->m_level, &destRect, updateRect.m_buffer->GetData(), pitch );
	}
}

void CRenderProxy_Terrain::PerformTerrainUpdateColorTileIncomplete( SClipmapLevelUpdate* update )
{
	RED_FATAL_ASSERT( !update->m_complete, "Wrong function called" );

	CTerrainTextureData colorMap( m_colorMapArray );
	colorMap.Create( update );

	if( colorMap.IsValid() )
	{
		const Uint32 numColorUpdateRects = update->m_colormapUpdateRects.Size();
		for ( Uint32 r = 0; r < numColorUpdateRects; ++r )
		{
			SColorMapRegionUpdateRectangle& updateRect = update->m_colormapUpdateRects[ r ];

			CTerrainTextureData::Fill( update, updateRect, colorMap );
		}
	}

	colorMap.Release( update );
}

void CRenderProxy_Terrain::PerformTerrainUpdate(CRenderTerrainUpdateData* updateData)
{
	FinishBuildingQuadTree();

	const TDynArray< SClipmapLevelUpdate* >& updates = updateData->GetUpdates();

	// Update colormap starting index
	m_colormapStartingIndex = (Uint32)updateData->GetColormapParams().X;

	// Update clipmap windows
	for ( Uint32 u=0; u<updates.Size(); ++u )
	{
		// Fetch a single update
		SClipmapLevelUpdate* update = updates[u];
		ASSERT( update->m_level < m_numLevels );

		// Update the world space that can be represented with this level's window
		m_clipmapWindows[update->m_level].m_worldRect = update->m_worldSpaceCoveredByThisLevel;
		m_clipmapWindows[update->m_level].SetValidTexels( update->m_validTexels, m_windowResolution );
	}

	// Calculate pigment clipmap indices
	Uint32 pigmentClipmapIndex = 0;
	Uint32 pigmentColorClipmapIndex = 0;
	{
		while ( pigmentClipmapIndex + 1 < m_clipmapWindows.Size() )
		{
			const Vector currSize = m_clipmapWindows[pigmentClipmapIndex].m_worldRect.CalcSize();
			if ( 1.001f * Max( currSize.X, currSize.Y ) > 200.f )
			{
				break;
			}

			++pigmentClipmapIndex;
		}

		pigmentColorClipmapIndex = Max( pigmentClipmapIndex, m_colormapStartingIndex );
	}

	// Update clipmap/colormap data
	Bool newIntermediateMapsData = false;
	Bool newPigmentMapData = pigmentClipmapIndex != m_pigmentData.m_state.m_clipmapIndex || pigmentColorClipmapIndex != m_pigmentData.m_state.m_colorClipmapIndex;
	for ( Uint32 u=0; u<updates.Size(); ++u )
	{
		// Fetch a single update
		SClipmapLevelUpdate* update = updates[u];

		if( update->m_complete )
		{
			PerformTerrainUpdateTileComplete( update );
			PerformTerrainUpdateColorTileComplete( update );
		}
		else
		{
			PerformTerrainUpdateTileIncomplete( update );
			PerformTerrainUpdateColorTileIncomplete( update );
		}

		if( update->m_updateRects.Size() > 0 )
		{
			newIntermediateMapsData = true;
			m_clipmapWindows[ update->m_level ].MarkIntermediateMapsRegeneration( true );
			RED_LOG( RED_LOG_CHANNEL( Terrain ), TXT("==================Flagging level %i for regen on frame %i."), update->m_level, GpuApi::FrameIndex() );

			if ( pigmentClipmapIndex == update->m_level )
			{
				newPigmentMapData = true;
			}
		}

		if( update->m_colormapUpdateRects.Size() > 0 )
		{
			if ( pigmentColorClipmapIndex == update->m_level )
			{
				newPigmentMapData = true;
			}
		}
	}

	// If material was changed - apply it.
	// Needs to be before intermediate maps generation because of the pigment map
	CRenderMaterial* newMaterial = static_cast< CRenderMaterial* >( updateData->m_material );
	if ( newMaterial && ( newMaterial != m_material ) )
	{
		if ( m_material )
		{
			m_material->Release();
		}

		m_material = newMaterial;
		m_material->AddRef();

		m_requestPigmentMapUpdateWhenDataExists = true;
	}

	// Update tesselation block errors maps and normal maps
	if ( newIntermediateMapsData )
	{
		m_requestIntermediateMapsUpdate = true;
	}

	if ( m_clipmapWindows[ 0 ].NeedsIntermediateMapsRegeneration() || m_requestGrassMapUpdateWhenDataExists )
	{
		if ( m_clipmapWindows[0].IsAllValid()
#ifndef NO_EDITOR
			&& !m_isEditing
#endif
			)
		{
			m_requestGrassMapUpdate = true;
		}
		else
		{
			m_requestGrassMapUpdateWhenDataExists = true;
		}
	}

#ifndef NO_HEIGHTMAP_EDIT
	// update stamp data
	m_renderStampVisualization = false;
	if ( updateData->IsStampUpdateValid() )
	{
		m_renderStampVisualization = true;

		bool haveStampHeight = ( updateData->GetStampData() != NULL );
		bool haveStampColor = ( updateData->GetStampColorData() != NULL );
		bool haveStampControl = ( updateData->GetStampControlData() != NULL );

		Float stampRotation = updateData->GetStampRotation();
		Float stampSize = updateData->GetStampSize();
		Vector2 stampCenter = updateData->GetStampCenter();

		m_stampCenterAxis.X = stampCenter.X;
		m_stampCenterAxis.Y = stampCenter.Y;
		m_stampCenterAxis.Z = MCos( stampRotation ) * stampSize / 2.0f;
		m_stampCenterAxis.W = MSin( stampRotation ) * stampSize / 2.0f;

		m_stampVertexSettings = Vector( updateData->GetStampHeightScale(), updateData->GetStampHeightOffset(), updateData->GetStampModeAdditive() ? 1.0f : 0.0f, 0.0f );

		m_stampPixelSettings.Z = (Float)updateData->GetStampOriginalTexelSize();
		m_stampPixelSettings.W = stampSize;


		if ( updateData->IsStampDataDirty() )
		{
			if ( haveStampHeight )
			{
				GpuApi::Rect r( 0, 0, updateData->GetStampDataPitch() / sizeof( Uint16 ), updateData->GetStampDataPitch() / sizeof( Uint16 ) );
				GpuApi::LoadTextureData2D( m_heightmapStamp, 0, 0, &r, updateData->GetStampData(), updateData->GetStampDataPitch() );
			}
		}
		if ( updateData->IsStampColorDataDirty() )
		{
			if ( haveStampColor )
			{
				GpuApi::Rect r( 0, 0, updateData->GetStampColorDataPitch() / sizeof( TColorMapType ), updateData->GetStampColorDataPitch() / sizeof( TColorMapType ) );
				GpuApi::LoadTextureData2D( m_colorStamp, 0, 0, &r, updateData->GetStampColorData(), updateData->GetStampColorDataPitch() );
			}
			m_stampPixelSettings.X = haveStampColor ? 1.0f : 0.0f;
		}
		if ( updateData->IsStampControlDataDirty() )
		{
			if ( haveStampControl )
			{
				GpuApi::Rect r( 0, 0, updateData->GetStampControlDataPitch() / sizeof( TControlMapType ), updateData->GetStampControlDataPitch() / sizeof( TControlMapType ) );
				GpuApi::LoadTextureData2D( m_controlStamp, 0, 0, &r, updateData->GetStampControlData(), updateData->GetStampControlDataPitch() );
			}
			m_stampPixelSettings.Y = haveStampControl ? 1.0f : 0.0f;
		}
	}
#endif

	// Update constants and texture params
	{
		m_clipMapShaderData.UpdateTerrainParams( this );
		m_clipMapShaderData.UpdateClipWindows( this );
		const Vector* textureParams = updateData->GetTextureParams();
		m_clipMapShaderData.UpdateTextureParams( textureParams );
	}

	// Request pigment map update
	if ( newPigmentMapData || m_requestPigmentMapUpdateWhenDataExists )
	{
		if ( m_clipmapWindows[pigmentClipmapIndex].IsAllValid() )
		{
			m_requestPigmentMapUpdate = true;
			m_pigmentMapUpdateClipmapIndex = pigmentClipmapIndex;
			m_pigmentMapUpdateColorClipmapIndex = pigmentColorClipmapIndex;
		}
		else
		{
			m_requestPigmentMapUpdateWhenDataExists = true;
		}
	}
}

void CRenderProxy_Terrain::FlushTerrainUpdates()
{
	while ( m_updateDataPending.Size() )
	{
		CRenderTerrainUpdateData* upd = m_updateDataPending.Front();
		RED_ASSERT( upd );
		PerformTerrainUpdate( upd );
		upd->Release();
		m_updateDataPending.Pop();
	}
}

CRenderMaterial* CRenderProxy_Terrain::GetNewestMaterial()
{
	// If we have pending updates, see if we have a material there to give.
	if ( !m_updateDataPending.Empty() )
	{
		// Since it's all in a queue, we only really have access to the ends. Shouldn't have more than one update though,
		// so we'll just give back (the last one that would be flushed)
		if ( CRenderTerrainUpdateData* upd = m_updateDataPending.Back() )
		{
			if ( upd->m_material != nullptr )
			{
				return static_cast< CRenderMaterial* >( upd->m_material );
			}
		}
	}
	return m_material;
}

IRenderProxy* CRenderInterface::CreateTerrainProxy( const IRenderObject* initData, const SClipmapParameters& clipmapParams )
{
	ASSERT( initData );
	const CRenderTerrainUpdateData* initDataCast = static_cast< const CRenderTerrainUpdateData* >( initData );
	ASSERT( initDataCast );
	return new CRenderProxy_Terrain( initDataCast, clipmapParams );
}

/*-------------------------------------------------------------------------------------------*/

CBuildQuadTreeTask::CBuildQuadTreeTask( CRenderProxy_Terrain* renderProxy,
#ifdef USE_UMBRA
									   const CRenderOcclusionData* occlusionData, 
#endif // USE_UMBRA
									   const CRenderFrameInfo& frameInfo, Bool shouldCollectWithUmbra )
	: m_renderProxy( renderProxy)
#ifdef USE_UMBRA
	, m_occlusionData( occlusionData )
#endif // USE_UMBRA
{
	m_cullTerrainWithUmbra	= frameInfo.IsShowFlagOn( SHOW_UmbraCullTerrain ) && shouldCollectWithUmbra;
	m_showSkirt				= frameInfo.IsShowFlagOn( SHOW_Skirt );
	m_renderDeepTerrain		= frameInfo.IsShowFlagOn( SHOW_RenderDeepTerrain );
	m_cullFullHeight		= frameInfo.IsShowFlagOn( SHOW_CullTerrainWithFullHeight );

	m_cameraPosition = frameInfo.m_occlusionCamera.GetPosition();
	m_frustum = new CFrustum( frameInfo.m_occlusionCamera.GetWorldToScreen() );
}

CBuildQuadTreeTask::~CBuildQuadTreeTask()
{
	if ( m_frustum )
	{
		delete m_frustum;
		m_frustum = nullptr;
	}
}

void CBuildQuadTreeTask::Run()
{
	PC_SCOPE( BuildQuadTreeScope );

	BuildQuadTree();	
}

void CBuildQuadTreeTask::BuildQuadTree()
{
	CRenderProxy_Terrain& renderProxy = (*m_renderProxy);
	Float terrainEdgeLengthPowerOf2 = renderProxy.m_terrainEdgeLengthPowerOf2;
	Float lowestElevation = renderProxy.m_lowestElevation;
	Float highestElevation = renderProxy.m_highestElevation;
	Int32 maxQuadTreeLevel = renderProxy.m_maxQuadTreeLevel;
	Uint16 numLevels = renderProxy.m_numLevels;
	Int32 tilesPerEdge = (Int32)renderProxy.m_tilesPerEdge;

	const TDynArray< SClipmapWindow >&      clipmapWindows = renderProxy.m_clipmapWindows;
	const TDynArray< Vector2 >&	            tileHeightRange = renderProxy.m_tileHeightRange;
	const TDynArray< Float >&			    minWaterLevels = renderProxy.m_minWaterLevels;

	TDynArray< SQuadTreeNode >&             quadTreeStack = renderProxy.m_quadTreeStack;
	TDynArray< STerrainPatchInstanceData >& instances = renderProxy.m_instances;
	TDynArray< SSkirtPatchInstanceData >&   skirtInstances = renderProxy.m_skirtInstances;

	const Box wholeClipRect = clipmapWindows.Back().m_worldRect;
	const Float worldToTileScale = tilesPerEdge / ( wholeClipRect.Max.X - wholeClipRect.Min.X );

	const Vector2& bias = Vector2( -terrainEdgeLengthPowerOf2 / 2.0f, -terrainEdgeLengthPowerOf2 / 2.0f );
	Int32 level = 0;

	// Scales of a quad tree nodes for different clipmap levels
	Float scales[MAX_CLIPMAP_COUNT];
	scales[maxQuadTreeLevel] = terrainEdgeLengthPowerOf2;
	for( Int32 i = maxQuadTreeLevel-1; i >= 0; i-- )
	{
		scales[i] = scales[i+1] / 2.0f;
	}

	// stack
	Int32 nodeIdx = 0;
	quadTreeStack[nodeIdx].bias = bias;
	quadTreeStack[nodeIdx].level = level;

	while( nodeIdx > -1 )
	{
		//pop
		const SQuadTreeNode& node = quadTreeStack[nodeIdx--];
		const Vector2& bias = node.bias;
		const Int32 level = node.level;

		Int32 correspondingClipmapLevel = maxQuadTreeLevel - level;
		Float scale = scales[correspondingClipmapLevel];

		// Compute bounding box of the node
		Box bb( Vector( bias.X, bias.Y, lowestElevation ), Vector( bias.X+scale, bias.Y+scale, highestElevation ) );

		Float minimumWater = highestElevation;
		Float minimumHeight = highestElevation;
		Float maximumHeight = lowestElevation;
		const Bool checkAgainstWater = !m_renderDeepTerrain && !minWaterLevels.Empty();
		if ( checkAgainstWater || !m_cullFullHeight )
		{
			// TODO : Maybe instead of a flat array with one value per tile, could build a mip chain sort of thing, so we can do a single
			// lookup here... Not worrying about it now, this doesn't seem to be very expensive (didn't see any noticeable impact on
			// xbox skellige).

			Int32 umin = (Int32)MFloor( ( bb.Min.X - wholeClipRect.Min.X ) * worldToTileScale );
			Int32 vmin = (Int32)MFloor( ( bb.Min.Y - wholeClipRect.Min.Y ) * worldToTileScale );
			Int32 umax = (Int32)MCeil(  ( bb.Max.X - wholeClipRect.Min.X ) * worldToTileScale );
			Int32 vmax = (Int32)MCeil(  ( bb.Max.Y - wholeClipRect.Min.Y ) * worldToTileScale );

			umin = Clamp( umin, 0, (Int32)tilesPerEdge - 1 );
			vmin = Clamp( vmin, 0, (Int32)tilesPerEdge - 1 );
			umax = Clamp( umax, 0, (Int32)tilesPerEdge );
			vmax = Clamp( vmax, 0, (Int32)tilesPerEdge );

			for ( Int32 v = vmin; v < vmax; ++v )
			{
				for ( Int32 u = umin; u < umax; ++u )
				{
					const Vector2& heightRange = tileHeightRange[u + v * tilesPerEdge];
					minimumHeight = Min( minimumHeight, heightRange.X );
					maximumHeight = Max( maximumHeight, heightRange.Y );

					if ( checkAgainstWater )
					{
						minimumWater = Min( minimumWater, minWaterLevels[u + v * tilesPerEdge] );
					}
				}
			}

			if ( !m_cullFullHeight )
			{
				bb.Min.Z = minimumHeight;
				bb.Max.Z = maximumHeight;
			}
		}

		if ( checkAgainstWater && maximumHeight < minimumWater - MAXIMUM_RENDER_DEPTH )
		{
			continue;
		}

		// Cull node
#ifdef USE_UMBRA
		if ( m_cullTerrainWithUmbra )
		{
#ifndef RED_FINAL_BUILD
			CTimeCounter timer;
#endif // RED_FINAL_BUILD
			// the thresholdSquared is the distance around the camera slightly lower than the squared radius that umbra data
			// is loaded within. Further than that distance we do not have certanity that we have the data loaded,
			// so calling IsDynamicObjectVisible() can return unpredicted results. We're using a fallback method then
			const Float thresholdSquared = m_occlusionData->GetDataDistanceThresholdSquared();
			Float distanceSquared = m_cameraPosition.DistanceSquaredTo2D( bb.CalcCenter() );
			Bool isVisible = true;
			if ( distanceSquared <= thresholdSquared )
			{
				isVisible = m_occlusionData->IsDynamicObjectVisible( bb );
			}
			else
			{
				isVisible = m_frustum->TestBox( bb ) != FR_Outside;
			}
#ifndef RED_FINAL_BUILD
			extern SceneRenderingStats GRenderingStats;
			GRenderingStats.m_occlusionTimeTerrain += timer.GetTimePeriodMS();
#endif // RED_FINAL_BUILD
			if ( !isVisible )
			{
				continue;
			}
		}
		else
#endif // USE_UMBRA
		{
			if ( m_frustum->TestBox( bb ) == FR_Outside )
			{
				continue;
			}
		}

		Bool split = correspondingClipmapLevel >= (Int32)numLevels;

		if ( !split && correspondingClipmapLevel - 1 >= 0 )
		{
			// See if we can split further
			const Box window = clipmapWindows[correspondingClipmapLevel - 1].m_worldRect;
			if ( bias.X > window.Min.X && bias.Y > window.Min.Y && bias.X+scale < window.Max.X && bias.Y+scale < window.Max.Y )
			{
				split = true;
			}
		}

		//if ( projectedError <= Criterion || level == m_maxQuadTreeLevel )
		if ( !split )
		{
			// Don't render nodes outside of the clipmap
			if ( clipmapWindows.Back().m_worldRect.Contains2D( bb ) )
			{
				if ( m_showSkirt && correspondingClipmapLevel - 1 >= 0 )
				{
					const Box w = clipmapWindows[correspondingClipmapLevel-1].m_worldRect;
					// temp variables
					Float xPlusScale = bias.X + scale;
					Float yPlusScale = bias.Y + scale;

					if( bias.Y >= w.Min.Y && yPlusScale <= w.Max.Y )
					{
						if( xPlusScale > w.Min.X && bias.X <= w.Min.X )
						{
							// left-border 
							skirtInstances.Grow(1);
							SSkirtPatchInstanceData& instance = skirtInstances.Back();
							instance.m_patchBias = Vector(xPlusScale, bias.Y, (Float)correspondingClipmapLevel, 0.0f);
						}
						else if( bias.X < w.Max.X && xPlusScale >= w.Max.X )
						{
							// right-border
							skirtInstances.Grow(1);
							SSkirtPatchInstanceData& instance = skirtInstances.Back();
							instance.m_patchBias = Vector(bias.X, yPlusScale, (Float)correspondingClipmapLevel, 1.0f);
						}
					}

					if( bias.X >= w.Min.X && xPlusScale <= w.Max.X )
					{
						if( bias.Y < w.Max.Y && yPlusScale >= w.Max.Y )
						{
							// top-border
							skirtInstances.Grow(1);
							SSkirtPatchInstanceData& instance = skirtInstances.Back();
							instance.m_patchBias = Vector(bias.X, bias.Y, (Float)correspondingClipmapLevel, 2.0f);

						}
						else if( bias.Y <= w.Min.Y && yPlusScale > w.Min.Y )
						{
							// bottom-border
							skirtInstances.Grow(1);
							SSkirtPatchInstanceData& instance = skirtInstances.Back();
							instance.m_patchBias = Vector(xPlusScale, yPlusScale, (Float)correspondingClipmapLevel, 3.0f);
						}
					}
				}

				// This is the detail level we need. Add instance for it.
				instances.Grow(1);
				STerrainPatchInstanceData& instance = instances.Back();
				instance.m_patchBias.AsVector2() = bias;
				instance.m_patchBias.Z = (Float)correspondingClipmapLevel;
				instance.m_patchSize = scale;

				if ( correspondingClipmapLevel == 0 )
				{
					++GNum0LevelInstances;
				}
			}
		}
		else
		{	
			Float halfScale = 0.5f * scale;
			Int32 newLevel = level + 1;

			SQuadTreeNode* nodePtr = &quadTreeStack[nodeIdx+1];
			nodePtr->bias = bias; 
			nodePtr->level = newLevel;
			nodePtr++;

			nodePtr->bias = bias + Vector2( halfScale, 0.0f ); 
			nodePtr->level = newLevel;
			nodePtr++;

			nodePtr->bias = bias + Vector2( 0.0f, halfScale ); 
			nodePtr->level = newLevel;
			nodePtr++;

			nodePtr->bias = bias + Vector2( halfScale, halfScale ); 
			nodePtr->level = newLevel;

			nodeIdx += 4; 
		}
	}

	Uint32 numInstances = instances.Size();
	Uint32 numSkirtInstances = skirtInstances.Size();
	if ( numInstances > 0 && numInstances + numSkirtInstances <= TERRAIN_PATCH_INSTANCES_BUFFER_SIZE )
	{
		// Sort instances front to back
		Sort( instances.Begin(), instances.End(), CompareTerrainPatchDistance( Vector2( m_cameraPosition.X, m_cameraPosition.Y ) ) );

		// Emit to instance buffer
		void* instancedPtr = m_renderProxy->m_instancedPtr;

		const Uint32 streamStride = sizeof( STerrainPatchInstanceData );
		Uint32 terrainInstanceBufferSize = numInstances * streamStride;
		Red::System::MemoryCopy( instancedPtr, instances.Data(), terrainInstanceBufferSize );
		instancedPtr = static_cast<Uint8*>(instancedPtr) + terrainInstanceBufferSize;
		Red::System::MemoryCopy( instancedPtr, skirtInstances.Data(), skirtInstances.Size() * streamStride );
	}
}

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderSpeedTreeResource.h"
#include "speedTreeRenderInterface.h"

#include "../../common/core/directory.h"
#include "../engine/baseTree.h"

#ifdef USE_SPEED_TREE

using namespace SpeedTree;

CRenderSpeedTreeResource::CRenderSpeedTreeResource()
	: m_tree( NULL ),
	m_initialized(false)
{
}

CRenderSpeedTreeResource::~CRenderSpeedTreeResource()
{
	if (m_tree)
	{
		if ( m_tree->GraphicsAreInitialized() )
		{
			m_tree->ReleaseGfxResources();
		}
		delete m_tree;
	}
}

IRenderObject* CRenderSpeedTreeResource::Create( const CSRTBaseTree* baseTreeRes )
{
	ASSERT( baseTreeRes );
	ASSERT( baseTreeRes->GetSRTData() );
	ASSERT( baseTreeRes->GetSRTDataSize() > 0 );

	CTreeRender* sptTree = new CTreeRender();

	if ( sptTree->LoadTree( (st_byte*)baseTreeRes->GetSRTData(), baseTreeRes->GetSRTDataSize(), true ) )
	{
		// Create render resource
		CRenderSpeedTreeResource* renderSpeedTreeResource = new CRenderSpeedTreeResource();
		const String path = baseTreeRes->GetFile()->GetDirectory()->GetDepotPath();
		renderSpeedTreeResource->SetSRTPath( path );
		renderSpeedTreeResource->SetSRTFileName( baseTreeRes->GetFile()->GetFileName() );

		// Bind speed tree object, to the render resource
		renderSpeedTreeResource->SetRenderBaseTree( sptTree );

		// Store extents for engine side checks
		CExtents ext = sptTree->GetExtents();
		baseTreeRes->SetBBox( Box( Vector( ext.Min().x, ext.Min().y, ext.Min().z ), Vector( ext.Max().x, ext.Max().y, ext.Max().z ) ) );
		baseTreeRes->SetTreeType( sptTree->IsCompiledAsGrass() ? BaseTreeTypeGrass : BaseTreeTypeTree );

		// setup search paths for textures & shaders
		const Int32 c_nMaxExpectedPaths = 4;
		CStaticArray<CFixedString> searchPaths( c_nMaxExpectedPaths, "UploadTreeTempshit", false );
		{
			// This one will be used for textures...
			searchPaths.push_back( CFixedString( UNICODE_TO_ANSI( path.AsChar() ) ) );

			// ... and this one for shaders
#ifdef RED_PLATFORM_ORBIS
			searchPaths.push_back( CFixedString( UNICODE_TO_ANSI( GFileManager->GetBaseDirectory().AsChar() ) ) + CFixedString( GpuApi::GetShaderRootPathAnsi() ) + CFixedString( "speedtree/" ) + CShaderTechnique::GetCompiledShaderFolder( ) );
#else
			searchPaths.push_back( CFixedString( GpuApi::GetShaderRootPathAnsi() ) + CFixedString( "speedtree/" ) + CShaderTechnique::GetCompiledShaderFolder( ) );
#endif
		}

		SAppState appState;
		appState.m_bMultisampling = false;
		appState.m_bAlphaToCoverage = false;
		appState.m_bDepthPrepass = false;
		appState.m_bDeferred = true;
		appState.m_eShadowConfig = SRenderState::SHADOW_CONFIG_2_MAPS;

		if ( !sptTree->InitGfx( appState, searchPaths, 8 ) )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( Foliage ), TXT("Failed to initialize tree %s gfx"), path.AsChar() );
		}

		// Return render resource created
		return renderSpeedTreeResource;
	}
	else
	{
		// failed to load
		delete sptTree;
		sptTree = NULL;

		return NULL;
	}
}

void CRenderSpeedTreeResource::GetCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision )
{
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( renderSpeedTreeResource );
	ASSERT( baseTreeRenderResource );
	CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
	ASSERT( treeRender );

	if ( treeRender->IsCompiledAsGrass() ) return;
	st_int32 nNumObjects = 0;
	const SCollisionObject* pObjects = treeRender->GetCollisionObjects( nNumObjects );
	collision.Reserve( nNumObjects * 2 );
	for( Int32 i = 0; i != nNumObjects; ++i )
	{
		collision.PushBack( Sphere( Vector( pObjects[ i ].m_vCenter1.x, pObjects[ i ].m_vCenter1.y, pObjects[ i ].m_vCenter1.z ), pObjects[ i ].m_fRadius ) );
		collision.PushBack( Sphere( Vector( pObjects[ i ].m_vCenter2.x, pObjects[ i ].m_vCenter2.y, pObjects[ i ].m_vCenter2.z ), pObjects[ i ].m_fRadius ) );
	}
}

void CRenderSpeedTreeResource::GetSpeedTreeStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& stats )
{
	// extract render speed tree object 
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( renderSpeedTreeResource );
	if( baseTreeRenderResource != nullptr )
	{
		CTreeRender* treeRender = baseTreeRenderResource->GetRenderBaseTree();
		if( treeRender != nullptr )
		{
			// gather information about textures
			GetSpeedTreeGeneralStats( treeRender, stats );
		}
	}
}

void CRenderSpeedTreeResource::ReleaseTextures()
{
	if ( m_tree )
	{
		m_tree->ReleaseTextures();
	}
}

void CRenderInterface::PopulateSpeedTreeMetrics( SSpeedTreeResourceMetrics& metrics )
{
#define POPULATE_STATS( source, target )						\
	target.m_currentBytes =		source.m_siCurrentUsage;		\
	target.m_peakBytes =		source.m_siPeakUsage;			\
	target.m_currentAllocs =	source.m_siCurrentQuantity;		\
	target.m_peakAllocs =		source.m_siPeakQuantity;	

	SpeedTree::CCore::SResourceSummary speedTreeResources = SpeedTree::CCore::GetSdkResourceUsage();
	POPULATE_STATS( speedTreeResources.m_sHeap, metrics.m_heapStats );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_VERTEX_BUFFER ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VB ] );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_INDEX_BUFFER ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_IB ] );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_VERTEX_SHADER ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_VERTEX_SHADER ] );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_PIXEL_SHADER ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_PIXEL_SHADER ] );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_TEXTURE ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_TEXTURE ] );
	POPULATE_STATS( speedTreeResources.m_asGfxResources[ SpeedTree::GFX_RESOURCE_OTHER ], metrics.m_resourceStats[ SSpeedTreeResourceMetrics::RESOURCE_OTHER ] );

	Int32 readIndex = SSpeedTreeRenderStats::ReadIndex();
	metrics.m_renderStats.m_grassLayerCount = SSpeedTreeRenderStats::s_grassLayerCount[ readIndex ];
	metrics.m_renderStats.m_visibleGrassCellCount = SSpeedTreeRenderStats::s_visibleGrassCellCount[ readIndex ];
	metrics.m_renderStats.m_visibleGrassCellArrayCapacity = SSpeedTreeRenderStats::s_visibleGrassCellArrayCapacity[ readIndex ];
	metrics.m_renderStats.m_visibleGrassCellArraySize = SSpeedTreeRenderStats::s_visibleGrassCellArraySize[ readIndex ];
	metrics.m_renderStats.m_visibleGrassInstanceCount = SSpeedTreeRenderStats::s_visibleGrassInstanceCount[ readIndex ];
	metrics.m_renderStats.m_visibleGrassInstanceArrayCapacity = SSpeedTreeRenderStats::s_visibleGrassInstanceArrayCapacity[ readIndex ];
	metrics.m_renderStats.m_visibleGrassInstanceArraySize = SSpeedTreeRenderStats::s_visibleGrassInstanceArraySize[ readIndex ];
	metrics.m_renderStats.m_visibleTreeCellCount = SSpeedTreeRenderStats::s_visibleTreeCellCount[ readIndex ];
	metrics.m_renderStats.m_visibleTreeCellArrayCapacity = SSpeedTreeRenderStats::s_visibleTreeCellArrayCapacity[ readIndex ];
	metrics.m_renderStats.m_visibleTreeCellArraySize = SSpeedTreeRenderStats::s_visibleTreeCellArraySize[ readIndex ];
	metrics.m_renderStats.m_visibleTreeInstanceCount = SSpeedTreeRenderStats::s_visibleTreeInstanceCount[ readIndex ];
	metrics.m_renderStats.m_visibleTreeInstanceArrayCapacity = SSpeedTreeRenderStats::s_visibleTreeInstanceArrayCapacity[ readIndex ];
	metrics.m_renderStats.m_visibleTreeInstanceArraySize = SSpeedTreeRenderStats::s_visibleTreeInstanceArraySize[ readIndex ];
	metrics.m_renderStats.m_maximumGrassLayerCullDistance = SSpeedTreeRenderStats::s_maxGrassLayerCullDistance[ readIndex ];
	metrics.m_renderStats.m_minGrassCellSize = SSpeedTreeRenderStats::s_minGrassCellSize[ readIndex ];
	metrics.m_renderStats.m_maxGrassCellSize = SSpeedTreeRenderStats::s_maxGrassCellSize[ readIndex ];
	metrics.m_renderStats.m_treeDrawcalls = SSpeedTreeRenderStats::s_treeDrawcalls[ readIndex ];
	metrics.m_renderStats.m_billboardDrawcalls = SSpeedTreeRenderStats::s_billboardDrawcalls[ readIndex ];
	metrics.m_renderStats.m_grassDrawcalls = SSpeedTreeRenderStats::s_grassDrawcalls[ readIndex ];
	metrics.m_renderStats.m_treesRendered = SSpeedTreeRenderStats::s_treesRendered[ readIndex ];
	metrics.m_renderStats.m_billboardsRendered = SSpeedTreeRenderStats::s_billboardsRendered[ readIndex ];
	metrics.m_renderStats.m_grassRendered = SSpeedTreeRenderStats::s_grassRendered[ readIndex ];
}

IRenderObject* CRenderInterface::CreateSpeedTreeResource( const CSRTBaseTree* baseTree )
{
	return CRenderSpeedTreeResource::Create( baseTree );
}

void CRenderInterface::GetSpeedTreeResourceCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision )
{
	CRenderSpeedTreeResource::GetCollision( renderSpeedTreeResource, collision );
}

void CRenderInterface::GetSpeedTreeTextureStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& statsArray )
{
	CRenderSpeedTreeResource::GetSpeedTreeStatistic( renderSpeedTreeResource, statsArray );
}

void CRenderInterface::ReleaseSpeedTreeTextures( IRenderObject* renderSpeedTreeResource )
{
	CRenderSpeedTreeResource* baseTreeRenderResource = static_cast< CRenderSpeedTreeResource* >( renderSpeedTreeResource );
	if( baseTreeRenderResource != nullptr )
	{
		baseTreeRenderResource->ReleaseTextures();
	}
}

#else
IRenderObject* CRenderInterface::CreateSpeedTreeResource( const CSRTBaseTree* baseTree )
{
	return nullptr;
}

void CRenderInterface::GetSpeedTreeResourceCollision( IRenderObject* renderSpeedTreeResource, TDynArray< Sphere >& collision )
{
}

void CRenderInterface::PopulateSpeedTreeMetrics( SSpeedTreeResourceMetrics& /*metrics*/ )
{
}


void CRenderInterface::GetSpeedTreeTextureStatistic( IRenderObject* renderSpeedTreeResource, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& /*statsArray*/ )
{
}

void CRenderInterface::ReleaseSpeedTreeTextures( IRenderObject* renderSpeedTreeResource )
{
}

#endif

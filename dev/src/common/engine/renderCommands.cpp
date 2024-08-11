/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCommands.h"
#include "umbraIncludes.h"
#include "umbraScene.h"
#include "dynamicCollisionCollector.h"
#include "mesh.h"
#include "materialDefinition.h"
#include "renderFence.h"
#include "renderMovie.h"
#include "umbraTile.h"
#include "renderVisibilityQuery.h"
#include "renderObject.h"
#include "renderProxy.h"
#include "..\physics\physicsParticleWrapper.h"
#include "texture.h"
#include "globalWaterUpdateParams.h"
#include "grassCellMask.h"
#include "renderSwarmData.h"
#include "loadingScreen.h"
#include "videoPlayer.h"
#include "renderTextureStreamRequest.h"
#include "renderFramePrefetch.h"
#include "renderVisibilityExclusion.h"

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ReleaseRenderObjectOnRenderThread::CRenderCommand_ReleaseRenderObjectOnRenderThread( IRenderObject* renderObject )
{
	m_renderObject = renderObject;
	if ( m_renderObject != nullptr )
	{
		m_renderObject->AddRef();
	}
}

CRenderCommand_ReleaseRenderObjectOnRenderThread::~CRenderCommand_ReleaseRenderObjectOnRenderThread()
{
	SAFE_RELEASE(m_renderObject);
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RenderScene::CRenderCommand_RenderScene( CRenderFrame* frame, IRenderScene* scene )
	: IRenderCommand()
	, m_frame( frame )
	, m_scene( scene )
#ifndef NO_EDITOR
	, m_forcePrefetch( false )
#endif
{
	ASSERT( m_frame );
	m_frame->AddRef();

	// Scene pointer is optional
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

#ifndef NO_EDITOR
CRenderCommand_RenderScene::CRenderCommand_RenderScene( CRenderFrame* frame, IRenderScene* scene, Bool forcePrefetch )
	: IRenderCommand()
	, m_frame( frame )
	, m_scene( scene )
	, m_forcePrefetch( forcePrefetch )
{
	ASSERT( m_frame );
	m_frame->AddRef();

	// Scene pointer is optional
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}
#endif

CRenderCommand_RenderScene::~CRenderCommand_RenderScene()
{
	// Scene pointer is optional
	if ( m_scene )
	{
		m_scene->Release();
		m_scene = NULL;
	}

	SAFE_RELEASE( m_frame );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_NewFrame::CRenderCommand_NewFrame()
{
}

CRenderCommand_NewFrame::~CRenderCommand_NewFrame()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_EndFrame::CRenderCommand_EndFrame()
{
}

CRenderCommand_EndFrame::~CRenderCommand_EndFrame()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_CancelTextureStreaming::CRenderCommand_CancelTextureStreaming()
	: m_flushOnlyUnused(false)
{
}

CRenderCommand_CancelTextureStreaming::CRenderCommand_CancelTextureStreaming(Bool flushOnlyUnused)
	: m_flushOnlyUnused(flushOnlyUnused)
{
}

CRenderCommand_CancelTextureStreaming::~CRenderCommand_CancelTextureStreaming()
{
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ShutdownTextureStreaming::CRenderCommand_ShutdownTextureStreaming()
{
}

CRenderCommand_ShutdownTextureStreaming::~CRenderCommand_ShutdownTextureStreaming()
{
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_StartTextureStreamRequest::CRenderCommand_StartTextureStreamRequest( IRenderTextureStreamRequest* request )
	: m_request( request )
{
	if ( m_request )
	{
		m_request->AddRef();
	}
}

CRenderCommand_StartTextureStreamRequest::~CRenderCommand_StartTextureStreamRequest()
{
	SAFE_RELEASE( m_request );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_CancelTextureStreamRequest::CRenderCommand_CancelTextureStreamRequest( IRenderTextureStreamRequest* request )
	: m_request( request )
{
	if ( m_request )
	{
		m_request->AddRef();
	}
}

CRenderCommand_CancelTextureStreamRequest::~CRenderCommand_CancelTextureStreamRequest()
{
	SAFE_RELEASE( m_request );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_StartFramePrefetch::CRenderCommand_StartFramePrefetch( IRenderFramePrefetch* prefetch )
	: m_prefetch( prefetch )
{
	RED_ASSERT( m_prefetch != nullptr );
	if ( m_prefetch != nullptr )
	{
		m_prefetch->AddRef();
	}
}

CRenderCommand_StartFramePrefetch::~CRenderCommand_StartFramePrefetch()
{
	SAFE_RELEASE( m_prefetch );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_PrepareInitialTextureStreaming::CRenderCommand_PrepareInitialTextureStreaming( IRenderScene* scene, Bool resetDistances )
	: m_scene( scene )
	, m_resetDistances( resetDistances )
{
	RED_ASSERT( m_scene != nullptr );
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

CRenderCommand_PrepareInitialTextureStreaming::~CRenderCommand_PrepareInitialTextureStreaming()
{
	SAFE_RELEASE( m_scene );
}


CRenderCommand_FinishInitialTextureStreaming::CRenderCommand_FinishInitialTextureStreaming()
{
}

CRenderCommand_FinishInitialTextureStreaming::~CRenderCommand_FinishInitialTextureStreaming()
{
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetupEnvProbesPrefetch::CRenderCommand_SetupEnvProbesPrefetch( IRenderScene* scene, const Vector &position )
	: m_scene( scene )
	, m_position( position )
{
	RED_ASSERT( m_scene != nullptr );
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

CRenderCommand_SetupEnvProbesPrefetch::~CRenderCommand_SetupEnvProbesPrefetch()
{
	SAFE_RELEASE( m_scene );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_TickTextureStreaming::CRenderCommand_TickTextureStreaming( Bool flushOnePrefetch /*= false*/ )
	: m_flushOnePrefetch( flushOnePrefetch )
{
}

CRenderCommand_TickTextureStreaming::~CRenderCommand_TickTextureStreaming()
{
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleCinematicMode::CRenderCommand_ToggleCinematicMode( const Bool cinematicMode )
{
	m_cinematicMode = cinematicMode;
}

CRenderCommand_ToggleCinematicMode::~CRenderCommand_ToggleCinematicMode()
{
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateProgressStatus::CRenderCommand_UpdateProgressStatus( const String& status )
{
	Red::System::StringCopy( m_status, status.AsChar(), ARRAY_COUNT( m_status ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateProgress::CRenderCommand_UpdateProgress( Float progress )
	: m_progress( progress )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_AddRenderingExclusionToScene::CRenderCommand_AddRenderingExclusionToScene( IRenderScene* scene, IRenderVisibilityExclusion* object )
	: m_scene( scene )
	, m_object( object )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "NULL scene pointer" );
	m_scene->AddRef();
	RED_FATAL_ASSERT( m_object != nullptr, "NULL object pointer" );
	m_object->AddRef();
}

CRenderCommand_AddRenderingExclusionToScene::~CRenderCommand_AddRenderingExclusionToScene()
{
	m_scene->Release();
	m_object->Release();
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RemoveRenderingExclusionToScene::CRenderCommand_RemoveRenderingExclusionToScene( IRenderScene* scene, IRenderVisibilityExclusion* object )
	: m_scene( scene )
	, m_object( object )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "NULL scene pointer" );
	m_scene->AddRef();
	RED_FATAL_ASSERT( m_object != nullptr, "NULL object pointer" );
	m_object->AddRef();
}

CRenderCommand_RemoveRenderingExclusionToScene::~CRenderCommand_RemoveRenderingExclusionToScene()
{
	m_scene->Release();
	m_object->Release();
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleRenderingExclusion::CRenderCommand_ToggleRenderingExclusion( IRenderScene* scene, IRenderVisibilityExclusion* object, const Bool state )
	: m_scene( scene )
	, m_object( object )
	, m_state( state )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "NULL scene pointer" );
	m_scene->AddRef();
	RED_FATAL_ASSERT( m_object != nullptr, "NULL object pointer" );
	m_object->AddRef();
}

CRenderCommand_ToggleRenderingExclusion::~CRenderCommand_ToggleRenderingExclusion()
{
	m_scene->Release();
	m_object->Release();
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_AddProxyToScene::CRenderCommand_AddProxyToScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene ) ;
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_AddProxyToScene::~CRenderCommand_AddProxyToScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_AddStripeToScene::CRenderCommand_AddStripeToScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene ) ;
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_AddStripeToScene::~CRenderCommand_AddStripeToScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_AddFurToScene::CRenderCommand_AddFurToScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene ) ;
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_AddFurToScene::~CRenderCommand_AddFurToScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateFurParams::CRenderCommand_UpdateFurParams( IRenderProxy* proxy, const Vector& wind, Float wet )
	: m_proxy( proxy )
	, m_wind( wind )
	, m_wetness( wet )
{
}

CRenderCommand_UpdateFurParams::~CRenderCommand_UpdateFurParams()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
CRenderCommand_EditorSetFurParams::CRenderCommand_EditorSetFurParams( IRenderProxy* proxy, GFSDK_HairInstanceDescriptor* newParams, Uint32 index )
	: m_proxy( proxy )
	, m_params( newParams )
	, m_materialIndex( index )
{
}

CRenderCommand_EditorSetFurParams::~CRenderCommand_EditorSetFurParams()
{
}
#endif //USE_NVIDIA_FUR
#endif //NO_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RemoveProxyFromScene::CRenderCommand_RemoveProxyFromScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();
	
	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveProxyFromScene::~CRenderCommand_RemoveProxyFromScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RemoveStripeFromScene::CRenderCommand_RemoveStripeFromScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveStripeFromScene::~CRenderCommand_RemoveStripeFromScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RemoveFurFromScene::CRenderCommand_RemoveFurFromScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveFurFromScene::~CRenderCommand_RemoveFurFromScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_UploadOcclusionDataToScene::CRenderCommand_UploadOcclusionDataToScene( CUmbraScene* umbraScene, IRenderScene* renderScene, IRenderObject* occlusionData, TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIDToIndexMap )
	: m_umbraScene( umbraScene )
	, m_renderScene( renderScene )
	, m_occlusionData( occlusionData )
	, m_remapTable( Move( remapTable ) )
	, m_objectIDToIndexMap( Move( objectIDToIndexMap ) )
{
	ASSERT( m_renderScene );
	m_renderScene->AddRef();

	ASSERT( m_occlusionData );
	m_occlusionData->AddRef();
}

CRenderCommand_UploadOcclusionDataToScene::~CRenderCommand_UploadOcclusionDataToScene()
{
	m_umbraScene->RemoveUnusedTilesAndTomeCollection();

	SAFE_RELEASE( m_renderScene );
	SAFE_RELEASE( m_occlusionData );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_SetDoorState::CRenderCommand_SetDoorState( IRenderScene* renderScene, TObjectIdType objectId, Bool opened )
	: m_scene( renderScene )
	, m_objectId( objectId )
	, m_opened( opened )
{
	RED_ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_SetDoorState::~CRenderCommand_SetDoorState()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_SetCutsceneModeForGates::CRenderCommand_SetCutsceneModeForGates( IRenderScene* renderScene, Bool isCutscene )
	: m_scene( renderScene )
	, m_isCutscene( isCutscene )
{
	RED_ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_SetCutsceneModeForGates::~CRenderCommand_SetCutsceneModeForGates()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_ClearOcclusionData::CRenderCommand_ClearOcclusionData( CUmbraScene* umbraScene, IRenderScene* renderScene )
	: m_umbraScene( umbraScene )
	, m_scene( renderScene )
{
	RED_ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_ClearOcclusionData::~CRenderCommand_ClearOcclusionData()
{
	m_umbraScene->RemoveUnusedTilesAndTomeCollection( true );
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_UploadObjectCache::CRenderCommand_UploadObjectCache( IRenderScene* scene, TObjectCache& objectCache )
	: m_scene( scene )
	, m_objectCache( Move( objectCache ) )
{
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

CRenderCommand_UploadObjectCache::~CRenderCommand_UploadObjectCache()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_SetValidityOfOcclusionData::CRenderCommand_SetValidityOfOcclusionData( IRenderScene* scene, Bool isDataValid )
	: m_scene( scene )
	, m_isDataValid( isDataValid )
{
	ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_SetValidityOfOcclusionData::~CRenderCommand_SetValidityOfOcclusionData()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA
CRenderCommand_UpdateQueryThreshold::CRenderCommand_UpdateQueryThreshold( IRenderScene* scene, Float value )
	: m_scene( scene )
	, m_value( value )
{
	ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_UpdateQueryThreshold::~CRenderCommand_UpdateQueryThreshold()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA

////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
#ifdef USE_UMBRA
CRenderCommand_DumpVisibleMeshes::CRenderCommand_DumpVisibleMeshes( IRenderScene* scene, const TLoadedComponentsMap& componentsMap, const String& path )
	: m_scene( scene )
	, m_map( componentsMap )
	, m_path( path )
{
	ASSERT( m_scene );
	m_scene->AddRef();
}

CRenderCommand_DumpVisibleMeshes::~CRenderCommand_DumpVisibleMeshes()
{
	SAFE_RELEASE( m_scene );
}
#endif // USE_UMBRA
#endif // NO_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_UMBRA
CRenderCommand_PerformVisibilityQueries::CRenderCommand_PerformVisibilityQueries( IRenderScene* scene, UmbraQueryBatch&& queryList )
	: m_scene( scene )
	, m_queryList( Move( queryList ) )
{
	
}
CRenderCommand_PerformVisibilityQueries::~CRenderCommand_PerformVisibilityQueries()
{

}

#endif // USE_UMBRA
////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_RelinkProxy::CRenderCommand_RelinkProxy( IRenderProxy* proxy, const RenderProxyUpdateInfo& data )
	: m_proxy( proxy )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}

	m_localToWorld = *data.m_localToWorld;

	m_boundingBox = *data.m_boundingBox;
}

CRenderCommand_RelinkProxy::~CRenderCommand_RelinkProxy()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_BatchSkinAndRelinkProxies::CRenderCommand_BatchSkinAndRelinkProxies( Uint8 proxyCount, IRenderProxy** proxies, IRenderObject** skinningData, const RenderProxyRelinkInfo* relinkInfos )
	: m_proxyCount( proxyCount )
{
	RED_FATAL_ASSERT( m_proxyCount > 0, "No proxies to update in the batch" );
	RED_FATAL_ASSERT( m_proxyCount <= 64, "Too many proxies to update in one batch" );

	Red::System::MemoryCopy( m_proxies,			proxies,			m_proxyCount * sizeof( IRenderProxy* ) );
	Red::System::MemoryCopy( m_skinningData,	skinningData,		m_proxyCount * sizeof( IRenderObject* ) );
	Red::System::MemoryCopy( m_relinkInfos,		relinkInfos,		m_proxyCount * sizeof( RenderProxyRelinkInfo ) );
}

CRenderCommand_BatchSkinAndRelinkProxies::~CRenderCommand_BatchSkinAndRelinkProxies()
{
	for ( Uint8 i = 0; i < m_proxyCount; ++i )
	{
		SAFE_RELEASE( m_proxies[i] );
		SAFE_RELEASE( m_skinningData[i] );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_EnvProbeParamsChanged::CRenderCommand_EnvProbeParamsChanged( IRenderResource* probe, const SEnvProbeParams& params )
	: m_probe ( probe )
	, m_params ( params )
{
	if ( m_probe )
	{
		m_probe->AddRef();
	}
}

CRenderCommand_EnvProbeParamsChanged::~CRenderCommand_EnvProbeParamsChanged()
{
	SAFE_RELEASE( m_probe );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateSkinningDataAndRelink::CRenderCommand_UpdateSkinningDataAndRelink( IRenderProxy* proxy, IRenderObject* data, const RenderProxyUpdateInfo& data2 )
	: m_proxy( proxy )
	, m_data( data )
	, m_transformOnly( true )
{
	if ( data2.m_localToWorld )
	{
		m_localToWorld = *data2.m_localToWorld;
	}

	if ( data2.m_boundingBox )
	{
		m_boundingBox = *data2.m_boundingBox;
		m_transformOnly = false;
	}

	if ( m_proxy )
	{
		m_proxy->AddRef();
	}

	if ( m_data )
	{
		m_data->AddRef();
	}
}


CRenderCommand_UpdateSkinningDataAndRelink::~CRenderCommand_UpdateSkinningDataAndRelink()
{
	SAFE_RELEASE( m_proxy );
	SAFE_RELEASE( m_data );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetDestructionMeshDissolving::CRenderCommand_SetDestructionMeshDissolving(IRenderProxy* proxy, Bool useDissolve )
	: m_proxy( proxy )
	, m_useDissolve( useDissolve )
{
	if ( m_proxy )
	{
		// Add ref
		m_proxy->AddRef();
	}
}

CRenderCommand_SetDestructionMeshDissolving::~CRenderCommand_SetDestructionMeshDissolving( )
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateDestructionMeshActiveIndices::CRenderCommand_UpdateDestructionMeshActiveIndices(IRenderProxy* proxy, TDynArray< Uint16 >&& activeIndices, TDynArray< Uint32 >&& newOffsets, TDynArray< Uint32 >&& chunkNumIndices )
	: m_proxy( proxy )
	, m_activeIndices( Move( activeIndices ) )
	, m_chunkOffsets( Move( newOffsets ) )
	, m_chunkNumIndices( Move( chunkNumIndices ) )
{
	if ( m_proxy )
	{
		// Add ref
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateDestructionMeshActiveIndices::~CRenderCommand_UpdateDestructionMeshActiveIndices( )
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateLightProxyParameter::CRenderCommand_UpdateLightProxyParameter( IRenderProxy* proxy, const Vector& data, ERenderLightParameter parameter )
	: m_proxy( proxy )
	, m_data( data )
	, m_parameter( parameter )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateLightProxyParameter::~CRenderCommand_UpdateLightProxyParameter()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_Fence::CRenderCommand_Fence( IRenderFence* fence )
	: m_fence( fence )
{
	if ( m_fence )
	{
		m_fence->AddRef();
	}
}

CRenderCommand_Fence::~CRenderCommand_Fence()
{
	SAFE_RELEASE( m_fence );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateHitProxyID::CRenderCommand_UpdateHitProxyID( IRenderProxy* proxy, const CHitProxyID& id )
	: m_proxy( proxy )
	, m_id( id )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateHitProxyID::~CRenderCommand_UpdateHitProxyID()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetAutoFade::CRenderCommand_SetAutoFade( IRenderScene* scene, IRenderProxy* proxy, EFadeType type )
	: m_proxy( proxy )
	, m_scene( scene )
	, m_type( type )
{
	if ( m_scene )
	{
		m_scene->AddRef();
	}

	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_SetAutoFade::~CRenderCommand_SetAutoFade()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_proxy );
}

//////////////////////////////////////////////////////////////////////////

CRenderCommand_SetTemporaryFade::CRenderCommand_SetTemporaryFade( IRenderProxy* proxy )
	: m_proxy( proxy )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
	else
	{
		RED_HALT("Trying to fade a null proxy, this will cause a crash!");
	}
}

CRenderCommand_SetTemporaryFade::~CRenderCommand_SetTemporaryFade()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetSelectionFlag::CRenderCommand_SetSelectionFlag( IRenderProxy* proxy, Bool selectionFlag )
	: m_proxy( proxy )
	, m_flag( selectionFlag )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_SetSelectionFlag::~CRenderCommand_SetSelectionFlag()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateEffectParameters::CRenderCommand_UpdateEffectParameters( IRenderProxy* proxy, const Vector& paramValue, Uint32 paramIndex )
	: m_proxy( proxy )
	, m_paramValue( paramValue )
	, m_paramIndex( paramIndex )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateEffectParameters::~CRenderCommand_UpdateEffectParameters()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateLightParameter::CRenderCommand_UpdateLightParameter( IRenderProxy* proxy, CName paramName, Float paramValue )
	: m_proxy( proxy )
	, m_paramValue( paramValue )
	, m_paramName( paramName )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateLightParameter::~CRenderCommand_UpdateLightParameter()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateLightColor::CRenderCommand_UpdateLightColor( IRenderProxy* proxy, Color color )
	: m_proxy( proxy )
	, m_color( color )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateLightColor::~CRenderCommand_UpdateLightColor()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////


CRenderCommand_UpdateColorShiftMatrices::CRenderCommand_UpdateColorShiftMatrices( IRenderProxy* proxy, const Matrix& region0, const Matrix& region1 )
	: m_proxy( proxy )
	, m_colorShift0( region0 )
	, m_colorShift1( region1 )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateColorShiftMatrices::~CRenderCommand_UpdateColorShiftMatrices()
{
	SAFE_RELEASE( m_proxy );
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateParticlesSimulatationContext::CRenderCommand_UpdateParticlesSimulatationContext( IRenderProxy* proxy, const SSimulationContextUpdate & update )
	:	m_proxy( proxy ),
		m_contextUpdate( update )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateParticlesSimulatationContext::~CRenderCommand_UpdateParticlesSimulatationContext()
{
	SAFE_RELEASE( m_proxy );

	if( m_contextUpdate.m_wrapper )
	{
		m_contextUpdate.m_wrapper->Release();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateParticlesSimulatationContextAndRelink::CRenderCommand_UpdateParticlesSimulatationContextAndRelink( IRenderProxy* proxy, const SSimulationContextUpdate & update, const RenderProxyUpdateInfo& data )
	:	m_proxy( proxy ),
		m_contextUpdate( update )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}

	m_localToWorld = *data.m_localToWorld;
}

CRenderCommand_UpdateParticlesSimulatationContextAndRelink::~CRenderCommand_UpdateParticlesSimulatationContextAndRelink()
{
	SAFE_RELEASE( m_proxy );

	// TOTHINK
	if ( m_contextUpdate.m_wrapper )
	{
		m_contextUpdate.m_wrapper->Release();
	}	
}

CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink::CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink( Uint8 proxyCount, SUpdateParticlesBatchedCommand* batchedCommands )
	: m_proxyCount( proxyCount )
{
	RED_FATAL_ASSERT( m_proxyCount > 0, "No proxies to update in the batch" );
	RED_FATAL_ASSERT( m_proxyCount <= 64, "Too many proxies to update in one batch" );

	Red::System::MemoryCopy( m_batchedCommands,			batchedCommands,			m_proxyCount * sizeof( SUpdateParticlesBatchedCommand ) );
}

CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink::~CRenderCommand_BatchUpdateParticlesSimulatationContextAndRelink()
{
	for ( Uint8 i = 0; i < m_proxyCount; ++i )
	{
		SUpdateParticlesBatchedCommand& batchedCmd = m_batchedCommands[i];
		SAFE_RELEASE( batchedCmd.m_renderProxy );
		if( batchedCmd.m_simulationContext.m_wrapper )
		{
			batchedCmd.m_simulationContext.m_wrapper->Release();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_TakeScreenshot::CRenderCommand_TakeScreenshot( const SScreenshotParameters& screenshotParameters )
	: IRenderCommand()
	, m_screenshotParameters( screenshotParameters )
{
}

CRenderCommand_TakeScreenshot::~CRenderCommand_TakeScreenshot()
{
}

CRenderCommand_TakeUberScreenshot::CRenderCommand_TakeUberScreenshot( CRenderFrame* frame, IRenderScene* scene, const SScreenshotParameters& screenshotParameters, Bool& status )
	: IRenderCommand()
	, m_frame( frame )
	, m_scene( scene )
	, m_screenshotParameters( screenshotParameters )
	, m_status( &status )
{
	m_frame->AddRef();

	// Scene pointer is optional
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

CRenderCommand_TakeUberScreenshot::~CRenderCommand_TakeUberScreenshot()
{
	// Scene pointer is optional
	if ( m_scene )
	{
		m_scene->Release();
		m_scene = NULL;
	}

	SAFE_RELEASE( m_frame );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_GrabMovieFrame::CRenderCommand_GrabMovieFrame( IRenderMovie* movie )
	: m_movie( movie )
{
	if ( m_movie )
	{
		m_movie->AddRef();
	}
}

CRenderCommand_GrabMovieFrame::~CRenderCommand_GrabMovieFrame()
{
	SAFE_RELEASE( m_movie );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleContinuousScreenshot::CRenderCommand_ToggleContinuousScreenshot( Bool isEnabled, EFrameCaptureSaveFormat saveFormat, Bool ubersample /*= false*/ )
	: m_isEnabled( isEnabled )
	, m_saveFormat( saveFormat )
	, m_useUbersampling( ubersample )
{
}

CRenderCommand_ToggleContinuousScreenshot::~CRenderCommand_ToggleContinuousScreenshot()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ShowLoadingScreenBlur::CRenderCommand_ShowLoadingScreenBlur( Float blurScale , Float timeScale, Bool useBlackFallback )
	: m_blurScale( blurScale )
	, m_timeScale( timeScale )
	, m_useFallback( useBlackFallback )
{
}

CRenderCommand_ShowLoadingScreenBlur::~CRenderCommand_ShowLoadingScreenBlur()
{
}


CRenderCommand_HideLoadingScreenBlur::CRenderCommand_HideLoadingScreenBlur( Float fadeTime )
	: m_fadeTime( fadeTime )
{
}

CRenderCommand_HideLoadingScreenBlur::~CRenderCommand_HideLoadingScreenBlur()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetLoadingScreenFence::CRenderCommand_SetLoadingScreenFence( ILoadingScreenFence* fence, Float fadeInTime, Bool hideAtStart /*=false*/ )
	: m_fence( fence )
	, m_fadeInTime( fadeInTime )
	, m_hideAtStart( hideAtStart )
{
	// Null is allowed
	if ( m_fence )
	{
		m_fence->AddRef();
	}
}

CRenderCommand_SetLoadingScreenFence::~CRenderCommand_SetLoadingScreenFence()
{
	SAFE_RELEASE( m_fence );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_FadeOutLoadingScreen::CRenderCommand_FadeOutLoadingScreen( ILoadingScreenFence* fence, Float fadeOutTime )
	: m_fence( fence )
	, m_fadeOutTime( fadeOutTime )
{
	RED_FATAL_ASSERT( m_fence, "No fence" );
	m_fence->AddRef();
}

CRenderCommand_FadeOutLoadingScreen::~CRenderCommand_FadeOutLoadingScreen()
{
	SAFE_RELEASE( m_fence );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetLoadingOverlayFlash::CRenderCommand_SetLoadingOverlayFlash( CFlashMovie* loadingOverlayFlash )
	: m_loadingOverlayFlash( loadingOverlayFlash )
{
}

CRenderCommand_SetLoadingOverlayFlash::~CRenderCommand_SetLoadingOverlayFlash()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleLoadingOverlay::CRenderCommand_ToggleLoadingOverlay( Bool visible, const String& caption )
	: m_caption( caption )
	, m_visible( visible )
{
}

CRenderCommand_ToggleLoadingOverlay::~CRenderCommand_ToggleLoadingOverlay()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetVideoFlash::CRenderCommand_SetVideoFlash( CFlashMovie* videoFlash )
	: m_videoFlash( videoFlash )
{
}

CRenderCommand_SetVideoFlash::~CRenderCommand_SetVideoFlash()
{
}

//////////////////////////////////////////////////////////////////////////

CRenderCommand_PlayVideo::CRenderCommand_PlayVideo( IRenderVideo* renderVideo, EVideoThreadIndex threadIndex )
	: m_renderVideo( renderVideo )
	, m_threadIndex( threadIndex )
{
	if ( m_renderVideo )
	{
		m_renderVideo->AddRef();
	}
}

CRenderCommand_PlayVideo::~CRenderCommand_PlayVideo()
{
	if ( m_renderVideo )
	{
		m_renderVideo->Release();
	}
}

CRenderCommand_W3HackSetVideoClearRGB::CRenderCommand_W3HackSetVideoClearRGB( const Color& rgb )
	: m_rgb( rgb )
{
}

CRenderCommand_W3HackSetVideoClearRGB::~CRenderCommand_W3HackSetVideoClearRGB()
{
}

CRenderCommand_W3HackShowVideoBackground::CRenderCommand_W3HackShowVideoBackground()
{
}

CRenderCommand_W3HackShowVideoBackground::~CRenderCommand_W3HackShowVideoBackground()
{
}

CRenderCommand_W3HackHideVideoBackground::CRenderCommand_W3HackHideVideoBackground()
{
}

CRenderCommand_W3HackHideVideoBackground::~CRenderCommand_W3HackHideVideoBackground()
{
}

CRenderCommand_PlayLoadingScreenVideo::CRenderCommand_PlayLoadingScreenVideo( IRenderVideo* renderVideo, ILoadingScreenFence* fence )
	: m_renderVideo( renderVideo )
	, m_fence( fence )
{
	RED_FATAL_ASSERT( m_renderVideo, "No video" );
	RED_FATAL_ASSERT( m_fence, "No fence" );
	m_renderVideo->AddRef();
	m_fence->AddRef();
}

CRenderCommand_PlayLoadingScreenVideo::~CRenderCommand_PlayLoadingScreenVideo()
{
	SAFE_RELEASE( m_renderVideo );
	SAFE_RELEASE( m_fence );
}

CRenderCommand_CancelVideo::CRenderCommand_CancelVideo( IRenderVideo* renderVideo )
	: m_renderVideo( renderVideo )
{
	if ( m_renderVideo )
	{
		m_renderVideo->AddRef();
	}
}

CRenderCommand_CancelVideo::~CRenderCommand_CancelVideo()
{
	if ( m_renderVideo )
	{
		m_renderVideo->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleLoadingVideoSkip::CRenderCommand_ToggleLoadingVideoSkip( ILoadingScreenFence* fence, Bool enabled )
	: m_fence( fence )
	, m_enabled( enabled )
{
	if ( m_fence )
	{
		m_fence->AddRef();
	}
}

CRenderCommand_ToggleLoadingVideoSkip::~CRenderCommand_ToggleLoadingVideoSkip()
{
	if ( m_fence )
	{
		m_fence->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetLoadingPCInput::CRenderCommand_SetLoadingPCInput( ILoadingScreenFence* fence, Bool enabled )
	: m_fence( fence )
	, m_enabled( enabled )
{
	if ( m_fence )
	{
		m_fence->AddRef();
	}
}

CRenderCommand_SetLoadingPCInput::~CRenderCommand_SetLoadingPCInput()
{
	if ( m_fence )
	{
		m_fence->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SetExpansionsAvailable::CRenderCommand_SetExpansionsAvailable(  ILoadingScreenFence* fence, Bool ep1, Bool ep2 )
	: m_fence( fence )
	, m_ep1( ep1 )
	, m_ep2( ep2 )
{
	if ( m_fence )
	{
		m_fence->AddRef();
	}
}

CRenderCommand_SetExpansionsAvailable::~CRenderCommand_SetExpansionsAvailable()
{
	if ( m_fence )
	{
		m_fence->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleMeshMaterialHighlight::CRenderCommand_ToggleMeshMaterialHighlight( CMesh* mesh, Uint32 materialIndex )
	: m_mesh( mesh ? mesh->GetRenderResource() : NULL )
{
	// Get chunks
	if ( m_mesh )
	{
		// Add ref
		m_mesh->AddRef();

		// Enumerate all mesh chunks with given material
		TDynArray< Uint32 > meshChunksWithSelectedMaterial;

		const auto& allChunks = mesh->GetChunks();
		for ( Uint32 i=0; i<allChunks.Size(); i++ )
		{
			if ( allChunks[i].m_materialID == (Uint32)materialIndex )
			{
				m_chunkIndices.PushBack( i );
			}
		}
	}
}

CRenderCommand_ToggleMeshMaterialHighlight::~CRenderCommand_ToggleMeshMaterialHighlight()
{
	SAFE_RELEASE( m_mesh );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleMeshChunkHighlight::CRenderCommand_ToggleMeshChunkHighlight( CMesh* mesh, Uint32 chunkIndex )
	: m_mesh( mesh ? mesh->GetRenderResource() : NULL )
{
	// Get chunks
	if ( m_mesh )
	{
		// Add ref
		m_mesh->AddRef();

		m_chunkIndices.PushBack(chunkIndex);
	}
}

CRenderCommand_ToggleMeshChunkHighlight::CRenderCommand_ToggleMeshChunkHighlight( CMesh* mesh, const TDynArray< Uint32 >& chunkIndices )
	: m_mesh( mesh ? mesh->GetRenderResource() : NULL )
{
	// Get chunks
	if ( m_mesh )
	{
		// Add ref
		m_mesh->AddRef();

		m_chunkIndices.PushBack( chunkIndices );
	}
}

CRenderCommand_ToggleMeshChunkHighlight::~CRenderCommand_ToggleMeshChunkHighlight()
{
	SAFE_RELEASE( m_mesh );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_UpdateMeshRenderParams::CRenderCommand_UpdateMeshRenderParams( IRenderProxy* proxy, const SMeshRenderParams& params )
	: m_meshProxy( proxy )
	, m_meshRenderParams( params )
{
	ASSERT( m_meshProxy );
	m_meshProxy->AddRef();
}

CRenderCommand_UpdateMeshRenderParams::~CRenderCommand_UpdateMeshRenderParams()
{
	m_meshProxy->Release();
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_SuppressSceneRendering::CRenderCommand_SuppressSceneRendering( IViewport* viewport, Bool state, IRenderScene* restoreScene /*= nullptr*/, CRenderFrame* restoreFrame /*= nullptr*/ )
	: m_restoreScene( restoreScene )
	, m_restoreFrame( restoreFrame )
	, m_viewport( viewport )
	, m_state( state )
{
	if ( m_restoreScene != nullptr )
	{
		m_restoreScene->AddRef();
	}
	if ( m_restoreFrame != nullptr )
	{
		m_restoreFrame->AddRef();
	}
}

CRenderCommand_SuppressSceneRendering::~CRenderCommand_SuppressSceneRendering()
{
	SAFE_RELEASE( m_restoreScene );
	SAFE_RELEASE( m_restoreFrame );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_FinishFades::CRenderCommand_FinishFades( IRenderScene* scene )
	: m_scene( scene )
{
	m_scene->AddRef();
}

CRenderCommand_FinishFades::~CRenderCommand_FinishFades()
{
	SAFE_RELEASE( m_scene );
}

/////////////////////////////////////////////////////////////////////////////////////////////

CRenderCommand_ToggleMeshSelectionOverride::CRenderCommand_ToggleMeshSelectionOverride( Bool state )
	: m_state( state ) 
{
}

CRenderCommand_ToggleMeshSelectionOverride::~CRenderCommand_ToggleMeshSelectionOverride()
{
}

CRenderCommand_OverrideProxyMaterial::CRenderCommand_OverrideProxyMaterial( IRenderProxy* proxy, IMaterial* material, Bool drawOriginal )
	: m_proxy( proxy )
	, m_drawOriginal( drawOriginal )
{
	if ( material )
	{
		m_parameters = material->GetRenderResource();
		if ( material->GetMaterialDefinition() )
		{
			m_material = material->GetMaterialDefinition()->GetRenderResource();
		}
	}
	
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
	if ( m_parameters )
	{
		m_parameters->AddRef();
	}
	if ( m_material )
	{
		m_material->AddRef();
	}
}

CRenderCommand_OverrideProxyMaterial::~CRenderCommand_OverrideProxyMaterial()
{
	SAFE_RELEASE( m_proxy );
	SAFE_RELEASE( m_parameters );
	SAFE_RELEASE( m_material );
}

CRenderCommand_DisableProxyMaterialOverride::CRenderCommand_DisableProxyMaterialOverride( IRenderProxy* proxy )
	: m_proxy( proxy )
{
	m_proxy->AddRef();
}

CRenderCommand_DisableProxyMaterialOverride::~CRenderCommand_DisableProxyMaterialOverride()
{
	m_proxy->Release();
}


CRenderCommand_SetNormalBlendMaterial::CRenderCommand_SetNormalBlendMaterial( IRenderProxy* proxy, IMaterial* normalBlendMaterial, IMaterial* sourceBaseMaterial, ITexture* sourceNormalTexture )
	: m_proxy( proxy )
	, m_material( NULL )	
	, m_parameters( NULL )
	, m_sourceBaseMaterial( NULL )
	, m_sourceNormalTexture( NULL )
{
	m_proxy->AddRef();
	if ( normalBlendMaterial && normalBlendMaterial->GetMaterialDefinition() )
	{
		m_material = normalBlendMaterial->GetMaterialDefinition()->GetRenderResource();
		m_parameters = normalBlendMaterial->GetRenderResource();

		m_material->AddRef();
		m_parameters->AddRef();
	}

	if ( sourceBaseMaterial && sourceBaseMaterial->GetMaterialDefinition() )
	{
		m_sourceBaseMaterial = sourceBaseMaterial->GetMaterialDefinition()->GetRenderResource();
		m_sourceBaseMaterial->AddRef();
	}

	if ( sourceNormalTexture )
	{
		m_sourceNormalTexture = sourceNormalTexture->GetRenderResource();
		m_sourceNormalTexture->AddRef();
	}
}
CRenderCommand_SetNormalBlendMaterial::~CRenderCommand_SetNormalBlendMaterial()
{
	m_proxy->Release();

	SAFE_RELEASE( m_material );
	SAFE_RELEASE( m_parameters );
	SAFE_RELEASE( m_sourceBaseMaterial );
	SAFE_RELEASE( m_sourceNormalTexture );
}


CRenderCommand_DefineNormalBlendAreas::CRenderCommand_DefineNormalBlendAreas( IRenderProxy* proxy,  Uint32 firstArea, Uint32 numAreas, const Vector* areas )
	: m_proxy( proxy )
	, m_firstArea( 0 )
	, m_numAreas( 0 )
{
	m_proxy->AddRef();

	if ( firstArea + numAreas <= NUM_NORMALBLEND_AREAS )
	{
		m_numAreas = numAreas;
		m_firstArea = firstArea;
		Red::System::MemoryCopy( m_areas, areas, sizeof( Vector ) * m_numAreas );
	}
}

CRenderCommand_DefineNormalBlendAreas::~CRenderCommand_DefineNormalBlendAreas()
{
	m_proxy->Release();
}

CRenderCommand_UpdateNormalBlendWeights::CRenderCommand_UpdateNormalBlendWeights( IRenderProxy* proxy, Uint32 firstWeight, Uint32 numWeights, const Float *weights )
	: m_proxy( proxy )
	, m_firstWeight( 0 )
	, m_numWeights( 0 )
{
	m_proxy->AddRef();

	if ( firstWeight + numWeights <= NUM_NORMALBLEND_AREAS )
	{
		m_numWeights = numWeights;
		m_firstWeight = firstWeight;
		Red::System::MemoryCopy( m_weights, weights, sizeof( Float ) * m_numWeights );
	}
}

CRenderCommand_UpdateNormalBlendWeights::~CRenderCommand_UpdateNormalBlendWeights()
{
	m_proxy->Release();
}

CRenderCommand_DisableAllGameplayEffects::CRenderCommand_DisableAllGameplayEffects()
{

}

CRenderCommand_DisableAllGameplayEffects::~CRenderCommand_DisableAllGameplayEffects()
{

}

CRenderCommand_AddFocusModePostFx::CRenderCommand_AddFocusModePostFx( Float desaturation, Float highlightBoost )
	: m_desaturation( desaturation )
	, m_highlightBoost( highlightBoost )
{

}

CRenderCommand_AddFocusModePostFx::~CRenderCommand_AddFocusModePostFx()
{
}

CRenderCommand_EnableExtendedFocusModePostFx::CRenderCommand_EnableExtendedFocusModePostFx( Float fadeInTime )
	: m_fadeInTime ( fadeInTime )
{
}

CRenderCommand_EnableExtendedFocusModePostFx::~CRenderCommand_EnableExtendedFocusModePostFx()
{
}

CRenderCommand_DisableExtendedFocusModePostFx::CRenderCommand_DisableExtendedFocusModePostFx( Float fadeOutTime )
	: m_fadeOutTime ( fadeOutTime )
{
}

CRenderCommand_DisableExtendedFocusModePostFx::~CRenderCommand_DisableExtendedFocusModePostFx()
{
}

CRenderCommand_RemoveFocusModePostFx::CRenderCommand_RemoveFocusModePostFx( Bool forceDisable )	
	: m_forceDisable ( forceDisable )
{

}

CRenderCommand_RemoveFocusModePostFx::~CRenderCommand_RemoveFocusModePostFx()
{

}

CRenderCommand_UpdateFocusHighlightFading::CRenderCommand_UpdateFocusHighlightFading( Float fadeNear, Float fadeFar, Float dimmingTime, Float dimmingSpeed )
	: m_fadeNear( fadeNear )
	, m_fadeFar( fadeFar )
	, m_dimmingTime( dimmingTime )
	, m_dimmingSpeed( dimmingSpeed )
{
}

CRenderCommand_UpdateFocusHighlightFading::~CRenderCommand_UpdateFocusHighlightFading()
{
}

CRenderCommand_SetDimmingFocusMode::CRenderCommand_SetDimmingFocusMode( Bool dimming )
	: m_dimming( dimming )
{
}

CRenderCommand_SetDimmingFocusMode::~CRenderCommand_SetDimmingFocusMode()
{
}


CRenderCommand_InitSurfacePostFx::CRenderCommand_InitSurfacePostFx( const Vector& fillColor )
	: m_fillColor( fillColor )
{	
}

CRenderCommand_InitSurfacePostFx::~CRenderCommand_InitSurfacePostFx()	
{	
}

CRenderCommand_AddSurfacePostFx::CRenderCommand_AddSurfacePostFx( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type )
	: m_position( position )
	, m_fadeInTime( fadeInTime )
	, m_fadeOutTime( fadeOutTime )
	, m_activeTime( activeTime)
	, m_range( range )
	, m_type( type )
{
}

CRenderCommand_AddSurfacePostFx::~CRenderCommand_AddSurfacePostFx()	
{
}

CRenderCommand_AddSepiaPostFx::CRenderCommand_AddSepiaPostFx( Float f )
: m_fadeInTime( f )
{
}

CRenderCommand_AddDrunkPostFx::CRenderCommand_AddDrunkPostFx( Float f )
	: m_fadeInTime( f )
{
}
CRenderCommand_AddDrunkPostFx::~CRenderCommand_AddDrunkPostFx()
{}

CRenderCommand_RemoveDrunkPostFx::CRenderCommand_RemoveDrunkPostFx( Float f )
	: m_fadeOutTime( f )
{
}
CRenderCommand_RemoveDrunkPostFx::~CRenderCommand_RemoveDrunkPostFx()
{}

CRenderCommand_ScaleDrunkPostFx::CRenderCommand_ScaleDrunkPostFx( Float s )
	: m_scale( s )
{
}
CRenderCommand_ScaleDrunkPostFx::~CRenderCommand_ScaleDrunkPostFx()
{}


CRenderCommand_FocusModeSetPlayerPosition::CRenderCommand_FocusModeSetPlayerPosition( const Vector& position )
	: m_position( position )
{
}

CRenderCommand_FocusModeSetPlayerPosition::~CRenderCommand_FocusModeSetPlayerPosition()
{
}

CRenderCommand_EnableCatViewPostFx::CRenderCommand_EnableCatViewPostFx( Float fadeInTime )
	: m_fadeInTime( fadeInTime )
{
}

CRenderCommand_EnableCatViewPostFx::~CRenderCommand_EnableCatViewPostFx( )
{
}

CRenderCommand_DisableCatViewPostFx::CRenderCommand_DisableCatViewPostFx( Float fadeOutTime )
	: m_fadeOutTime( fadeOutTime )
{
}

CRenderCommand_DisableCatViewPostFx::~CRenderCommand_DisableCatViewPostFx( )
{
}

CRenderCommand_CatViewSetPlayerPosition::CRenderCommand_CatViewSetPlayerPosition( const Vector& position, Bool autoPositioning )
	: m_position( position )
	, m_autoPositioning( autoPositioning )
{
}

CRenderCommand_CatViewSetPlayerPosition::~CRenderCommand_CatViewSetPlayerPosition( )
{
}

CRenderCommand_CatViewSetTintColors::CRenderCommand_CatViewSetTintColors( const Vector& tintNear , const Vector& tintFar , Float desaturation )
	: m_tintNear( tintNear )
	, m_tintFar( tintFar )
	, m_desaturation( desaturation )
{
}

CRenderCommand_CatViewSetTintColors::~CRenderCommand_CatViewSetTintColors( )
{
}

CRenderCommand_CatViewSetBrightness::CRenderCommand_CatViewSetBrightness( Float brightStrength )
	: m_brightStrength( brightStrength )
{
}

CRenderCommand_CatViewSetBrightness::~CRenderCommand_CatViewSetBrightness( )
{
}


CRenderCommand_CatViewSetViewRange::CRenderCommand_CatViewSetViewRange( Float viewRange )
	: m_viewRanger( viewRange )
{
}

CRenderCommand_CatViewSetViewRange::~CRenderCommand_CatViewSetViewRange( )
{
}

CRenderCommand_CatViewSetPulseParams::CRenderCommand_CatViewSetPulseParams( Float base, Float scale, Float speed )
	: m_base( base )
	, m_scale( scale )
	, m_speed( speed )
{
}

CRenderCommand_CatViewSetPulseParams::~CRenderCommand_CatViewSetPulseParams( )
{
}

CRenderCommand_CatViewSetHightlight::CRenderCommand_CatViewSetHightlight( Vector color , Float hightlightInterior , Float blurSize )
	: m_color( color )
	, m_hightlightInterior( hightlightInterior )
	, m_blurSize( blurSize )
{
}

CRenderCommand_CatViewSetHightlight::~CRenderCommand_CatViewSetHightlight()
{
}
CRenderCommand_CatViewSetFog::CRenderCommand_CatViewSetFog( Float fogDensity, Float fogStartOffset )
	: m_fogDensity( fogDensity )
	, m_fogStartOffset( fogStartOffset )
{
}

CRenderCommand_CatViewSetFog::~CRenderCommand_CatViewSetFog()
{
}


CRenderCommand_AddSepiaPostFx::~CRenderCommand_AddSepiaPostFx()
{

}

CRenderCommand_RemoveSepiaPostFx::CRenderCommand_RemoveSepiaPostFx( Float f )
: m_fadeOutTime( f )
{

}

CRenderCommand_RemoveSepiaPostFx::~CRenderCommand_RemoveSepiaPostFx()
{

}

CRenderCommand_ScreenFadeOut::CRenderCommand_ScreenFadeOut( const Color& color, Float time )
	: m_color( color )
	, m_time( time )
{
}

CRenderCommand_ScreenFadeOut::~CRenderCommand_ScreenFadeOut()
{
}

CRenderCommand_ScreenFadeIn::CRenderCommand_ScreenFadeIn( Float time )
	: m_time( time )
{
}

CRenderCommand_ScreenFadeIn::~CRenderCommand_ScreenFadeIn()
{
}

CRenderCommand_SetScreenFade::CRenderCommand_SetScreenFade( Bool isIn, Float progress, const Color& color )
	: m_isIn( isIn )
	, m_progress( progress )
	, m_color( color )
{}

CRenderCommand_UpdateFadeParameters::CRenderCommand_UpdateFadeParameters( Float deltaTime )
	: m_deltaTime( deltaTime )
{

}

CRenderCommand_UpdateFadeParameters::~CRenderCommand_UpdateFadeParameters()
{

}

CRenderCommand_UpdateOverrideParametersBatch::CRenderCommand_UpdateOverrideParametersBatch( IRenderProxy* proxy, const Vector& paramValue )
: m_proxy( proxy )
, m_paramValue( paramValue )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateOverrideParametersBatch::~CRenderCommand_UpdateOverrideParametersBatch()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_ChangeSceneForcedLOD::CRenderCommand_ChangeSceneForcedLOD( IRenderScene* scene, Int32 forceLOD )
	: m_scene( scene )
	, m_forceLOD( forceLOD )
{
	if ( m_scene )
	{
		m_scene->AddRef();
	}
}

CRenderCommand_ChangeSceneForcedLOD::~CRenderCommand_ChangeSceneForcedLOD()
{
	SAFE_RELEASE( m_scene );
}

//////////////////////////////////////////////////////////////////////////
// Editor-only commands
//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR

CRenderCommand_UpdateCreateParticleEmitter::CRenderCommand_UpdateCreateParticleEmitter( IRenderProxy* proxy, IRenderResource* emitter, IRenderResource* material, IRenderResource* materialParameters, EEnvColorGroup envColorGroup )
	: m_renderProxy( proxy )
	, m_emitter( emitter )
	, m_material( material )
	, m_materialParameters( materialParameters )
	, m_envColorGroup( envColorGroup )
{
	m_renderProxy->AddRef();
	m_emitter->AddRef();
	m_material->AddRef();
	m_materialParameters->AddRef();
}

CRenderCommand_UpdateCreateParticleEmitter::~CRenderCommand_UpdateCreateParticleEmitter()
{
	m_renderProxy->Release();
	m_emitter->Release();
	m_material->Release();
	m_materialParameters->Release();
}

//////////////////////////////////////////////////////////////////////////

CRenderCommand_RemoveParticleEmitter::CRenderCommand_RemoveParticleEmitter( IRenderProxy* proxy, Int32 uniqueId )
	: m_uniqueId( uniqueId )
	, m_renderProxy( proxy )
{
	m_renderProxy->AddRef();
}

CRenderCommand_RemoveParticleEmitter::~CRenderCommand_RemoveParticleEmitter()
{
	m_renderProxy->Release();
}

#endif


//////////////////////////////////////////////////////////////////////////
// Speed Tree render commands
CRenderCommand_UpdateFoliageRenderParams::CRenderCommand_UpdateFoliageRenderParams( IRenderObject* ro, SFoliageRenderParams params)
	: m_proxy( ro )
	, m_foliageSRenderParams( params )
{
	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_UpdateFoliageRenderParams::~CRenderCommand_UpdateFoliageRenderParams()
{
	m_proxy->Release();
}

CRenderCommand_AddSpeedTreeProxyToScene::CRenderCommand_AddSpeedTreeProxyToScene( IRenderScene* scene, IRenderObject* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_AddSpeedTreeProxyToScene::~CRenderCommand_AddSpeedTreeProxyToScene()
{
	m_scene->Release();
	m_proxy->Release();
}

CRenderCommand_RemoveSpeedTreeProxyFromScene::CRenderCommand_RemoveSpeedTreeProxyFromScene( IRenderScene* scene, IRenderObject* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveSpeedTreeProxyFromScene::~CRenderCommand_RemoveSpeedTreeProxyFromScene()
{
	m_scene->Release();
	m_proxy->Release();
}

CRenderCommand_UpdateSpeedTreeInstances::CRenderCommand_UpdateSpeedTreeInstances( RenderObjectHandle speedTreeProxy, SFoliageUpdateRequest updates )
	:	m_speedTreeProxy( speedTreeProxy ),
		m_updates( std::move( updates ) )
{}

CRenderCommand_UpdateSpeedTreeInstances::~CRenderCommand_UpdateSpeedTreeInstances()
{}

CRenderCommand_CreateSpeedTreeInstances::CRenderCommand_CreateSpeedTreeInstances( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const FoliageInstanceContainer & instancesData, const Box & box )
	:	m_speedTreeProxy( speedTreeProxy ),
		m_baseTree( baseTree ),
		m_instancesData( instancesData ),
		m_box( box )
{}

CRenderCommand_CreateSpeedTreeInstances::~CRenderCommand_CreateSpeedTreeInstances()
{}

CRenderCommand_CreateSpeedTreeDynamicInstances::CRenderCommand_CreateSpeedTreeDynamicInstances( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const FoliageInstanceContainer & instancesData, const Box & box )
	:	CRenderCommand_CreateSpeedTreeInstances( speedTreeProxy, baseTree, instancesData, box )
{}

CRenderCommand_CreateSpeedTreeDynamicInstances::~CRenderCommand_CreateSpeedTreeDynamicInstances()
{}

CRenderCommand_RemoveSpeedTreeInstancesRadius::CRenderCommand_RemoveSpeedTreeInstancesRadius( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Vector3& position, Float radius )
	:	m_speedTreeProxy( speedTreeProxy ),
		m_baseTree( baseTree ),
		m_position( position ),
		m_radius( radius )
{}

CRenderCommand_RemoveSpeedTreeInstancesRadius::~CRenderCommand_RemoveSpeedTreeInstancesRadius()
{}

CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius::CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Vector3& position, Float radius )
	:	CRenderCommand_RemoveSpeedTreeInstancesRadius( speedTreeProxy, baseTree, position, radius )
{}

CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius::~CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius()
{}

CRenderCommand_RemoveSpeedTreeInstancesRect::CRenderCommand_RemoveSpeedTreeInstancesRect( RenderObjectHandle speedTreeProxy, RenderObjectHandle baseTree, const Box& rect )
	:	m_speedTreeProxy( speedTreeProxy ),
		m_baseTree( baseTree ),
		m_rect(rect)
{}

CRenderCommand_RemoveSpeedTreeInstancesRect::~CRenderCommand_RemoveSpeedTreeInstancesRect()
{}

CRenderCommand_RefreshGenericGrass::CRenderCommand_RefreshGenericGrass( RenderObjectHandle speedTreeProxy )
	: m_speedTreeProxy( speedTreeProxy )
{}

CRenderCommand_RefreshGenericGrass::~CRenderCommand_RefreshGenericGrass()
{}

CRenderCommand_UpdateDynamicGrassColissions::CRenderCommand_UpdateDynamicGrassColissions( RenderObjectHandle speedTreeProxy, const TDynArray< SDynamicCollider >& collisionPos )
	:	m_speedTreeProxy( speedTreeProxy ),
		m_collisionsPos( collisionPos )
{}

CRenderCommand_UpdateDynamicGrassColissions::~CRenderCommand_UpdateDynamicGrassColissions()
{}

CRenderCommand_UpdateGenericGrassMask::CRenderCommand_UpdateGenericGrassMask( IRenderProxy* terrainProxy, Uint8* grassMaskUpdate, Uint32 grassMaskResUpdate )
{
	ASSERT( terrainProxy );

	m_terrainProxy = terrainProxy;
	m_terrainProxy->AddRef();

	if ( grassMaskUpdate )
	{
		m_grassMaskRes = grassMaskResUpdate;

		const Uint32 bitmapSize = ( m_grassMaskRes * m_grassMaskRes ) / 8;

		m_grassMask = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_FoliageData, MC_FoliageGrassMask, bitmapSize ) );
		Red::System::MemoryCopy( m_grassMask, grassMaskUpdate, bitmapSize );
	}
	else
	{
		m_grassMask = NULL;
		m_grassMaskRes = 0;
	}
}

CRenderCommand_UpdateGenericGrassMask::~CRenderCommand_UpdateGenericGrassMask()
{
	m_terrainProxy->Release();
	if ( m_grassMask )
	{
		RED_MEMORY_FREE( MemoryPool_FoliageData, MC_FoliageGrassMask, m_grassMask );
	}
}

CRenderCommand_UploadGenericGrassOccurrenceMap::CRenderCommand_UploadGenericGrassOccurrenceMap( RenderObjectHandle proxy, const TDynArray< CGrassCellMask >& cellMasks )
	: m_proxy( proxy )
	, m_cellMasks( cellMasks )
{
}

CRenderCommand_UploadGenericGrassOccurrenceMap::~CRenderCommand_UploadGenericGrassOccurrenceMap()
{
}

CRenderCommand_UpdateFoliageBudgets::CRenderCommand_UpdateFoliageBudgets( IRenderProxy* speedTreeProxy, Float grassInstancesPerSqM, Float treeInstancesPerSqM, Float grassLayersPerSqM )
	: m_speedTreeProxy( speedTreeProxy )
	, m_grassInstancesPerSqM( grassInstancesPerSqM )
	, m_treeInstancesPerSqM( treeInstancesPerSqM )
	, m_grassLayersPerSqM( grassLayersPerSqM )
{
	m_speedTreeProxy->AddRef();
}

CRenderCommand_UpdateFoliageBudgets::~CRenderCommand_UpdateFoliageBudgets()
{
	m_speedTreeProxy->Release();
}

CRenderCommand_SetFoliageVisualisation::CRenderCommand_SetFoliageVisualisation( IRenderProxy* speedTreeProxy, EFoliageVisualisationMode mode )
	: m_speedTreeProxy( speedTreeProxy )
	, m_mode( mode )
{
	m_speedTreeProxy->AddRef();
}

CRenderCommand_SetFoliageVisualisation::~CRenderCommand_SetFoliageVisualisation()
{
	m_speedTreeProxy->Release();
}


CRenderCommand_SetupTreeFading::CRenderCommand_SetupTreeFading( Bool enable )
	: m_enable( enable )
{
}

CRenderCommand_SetupTreeFading::~CRenderCommand_SetupTreeFading()
{
}

CRenderCommand_SetupCachets::CRenderCommand_SetupCachets( Bool enable )
	: m_enable( enable )
{
}

CRenderCommand_SetupCachets::~CRenderCommand_SetupCachets()
{
}

CRenderCommand_SetTreeFadingReferencePoints::CRenderCommand_SetTreeFadingReferencePoints( IRenderProxy* speedTreeProxy, const Vector& left, const Vector& right, const Vector& center )
	: m_speedTreeProxy( speedTreeProxy )
	, m_leftReference( left )
	, m_rightReference( right )
	, m_centerReference( center )
{
	m_speedTreeProxy->AddRef();
}

CRenderCommand_SetTreeFadingReferencePoints::~CRenderCommand_SetTreeFadingReferencePoints()
{
	m_speedTreeProxy->Release();
}


CRenderCommand_UpdateClipmap::CRenderCommand_UpdateClipmap( IRenderProxy* terrainProxy, IRenderObject* update )
	: m_terrainProxy( terrainProxy )
	, m_update( update )
{
	ASSERT( m_terrainProxy );
	m_terrainProxy->AddRef();

	ASSERT( m_update );
	m_update->AddRef();
}

CRenderCommand_UpdateClipmap::~CRenderCommand_UpdateClipmap()
{
	m_terrainProxy->Release();
	m_update->Release();
}

CRenderCommand_UpdateGrassSetup::CRenderCommand_UpdateGrassSetup( IRenderProxy* terrainProxy, IRenderObject* vegetationProxy, IRenderObject* update )
	: m_terrainProxy( terrainProxy )
	, m_vegetationProxy( vegetationProxy )
	, m_update( update )
{
	ASSERT( m_terrainProxy );
	m_terrainProxy->AddRef();

	ASSERT( m_vegetationProxy );
	m_vegetationProxy->AddRef();

	ASSERT( m_update );
	m_update->AddRef();
}

CRenderCommand_UpdateGrassSetup::~CRenderCommand_UpdateGrassSetup()
{
	m_terrainProxy->Release();
	m_vegetationProxy->Release();
	m_update->Release();
}


CRenderCommand_SetTerrainCustomOverlay::CRenderCommand_SetTerrainCustomOverlay( IRenderProxy* terrainProxy, const TDynArray< Uint32 >& data, Uint32 width, Uint32 height )
	: m_terrainProxy( terrainProxy )
	, m_data( data )
	, m_width( width )
	, m_height( height )
{
	RED_ASSERT( m_data.Size() == m_width * m_height );
	RED_ASSERT( m_terrainProxy != nullptr );
	if ( m_terrainProxy != nullptr )
	{
		m_terrainProxy->AddRef();
	}
}

CRenderCommand_SetTerrainCustomOverlay::~CRenderCommand_SetTerrainCustomOverlay()
{
	SAFE_RELEASE( m_terrainProxy );
}

CRenderCommand_ClearTerrainCustomOverlay::CRenderCommand_ClearTerrainCustomOverlay( IRenderProxy* terrainProxy )
	: m_terrainProxy( terrainProxy )
{
	RED_ASSERT( m_terrainProxy != nullptr );
	if ( m_terrainProxy != nullptr )
	{
		m_terrainProxy->AddRef();
	}
}

CRenderCommand_ClearTerrainCustomOverlay::~CRenderCommand_ClearTerrainCustomOverlay()
{
	SAFE_RELEASE( m_terrainProxy );
}

CRenderCommand_SetTerrainProxyToScene::CRenderCommand_SetTerrainProxyToScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_SetTerrainProxyToScene::~CRenderCommand_SetTerrainProxyToScene()
{
	m_scene->Release();
	m_proxy->Release();
}

CRenderCommand_RemoveTerrainProxyFromScene::CRenderCommand_RemoveTerrainProxyFromScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveTerrainProxyFromScene::~CRenderCommand_RemoveTerrainProxyFromScene()
{
	m_scene->Release();
	m_proxy->Release();
}


CRenderCommand_UpdateTerrainWaterLevels::CRenderCommand_UpdateTerrainWaterLevels( IRenderProxy* terrainProxy, const TDynArray< Float >& minWaterLevels )
	: m_terrainProxy( terrainProxy )
	, m_minWaterLevels( minWaterLevels )
{
	ASSERT( m_terrainProxy );
	m_terrainProxy->AddRef();
}

CRenderCommand_UpdateTerrainWaterLevels::~CRenderCommand_UpdateTerrainWaterLevels()
{
	SAFE_RELEASE( m_terrainProxy );
}


CRenderCommand_UpdateTerrainTileHeightRanges::CRenderCommand_UpdateTerrainTileHeightRanges( IRenderProxy* terrainProxy, const TDynArray< Vector2 >& tileHeightRanges )
	: m_terrainProxy( terrainProxy )
	, m_tileHeightRanges( tileHeightRanges )
{
	ASSERT( m_terrainProxy );
	m_terrainProxy->AddRef();
}

CRenderCommand_UpdateTerrainTileHeightRanges::~CRenderCommand_UpdateTerrainTileHeightRanges()
{
	SAFE_RELEASE( m_terrainProxy );
}

CRenderCommand_SetupEnvironmentElementsVisibility::CRenderCommand_SetupEnvironmentElementsVisibility( IRenderScene* scene, Bool terrainVisible, Bool foliageVisible, Bool waterVisible )
	: m_scene( scene )
	, m_terrainVisible( terrainVisible )
	, m_foliageVisible( foliageVisible )
	, m_waterVisible( waterVisible )
{
}

CRenderCommand_SetupEnvironmentElementsVisibility::~CRenderCommand_SetupEnvironmentElementsVisibility()
{
}

//
CRenderCommand_SetWaterProxyToScene::CRenderCommand_SetWaterProxyToScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_SetWaterProxyToScene::~CRenderCommand_SetWaterProxyToScene()
{
	m_scene->Release();
	m_proxy->Release();
}

CRenderCommand_RemoveWaterProxyFromScene::CRenderCommand_RemoveWaterProxyFromScene( IRenderScene* scene, IRenderProxy* proxy )
	: m_scene( scene )
	, m_proxy( proxy )
{
	ASSERT( m_scene );
	m_scene->AddRef();

	ASSERT( m_proxy );
	m_proxy->AddRef();
}

CRenderCommand_RemoveWaterProxyFromScene::~CRenderCommand_RemoveWaterProxyFromScene()
{
	m_scene->Release();
	m_proxy->Release();
}


CRenderCommand_UpdateWaterProxy::CRenderCommand_UpdateWaterProxy( IRenderProxy* waterProxy, IRenderObject* textureArray )
	: m_waterProxy( waterProxy )
	, m_textureArray( textureArray )		
{
	ASSERT( m_waterProxy );
	m_waterProxy->AddRef();

	if( m_textureArray )
	{
		m_textureArray->AddRef();	
	}
	ASSERT( m_textureArray );
}

CRenderCommand_UpdateWaterProxy::~CRenderCommand_UpdateWaterProxy()
{
	SAFE_RELEASE( m_waterProxy );
	SAFE_RELEASE( m_textureArray );
}

CRenderCommand_SimulateWaterProxy::CRenderCommand_SimulateWaterProxy( IRenderProxy* waterProxy, CGlobalWaterUpdateParams* waterParams )
	: m_waterProxy( waterProxy )
	, m_waterParams( waterParams )
{
	m_waterProxy->AddRef();
	m_waterParams->AddRef();
}

CRenderCommand_SimulateWaterProxy::~CRenderCommand_SimulateWaterProxy()
{
	m_waterProxy->Release();
	m_waterParams->Release();
}

CRenderCommand_AddWaterProxyLocalShape::CRenderCommand_AddWaterProxyLocalShape( IRenderProxy* waterProxy, CLocalWaterShapesParams* shapes )
	: m_waterProxy( waterProxy )
	, m_shapesParams( shapes )
{
	RED_ASSERT( m_waterProxy != nullptr && m_shapesParams != nullptr );
	m_waterProxy->AddRef();
	m_shapesParams->AddRef();
}

CRenderCommand_AddWaterProxyLocalShape::~CRenderCommand_AddWaterProxyLocalShape()
{
	SAFE_RELEASE( m_waterProxy );
	SAFE_RELEASE( m_shapesParams );
}

CRenderCommand_SkyboxSetup::CRenderCommand_SkyboxSetup( IRenderScene *scene, const SSkyboxSetupParameters &setupParams )
	: m_scene( scene )
	, m_params( setupParams )
{
	if ( m_scene )
	{
		m_scene->AddRef();
	}

	m_params.AddRefAll();
}

CRenderCommand_SkyboxSetup::~CRenderCommand_SkyboxSetup()
{
	SAFE_RELEASE( m_scene );
}

CRenderCommand_LensFlareSetup::CRenderCommand_LensFlareSetup( IRenderScene *scene, const SLensFlareGroupsSetupParameters &setupParams )
	: m_scene( scene )
	, m_params( setupParams )
{
	if ( m_scene )
	{
		m_scene->AddRef();
	}

	m_params.AddRefAll();
}

CRenderCommand_LensFlareSetup::~CRenderCommand_LensFlareSetup()
{
	SAFE_RELEASE( m_scene );
}

CRenderCommand_HandleResizeEvent::CRenderCommand_HandleResizeEvent( Uint32 width, Uint32 height )
	: m_width( width )
	, m_height( height )
{
	/* Intentionally empty */
}

CRenderCommand_SetEntityGroupHiResShadows::CRenderCommand_SetEntityGroupHiResShadows( IRenderEntityGroup* group, Bool flag )
	: m_group( group )
	, m_flag( flag )
{
	if ( m_group )
	{
		m_group->AddRef();
	}
}

CRenderCommand_SetEntityGroupHiResShadows::~CRenderCommand_SetEntityGroupHiResShadows()
{
	if ( m_group != NULL )
	{
		m_group->Release();
		m_group = NULL;
	}
}

CRenderCommand_SetEntityGroupShadows::CRenderCommand_SetEntityGroupShadows( IRenderEntityGroup* group, Bool flag )
	: m_group( group )
	, m_flag( flag )
{
	if ( m_group )
	{
		m_group->AddRef();
	}
}

CRenderCommand_SetEntityGroupShadows::~CRenderCommand_SetEntityGroupShadows()
{
	if ( m_group != NULL )
	{
		m_group->Release();
		m_group = NULL;
	}
}

CRenderCommand_BindEntityGroupToProxy::CRenderCommand_BindEntityGroupToProxy( IRenderEntityGroup* group, IRenderProxy*	proxy )
	: m_group( group )
	, m_proxy( proxy )
{
	if ( NULL != m_group )
	{
		m_group->AddRef();
	}

	if ( NULL != m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_BindEntityGroupToProxy::~CRenderCommand_BindEntityGroupToProxy()
{
	if ( NULL != m_group )
	{
		m_group->Release();
		m_group = NULL;
	}

	if ( NULL != m_proxy )
	{
		m_proxy->Release();
		m_proxy = NULL;
	}
}

#ifdef USE_APEX
CRenderCommand_UpdateApexRenderable::CRenderCommand_UpdateApexRenderable( IRenderProxy* proxy, physx::apex::NxApexRenderable* renderable, const Box& bbox, const Matrix& l2w, Float wet )
	: m_proxy( proxy )
	, m_renderable( renderable )
	, m_boundingBox( bbox )
	, m_localToWorld( l2w )
	, m_wetness( wet )
{
	ASSERT( m_proxy && m_renderable );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateApexRenderable::~CRenderCommand_UpdateApexRenderable()
{
	SAFE_RELEASE( m_proxy );
}
#endif


CRenderCommand_UpdateTerrainShadows::CRenderCommand_UpdateTerrainShadows( IRenderScene* renderScene )
{
	m_renderScene = renderScene;
	m_renderScene->AddRef();
}
CRenderCommand_UpdateTerrainShadows::~CRenderCommand_UpdateTerrainShadows()
{
	m_renderScene->Release();
}


CRenderCommand_SetProxyLightChannels::CRenderCommand_SetProxyLightChannels( IRenderProxy* proxy, Uint8 lightChannels, Uint8 mask )
	: m_proxy( proxy )
	, m_lightChannels( lightChannels )
	, m_mask( mask )
{
	ASSERT( m_proxy );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_SetProxyLightChannels::~CRenderCommand_SetProxyLightChannels()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_UpdateMorphRatio::CRenderCommand_UpdateMorphRatio( IRenderProxy* proxy, Float ratio )
	: m_proxy( proxy )
	, m_ratio( ratio )
{
	ASSERT( m_proxy );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateMorphRatio::~CRenderCommand_UpdateMorphRatio()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_SetClippingEllipseMatrix::CRenderCommand_SetClippingEllipseMatrix( IRenderProxy* proxy, const Matrix& meshToEllipse )
	: m_proxy( proxy )
	, m_meshToEllipse( meshToEllipse )
{
	RED_ASSERT( m_proxy );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_SetClippingEllipseMatrix::~CRenderCommand_SetClippingEllipseMatrix()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_ClearClippingEllipseMatrix::CRenderCommand_ClearClippingEllipseMatrix( IRenderProxy* proxy )
	: m_proxy( proxy )
{
	RED_ASSERT( m_proxy );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_ClearClippingEllipseMatrix::~CRenderCommand_ClearClippingEllipseMatrix()
{
	SAFE_RELEASE( m_proxy );
}



CRenderCommand_AddDynamicDecalToScene::CRenderCommand_AddDynamicDecalToScene( IRenderScene* scene, IRenderResource* decal, Bool projectOnlyOnStatic /*= false*/ )
	: m_scene( scene )
	, m_decal( decal )
	, m_projectOnlyOnStatic( projectOnlyOnStatic )
{
	RED_ASSERT( m_scene );
	if ( m_scene )
	{
		m_scene->AddRef();
	}
	RED_ASSERT( m_decal );
	if ( m_decal )
	{
		m_decal->AddRef();
	}
}

CRenderCommand_AddDynamicDecalToScene::~CRenderCommand_AddDynamicDecalToScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_decal );
}

CRenderCommand_AddDynamicDecalToSceneForProxies::CRenderCommand_AddDynamicDecalToSceneForProxies( IRenderScene* scene, IRenderResource* decal, const TDynArray< IRenderProxy* >& proxies )
	: m_scene( scene )
	, m_decal( decal )
	, m_targetProxies( proxies )
{
	RED_ASSERT( m_scene );
	if ( m_scene )
	{
		m_scene->AddRef();
	}
	RED_ASSERT( m_decal );
	if ( m_decal )
	{
		m_decal->AddRef();
	}

	for ( Uint32 i = 0; i < m_targetProxies.Size(); ++i )
	{
		RED_ASSERT( m_targetProxies[i] );
		m_targetProxies[i]->AddRef();
	}
}

CRenderCommand_AddDynamicDecalToSceneForProxies::~CRenderCommand_AddDynamicDecalToSceneForProxies()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_decal );

	for ( Uint32 i = 0; i < m_targetProxies.Size(); ++i )
	{
		SAFE_RELEASE( m_targetProxies[i] );
	}
}

CRenderCommand_RemoveDynamicDecalFromScene::CRenderCommand_RemoveDynamicDecalFromScene( IRenderScene* scene, IRenderResource* decal )
	: m_scene( scene )
	, m_decal( decal )
{
	RED_ASSERT( m_scene );
	if ( m_scene )
	{
		m_scene->AddRef();
	}
	RED_ASSERT( m_decal );
	if ( m_decal )
	{
		m_decal->AddRef();
	}
}

CRenderCommand_RemoveDynamicDecalFromScene::~CRenderCommand_RemoveDynamicDecalFromScene()
{
	SAFE_RELEASE( m_scene );
	SAFE_RELEASE( m_decal );
}

CRenderCommand_UpdateStripeProperties::CRenderCommand_UpdateStripeProperties( IRenderProxy* proxy, struct SRenderProxyStripeProperties* properties )
	: m_proxy( proxy )
	, m_properties( properties )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateStripeProperties::~CRenderCommand_UpdateStripeProperties()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold::CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold( IRenderProxy* proxy, Float screenSpaceErrorThreshold )
	: m_proxy( proxy )
	, m_terrainScreenSpaceErrorThreshold( screenSpaceErrorThreshold )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold::~CRenderCommand_UpdateTerrainScreenSpaceErrorThreshold()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_SetParticlePriority::CRenderCommand_SetParticlePriority( IRenderProxy* proxy, Uint8 priority )
	: m_proxy( proxy )
	, m_priority( priority )
{
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}
}

CRenderCommand_SetParticlePriority::~CRenderCommand_SetParticlePriority()
{
	SAFE_RELEASE( m_proxy );
}

CRenderCommand_UpdateSwarmData::CRenderCommand_UpdateSwarmData( IRenderProxy* proxy, IRenderSwarmData* data, Uint32 numBoids )
	: m_proxy( proxy )
	, m_data( data )
	, m_numBoids( numBoids )
{
	ASSERT( m_proxy && m_data );
	if ( m_proxy )
	{
		m_proxy->AddRef();
	}

	if( m_data )
	{
		m_data->AddRef();
	}
}

CRenderCommand_UpdateSwarmData::~CRenderCommand_UpdateSwarmData()
{
	SAFE_RELEASE( m_proxy );
	SAFE_RELEASE( m_data );
}

CRenderCommand_ToggleVideoPause::CRenderCommand_ToggleVideoPause( Bool pause )
	: m_pause( pause )
{
}

CRenderCommand_ToggleVideoPause::~CRenderCommand_ToggleVideoPause()
{
}


CRenderCommand_SetVideoMasterVolume::CRenderCommand_SetVideoMasterVolume( Float volumePercent )
	: m_volumePercent( volumePercent )
{
}

CRenderCommand_SetVideoMasterVolume::~CRenderCommand_SetVideoMasterVolume()
{
}

CRenderCommand_SetVideoVoiceVolume::CRenderCommand_SetVideoVoiceVolume( Float volumePercent )
	: m_volumePercent( volumePercent )
{
}

CRenderCommand_SetVideoVoiceVolume::~CRenderCommand_SetVideoVoiceVolume()
{
}

CRenderCommand_SetVideoEffectsVolume::CRenderCommand_SetVideoEffectsVolume( Float volumePercent )
	: m_volumePercent( volumePercent )
{
}

CRenderCommand_SetVideoEffectsVolume::~CRenderCommand_SetVideoEffectsVolume()
{
}

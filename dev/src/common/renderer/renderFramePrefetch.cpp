/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderFramePrefetch.h"
#include "renderInterface.h"
#include "renderScene.h"
#include "renderElementMap.h"
#include "renderMaterial.h"
#include "renderTextureBase.h"
#include "renderTextureStreaming.h"
#include "renderTextureStreamRequest.h"

#include "renderProxySpeedTree.h"
#include "renderDynamicDecal.h"
#include "renderSkybox.h"

#include "../engine/renderFrame.h"
#include "../engine/renderCommands.h"



namespace Config
{
	TConfigVar< Float > cvSoftFramePrefetchTimeout( "Rendering", "SoftFramePrefetchTimeout", 10.0f );
	TConfigVar< Float > cvMaxFramePrefetchTimeout( "Rendering", "MaxFramePrefetchTimeout", 15.0f );
}



class CPrefetchSceneCollectTask : public CTask
{
private:
	CRenderFramePrefetch*			m_prefetch;

public:
	CPrefetchSceneCollectTask( CRenderFramePrefetch* prefetch )
		: m_prefetch( prefetch )
	{
		m_prefetch->AddRef();
	}

	virtual ~CPrefetchSceneCollectTask()
	{
		SAFE_RELEASE( m_prefetch );
	}

public:
	virtual void Run()
	{
		PC_SCOPE_PIX( CPrefetchSceneCollectTask )

		// TODO : Might be nice to also do a shadows query?

#ifdef USE_UMBRA
		CRenderFrame* frame = m_prefetch->GetFrame();
		CRenderSceneEx* scene = m_prefetch->GetScene();

		ExtraOcclusionQueryResults* queryResults = nullptr;
		if ( CRenderOcclusionData* occlusionData = scene->GetOcclusionData() )
		{
			queryResults = occlusionData->PerformExtraOcclusionQuery( scene, frame, m_prefetch->UseOcclusionQuery() );
		}

		if ( queryResults == nullptr )
		{
			return;
		}

		SExtraSceneCollection sceneCollection;


		// Only support collection through render element map. Even if umbra is turned off in the editor, we still have REM and
		// occlusion tomes will be updated.
		if ( CRenderElementMap* elementMap = scene->GetRenderElementMap() )
		{
			PC_SCOPE_PIX( FramePrefetch_CollectProxies );
			elementMap->CollectProxiesNoModify( queryResults, frame, sceneCollection );
		}


		for ( const IRenderProxyBase* proxy : sceneCollection.m_proxies )
		{
			proxy->Prefetch( m_prefetch );
		}

		for ( const CRenderDynamicDecal* decal : sceneCollection.m_dynamicDecals )
		{
			decal->Prefetch( m_prefetch );
		}


		// There are some things that aren't included in the render element map, so we need to grab those separately.
		{
			PC_SCOPE_PIX( FramePrefetch_CollectMisc );

#ifdef USE_SPEED_TREE
			CRenderProxy_SpeedTree* speedtree = scene->GetSpeedTree();
			if ( speedtree )
			{
				// Finish processing tree instances before doing the prefetch as it uses grid data
				speedtree->FinalizeParallelInstanceUpdates();

				// Add / Remove dynamic instances that were queued during processing of commands
				speedtree->ProcessQueuedInstances();

				speedtree->Prefetch( m_prefetch );
			}
#endif

			// Not IPrefetchable, but oh well...
			CRenderSkybox* skybox = scene->GetSkybox();
			if ( skybox != nullptr )
			{
				const auto& params = skybox->GetParameters();
				m_prefetch->AddMaterialBind( (CRenderMaterial*)params.m_cloudsMaterialResource,	(CRenderMaterialParameters*)params.m_cloudsMaterialParamsResource, 0.0f );
				m_prefetch->AddMaterialBind( (CRenderMaterial*)params.m_moonMaterialResource,	(CRenderMaterialParameters*)params.m_moonMaterialParamsResource, 0.0f );
				m_prefetch->AddMaterialBind( (CRenderMaterial*)params.m_skyboxMaterialResource,	(CRenderMaterialParameters*)params.m_skyboxMaterialParamsResource, 0.0f );
				m_prefetch->AddMaterialBind( (CRenderMaterial*)params.m_sunMaterialResource,	(CRenderMaterialParameters*)params.m_sunMaterialParamsResource, 0.0f );
			}

			if ( !frame->GetFrameInfo().m_envParametersDayPoint.m_cloudsShadowTexture.IsNull() )
			{
				m_prefetch->AddTextureBind( frame->GetFrameInfo().m_envParametersDayPoint.m_cloudsShadowTexture.Get< CRenderTextureBase >(), 0.0f );
			}
			else
			{
				m_prefetch->AddTextureBind( frame->GetFrameInfo().m_envParametersDayPoint.m_fakeCloudsShadowTexture.Get< CRenderTextureBase >(), 0.0f );
			}
		}

		queryResults->Release();
#endif // USE_UMBRA
	}

#ifndef NO_DEBUG_PAGES
public:
	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT("FramePrefetch_Collect"); }

	//! Get debug color
	virtual Uint32 GetDebugColor() const override { return Color::BROWN.ToUint32(); }
#endif

};



CRenderFramePrefetch::CRenderFramePrefetch( CRenderFrame* frame, CRenderSceneEx* scene, Bool useOcclusionQuery /*= true*/ )
	: m_frame( frame )
	, m_scene( scene )
	, m_finishedCollection( false )
	, m_pendingTextureRequestCreate( false )
	, m_collectTask( nullptr )
	, m_textureRequest( nullptr )
	, m_didEnqueue( false )
	, m_useOcclusionQuery( useOcclusionQuery )
{
	RED_ASSERT( m_frame != nullptr && m_scene != nullptr, TXT("Must have a frame and scene") );
	m_frame->AddRef();
	m_scene->AddRef();
}


CRenderFramePrefetch::~CRenderFramePrefetch()
{
	// Unregister from streaming manager first, so we don't get a request to apply in the middle of destruction.
	if ( GetRenderer() && GetRenderer()->GetTextureStreamingManager() )
	{
		GetRenderer()->GetTextureStreamingManager()->RemoveFinishedFramePrefetch( this );
	}


	SAFE_RELEASE( m_frame );
	SAFE_RELEASE( m_scene );

	RED_FATAL_ASSERT( m_collectTask == nullptr, "Still have a collect task. FinishSceneCollect was probably not called!" );

	for ( auto& iter : m_textureDistances )
	{
		iter.m_first->Release();
	}

	if ( m_textureRequest != nullptr )
	{
		// Cancel the request. We've either already started the request, or we won't get there (can't be in the process of
		// calling it or about to call, from another thread, since we're being destroyed already).
		m_textureRequest->Cancel();
		SAFE_RELEASE( m_textureRequest );
	}
}


const Vector& CRenderFramePrefetch::GetCameraPosition() const
{
	return m_frame->GetFrameInfo().m_occlusionCamera.GetPosition();
}

Float CRenderFramePrefetch::GetCameraFovMultiplierUnclamped() const
{
	return m_frame->GetFrameInfo().m_camera.GetFOVMultiplierUnclamped();
}


void CRenderFramePrefetch::BeginSceneCollect()
{
	PC_SCOPE_PIX( FramePrefetch_BeginSceneCollect );

#ifdef USE_UMBRA
	// If there's no occlusion data, then we can't do the prefetch. There's no point in launching any extra tasks.
	if ( m_scene->GetOcclusionData() == nullptr )
	{
		return;
	}
#else
	return;
#endif // USE_UMBRA

	m_timeout.ResetTimer();

	// Start asynchronous occlusion query.
	m_collectTask = new ( CTask::Root ) CPrefetchSceneCollectTask( this );
	GTaskManager->Issue( *m_collectTask );
}


void CRenderFramePrefetch::FinishSceneCollect()
{
	PC_SCOPE_PIX( FramePrefetch_FinishSceneCollect );

	if ( m_collectTask == nullptr )
	{
		OnFinishedCollectTask();
		return;
	}


	// Make sure scene collection is done.
	if ( m_collectTask->MarkRunning() )
	{
		WARN_RENDERER( TXT("FramePrefetch query not started yet! Running locally") );
		m_collectTask->Run();
		m_collectTask->MarkFinished();
	}
	else if ( !m_collectTask->IsFinished() )
	{
		WARN_RENDERER( TXT("FramePrefetch query not finished yet! Waiting") );
		while ( !m_collectTask->IsFinished() )
		{
			RED_BUSY_WAIT();
			// Idle until it finishes.
		}
	}

	m_collectTask->Release();
	m_collectTask = nullptr;


	OnFinishedCollectTask();
}


void CRenderFramePrefetch::DoSceneCollectSync()
{
	PC_SCOPE_PIX( FramePrefetch_DoSceneCollectSync );

#ifdef USE_UMBRA
	// If there's no occlusion data, then we can't do the prefetch. There's no point in launching any extra tasks.
	if ( m_scene->GetOcclusionData() != nullptr )
	{
		m_timeout.ResetTimer();

		CPrefetchSceneCollectTask* task = new ( CTask::Root ) CPrefetchSceneCollectTask( this );
		task->Run();

		task->Release();
	}
#endif // USE_UMBRA

	OnFinishedCollectTask();
}


void CRenderFramePrefetch::OnFinishedCollectTask()
{
	if ( !m_textureDistances.Empty() )
	{
		m_pendingTextureRequestCreate.SetValue( true );
	}

	m_finishedCollection.SetValue( true );
}



void CRenderFramePrefetch::AddMaterialBind( CRenderMaterial* material, CRenderMaterialParameters* parameters, Float distanceSq )
{
	if ( material == nullptr || parameters == nullptr )
	{
		return;
	}

	for ( IRenderResource* resource : parameters->m_textures )
	{
		AddTextureBind( static_cast< CRenderTextureBase* >( resource ), distanceSq );
	}
}

void CRenderFramePrefetch::AddTextureBind( CRenderTextureBase* texture, Float distanceSq )
{
	if ( texture == nullptr )
	{
		return;
	}

	if ( distanceSq < FLT_MAX )
	{
		Float& texDist = m_textureDistances.GetRef( texture, FLT_MAX );

		// If the distance we got is FLT_MAX, then this is the first time we've hit this texture (we aren't adding it if distanceSq
		// is FLT_MAX, so the only time it can have that is if it uses the default value from GetRef), so we addref it.
		if ( texDist == FLT_MAX )
		{
			texture->AddRef();
		}

		texDist = Min( texDist, distanceSq );
	}
}



void CRenderFramePrefetch::ApplyTextureResults()
{
	if ( !m_textureDistances.Empty() )
	{
		// Apply texture distances.
		for ( auto& iter : m_textureDistances )
		{
			iter.m_first->UpdateLastBindDistance( iter.m_second );
		}

		if ( m_pendingTextureRequestCreate.GetValue() )
		{
			RED_ASSERT( m_pendingTextureRequestCreate.GetValue(), TXT("m_pendingTextureRequestCreate wasn't set to true!") );
			RED_ASSERT( m_textureRequest == nullptr, TXT("Already have a texture request") );
			SAFE_RELEASE( m_textureRequest );

			// Don't lock the textures. We don't need to mess with their priorities, since we're already setting
			// new distances. Just want to be notified when they're either streamed in or deemed out-of-budget.
			m_textureRequest = ( CRenderTextureStreamRequest* )GRender->CreateTextureStreamRequest( false );

			// Apply texture distances.
			for ( auto& iter : m_textureDistances )
			{
				m_textureRequest->AddRenderTexture( iter.m_first );
			}

			// Start now. This is on the render thread, should be okay to do now rather than queuing in a render command.
			// It's important, because if we're prefetching a new texture, we don't want to start a new texture streaming task before
			// we can set up the request
			m_textureRequest->Start();

			m_pendingTextureRequestCreate.SetValue( false );
		}
	}
}


Bool CRenderFramePrefetch::IsFinished() const
{
	// Make sure we aren't stuck waiting indefinitely.
	if ( m_timeout.GetTimePeriod() > Config::cvSoftFramePrefetchTimeout.Get() )
	{
		ERR_RENDERER( TXT("CRenderFramePrefetch seems to be taking too long! Will timeout soon!") );
	}
	if ( m_timeout.GetTimePeriod() > Config::cvMaxFramePrefetchTimeout.Get() )
	{
		ERR_RENDERER( TXT("CRenderFramePrefetch has timed out, waited for %0.1fs"), m_timeout.GetTimePeriod() );
		return true;
	}


	// If the background task isn't done yet, we can't be finished.
	if ( !m_finishedCollection.GetValue() )
	{
		return false;
	}

	// If we finished the prefetch task, and ended up with some collected textures, we need to wait until the texture request is made.
	if ( m_pendingTextureRequestCreate.GetValue() )
	{
		return false;
	}

	// If we had a texture request, wait for it to complete.
	// NOTE : m_textureRequest is only assigned before m_pendingTextureRequestCreate is set from true to false. If we have no textures,
	// then m_pendingTextureRequestCreate stays false, and m_textureRequest stays null. If we get textures, we don't hit this point
	// until m_pendingTextureRequestCreate has been switched, at which point we've already set up m_textureRequest.
	if ( m_textureRequest != nullptr && !m_textureRequest->IsReady() )
	{
		return false;
	}

	// Wait until meshes are done loading.
	// TODO : Maybe allow meshes to be added to the prefetch, and just wait for those?
	if ( GRender->IsStreamingMeshes() )
	{
		return false;
	}

	return true;
}



//////////////////////////////////////////////////////////////////////////


IRenderFramePrefetch* CRenderInterface::CreateRenderFramePrefetch( CRenderFrame* frame, IRenderScene* scene, Bool useOcclusion /*= true*/ )
{
	if ( frame == nullptr || scene == nullptr )
	{
		RED_HALT( "Creating a frame prefetch with missing frame or scene" );
		return nullptr;
	}

	return new CRenderFramePrefetch( frame, static_cast< CRenderSceneEx* >( scene ), useOcclusion );
}


void CRenderInterface::EnqueueFramePrefetch( IRenderFramePrefetch* prefetch )
{
	RED_ASSERT( prefetch != nullptr, TXT("Cannot enqueue a null frame prefetch") );
	if ( prefetch == nullptr )
	{
		return;
	}

	CRenderFramePrefetch* renderPrefetch = static_cast< CRenderFramePrefetch* >( prefetch );

	RED_ASSERT( !renderPrefetch->HasBeenEnqueued(), TXT("Cannot enqueue a frame prefetch multiple times") );
	if ( renderPrefetch->HasBeenEnqueued() )
	{
		return;
	}
	renderPrefetch->SetEnqueued();

	// At this point if the prefetch is finished, then it must have timed out. Unusual, but not necessarily an error.
	if ( renderPrefetch->IsFinished() )
	{
		WARN_RENDERER( TXT("Trying to enqueue a prefetch that has timed out. This shouldn't happen normally, unless you've broken into the debugger for some time after creating the prefetch, but before enqueuing it.") );
		return;
	}

	m_numPendingPrefetches.Increment();
	renderPrefetch->AddRef();
	m_pendingFramePrefetches.Push( renderPrefetch );
}


void CRenderInterface::FlushOneFramePrefetch()
{
	if ( m_pendingFramePrefetches.Empty() )
	{
		return;
	}

	CRenderFramePrefetch* prefetch = m_pendingFramePrefetches.Front();
	m_pendingFramePrefetches.Pop();

	prefetch->DoSceneCollectSync();
	m_textureStreamingManager->AddFinishedFramePrefetch( prefetch );
	prefetch->Release();

	Int32 newVal = m_numPendingPrefetches.Decrement();
	RED_UNUSED( newVal );
	RED_ASSERT( newVal >= 0, TXT("Invalid value for m_numPendingPrefetches: %d"), newVal );
}


CRenderFramePrefetch* CRenderInterface::GetNextPendingFramePrefetch() const
{
	if ( m_pendingFramePrefetches.Empty() )
	{
		return nullptr;
	}

	return m_pendingFramePrefetches.Front();
}

CRenderFramePrefetch* CRenderInterface::StartNextFramePrefetch()
{
	if ( m_pendingFramePrefetches.Empty() )
	{
		return nullptr;
	}

	CRenderFramePrefetch* prefetch = m_pendingFramePrefetches.Front();
	m_pendingFramePrefetches.Pop();

	prefetch->BeginSceneCollect();

	return prefetch;
}

void CRenderInterface::FinishFramePrefetch( CRenderFramePrefetch* prefetch )
{
	RED_ASSERT( prefetch != nullptr );

	prefetch->FinishSceneCollect();

	m_textureStreamingManager->AddFinishedFramePrefetch( prefetch );

	prefetch->Release();
	prefetch = nullptr;

	Int32 newVal = m_numPendingPrefetches.Decrement();
	RED_UNUSED( newVal );
	RED_ASSERT( newVal >= 0, TXT("Invalid value for m_numPendingPrefetches: %d"), newVal );
}

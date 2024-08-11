/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementMap.h"
#include "renderScene.h"

#include "renderTerrainShadows.h"
#include "renderProxyParticles.h"
#include "renderProxyDecal.h"
#include "renderProxyStripe.h"
#include "renderProxyFur.h"
#include "renderProxySpeedTree.h"
#include "renderProxyTerrain.h"
#include "renderProxyWater.h"
#include "renderProxyFlare.h"
#include "renderProxyMesh.h"
#include "renderProxyLight.h"
#include "renderProxyDimmer.h"
#include "renderVisibilityQuery.h"
#include "renderMeshBatcher.h"
#include "renderDynamicDecal.h"
#include "renderSkybox.h"
#include "renderVisibilityQueryManager.h"
#include "renderVisibilityExclusionMap.h"
#include "../engine/umbraScene.h"
#include "../engine/renderSettings.h"


namespace
{
	Int32 FindEnvProbeLight( const TDynArray< SSceneEnvProbeLightInfo > &lightsArray, IRenderProxyLight *light )
	{
		for ( Uint32 i=0; i<lightsArray.Size(); ++i )
		{
			if ( lightsArray[i].m_light == light )
			{
				return (Int32)i;
			}
		}

		return -1;
	}

	void AddEnvProbeLight( TDynArray< SSceneEnvProbeLightInfo > &lightsArray, IRenderProxyLight *light )
	{
		RED_ASSERT( -1 == FindEnvProbeLight( lightsArray, light ) );
		RED_ASSERT( -1 == light->GetSceneEnvProbeLightIndex() );
		if ( light->IsRenderedToEnvProbes() )
		{
			const Int32 insertIndex = (Int32)lightsArray.Size();
			lightsArray.PushBack( SSceneEnvProbeLightInfo( light->GetBasePosition(), light->GetRadius(), light ) );
			light->SetSceneEnvProbeLightIndex( insertIndex );
		}
	}

	void RemoveEnvProbeLight( TDynArray< SSceneEnvProbeLightInfo > &lightsArray, IRenderProxyLight *light )
	{
		RED_ASSERT( light->GetSceneEnvProbeLightIndex() == FindEnvProbeLight( lightsArray, light ) );
		if ( -1 != light->GetSceneEnvProbeLightIndex() )
		{
			Int32 entryIndex = light->GetSceneEnvProbeLightIndex();
			if ( entryIndex + 1 < (Int32)lightsArray.Size() )
			{
				lightsArray[entryIndex] = lightsArray.Back();
				lightsArray[entryIndex].m_light->SetSceneEnvProbeLightIndex( entryIndex );				
			}
			light->SetSceneEnvProbeLightIndex( -1 );
			lightsArray.PopBack();
			RED_ASSERT( -1 == FindEnvProbeLight( lightsArray, light ) );
		}
	}

	void RelinkEnvProbeLight( TDynArray< SSceneEnvProbeLightInfo > &lightsArray, IRenderProxyLight *light )
	{
		if ( light->IsRenderedToEnvProbes() && -1 != light->GetSceneEnvProbeLightIndex() )
		{
			RED_ASSERT( light->GetSceneEnvProbeLightIndex() == FindEnvProbeLight( lightsArray, light ) );
			SSceneEnvProbeLightInfo &lightInfo = lightsArray[ light->GetSceneEnvProbeLightIndex() ];
			lightInfo.SetPosAndRadius( light->GetBasePosition(), light->GetRadius() );
		}
		else
		{
			RemoveEnvProbeLight( lightsArray, light );
			AddEnvProbeLight( lightsArray, light );
		}
	}
}


void SQueuedDecalSpawn::Reset()
{
	SAFE_RELEASE( m_decal );
	for ( IRenderProxyDrawable* target : m_targets )
	{
		SAFE_RELEASE( target );
	}
	m_targets.ClearFast();
}


CRenderSceneEx::CRenderSceneEx( Bool isWorldScene )
	: m_speedTree( nullptr )
	, m_frameCounter( 1 )
	, m_updateTerrainLOD( false )
	, m_forcedLOD( -1 )
	, m_terrain( NULL )
	, m_water( NULL )
	, m_skybox( NULL )
	, m_isWorldScene( isWorldScene )
	, m_visibilityQueryManager( nullptr )
#ifdef USE_UMBRA
	, m_occlusionData( nullptr )
	, m_isTomeCollectionValid( false )
#endif
	, m_currentFramePrefetch( nullptr )
	, m_cameraInteriorNumValuesCopied( 0 )
	, m_cameraInteriorFactor ( 0 )
	, m_terrainShadows( nullptr )
	, m_isRendering( false )
{
	ASSERT( GetRenderer() );
	GetRenderer()->GetRenderScenesList().PushBackUnique( this );

	// Create skybox
	m_skybox = new CRenderSkybox();

	m_renderElementMap = new CRenderElementMap();

	m_visibilityExclusionMap = new CRenderVisibilityExclusionMap();

	// Create the visibility query manager and terrain shadows manager for the world scene only
	if ( isWorldScene )
	{
		m_terrainShadows = new CRenderTerrainShadows();
		m_visibilityQueryManager = new CRenderVisibilityQueryManager();
	}

	// Create camera interior value helpers
	EnsureCameraInteriorHelpersCreated();

#ifdef USE_UMBRA
	if ( m_isWorldScene )
	{
		m_occlusionQueryAdditionalMemorySize = Config::cvOcclusionQueryAdditionalMemory.Get() * ( 1024u * 1024u );
		ASSERT( m_occlusionQueryAdditionalMemorySize > 0 );
		m_occlusionQueryAdditionalMemory = reinterpret_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Umbra, MC_UmbraQueryAdditionalMemory, m_occlusionQueryAdditionalMemorySize ) );
	}
#endif // USE_UMBRA
}

CRenderSceneEx::~CRenderSceneEx()
{
	RED_ASSERT( !m_isRendering, TXT("Destroying render scene, but m_isRendering is still true??") );

	// Delete terrain shadows manager
	if ( m_terrainShadows )
	{
		delete m_terrainShadows;
		m_terrainShadows = nullptr;
	}

	// Delete visibility query manager
	if ( m_visibilityQueryManager )
	{
		delete m_visibilityQueryManager;
		m_visibilityQueryManager = nullptr;
	}

	// Delete skybox
	if ( m_skybox )
	{
		delete m_skybox;
		m_skybox = nullptr;
	}

	// Destroy camera staging textures
	DestroyCameraInteriorHelpers();

	// Release pending fadeout removals
	RemovePendingFadeOutRemovals( true );

	// Make sure any remaining dynamic decal removals are dealt with
	FlushRemovedDynamicDecals();

	// Clean up any potentially queued decal spawns
	for ( SQueuedDecalSpawn& spawn : m_dynamicDecalsToSpawn )
	{
		spawn.Reset();
	}
	m_dynamicDecalsToSpawn.ClearFast();


	if ( m_renderElementMap )
	{
		delete m_renderElementMap;
		m_renderElementMap = nullptr;
	}

	// Create visibility exclusion map
	if ( m_visibilityExclusionMap )
	{
		delete m_visibilityExclusionMap;
		m_visibilityExclusionMap = nullptr;
	}

#ifdef USE_UMBRA
	SAFE_RELEASE( m_occlusionData );
	if ( m_occlusionQueryAdditionalMemory )
	{
		RED_MEMORY_FREE( MemoryPool_Umbra, MC_UmbraQueryAdditionalMemory, m_occlusionQueryAdditionalMemory );
	}	
#endif // USE_UMBRA

	for( auto iter = m_renderingGroups.Begin(), end = m_renderingGroups.End(); iter != end; ++iter )
	{
		iter->m_second->Release();
	}

	m_renderingGroups.Clear();
	
	ASSERT( GetRenderer() );
	GetRenderer()->GetRenderScenesList().Remove( this );
}

void CRenderSceneEx::BeginRendering()
{
	RED_ASSERT( !m_isRendering, TXT("Scene is already being rendered!") );
	m_isRendering = true;
	m_repeatedFrameCounter++;
}

void CRenderSceneEx::EndRendering()
{
	RED_ASSERT( m_isRendering, TXT("Scene is not being rendered!") );

	m_dissolveSynchronizer.Advance( GetRenderer()->GetLastTickDelta() );

	m_isRendering = false;
	FlushRemovedDynamicDecals();
}


void CRenderSceneEx::AllocateFrame()
{
	++m_frameCounter;
}

void CRenderSceneEx::AddProxy( IRenderProxyBase* proxy )
{
	ASSERT( proxy );

	IRenderProxyDrawable* drawableProxy = static_cast< IRenderProxyDrawable* >( proxy );

	// Attach to scene
	proxy->AttachToScene( this );

	// Proxy type
	const ERenderProxyType type = proxy->GetType();

	if ( proxy->ShouldBeManagedByRenderElementMap() )
	{
		proxy->Register( GetRenderElementMap() );
	}

	// Add by type
	if ( type == RPT_PointLight || type == RPT_SpotLight )
	{
		AddEnvProbeLight( m_envProbeLights, static_cast< IRenderProxyLight* >( proxy ) );
	}
	else if ( type == RPT_Flare && ( static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Sun || static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Moon ) )
	{
		m_backgroundDrawables.PushBack( drawableProxy );
	}
	else if ( (type == RPT_Mesh || type == RPT_Particles || type == RPT_Flare ) 
		&& ( drawableProxy->GetRenderingPlane() == RPl_Background ) )
	{
		m_backgroundDrawables.PushBack( drawableProxy );
	}
}

void CRenderSceneEx::RemoveProxy( IRenderProxyBase* proxy )
{
	ASSERT( proxy );
	ASSERT( m_fadeablesToRemove.End() == Find(m_fadeablesToRemove.Begin(), m_fadeablesToRemove.End(), proxy) );

	IRenderProxyDrawable* drawableProxy = static_cast< IRenderProxyDrawable* >( proxy );

	// Unlink proxy from this scene
	proxy->DetachFromScene( this );

	// Proxy type
	const ERenderProxyType type = proxy->GetType();

	if ( proxy->ShouldBeManagedByRenderElementMap() )
	{
		RED_VERIFY( proxy->Unregister( GetRenderElementMap() ), TXT("Failed to undegister proxy from RenderElementMap!") );
	}

	// Remove by type
	if ( type == RPT_PointLight || type == RPT_SpotLight )
	{
		RemoveEnvProbeLight( m_envProbeLights, static_cast< IRenderProxyLight* >( proxy ) );
	}

	else if ( type == RPT_Flare && ( static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Sun || static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Moon ) )
	{
		m_backgroundDrawables.Remove( drawableProxy );
	}
	else if ( (type == RPT_Mesh || type == RPT_Particles || type == RPT_Flare ) 
		&& ( drawableProxy->GetRenderingPlane() == RPl_Background ) )
	{
		m_backgroundDrawables.Remove( drawableProxy );
	}
}


Bool CRenderSceneEx::KeepUnderDynamicDecalBudget()
{
	// TODO : Maybe use some visibility info, to discard a decal that wasn't drawn for a few frames? or if past auto-hide?
	// TODO : Keep all persistent decals at end of list, or in separate list, so we don't have to skip over them?


	// Maintain both budgets (decals and decals with chunks). The basic idea is the same for both: Find oldest non-persistent
	// decal that is consuming that budget and remove it (or its chunks). If no non-persistent decal is found, remove the
	// oldest persistent one. Since new decals are always pushed to the end, and we don't rearrange them, we can just go
	// through the list in order and find the first one.


	const Int32 MaxDecalBudget = Config::cvDynamicDecalsLimit.Get();
	const Int32 MaxDecalChunkBudget = Config::cvDynamicDecalsChunkLimit.Get();

	// Make sure we're under the chunk budget. Removing chunks may also reduce the overall number of decals (e.g. if some
	// don't have a screenspace decal), so it should come first.
	while ( m_dynamicDecalChunkBudgetUsage.GetValue() > MaxDecalChunkBudget )
	{
		Int32 indexToFree = -1;
		Int32 firstWithChunks = -1;

		const Int32 numDecals = m_dynamicDecals.SizeInt();
		for ( Int32 i = 0; i < numDecals; ++i )
		{
			// Skip over decals with no chunk budget used
			if ( m_dynamicDecalsChunkUsage[i] > 0 )
			{
				// Track the index of the first decal with chunks. If we don't find a non-persistent decal with chunks,
				// we'll remove this one (oldest persistent with chunks).
				if ( firstWithChunks < 0 )
				{
					firstWithChunks = i;
				}

				// If it's not persistent, then this is the decal we should remove chunks from.
				if ( m_dynamicDecals[i]->GetTimeToLive() < NumericLimits< Float >::Infinity() )
				{
					indexToFree = i;
					break;
				}
			}
		}

		// If we didn't find one, we only have persistent decals... These are still contributing to the budget, so if
		// we don't do anything we won't be able to create any new decals. Therefore, we'll destroy the oldest (first
		// in the list)...
		if ( indexToFree == -1 )
		{
			indexToFree = firstWithChunks;
		}

		RED_ASSERT( indexToFree >= 0, TXT("Over budget, but didn't find a decal with chunks!") );

		RemoveDynamicDecalChunks( m_dynamicDecals[indexToFree] );
	}


	// If we have too many decals already, free some until we are under budget again.
	while ( m_dynamicDecalBudgetUsage.GetValue() > MaxDecalBudget )
	{
		Int32 indexToFree = -1;

		const Int32 numDecals = m_dynamicDecals.SizeInt();
		for ( Int32 i = 0; i < numDecals; ++i )
		{
			// Skip over persistent decals.
			if ( m_dynamicDecals[i]->GetTimeToLive() < NumericLimits< Float >::Infinity() )
			{
				indexToFree = i;
				break;
			}
		}

		// If we didn't find one, we only have persistent decals... These are still contributing to the budget, so if
		// we don't do anything we won't be able to create any new decals. Therefore, we'll destroy the oldest (first
		// in the list)...
		if ( indexToFree == -1 )
		{
			indexToFree = 0;
		}

		RED_ASSERT( indexToFree >= 0 && indexToFree < m_dynamicDecals.SizeInt(), TXT("Over dynamic decal budget, but no decals??") );

		RemoveDynamicDecal( m_dynamicDecals[indexToFree] );
	}
	return true;
}

void CRenderSceneEx::ConsumeDynamicDecalBudget( Int32 chunksAmount, Int32 decalsAmount )
{
	m_dynamicDecalChunkBudgetUsage.ExchangeAdd( chunksAmount );
	m_dynamicDecalBudgetUsage.ExchangeAdd( decalsAmount );

	// Make sure we haven't gone over budget.
	KeepUnderDynamicDecalBudget();
}

void CRenderSceneEx::ReturnDynamicDecalBudget( Int32 chunksAmount, Int32 decalsAmount )
{
	m_dynamicDecalChunkBudgetUsage.ExchangeAdd( -chunksAmount );
	m_dynamicDecalBudgetUsage.ExchangeAdd( -decalsAmount );
}



#ifndef RED_FINAL_BUILD
Bool CRenderSceneEx::EnsureDynamicDecalNotInScene( IRenderResource* decal ) const
{
	auto toSpawnIter = FindIf( m_dynamicDecalsToSpawn.Begin(), m_dynamicDecalsToSpawn.End(), [decal]( const SQueuedDecalSpawn& spawn ) {
		return spawn.m_decal == decal;
	} );
	if ( toSpawnIter != m_dynamicDecalsToSpawn.End() )
	{
		RED_HALT( "Decal has already been queued for spawn!" );
		return false;
	}

	if ( m_dynamicDecals.Exist( static_cast< CRenderDynamicDecal* >( decal ) ) )
	{
		RED_HALT( "Decal is already in the scene!" );
		return false;
	}

	return true;
}
#endif

void CRenderSceneEx::QueueDynamicDecalSpawn( IRenderResource* decal, Bool projectOnlyOnStatic /*= false*/ )
{
	RED_ASSERT( decal != nullptr, TXT("Cannot queue a null decal") );
	if ( decal == nullptr )
	{
		return;
	}

#ifndef RED_FINAL_BUILD
	if ( !EnsureDynamicDecalNotInScene( decal ) )
	{
		return;
	}
#endif

	SQueuedDecalSpawn* spawn	= new ( m_dynamicDecalsToSpawn ) SQueuedDecalSpawn;
	spawn->m_decal				= static_cast< CRenderDynamicDecal* >( decal );
	spawn->m_staticOnly			= projectOnlyOnStatic;
	decal->AddRef();
}

void CRenderSceneEx::QueueDynamicDecalSpawn( IRenderResource* decal, const TDynArray< IRenderProxy* >& targetProxies )
{
	RED_ASSERT( decal != nullptr, TXT("Cannot queue a null decal") );
	if ( decal == nullptr )
	{
		return;
	}

#ifndef RED_FINAL_BUILD
	if ( !EnsureDynamicDecalNotInScene( decal ) )
	{
		return;
	}
#endif

	TDynArray< IRenderProxyDrawable* > targets;
	for ( IRenderProxy* target : targetProxies )
	{
		if ( target != nullptr && static_cast< IRenderProxyBase* >( target )->IsDrawable() )
		{
			targets.PushBack( static_cast< IRenderProxyDrawable* >( target ) );
			target->AddRef();
		}
	}

	// If there are no valid targets, we don't have to do anything.
	if ( targets.Empty() )
	{
		return;
	}

	SQueuedDecalSpawn* spawn	= new ( m_dynamicDecalsToSpawn ) SQueuedDecalSpawn;
	spawn->m_decal				= static_cast< CRenderDynamicDecal* >( decal );
	spawn->m_staticOnly			= false;
	spawn->m_targets.SwapWith( targets );
	decal->AddRef();
}

void CRenderSceneEx::SpawnQueuedDynamicDecals()
{
	PC_SCOPE_PIX( RenderScene_SpawnQueuedDynamicDecals )	

	for ( SQueuedDecalSpawn& spawn : m_dynamicDecalsToSpawn )
	{
		CRenderDynamicDecal* decal = spawn.m_decal;

		// NOTE : Code-wise, the following are just opposites. But they represent different things, so I've kept them separate.
		const Bool haveExplicitTargetList	= !spawn.m_targets.Empty();
		const Bool spawnStaticDecal			= !haveExplicitTargetList;

		// If targets list is empty and we aren't doing static-only, gather all nearby drawables. Add them to the decal.
		if ( !haveExplicitTargetList && !spawn.m_staticOnly )
		{
			Box decalBounds = decal->GetInitialBounds();

			// Collect all proxies in range of the decal.
			TDynArray< IRenderProxyBase* > proxiesInDecal;
			m_renderElementMap->Collect( decalBounds, CRenderProxyTypeFlag( RPT_Mesh ) | CRenderProxyTypeFlag( RPT_Apex ), proxiesInDecal );

			// Add any dynamic proxies to the decal. Don't add statics, since those will be covered by the screen-space decal.
			for ( IRenderProxyBase* proxy : proxiesInDecal )
			{
				// we can safely cast to IRenderProxyDrawable, because we collected Meshes and Apex
				IRenderProxyDrawable* drawableProxy = static_cast< IRenderProxyDrawable* >( proxy );
				RED_FATAL_ASSERT( drawableProxy, "Invalid proxy" );
				if ( drawableProxy->IsDynamic() )
				{
					if ( IDynamicDecalTarget* target = drawableProxy->ToDynamicDecalTarget() )
					{
						target->AddDynamicDecal( decal );
					}
				}
			}
		}
		// Given an explicit list of proxies, so just add them all. Don't check for dynamics here, we assume that if we get a
		// proxy here, then it should be applied.
		else
		{
			for ( IRenderProxyDrawable* proxy : spawn.m_targets )
			{
				if ( IDynamicDecalTarget* target = proxy->ToDynamicDecalTarget() )
				{
					target->AddDynamicDecal( decal );
				}
			}
		}

		// Only keep it if it actually has some chunks. Again, since the caller has given us an explicit list of proxies,
		// we assume they don't also need the static decal.
		if ( decal->GetNumChunks() > 0 || spawnStaticDecal )
		{
			// Chunk budget is just the number of decals with chunks. We could limit the number of chunks total, but this
			// is simpler.
			Uint8 chunkBudget = decal->GetNumChunks() > 0 ? 1 : 0;

			// Add to the dynamic decals list. The reference held by having it queued is carried over here, so we don't have
			// to addref and then immediately release it.
			m_dynamicDecals.PushBack( decal );
			m_dynamicDecalsChunkUsage.PushBack( chunkBudget );
			RED_FATAL_ASSERT( m_dynamicDecals.Size() == m_dynamicDecalsChunkUsage.Size(), "m_dynamicDecals size (%u) doesn't match m_dynamicDecalsChunkUsage (%u)", m_dynamicDecals.Size(), m_dynamicDecalsChunkUsage.Size() );

			decal->AttachToScene( this, spawnStaticDecal );

			ConsumeDynamicDecalBudget( chunkBudget, 1 );
		}
		else
		{
			// Didn't get added, so release the reference we hold.
			decal->Release();
		}

		// Release all target proxies if needed.
		if ( haveExplicitTargetList )
		{
			for ( auto& target : spawn.m_targets )
			{
				SAFE_RELEASE( target );
			}
		}
	}

	m_dynamicDecalsToSpawn.ClearFast();
}

void CRenderSceneEx::RemoveDynamicDecal( IRenderResource* decal )
{
	if ( decal == nullptr )
	{
		return;
	}

	CRenderDynamicDecal* decalPtr = static_cast< CRenderDynamicDecal* >( decal );

	// First try to remove it from our list. If it wasn't there to remove, then it must have been destroyed earlier.
	// This is okay, because it could have been destroyed after TTL, or if it had no chunks, but whoever originally
	// created it wouldn't know that and could later want to remove it.
	Int32 index = (Int32)m_dynamicDecals.GetIndex( decalPtr );
	if ( index >= 0 )
	{
		RED_FATAL_ASSERT( (Uint32)index < m_dynamicDecals.Size(), "Found index out of range (%u >= %u)", (Uint32)index, m_dynamicDecals.Size() );

		const Uint8 chunkUsage = m_dynamicDecalsChunkUsage[ index ];

		m_dynamicDecals.RemoveAt( index );
		m_dynamicDecalsChunkUsage.RemoveAt( index );
		RED_FATAL_ASSERT( m_dynamicDecals.Size() == m_dynamicDecalsChunkUsage.Size(), "m_dynamicDecals size (%u) doesn't match m_dynamicDecalsChunkUsage (%u)", m_dynamicDecals.Size(), m_dynamicDecalsChunkUsage.Size() );

		ReturnDynamicDecalBudget( chunkUsage, 1 );

		if ( m_isRendering )
		{
			// Don't actually destroy the decal yet. It's possible that this happens during rendering (e.g. if a particle
			// decal spawner pushed us over budget), and some of our chunks are collected and assumed to be alive. Instead,
			// we'll just add to a list, to be destroyed at the end of rendering.
			m_removedDynamicDecals.PushBack( decalPtr );
		}
		else
		{
			// We aren't mid-render, so it's safe to just destroy the decal now.
			decalPtr->DetachFromScene( this );
			decalPtr->DestroyDecal();
			decalPtr->Release();
		}
	}
	// If it wasn't removed from the scene, it may be sitting in the pending spawn list. For this we can just remove it
	// directly without going through the deferred removal, because that only matters when the decal is actually in the scene.
	else
	{
		auto iter = FindIf( m_dynamicDecalsToSpawn.Begin(), m_dynamicDecalsToSpawn.End(), [decal]( const SQueuedDecalSpawn& spawn ) {
			return spawn.m_decal == decal;
		} );
		if ( iter != m_dynamicDecalsToSpawn.End() )
		{
			iter->Reset();
			m_dynamicDecalsToSpawn.EraseFast( iter );
		}
	}
}


void CRenderSceneEx::RemoveDynamicDecalChunks( IRenderResource* decal )
{
	if ( decal == nullptr )
	{
		return;
	}

	CRenderDynamicDecal* decalPtr = static_cast< CRenderDynamicDecal* >( decal );

	Int32 index = (Int32)m_dynamicDecals.GetIndex( decalPtr );
	if ( index < 0 )
	{
		return;
	}

	RED_FATAL_ASSERT( (Uint32)index < m_dynamicDecals.Size(), "Found index out of range (%u >= %u)", (Uint32)index, m_dynamicDecals.Size() );

	// Get chunk budget usage, and clear to 0. Important to clear it, so if we later remove the whole decal, we won't
	// double-return the chunk budget.
	const Uint8 chunkUsage = m_dynamicDecalsChunkUsage[ index ];
	m_dynamicDecalsChunkUsage[ index ] = 0;

	// Return only the chunk budget.
	ReturnDynamicDecalBudget( chunkUsage, 0 );

	if ( m_isRendering )
	{
		// Don't actually remove the chunks yet. It's possible that this happens during rendering (e.g. if a particle
		// decal spawner pushed us over budget), and some of our chunks are collected and assumed to be alive. Instead,
		// we'll just add to a list, to be destroyed at the end of rendering.
		m_removedDynamicDecalChunks.PushBack( decalPtr );
	}
	else
	{
		if ( decalPtr->DestroyChunks() )
		{
			RemoveDynamicDecal( decalPtr );
		}
	}
}


void CRenderSceneEx::FlushRemovedDynamicDecals()
{
	RED_ASSERT( !m_isRendering, TXT("FlushRemovedDynamicDecals() must be called outside of rendering!") );
	if ( m_isRendering )
	{
		return;
	}

	for ( CRenderDynamicDecal* decal : m_removedDynamicDecalChunks )
	{
		if ( decal->DestroyChunks() )
		{
			RemoveDynamicDecal( decal );
		}
	}
	m_removedDynamicDecalChunks.ClearFast();

	for ( CRenderDynamicDecal* decal : m_removedDynamicDecals )
	{
		decal->DetachFromScene( this );
		decal->DestroyDecal();
		decal->Release();
	}
	m_removedDynamicDecals.ClearFast();
}

CRenderProxyObjectGroup* CRenderSceneEx::GetRenderingGroup( const Uint64 hash )
{
	// invalid group
	if ( !hash )
		return nullptr;

	// find existing one
	CRenderProxyObjectGroup* group = nullptr;
	if ( m_renderingGroups.Find( hash, group ) && group )
		return group;

	// create new group
	group = new CRenderProxyObjectGroup( hash );
	m_renderingGroups.Set( hash, group );
	return group;
}

void CRenderSceneEx::AddStripe( IRenderProxy* proxy )
{
	CRenderProxy_Stripe* stripeProxy = static_cast< CRenderProxy_Stripe* >( proxy );

	// Add to proxies
	m_stripeProxies.PushBack( stripeProxy );

	m_renderElementMap->Register( stripeProxy );
}

void CRenderSceneEx::RemoveStripe( IRenderProxy* proxy )
{
	ASSERT( proxy );

	CRenderProxy_Stripe* stripeProxy = static_cast< CRenderProxy_Stripe* >( proxy );

	// Remove from array
	m_stripeProxies.Remove( stripeProxy );

	m_renderElementMap->Unregister( stripeProxy );
}

void CRenderSceneEx::AddFur( IRenderProxy* proxy )
{
	CRenderProxy_Fur* furProxy = static_cast< CRenderProxy_Fur* >( proxy );

	ASSERT( !m_furProxies.Exist( furProxy ), TXT("fur already added") );

	// Add to proxies
	m_furProxies.PushBackUnique( furProxy );
}

void CRenderSceneEx::RemoveFur( IRenderProxy* proxy )
{
	ASSERT( proxy );
	ASSERT( proxy->GetType() == RPT_Fur, TXT("not a fur proxy?") );

	CRenderProxy_Fur* furProxy = static_cast< CRenderProxy_Fur* >( proxy );

	if ( furProxy )
	{
		// Remove from array
		VERIFY( m_furProxies.Remove( furProxy ), TXT("fur proxy not removed") );
	}
}

void CRenderSceneEx::AddSpeedTreeProxy( IRenderObject* proxy )
{
	ASSERT( proxy );

#ifdef USE_SPEED_TREE
	RED_FATAL_ASSERT( m_speedTree == nullptr, "CRenderSceneEx::AddSpeedTreeProxy - There already is a SpeedTree proxy." );

	CRenderProxy_SpeedTree* speedTreeProxy = static_cast< CRenderProxy_SpeedTree* >( proxy );

	// Addref this proxy
	speedTreeProxy->AddRef();

	speedTreeProxy->AttachToScene();

	// Add to speed tree proxies
	m_speedTree = speedTreeProxy;
#endif
}

void CRenderSceneEx::RemoveSpeedTreeProxy( IRenderObject* proxy )
{
	ASSERT( proxy );

#ifdef USE_SPEED_TREE
	RED_FATAL_ASSERT( m_speedTree != nullptr, "CRenderSceneEx::RemoveSpeedTreeProxy - There is no SpeedTree proxy." );
	CRenderProxy_SpeedTree* speedTreeProxy = static_cast< CRenderProxy_SpeedTree* >( proxy );
	RED_FATAL_ASSERT( m_speedTree == speedTreeProxy, "CRenderSceneEx::RemoveSpeedTreeProxy - There is a different SpeedTree proxy in the scene." );

	speedTreeProxy->DetachFromScene();

	// Release this proxy
	speedTreeProxy->Release();
	m_speedTree = nullptr;
#endif
}

void CRenderSceneEx::SetLensFlareGroupsParameters( const SLensFlareGroupsSetupParameters &params )
{ 
	m_lensFlareGroupsParams.ReleaseAll();
	m_lensFlareGroupsParams = params;
	m_lensFlareGroupsParams.AddRefAll();
}

void CRenderSceneEx::SetTerrainProxy( IRenderProxy* proxy )
{
	ASSERT( proxy );

	CRenderProxy_Terrain* terrainProxy = static_cast< CRenderProxy_Terrain* >( proxy );

	// Addref this proxy
	terrainProxy->AddRef();

	// Release old proxy
	if ( m_terrain )
	{
		m_terrain->Release();
		m_terrain = NULL;
	}

	// Set proxy
	m_terrain = terrainProxy;
}

void CRenderSceneEx::RemoveTerrainProxy( IRenderProxy* proxy )
{
	ASSERT( proxy );

	CRenderProxy_Terrain* terrainProxy = static_cast< CRenderProxy_Terrain* >( proxy );

	if ( m_terrain == terrainProxy )
	{
		// Release this proxy
		m_terrain->Release();
		m_terrain = NULL;
	}
}

void CRenderSceneEx::SetWaterProxy( IRenderProxy* proxy )
{
	ASSERT( proxy );

	CRenderProxy_Water* waterProxy = static_cast< CRenderProxy_Water* >( proxy );

	// Addref this proxy
	waterProxy->AddRef();

	// Release old proxy
	if ( m_water )
	{
		m_water->Release();
		m_water = NULL;
	}

	// Set proxy
	m_water = waterProxy;

	m_water->AttachToScene();
}

void CRenderSceneEx::RemoveWaterProxy( IRenderProxy* proxy )
{
	ASSERT( proxy );

	CRenderProxy_Water* waterProxy = static_cast< CRenderProxy_Water* >( proxy );

	if ( m_water == waterProxy )
	{
		// Release this proxy
		m_water->Release();
		m_water = NULL;
	}
}

void CRenderSceneEx::AddVisibilityExclusionObject( IRenderVisibilityExclusion* object )
{
	m_visibilityExclusionMap->AddList( static_cast< CRenderVisibilityExclusionList* >( object ) );
}

void CRenderSceneEx::RemoveVisibilityExclusionObject( IRenderVisibilityExclusion* object )
{
	m_visibilityExclusionMap->RemoveList( static_cast< CRenderVisibilityExclusionList* >( object ) );
}

void CRenderSceneEx::RefreshVisibilityExclusionObject( IRenderVisibilityExclusion* object )
{
	CRenderVisibilityExclusionList* list = static_cast< CRenderVisibilityExclusionList* >( object );
	m_visibilityExclusionMap->MarkDirty( list );
}

void CRenderSceneEx::SetupActiveFlare( CRenderProxy_Flare *flare, Bool setActive )
{
	ASSERT( flare );
	ASSERT( this == flare->GetScene() );
	if ( -1 != flare->GetActiveFlareIndex() ) // is currently active
	{
		const Int32 idx = flare->GetActiveFlareIndex();
		ASSERT( m_activeFlares[idx] == flare );
		if ( !setActive )
		{
			ASSERT( m_activeFlares.Back()->GetActiveFlareIndex() == (Int32)m_activeFlares.Size() - 1 );
			m_activeFlares[idx] = m_activeFlares.Back();
			m_activeFlares[idx]->SetActiveFlareIndex( idx );
			m_activeFlares.PopBack();
			flare->SetActiveFlareIndex( -1 );
		}
	}
	else // is currently inactive
	{
		if ( setActive )
		{
			// HANDS-ON DEMO HACK: processing of flares in the entity is fucked up, there are multiple 
			// render proxies for the same flare on scene.
			for( Uint32 kk = 0; kk < m_activeFlares.Size(); ++kk )
			{
				if( flare->GetLocalToWorld().GetTranslation() == m_activeFlares[kk]->GetLocalToWorld().GetTranslation() )
				{
					return;
				}
			}

			ASSERT( !m_activeFlares.Exist( flare ) );
			m_activeFlares.PushBack( flare );
			flare->SetActiveFlareIndex( (Int32)m_activeFlares.Size() - 1 );
		}
	}
}

void CRenderSceneEx::RegisterFadeOutRemoval( IRenderProxyFadeable* proxy )
{
	RED_ASSERT( proxy );
	RED_ASSERT( m_fadeablesToRemove.End() == Find( m_fadeablesToRemove.Begin(), m_fadeablesToRemove.End(), proxy ) );

	proxy->AddRef();
	m_fadeablesToRemove.PushBack( proxy );

	proxy->Unregister( GetRenderElementMap(), true );
}

void CRenderSceneEx::RelinkProxy( IRenderProxyBase* proxy )
{
	RED_ASSERT( proxy );
	RED_ASSERT( proxy->GetScene() == this );

	IRenderProxyDrawable* drawableProxy = static_cast< IRenderProxyDrawable* >( proxy );

	// Relink
	// Proxy type
	const ERenderProxyType type = proxy->GetType();
	if ( m_renderElementMap && proxy->GetRegCount() > 0 )
	{
		const Box& bbox = proxy->GetBoundingBox();
		const Vector& referencePosition = type == RPT_Particles ? proxy->GetLocalToWorld().GetTranslationRef() : bbox.CalcCenter();
		m_renderElementMap->UpdateProxyInfo( proxy->GetEntryID(), referencePosition, bbox );
	}

	// Add by type
	if ( type == RPT_PointLight || type == RPT_SpotLight )
	{
		RelinkEnvProbeLight( m_envProbeLights, static_cast< IRenderProxyLight* >( proxy ) );
	}
	else if ( type == RPT_Flare && ( static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Sun || static_cast< CRenderProxy_Flare* >( proxy )->GetParameters().m_category == FLARECAT_Moon ) )
	{
		ASSERT( m_backgroundDrawables.Exist( drawableProxy ) );
	}
	else if ( (type == RPT_Mesh || type == RPT_Particles || type == RPT_Flare ) 
		&& ( drawableProxy->GetRenderingPlane() == RPl_Background ) )
	{
		ASSERT( m_backgroundDrawables.Exist( drawableProxy ) );
	}
}

void CRenderSceneEx::RemovePendingFadeOutRemovals( bool forceRemoveAll )
{
	PC_SCOPE_PIX( RenderScene_RemovePendingFadeOutRemovals )

	// Remove pending drawables
	for ( Int32 drawable_i=(Int32)m_fadeablesToRemove.Size()-1; drawable_i>=0; --drawable_i )
	{
		IRenderProxyFadeable *proxy = m_fadeablesToRemove[drawable_i];
		if ( !forceRemoveAll && !proxy->IsFadeAndDestroyFinished( m_frameCounter ) )
		{
			continue;
		}
		
		// Remove from pending list
		m_fadeablesToRemove[drawable_i] = m_fadeablesToRemove.Back();
		m_fadeablesToRemove.PopBack();

		// Destroy
		RemoveProxy( proxy );
		proxy->Release();
	}
}

void CRenderSceneEx::CollectEnvProbeLights( const Vector &collectOrigin, Float collectRadius, TDynArray< SCollectedEnvProbeLight >& lights ) const
{
	if ( collectRadius <= 0 )
	{
		return;
	}
	
	Uint32 num = lights.Size();
	lights.ResizeFast( num + m_envProbeLights.Size() );
	
	for ( Uint32 proxy_i=0; proxy_i<m_envProbeLights.Size(); ++proxy_i )
	{
		const SSceneEnvProbeLightInfo &lightInfo = m_envProbeLights[ proxy_i ];
		IRenderProxyLight* light = lightInfo.m_light;
		RED_ASSERT( light->IsRenderedToEnvProbes() );
		RED_ASSERT( SSceneEnvProbeLightInfo ().SetPosAndRadius( light->GetBasePosition(), light->GetRadius() ).m_posAndRadius == lightInfo.m_posAndRadius );

		const Float dist = collectOrigin.DistanceTo( lightInfo.m_posAndRadius ) - lightInfo.m_posAndRadius.W;
		if ( dist > collectRadius )
		{
			continue;
		}
		
		//
		lights[ num ].m_sortKey = dist;
		lights[ num ].m_light = light;
		++num;
	}

	lights.ResizeFast( num );
}

void CRenderSceneEx::UpdateEnvProbeLightParameters( IRenderProxyLight &light )
{
	RelinkEnvProbeLight( m_envProbeLights, &light );
}

struct SortDimmers
{
	Vector m_camera;

	SortDimmers( const Vector& camera )
		: m_camera( camera )
	{}

	RED_FORCE_INLINE Bool operator() ( const CRenderProxy_Dimmer* dimmerA, const CRenderProxy_Dimmer* dimmerB ) const
	{
		// center dist instead of obb dist since it's faster and so much accuracy is not needed in this case
		const Float distanceA = dimmerA->GetLocalToWorld().GetTranslation().DistanceSquaredTo( m_camera );
		const Float distanceB = dimmerB->GetLocalToWorld().GetTranslation().DistanceSquaredTo( m_camera );

		return distanceA < distanceB;
	}
};

void UpdateDimmersBudgetAlpha( const CRenderFrameInfo &info, TDynArray< CRenderProxy_Dimmer* >& proxies )
{
	COMPILE_ASSERT( CRenderInterface::TILED_DEFERRED_DIMMERS_FADE_TAIL < CRenderInterface::TILED_DEFERRED_DIMMERS_CAPACITY );
	const Uint32 fullCapacity = CRenderInterface::TILED_DEFERRED_DIMMERS_CAPACITY;
	const Uint32 mainCapacity = fullCapacity - CRenderInterface::TILED_DEFERRED_DIMMERS_FADE_TAIL;
	
	// Sort
	{
		PC_SCOPE_PIX( DimmersSort );

		// sort dimmers because we have a limited number of slots
		if ( proxies.Size() > mainCapacity )
		{
			Sort( proxies.Begin(), proxies.End(), SortDimmers( info.m_camera.GetPosition() ) );
		}
	}	

	// Update budget alpha
	{
		PC_SCOPE_PIX( DimmersBudgetUpdate );

		const Float frameTime = GetRenderer()->GetLastTickDelta();
		for ( Uint32 proxy_i=0, num_update=Min(fullCapacity, proxies.Size()); proxy_i<num_update; ++proxy_i )
		{
			proxies[proxy_i]->UpdateBudgetFadeAlpha( frameTime, proxy_i < mainCapacity );
		}
	}
}

void CRenderSceneEx::CollectStaticDrawables( CRenderCollector& collector )
{
	PC_SCOPE_PIX( CollectStaticDrawables );
	m_renderElementMap->CollectStaticProxies( collector );
}

void CRenderSceneEx::CollectDrawables( CRenderCollector& collector )
{
	PC_SCOPE_PIX( CollectDrawables );
	m_renderElementMap->CollectNonStaticProxies( collector );
	// Update dimmers budget
	UpdateDimmersBudgetAlpha( *collector.m_info, collector.m_renderCollectorData->m_dimmers );
}

void CRenderSceneEx::CollectActiveFlares( CRenderCollector& collector )
{
	PC_SCOPE_PIX( CollectActiveFlares );

	for ( Uint32 flare_i=0; flare_i<m_activeFlares.Size(); ++flare_i )
	{
		CRenderProxy_Flare *flare = m_activeFlares[flare_i];
		collector.m_renderCollectorData->m_flaresByCategory[ flare->GetParameters().m_category ].PushBack( flare );
	}
}

void CRenderSceneEx::CollectBackgroundDrawables( CRenderCollector& collector )
{
	PC_SCOPE_PIX( CollectBackgroundDrawables );
	// Collect background drawables
	for ( Uint32 i = 0; i < m_backgroundDrawables.Size(); ++i )
	{
		m_backgroundDrawables[i]->CollectElements( collector );
	}
}

void CRenderSceneEx::TickProxies( Float timeDelta )
{
	{
		PC_SCOPE_PIX( TickDynamicDecals );
		for ( Int32 i = m_dynamicDecals.Size() - 1; i >= 0; --i )
		{
			CRenderDynamicDecal* decal = m_dynamicDecals[i];

			// If TTL is expired, remove from our list.
			if ( !decal->Tick( this, timeDelta ) )
			{
				// Destroy the decal and remove it from the list.
				RemoveDynamicDecal( decal );
			}
		}
	}

	// Tick skybox
	if ( m_skybox )
	{
		PC_SCOPE_PIX( TickSkybox );
		m_skybox->Tick( timeDelta );
	}
}

void CRenderSceneEx::UpdateActiveFlares( const CRenderCamera &camera, const CFrustum &frustum, Float timeDelta )
{
	for ( Uint32 flare_i=0; flare_i<m_activeFlares.Size(); ++flare_i )	
	{
		const Uint32 prevFlaresCount = m_activeFlares.Size();

		CRenderProxy_Flare *flare = m_activeFlares[flare_i];
		ASSERT( -1 != flare->GetActiveFlareIndex() );
		flare->Update( this, camera, frustum, timeDelta );

		if ( -1 == flare->GetActiveFlareIndex() )
		{
			RED_UNUSED( prevFlaresCount );
			ASSERT( m_activeFlares.Size() + 1 == prevFlaresCount );
			--flare_i;
		}
	}

#ifndef RED_FINAL_BUILD
	extern SceneRenderingStats GRenderingStats;
	GRenderingStats.m_numActiveFlares += m_activeFlares.Size();
#endif
}

void CRenderSceneEx::FinishDissolves()
{
	m_dissolveSynchronizer.Finish();
	m_fadeFinishRequested = true;
}

const Bool CRenderSceneEx::LatchFadeFinishRequested()
{
	const Bool ret = m_fadeFinishRequested;
	m_fadeFinishRequested = false;
	return ret;
}

void CRenderSceneEx::SetForcedLOD( Int32 forcedLODLevel )
{
	m_forcedLOD = forcedLODLevel;
}

void CRenderSceneEx::UpdateSceneStats( const SceneRenderingStats& stats )
{
	m_renderingStats = stats;
}

//dex++
void CRenderSceneEx::PrepareSpeedTreeCascadeShadows( const CRenderCollector& collector )
{
#ifdef USE_SPEED_TREE
	if ( m_speedTree )
	{
		m_speedTree->PrepareCascadeShadows( collector );
	}
#endif
}
//dex--

//dex++
void CRenderSceneEx::RenderSpeedTreeCascadeShadows( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, const SShadowCascade* cascade )
{
#ifdef USE_SPEED_TREE
	if ( m_speedTree )
	{
		m_speedTree->RenderCascadeShadows( context, frameInfo, cascade );
	}
#endif
}
//dex--

void CRenderSceneEx::BuildTerrainQuadTree( const CRenderSceneEx* scene, const CRenderFrameInfo& frameInfo )
{
	if ( m_terrain)
	{
		m_terrain->StartBuildingQuadTree( scene, frameInfo );
	}
}

void CRenderSceneEx::RenderTerrain( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, MeshDrawingStats& stats )
{
	PC_SCOPE_PIX(RenderClipmap);

	if ( m_terrain )
	{
		m_terrain->Render( context, frameInfo, this, stats );
	}
}

Bool CRenderSceneEx::ShouldRenderWater( const CRenderFrameInfo &frameInfo ) const
{
	return 
		nullptr != m_water && 
		frameInfo.m_envParametersGame.m_displaySettings.m_allowWaterShader;
}

Bool CRenderSceneEx::ShouldRenderUnderwater( const CRenderFrameInfo& frameInfo ) const
{
	return ShouldRenderWater( frameInfo ) && m_water->ShouldRenderUnderwater();
}

void CRenderSceneEx::SimulateWater(const CRenderFrameInfo& frameInfo)
{
	GpuApi::TextureRef heightMap;
	if( m_terrain && m_terrain->GetClipMapData().WasInit() ) 
	{
		heightMap = m_terrain->GetHeightMapArray();
	}
	m_water->Simulate( heightMap, frameInfo );
}

void CRenderSceneEx::RenderWater( const class RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	// draw global ocean waves based on actual terrain height [temporary?]
	if( m_terrain && m_terrain->GetClipMapData().WasInit() ) 
	{			
		m_water->Render( this, context, frameInfo, m_terrain->GetHeightMapArray(), m_terrain->GetClipMapData() );
	}
	else
	{
		m_water->Render( this, context, frameInfo );
	}
}

void CRenderSceneEx::RenderStripes( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderCollector* renderCollector, Bool projected )
{
	for ( Uint32 i = 0; i < m_stripeProxies.Size(); ++i )
	{
		// TODO: keep separate lists when NO_EDITOR is specified
		if ( m_stripeProxies[ i ]->IsProjected() == projected )
		{
			m_stripeProxies[ i ]->Render( context, frameInfo, renderCollector );
		}
	}
}

#ifdef USE_NVIDIA_FUR
void CRenderSceneEx::UpdateFurSkinning()
{
	if ( m_furProxies.Size() <= 0 )
		return;

	for ( Uint32 i = 0; i < m_furProxies.Size(); ++i )
	{
		m_furProxies[ i ]->UpdateFurSkinning();
	}
}

void CRenderSceneEx::RenderFur( const CRenderCollector &collector, const class RenderingContext& context )
{
	if ( m_furProxies.Size() <= 0 )
		return;

	for ( Uint32 i = 0; i < m_furProxies.Size(); ++i )
	{
		m_furProxies[ i ]->Render( collector, context );
	}
}
#endif

#ifdef USE_UMBRA
void CRenderSceneEx::UploadOcclusionData( CRenderOcclusionData* occlusionData )
{
	CTimeCounter timeCounter;
	if ( m_currentFramePrefetch )
	{
		while ( !m_currentFramePrefetch->IsFinished() )
		{
			Red::Threads::SleepOnCurrentThread( 3 );
		}
		LOG_RENDERER( TXT("UploadTomeCollection, waiting for prefetch took %1.4f ms"), (Float)timeCounter.GetTimePeriodMS() );
	}
	Float queryThreshold = -1.0f;
	Bool cutsceneMode = false;
	if ( m_occlusionData )
	{
		queryThreshold = m_occlusionData->GetQueryThreshold();
		cutsceneMode = m_occlusionData->IsInCutsceneMode();
	}
	SAFE_COPY( m_occlusionData, occlusionData );
	if ( m_occlusionData )
	{
		m_occlusionData->SetQueryThreshold( queryThreshold );
		m_occlusionData->SetCutsceneModeForGates( cutsceneMode );
	}
	RED_LOG( UmbraInfo, TXT("Uploading TomeCollection took: %1.4f ms"), (Float)timeCounter.GetTimePeriodMS() );
}

void CRenderSceneEx::UpdateQueryThresholdParameter( Float val )
{
	if ( m_occlusionData )
	{
		m_occlusionData->SetQueryThreshold( val );
	}
}

void CRenderSceneEx::PerformOcclusionQuery( CRenderFrame* frame )
{
	PC_SCOPE_PIX( PerformOcclusionQuery );

	RED_ASSERT( ShouldCollectWithUmbra() );
	m_occlusionData->PerformOcclusionQuery( this, frame );
}

void CRenderSceneEx::PerformVisibilityQueriesBatch( const UmbraQueryBatch& queries )
{
	Bool performQuery = ShouldCollectWithUmbra() && m_occlusionData->HasOcclusionDataReady();

	if ( !performQuery )
	{
		UmbraQueryBatch::Iterator it ( queries );
		while ( it )
		{
			it->SetResult( true );
			++it;
		}
	}
	else
	{
		UmbraQueryBatch::Iterator it ( queries );
		while ( it )
		{
			it->SetResult( !m_occlusionData->IsDynamicObjectVisible( it->GetQueryBox() ) );
			++it;
		}
	}
}

void CRenderSceneEx::SetDoorState( TObjectIdType objectId, Bool opened )
{
	if ( m_occlusionData )
	{
		m_occlusionData->SetGateState( objectId, opened );
	}
}

void CRenderSceneEx::SetCutsceneModeForGates( Bool isCutscene )
{
	if ( m_occlusionData )
	{
		m_occlusionData->SetCutsceneModeForGates( isCutscene );
	}
}

#ifndef NO_EDITOR
void CRenderSceneEx::DumpVisibleMeshes( const TLoadedComponentsMap& componentsMap, const String& path )
{
	if ( m_renderElementMap )
	{
		m_renderElementMap->DumpVisibleMeshes( componentsMap, path );
	}
}
#endif // NO_EDITOR
#endif // USE_UMBRA

TRenderVisibilityQueryID CRenderSceneEx::CreateVisibilityQuery()
{
	TRenderVisibilityQueryID id = 0;

	if ( m_visibilityQueryManager != nullptr )
	{
		id = m_visibilityQueryManager->AllocQuerryId();
		if ( !id )
		{
			WARN_RENDERER( TXT("Out of visibility queried - that means that there's a 1M entities in the world or we are leaking the query resources. Debug this ASAP.") );
		}
	}

	return id;
}

void CRenderSceneEx::ReleaseVisibilityQuery( const TRenderVisibilityQueryID id )
{
	if ( m_visibilityQueryManager != nullptr )
	{
		m_visibilityQueryManager->ReleaseQueryId( id );
	}
}

enum ERenderVisibilityResult CRenderSceneEx::GetVisibilityQueryResult( const TRenderVisibilityQueryID queryId ) const
{
	if ( m_visibilityQueryManager != nullptr && queryId )
	{
		return m_visibilityQueryManager->TestQuery( queryId );
	}

	return RVR_NotTested;
}

Bool CRenderSceneEx::GetVisibilityQueryResult( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const
{
	if ( m_visibilityQueryManager != nullptr && elementsCount && indexes )
	{
		return m_visibilityQueryManager->TestQuery( elementsCount, indexes, inputPos, outputPos, stride );
	}

	return false;
}

Bool CRenderSceneEx::EnsureCameraInteriorHelpersCreated()
{
	while ( m_cameraInteriorStagingTextures.Size() < GpuApi::GetNumberOfDelayedFrames() )
	{
		GpuApi::TextureDesc desc;
		{
			desc.type		= GpuApi::TEXTYPE_2D;
			desc.initLevels	= 1;
			desc.usage		= GpuApi::TEXUSAGE_Staging;
			desc.format		= GpuApi::TEXFMT_R8G8B8A8;
			desc.width		= 1;
			desc.height		= 1;
		}

		GpuApi::TextureRef tex = GpuApi::CreateTexture( desc, GpuApi::TEXG_System );
		RED_ASSERT( !tex.isNull() );
		if ( !tex )
		{
			return false;
		}
		GpuApi::SetTextureDebugPath( tex, "cameraInteriorHelper" );
		m_cameraInteriorStagingTextures.PushBack( tex );
	}

	RED_ASSERT( m_cameraInteriorStagingTextures.Size() <= 10 && "Sanity check" );
	return !m_cameraInteriorStagingTextures.Empty();
}

void CRenderSceneEx::DestroyCameraInteriorHelpers()
{
	for ( Uint32 i=0; i<m_cameraInteriorStagingTextures.Size(); ++i )
	{
		GpuApi::SafeRelease( m_cameraInteriorStagingTextures[i] );
	}

	m_cameraInteriorStagingTextures.Clear();
	m_cameraInteriorNumValuesCopied = 0;
	m_cameraInteriorFactor = 0;
}

void CRenderSceneEx::UpdateCameraInteriorFactor( GpuApi::TextureRef texValueSource, Bool forceImmediate )
{
	struct Local
	{
		static Float ReadInteriorFactor( GpuApi::TextureRef tex )
		{
			Float result = 0.f;

			Uint32 pitch = 0;
			Uint8 *data = (Uint8*)GpuApi::LockLevel( tex, 0, 0, GpuApi::BLF_Read, pitch );
			RED_ASSERT( data );
			if ( data )
			{
				result = data[0] > 127 ? 0.f : 1.f;
				GpuApi::UnlockLevel( tex, 0, 0 );
			}

			return result;
		}
	};

	if ( !texValueSource || !EnsureCameraInteriorHelpersCreated() )
	{
		return;
	}

	if ( forceImmediate )
	{
		GetRenderer()->CopyTextureData( m_cameraInteriorStagingTextures.Front(), 0, 0, texValueSource, 0, 0 );
		m_cameraInteriorFactor = Local::ReadInteriorFactor( m_cameraInteriorStagingTextures.Front() );
		m_cameraInteriorNumValuesCopied = 1;
	}
	else
	{
		if ( m_cameraInteriorNumValuesCopied > 0 )
		{
			m_cameraInteriorFactor = Local::ReadInteriorFactor( m_cameraInteriorStagingTextures[m_cameraInteriorNumValuesCopied - 1] );
		}

		m_cameraInteriorStagingTextures.Insert( 0, m_cameraInteriorStagingTextures.PopBack() );
		m_cameraInteriorNumValuesCopied = Min( m_cameraInteriorStagingTextures.Size(), m_cameraInteriorNumValuesCopied + 1 );
		GetRenderer()->CopyTextureData( m_cameraInteriorStagingTextures.Front(), 0, 0, texValueSource, 0, 0 );
	}
}

Float CRenderSceneEx::GetDelayedCameraInteriorFactor() const
{
	return m_cameraInteriorFactor;
}

#include "build.h"
#include "renderCollector.h"
#include "renderDynamicDecal.h"
#include "renderDynamicDecalChunk.h"
#include "renderElementMap.h"
#include "renderEntityGroup.h"
#include "renderProxyApex.h"
#include "renderProxyParticles.h"
#include "renderProxyDecal.h"
#include "renderProxyDimmer.h"
#include "renderProxyDrawable.h"
#include "renderProxyFur.h"
#include "renderProxyMesh.h"
#include "renderProxy.h"
#include "renderProxyLight.h"
#include "renderProxyMesh.h"
#include "../engine/meshEnum.h"
#include "../engine/umbraIncludes.h"
#include "../core/configVar.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

namespace Config
{
	TConfigVar< Bool > cvDisableManualProxiesCulling( "Rendering", "DisableManualProxiesCulling", false );
}

#ifdef PROFILE_CODE_ENABLED
struct ScopeTimer
{
	Double&			m_value;

	CTimeCounter	m_timer;

	ScopeTimer( Double& value )
		: m_value( value )
	{}

	~ScopeTimer()
	{
		m_value += m_timer.GetTimePeriodMS();
	}
};
#define SCOPE_TIMER( value ) ScopeTimer _timer( value )
#else
#define SCOPE_TIMER( value )
#endif

//////////////////////////////////////////////////////////////////////////

class Collector_RenderCollector
{
private:
	CRenderCollector&				m_collector;
	const TVisibleChunksIndices&	m_visibleStatics;
	Int32							m_visibleStaticsCount;

	Vector							m_cameraPosition;
	Float							m_cameraFOVMultiplier;
	Float							m_dataDistanceThresholdSquared;

	CFrustum						m_frustum;
	
public:

#ifdef PROFILE_CODE_ENABLED
	MeshDrawingStats&		m_sceneStats;
	Uint32&					m_collectedMeshes;
#endif

	Collector_RenderCollector( CRenderCollector& collector, const TVisibleChunksIndices& visibleStatics, Int32 numVisibleStatics = 0 )
		: m_collector( collector )
		, m_visibleStatics( visibleStatics )
		, m_visibleStaticsCount( numVisibleStatics )
#ifdef PROFILE_CODE_ENABLED
		, m_sceneStats( collector.m_sceneStats )
		, m_collectedMeshes( collector.m_collectedMeshes )
#endif // PROFILE_CODE_ENABLED
		, m_dataDistanceThresholdSquared( 0.0f )
	{
		m_cameraPosition = collector.m_camera->GetPosition();
		m_cameraFOVMultiplier = collector.m_camera->GetFOVMultiplier();
#ifdef USE_UMBRA
		if ( collector.m_occlusionData )
		{
			m_dataDistanceThresholdSquared = collector.m_occlusionData->GetDataDistanceThresholdSquared();
		}
#endif // USE_UMBRA
		m_frustum.InitFromCamera( collector.m_camera->GetWorldToScreen() );
	}

	RED_FORCE_INLINE Bool IsWithinLoadedDataDistance( const Vector& pos )
	{
		const Float distanceSquared = m_cameraPosition.DistanceSquaredTo2D( pos );
		return distanceSquared <= m_dataDistanceThresholdSquared;
	}

	RED_FORCE_INLINE Bool FrustumTest( const Box& bounds )
	{
		return m_frustum.TestBox( bounds ) != FR_Outside;
	}

	RED_FORCE_INLINE void CollectProxy( IRenderProxyBase* proxy )
	{
		m_collector.CollectElement( proxy );
	}

	RED_FORCE_INLINE void ProcessAutoHideProxy( IRenderProxyBase* proxy )
	{
		m_collector.ProcessAutoHideProxy( proxy );
	}

	void PostCollectProxy( IRenderProxyBase* proxy, ERenderProxyType type )
	{
		if( type == RPT_Particles )
		{
			m_collector.PushOffScreenParticle( static_cast< CRenderProxy_Particles* >( proxy ) );
		}
	}

	RED_FORCE_INLINE void CollectDynamicDecalChunk( CRenderDynamicDecalChunk* chunk )
	{
		m_collector.PushDynamicDecalChunk( chunk );
	}

	RED_FORCE_INLINE const Bool CollectAllDynamics() const
	{
		return m_collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraShowOccludedNonStaticGeometry );
	}

	RED_FORCE_INLINE const Bool TestVisibility( const Box& bounds ) const
	{
#ifdef USE_UMBRA
		return m_collector.m_occlusionData->IsDynamicObjectVisible( bounds );
#else
		return TestSimpleVisibility( bounds );
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE const Bool TestVisibility( const Vector3& bbMin, const Vector3& bbMax ) const
	{
#ifdef USE_UMBRA
		return m_collector.m_occlusionData->IsDynamicObjectVisible( bbMin, bbMax );
#else
		return TestSimpleVisibility( bbMin, bbMax );
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE const Bool TestVisibilityDistance( const Vector& pos, const Float hideDistSq ) const
	{
#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			return true;
		}
#endif // USE_ANSEL

		const Float distSq = m_cameraPosition.DistanceSquaredTo( pos );
		const Float distSqAdj = distSq * m_cameraFOVMultiplier;
		return ( distSqAdj <= hideDistSq );
	}

	RED_FORCE_INLINE const Bool TestSimpleVisibility( const Box& bounds ) const
	{
		return m_frustum.TestBox( bounds ) != FR_Outside;
	}

	RED_FORCE_INLINE const Bool TestSimpleVisibility( const Vector3& bbMin, const Vector3& bbMax ) const
	{
		return m_frustum.TestBox( bbMin, bbMax ) != FR_Outside;
	}

	RED_FORCE_INLINE Bool TestSphereVisibility( const Box& bounds ) const
	{
		// TODO: some threshold?
#ifdef USE_UMBRA
		Vector center = bounds.CalcCenter();
		Float diameter = bounds.Min.DistanceTo( bounds.Max );
		return m_collector.m_occlusionData->IsDynamicSphereVisible( center, diameter * 0.5f );
#else
		return m_frustum.TestBox( bounds ) != FR_Outside;
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE Bool TestLineVisibility( const Vector& end, const Vector& start ) const
	{
#ifdef USE_UMBRA
		return m_collector.m_occlusionData->PerformLineQuery( start, end );
#else
		return true;
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE void TestLineVisibility( CharacterOcclusionInfo* characterInfos, Int32 numberOfCharacterInfos ) const
	{
#ifdef USE_UMBRA
		m_collector.m_occlusionData->PerformLineQueries( m_cameraPosition, characterInfos, numberOfCharacterInfos );
#else
		for ( Int32 i = 0; i < numberOfCharacterInfos; ++i )
		{
			characterInfos[ i ].m_positionVisible = m_frustum.IsPointInside( characterInfos[ i ].m_position );
		}
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE const TVisibleChunksIndices& GetVisibleStatics() const
	{
		return m_visibleStatics;
	}

	RED_FORCE_INLINE Int32 GetVisibleStaticsCount() const
	{
		return m_visibleStaticsCount;
	}
};

class Collector_ExtraCollection
{
private:
	SExtraSceneCollection&				m_collector;

	const ExtraOcclusionQueryResults*	m_occlusionResults;
	const CRenderFrame*					m_frame;

	CFrustum							m_frustum;

	Vector								m_cameraPosition;
	Float								m_cameraFOVMultiplier;

#ifndef USE_UMBRA
	TVisibleChunksIndices				m_emptyStaticIndices;
#endif // USE_UMBRA

public:
#ifdef PROFILE_CODE_ENABLED
	MeshDrawingStats					m_sceneStats;
	Uint32								m_collectedMeshes;
#endif // PROFILE_CODE_ENABLED


	Collector_ExtraCollection( SExtraSceneCollection& collector, const ExtraOcclusionQueryResults* occlusionResults, CRenderFrame* frame )
		: m_collector( collector )
		, m_occlusionResults( occlusionResults )
		, m_frame( frame )
	{
		const CRenderCamera& camera = frame->GetFrameInfo().m_occlusionCamera;
		m_frustum.InitFromCamera( camera.GetWorldToScreen() );
		m_cameraPosition = camera.GetPosition();
		m_cameraFOVMultiplier = camera.GetFOVMultiplier();
	}

	RED_FORCE_INLINE Bool IsWithinLoadedDataDistance( const Vector& pos )
	{
		return true;
	}

	RED_FORCE_INLINE Bool FrustumTest( const Box& bounds )
	{
		return true;
	}

	RED_FORCE_INLINE void CollectProxy( IRenderProxyBase* proxy )
	{
		m_collector.m_proxies.PushBackUnique( proxy );
	}

	RED_FORCE_INLINE void ProcessAutoHideProxy( IRenderProxyBase* proxy )
	{
		// Do nothing
	}

	void PostCollectProxy( IRenderProxyBase* proxy, ERenderProxyType type )
	{
		// Not needed in the prefetch. They will be gathered during normal render frame anyway.
	}

	RED_FORCE_INLINE void CollectDynamicDecalChunk( CRenderDynamicDecalChunk* chunk )
	{
		m_collector.m_dynamicDecals.PushBackUnique( chunk->GetOwnerDecal() );
	}

	RED_FORCE_INLINE const Bool CollectAllDynamics() const
	{
		// Always test dynamics against occlusion results.
		return false;
	}

	RED_FORCE_INLINE const Bool TestVisibilityDistance( const Vector& pos, const Float hideDistSq ) const
	{
		const Float distSq = m_cameraPosition.DistanceSquaredTo( pos );
		const Float distSqAdj = distSq * m_cameraFOVMultiplier;
		return ( distSqAdj <= hideDistSq );
	}

	RED_FORCE_INLINE const Bool TestVisibility( const Box& bounds ) const
	{
#ifdef USE_UMBRA
		return m_occlusionResults->IsDynamicObjectVisible( bounds );
#else
		return TestSimpleVisibility( bounds );
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE const Bool TestVisibility( const Vector3& bbMin, const Vector3& bbMax ) const
	{
#ifdef USE_UMBRA
		return m_occlusionResults->IsDynamicObjectVisible( bbMin, bbMax );
#else
		return TestSimpleVisibility( bbMin, bbMax );
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE const Bool TestSimpleVisibility( const Box& bounds ) const
	{
		return m_frustum.TestBox( bounds ) != FR_Outside;
	}

	RED_FORCE_INLINE const Bool TestSimpleVisibility( const Vector3& bbMin, const Vector3& bbMax ) const
	{
		return m_frustum.TestBox( bbMin, bbMax ) != FR_Outside;
	}

	RED_FORCE_INLINE Bool TestSphereVisibility( const Box& bounds ) const
	{
		return true;
	}

	RED_FORCE_INLINE Bool TestLineVisibility( const Vector& end, const Vector& start ) const
	{
		return true;
	}

	RED_FORCE_INLINE void TestLineVisibility( CharacterOcclusionInfo* characterInfos, Int32 numberOfCharacterInfos ) const
	{
		for ( Int32 i = 0; i < numberOfCharacterInfos; ++i )
		{
			characterInfos[i].m_positionVisible = true;
		}
	}

	RED_FORCE_INLINE const TVisibleChunksIndices& GetVisibleStatics() const
	{
#ifdef USE_UMBRA
		return m_occlusionResults->GetVisibleStatics();
#else
		return m_emptyStaticIndices;
#endif // USE_UMBRA
	}

	RED_FORCE_INLINE Int32 GetVisibleStaticsCount() const
	{
#ifdef USE_UMBRA
		return m_occlusionResults->GetVisibleStatics().SizeInt();
#else
		return 0;
#endif // USE_UMBRA
	}
};

//////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA

template< typename TCollector >
static void CollectDynamicProxyList( TCollector& collector, const CRenderElementMap::SRenderElementMapEntry* entries, Uint32 lastEntryId )
{
	RED_FATAL_ASSERT( lastEntryId < MAX_REGISTERED_PROXIES, "Too many registered proxies" );
	PC_SCOPE_PIX( CollectDynamicProxyList );

	// Fill a single array from both ends with proxies that are either to be rendered, or are past their autohide distance.
	IRenderProxyBase* ptrsToCollect[ MAX_COLLECTED_PROXIES ];
	Uint32 numVisibleProxies = 0;
	Uint32 numAutoHideProxies = 0;

	{
		PC_SCOPE_PIX( CollectDynamicProxyList_VisibilityTests );
		for ( Uint32 i = 1; i <= lastEntryId; ++i )
		{
			const CRenderElementMap::SRenderElementMapEntry& entry = entries[ i ];
			if ( !entry.HasFlag( REMEF_Valid ) )
			{
				// do not collect invalid ones
				continue;
			}
			if ( entry.HasFlag( REMEF_CharacterProxy ) )
			{
				// do not collect character proxies - they will be collected in the character pass
				continue;
			}
			if ( entry.HasFlag( REMEF_VolumeMesh ) )
			{
				// do not collect volume meshes (regular or deferredCollection). These are handled either as statics, or
				// as non-static volumes. In both cases they need to be included in the "static" collection group.
				continue;
			}
			if ( entry.HasFlag( REMEF_Dynamic ) || entry.HasFlag( REMEF_Pass_DeferredCollection ) )
			{
				RED_FATAL_ASSERT( entry.m_type != RPT_Mesh || !((CRenderProxy_Mesh*)entry.m_proxy)->CheckMeshProxyFlag(RMPF_IsVolumeMesh), "Collecting volume mesh as dynamic" );

				Bool visible = false;
				Bool withinAutoHideDistance = true;

				// Only test distance visibility if the proxy has detected that it's past the autohide distance, and it is
				// not fading out. This way it can get collected once and has a chance to start fading out if needed. 
				if ( entry.HasFlag( REMEF_PastAutoHide ) && !entry.HasFlag( REMEF_FadingOut ) )
				{
					SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeVisibilityByDistance );
					// If proxy is fading out, we skip the distance test. We want to continue drawing it even if it's beyond the
					// auto-hide distance, so that it can fade out properly.
					withinAutoHideDistance = collector.TestVisibilityDistance( entry.m_refPos, entry.m_autoHideDistanceSquared );
				}

				if ( withinAutoHideDistance )
				{
					SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeDynamicObjects );

					if ( entry.HasFlag( REMEF_SkipOcclusionTest ) )
					{
						// frustum check
						visible = collector.FrustumTest( Box( entry.m_boundingBoxMin, entry.m_boundingBoxMax ) );
					}
					else if ( !collector.IsWithinLoadedDataDistance( entry.m_refPos ) )
					{
						// PROXY MESHES - frustum + occlusion check
						visible = collector.TestVisibility( entry.m_boundingBoxMin, entry.m_boundingBoxMax );
					}
					else
					{
						// frustum + occlusion check
						visible = collector.TestVisibility( entry.m_boundingBoxMin, entry.m_boundingBoxMax );
					}
				}

				if ( visible )
				{
					ptrsToCollect[ numVisibleProxies ] = entry.m_proxy;
					++numVisibleProxies;
				}
				else if ( withinAutoHideDistance )
				{
					// post-collect offscreen particles
					collector.PostCollectProxy( entry.m_proxy, entry.m_type );
#ifdef PROFILE_CODE_ENABLED
					++collector.m_sceneStats.m_occludedDynamicProxies;
#endif // PROFILE_CODE_ENABLED

				}
				else
				{
					ptrsToCollect[ MAX_COLLECTED_PROXIES - numAutoHideProxies - 1 ] = entry.m_proxy;
					++numAutoHideProxies;

#ifdef PROFILE_CODE_ENABLED
					++collector.m_sceneStats.m_occludedDynamicProxies;
#endif // PROFILE_CODE_ENABLED
				}
			}
		}
	}

	RED_FATAL_ASSERT( numVisibleProxies + numAutoHideProxies < MAX_COLLECTED_PROXIES, "Collected too many proxies somehow! visible: %u, autohide: %u, max: %u", numVisibleProxies, numAutoHideProxies, MAX_COLLECTED_PROXIES );
	
	{
		PC_SCOPE_PIX( CollectDynamicProxyList_Collection );
		for ( Uint32 i = 0; i < numVisibleProxies; ++i )
		{
			collector.CollectProxy( ptrsToCollect[i] );
		}
	}

	{
		PC_SCOPE_PIX( CollectDynamicProxyList_AutoHideCollection );
		const Uint32 autoHideProxyStart = MAX_COLLECTED_PROXIES - numAutoHideProxies;
		for ( Uint32 i = 0; i < numAutoHideProxies; ++i )
		{
			collector.ProcessAutoHideProxy( ptrsToCollect[ autoHideProxyStart + i ] );
		}
	}
}

#endif // USE_UMBRA

#define MAX_CHARACTERS 300

template< typename TCollector >
static void CollectCharactersProxyList( TCollector& collector, const TDynArray< CRenderEntityGroup* >& characters )
{
	PC_SCOPE_PIX( CollectCharactersProxyList );
	if ( characters.Empty() )
	{
		// early exit, nothing to test
		return;
	}

	// we are performing a visibility test per-character (entity group) instead of performing it per mesh/apex/fur
	// first, a cheap line test is done, bounding boxes are tested only when line test fails
	const Int32 numberOfCharacters = characters.SizeInt();
	if ( numberOfCharacters > MAX_CHARACTERS )
	{
		WARN_RENDERER( TXT( "Extremely high numbers of CRenderEntityGroup objects for character occlusion [numberOfCharacters=%d, MAX=%d]" ), numberOfCharacters, MAX_CHARACTERS );
	}
	CharacterOcclusionInfo* characterInfos = new CharacterOcclusionInfo[ numberOfCharacters ];
	{
		PC_SCOPE_PIX( CollectCharactersProxyList_EntityGroupBoundingBoxUpdate );
		for ( Int32 i = 0; i < numberOfCharacters; ++i )
		{
			RED_ASSERT( characters[ i ] );
			characterInfos[ i ].m_boundingBox = characters[ i ]->CalculateBoundingBox();
			characterInfos[ i ].m_position = characterInfos[i].m_boundingBox.CalcCenter();
			characterInfos[ i ].m_positionVisible = false;
		}
	}
	{
		PC_SCOPE_PIX( CollectCharactersProxyList_LineVisibilityTest );
		collector.TestLineVisibility( characterInfos, numberOfCharacters );
	}

	{
		PC_SCOPE_PIX( CollectCharactersProxyList_Collection );
		for ( Int32 i = 0; i < numberOfCharacters; ++i )
		{
			Bool visible = characterInfos[ i ].m_positionVisible;
			if ( !visible )
			{
				SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeDynamicObjects );
				visible = collector.TestVisibility( characterInfos[ i ].m_boundingBox );
			}
			if ( visible )
			{
				CRenderEntityGroup* entityGroup = characters[i];
				for ( Uint32 category_i=0; category_i<CRenderEntityGroup::PROXYCAT_MAX; ++category_i )
				{
					for ( auto proxy : entityGroup->GetProxies( (CRenderEntityGroup::eProxyCategory)category_i ) )
					{
						collector.CollectProxy( proxy->AsBaseProxy() );
					}
				}
			}
		}
	}

	delete [] characterInfos;
	characterInfos = nullptr;
}

#ifdef USE_UMBRA

template< typename TCollector >
static void CollectVisibleStaticsList( TCollector& collector, const CRenderElementMap::SRenderElementMapEntry* entries, const TDynArray< Uint32 >& entryMap, const TVisibleChunksIndices& visibleIndices, Int32 count )
{
	PC_SCOPE_PIX( CollectVisibleStaticsList );
	RED_ASSERT( count <= visibleIndices.SizeInt() );

	for ( Int32 visibleIndexIter = 0; visibleIndexIter < count; ++visibleIndexIter )
	{
		Int32 objectIndex = visibleIndices[ visibleIndexIter ];
		if ( objectIndex < 0 || objectIndex >= entryMap.SizeInt() )
		{
			// TODO: figure out why it happens, this is just for crash fix
			continue;
		}
		Uint32 entryID = entryMap[ objectIndex ];
		const CRenderElementMap::SRenderElementMapEntry& entry = entries[ entryID ];
		if ( !entry.HasFlag( REMEF_Valid ) )
		{
			continue;
		}
		if ( !entry.HasFlag( REMEF_Static ) )
		{
			continue;
		}

		RED_FATAL_ASSERT( !entry.HasFlag( REMEF_Dynamic ), "Dynamic Entry collected as static" );
		RED_FATAL_ASSERT( !entry.HasFlag( REMEF_NonStaticVolumeMesh ), "Non-Static Volume Entry collected as static" );
		RED_FATAL_ASSERT( !entry.HasFlag( REMEF_Pass_DeferredCollection ), "Deferred Entry collected as static" );
		RED_FATAL_ASSERT( entry.m_proxy, "Null proxy in valid RenderElementMap entry" );
		collector.CollectProxy( entry.m_proxy );
	}
}

template< typename TCollector >
static void CollectVisibleNonStaticVolumesList( TCollector& collector, const CRenderElementMap::SRenderElementMapEntry* entries, const THashSet< Uint32 >& volumeIndices )
{
	PC_SCOPE_PIX( CollectVisibleNonStaticVolumesList );

	for ( Uint32 entryID : volumeIndices )
	{
		const CRenderElementMap::SRenderElementMapEntry& entry = entries[ entryID ];
		if ( !entry.HasFlag( REMEF_Valid ) )
		{
			continue;
		}

		RED_FATAL_ASSERT( entry.HasFlag( REMEF_NonStaticVolumeMesh ), "Entry in non-static volume isn't a non-static volume mesh" );
		RED_FATAL_ASSERT( !entry.HasFlag( REMEF_Dynamic ), "Dynamic Entry collected as non-static volume" );
		RED_FATAL_ASSERT( !entry.HasFlag( REMEF_Static ), "Static Volume Entry collected as non-static volume" );
		RED_FATAL_ASSERT( entry.m_proxy, "Null proxy in valid RenderElementMap entry" );

		if ( collector.TestVisibility( entry.m_boundingBoxMin, entry.m_boundingBoxMax ) )
		{
			collector.CollectProxy( entry.m_proxy );
		}
	}
}


template< typename TCollector >
static void CollectDynamicDecalList( TCollector& collector, const TDynArray< CRenderDynamicDecalChunk* >& dynamicDecalChunks )
{
	PC_SCOPE_PIX( CollectDynamicDecalList );

	// NOTE: dynamic decals that are getting close to it's hide distance should be faded out ASAP
	// There's no much point in testing it by distance
	Bool decalVisible = false;
	for ( Uint32 i = 0; i < dynamicDecalChunks.Size(); ++i )
	{
		CRenderDynamicDecalChunk* decalChunk = dynamicDecalChunks[i];
		const Box& bbox = decalChunk->GetBoundingBox();
		Bool visible = true;
		if ( visible )
		{
			SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeDynamicObjects );
			visible = collector.TestVisibility( bbox );
		}

#ifdef PROFILE_CODE_ENABLED
		if ( visible )
		{
			++collector.m_sceneStats.m_renderedDynamicDecals;
			decalVisible = true;
		}
		else
		{
			++collector.m_sceneStats.m_occludedDynamicDecals;
		}
#endif // PROFILE_CODE_ENABLED

		if ( visible )
		{
			collector.CollectDynamicDecalChunk( decalChunk );
		}
	}

#ifdef PROFILE_CODE_ENABLED
	if ( decalVisible )
	{
		++collector.m_sceneStats.m_renderedDynamicDecalsCount;
	}
#endif
}

#endif // USE_UMBRA

#ifdef USE_UMBRA
template< typename TCollector >
void CRenderElementMap::CollectProxiesGeneric( TCollector& collector, Uint8 flags )
{
#ifdef PROFILE_CODE_ENABLED
	collector.m_collectedMeshes = 0;
#endif // PROFILE_CODE_ENABLED

	if ( flags & COLLECT_Static )
	{
		CollectVisibleStaticsList( collector, m_entries, m_statics, collector.GetVisibleStatics(), collector.GetVisibleStaticsCount() );
		CollectVisibleNonStaticVolumesList( collector, m_entries, m_nonStaticVolumes );

#ifdef PROFILE_CODE_ENABLED
		collector.m_sceneStats.m_renderedStaticProxies = collector.m_collectedMeshes;
		collector.m_collectedMeshes = 0;
#endif // PROFILE_CODE_ENABLED
	}

	if ( flags & COLLECT_NonStatic )
	{
		// Collect Dynamic Geometry and DeferredDestroy - These are meshes that are being removed with FT_FadeOutAndDestroy.
		// They've been removed from the normal lists and can be destroyed soon, but we want to keep rendering as they fade out.
		CollectDynamicProxyList( collector, m_entries, m_lastEntryId );
		CollectCharactersProxyList( collector, m_characterEntities );

#ifdef PROFILE_CODE_ENABLED
		collector.m_sceneStats.m_renderedDynamicProxies = collector.m_collectedMeshes;
		collector.m_collectedMeshes = 0;
#endif // PROFILE_CODE_ENABLED

		CollectDynamicDecalList( collector, m_dynamicDecalChunks );
	}
}
#endif // USE_UMBRA

template< typename TCollector >
void CRenderElementMap::CollectProxiesGenericFrustum( TCollector& collector, Uint8 flags )
{
	// Fill a single array from both ends with proxies that are either to be rendered, or are past their autohide distance.
	IRenderProxyBase* ptrsToCollect[ MAX_COLLECTED_PROXIES ];
	Uint32 numVisibleProxies = 0;
	Uint32 numAutoHideProxies = 0;

	{
		PC_SCOPE_PIX( CollectProxiesGenericFrustum_VisibilityTests );
		for ( Uint32 i = 1; i <= m_lastEntryId; ++i )
		{
			const CRenderElementMap::SRenderElementMapEntry& entry = m_entries[ i ];
			if ( !entry.HasFlag( REMEF_Valid ) )
			{
				// do not collect invalid ones
				continue;
			}

			// mask out with flags
			if ( ( entry.HasFlag( REMEF_Static ) || entry.HasFlag( REMEF_NonStaticVolumeMesh ) ) && ( ( flags & COLLECT_Static ) == 0 ) )
			{
				continue;
			}
			if ( ( entry.HasFlag( REMEF_Dynamic ) || entry.HasFlag( REMEF_CharacterProxy ) ) && ( ( flags & COLLECT_NonStatic ) == 0 ) )
			{
				continue;
			}

			Bool visible = false;
			Bool withinAutoHideDistance = true;

			// Only test distance visibility if the proxy has detected that it's past the autohide distance, and it is
			// not fading out. This way it can get collected once and has a chance to start fading out if needed. 
#ifdef USE_ANSEL
			if ( !isAnselSessionActive )
#endif // USE_ANSEL
			{
				if ( entry.HasFlag( REMEF_PastAutoHide ) && !entry.HasFlag( REMEF_FadingOut ) )
				{
					SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeVisibilityByDistance );
					// If proxy is fading out, we skip the distance test. We want to continue drawing it even if it's beyond the
					// auto-hide distance, so that it can fade out properly.
					withinAutoHideDistance = collector.TestVisibilityDistance( entry.m_refPos, entry.m_autoHideDistanceSquared );
				}
			}

			if ( withinAutoHideDistance )
			{
				SCOPE_TIMER( collector.m_sceneStats.m_occlusionTimeDynamicObjects );
				visible = collector.FrustumTest( Box( entry.m_boundingBoxMin, entry.m_boundingBoxMax ) );
			}

			if ( visible )
			{
				ptrsToCollect[ numVisibleProxies ] = entry.m_proxy;
				++numVisibleProxies;
			}
			else if ( withinAutoHideDistance )
			{
				// post-collect offscreen particles
				collector.PostCollectProxy( entry.m_proxy, entry.m_type );
			}
			else
			{
				ptrsToCollect[ MAX_COLLECTED_PROXIES - numAutoHideProxies - 1 ] = entry.m_proxy;
				++numAutoHideProxies;
			}
		}
	}

	RED_FATAL_ASSERT( numVisibleProxies + numAutoHideProxies < MAX_COLLECTED_PROXIES, "Collected too many proxies somehow! visible: %u, autohide: %u, max: %u", numVisibleProxies, numAutoHideProxies, MAX_COLLECTED_PROXIES );

	{
		PC_SCOPE_PIX( CollectProxiesGenericFrustum_Collection );
		for ( Uint32 i = 0; i < numVisibleProxies; ++i )
		{
			collector.CollectProxy( ptrsToCollect[i] );
		}
	}

	{
		PC_SCOPE_PIX( CollectProxiesGenericFrustum_AutoHideCollection );
		const Uint32 autoHideProxyStart = MAX_COLLECTED_PROXIES - numAutoHideProxies;
		for ( Uint32 i = 0; i < numAutoHideProxies; ++i )
		{
			collector.ProcessAutoHideProxy( ptrsToCollect[ autoHideProxyStart + i ] );
		}
	}

	if ( flags & COLLECT_NonStatic )
	{
		PC_SCOPE_PIX( CollectProxiesGenericFrustum_DynamicDecals );
		for ( CRenderDynamicDecalChunk* decalChunk : m_dynamicDecalChunks )
		{
			if ( collector.FrustumTest( decalChunk->GetBoundingBox() ) )
			{
				collector.CollectDynamicDecalChunk( decalChunk );
			}
		}
	}
}

void CRenderElementMap::CollectProxiesNoModify( const ExtraOcclusionQueryResults* occlusionResults, CRenderFrame* frame, SExtraSceneCollection& collector )
{
#ifdef USE_UMBRA
	RED_FATAL_ASSERT( occlusionResults != nullptr, "" );
	Collector_ExtraCollection collectorInterface( collector, occlusionResults, frame );
	CollectProxiesGeneric( collectorInterface, COLLECT_Static | COLLECT_NonStatic );
#else
	Collector_ExtraCollection collectorInterface( collector, occlusionResults, frame );
	CollectProxiesGenericFrustum( collectorInterface, COLLECT_Static | COLLECT_NonStatic );
#endif // USE_UMBRA
}

void CRenderElementMap::CollectStaticProxies( CRenderCollector& collector )
{
#ifdef USE_UMBRA
	if ( collector.m_collectWithUmbra )
	{
		Collector_RenderCollector collectorInterface( collector, m_visibleObjectsIndices, m_visibleObjectsCount );
		CollectProxiesGeneric( collectorInterface, COLLECT_Static );
	}
	else
#endif // USE_UMBRA
	{
		Collector_RenderCollector collectorInterface( collector, TVisibleChunksIndices() );
		CollectProxiesGenericFrustum( collectorInterface, COLLECT_Static );
	}
}

void CRenderElementMap::CollectNonStaticProxies( CRenderCollector& collector )
{
#ifdef USE_UMBRA
	if ( collector.m_collectWithUmbra )
	{
		Collector_RenderCollector collectorInterface( collector, m_visibleObjectsIndices, m_visibleObjectsCount );
		CollectProxiesGeneric( collectorInterface, COLLECT_NonStatic );
	}
	else
#endif // USE_UMBRA
	{
		Collector_RenderCollector collectorInterface( collector, TVisibleChunksIndices() );
		CollectProxiesGenericFrustum( collectorInterface, COLLECT_NonStatic );
	}
}

struct CullResult
{
	IRenderProxyBase*	m_ptr;
	ERenderProxyType	m_type;
	Uint32				m_cascades;
};

struct QueryResult
{
	CullResult*	m_results;
	Uint32		m_numResults;
	Uint32		m_maxResults;
};

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

template< Int32 NUM_QUERIES >
static void DoCulling( const CFrustum* queries, CRenderElementMap::SRenderElementMapEntry* entries, const Uint32 numEntries, QueryResult& outResults )
{
	// setup write pointers
	CullResult* writePtr = outResults.m_results;

	static const Uint8 testmasks[] = { MCR_Cascade1, MCR_Cascade2, MCR_Cascade3, MCR_Cascade4 };

	// process culling lists
	CRenderElementMap::SRenderElementMapEntry* const end = entries + numEntries;
	for ( CRenderElementMap::SRenderElementMapEntry* cur = entries; cur != end; ++cur )
	{
		if ( cur->m_type != RPT_Apex && cur->m_type != RPT_Mesh && cur->m_type != RPT_Fur )
		{
			continue;
		}
		if ( !cur->HasFlag(REMEF_Valid) )
		{
			continue;
		}
#ifdef USE_ANSEL
		if ( isAnselSessionActive && cur->m_skipCollection )
		{
			// HACKY HACKY HACK FOR Blood and Wine castle shadowmesh proxy
			continue;
		}
#endif // USE_ANSEL
		Uint32 testResults = 0;
		for ( Int32 q = NUM_QUERIES - 1; q >= 0; --q )
		{
			testResults <<= 2;
			if ( ( cur->m_renderMask & testmasks[ q ] ) == 0 )
			{
				continue;
			}
			const __m128 bbMin = _mm_set_ps( 1.f, cur->m_boundingBoxMin.Z, cur->m_boundingBoxMin.Y, cur->m_boundingBoxMin.X );
			const __m128 bbMax = _mm_set_ps( 1.f, cur->m_boundingBoxMax.Z, cur->m_boundingBoxMax.Y, cur->m_boundingBoxMax.X );
			testResults |= queries[ q ].TestBox( bbMin, bbMax );
		}

		const Uint32 mergedMask = testResults | cur->m_testLastFrame;

		cur->m_testLastFrame = testResults;

		if ( mergedMask == 0 ) 
		{
			continue;
		}

		writePtr->m_ptr			= cur->m_proxy;
		writePtr->m_type		= cur->m_type;
		writePtr->m_cascades	= mergedMask;
		writePtr += 1;
		
	}

	// compute counts
	outResults.m_numResults = Uint32( writePtr - outResults.m_results );
}

template < Uint32 NUM_ENTRIES >
void Cull( const CFrustum* queries, Uint16 numQueries, CRenderElementMap::SRenderElementMapEntry* entries, Uint32 lastEntryId, QueryResult& outResults )
{
	switch ( numQueries )
	{
	case 1: DoCulling<1>( queries, entries + 1, lastEntryId, outResults ); break;
	case 2: DoCulling<2>( queries, entries + 1, lastEntryId, outResults ); break;
	case 3: DoCulling<3>( queries, entries + 1, lastEntryId, outResults ); break;
	case 4: DoCulling<4>( queries, entries + 1, lastEntryId, outResults ); break;
	}
}

#define NUM_TEST_OBJECTS 32000
#define MAX_CASCADES 4

//////////////////////////////////////////////////////////////////////////

void CRenderElementMap::CollectShadowProxies( SMergedShadowCascades& cascades )
{
	PC_SCOPE_PIX( CollectShadowProxies_Umbra );

	// compute test frustum
	static CFrustum queries[ MAX_CASCADES ];
	for ( Uint32 i = 0; i < cascades.m_numCascades; ++i )
	{
		queries[ i ].InitFromCamera( cascades.m_cascades[ i ].m_jitterCamera.GetWorldToScreen() );
	}

	QueryResult results;
	static CullResult resultBuffer[ NUM_TEST_OBJECTS ];
	results.m_results = &resultBuffer[ 0 ];
	results.m_maxResults = NUM_TEST_OBJECTS;
	results.m_numResults = 0;
	
	{
		PC_SCOPE_PIX( CollectShadowProxies_Culling );
		Cull< MAX_REGISTERED_PROXIES >( queries, cascades.m_numCascades, m_entries, m_lastEntryId, results );
	}

	{
		PC_SCOPE_PIX( CollectShadowProxies_Collection );
		for ( Uint32 i = 0; i < results.m_numResults; ++i )
		{
			CullResult& ptrToCollect = results.m_results[ i ];
			switch ( ptrToCollect.m_type )
			{
#ifdef USE_APEX
			case RPT_Apex:
				static_cast< CRenderProxy_Apex* >( ptrToCollect.m_ptr )->CollectCascadeShadowElements( cascades, ptrToCollect.m_cascades );
				break;
#endif
			case RPT_Mesh:
				static_cast< CRenderProxy_Mesh* >( ptrToCollect.m_ptr )->CollectCascadeShadowElements( cascades, ptrToCollect.m_cascades );
				break;
			case RPT_Fur:
				static_cast< CRenderProxy_Fur* >( ptrToCollect.m_ptr )->CollectCascadeShadowElements( cascades, ptrToCollect.m_cascades );
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CRenderElementMap::CRenderElementMap()
	: m_lastEntryId( 0 )
{
#ifdef PROFILE_CODE_ENABLED
	Red::MemoryZero( &m_stats, sizeof( m_stats ) );
#endif // PROFILE_CODE_ENABLED
	m_staticProxies.Reserve( MAX_REGISTERED_STATIC_PROXIES );
	
	const Uint64 align = sizeof(SRenderElementMapEntry);

	// Align up array of m_entries to full 64B line cache for optimization manually.
	m_entries = (SRenderElementMapEntry*)( ( (Uint64)m_entriesMemoryPlaceholder + align - 1ULL ) & ~( align - 1ULL ) );

	// Just sanity checks of what the true padding/alignments are
	RED_FATAL_ASSERT( ( (Uint64)m_entries % 64 ) == 0 , "Unefficent memory aligment for RenderElementMapEntry" );
}

CRenderElementMap::~CRenderElementMap()
{
	for ( Uint32 i = 1; i < MAX_REGISTERED_PROXIES; ++i )
	{
		if ( m_entries[ i ].HasFlag( REMEF_Valid ) )
		{
			RED_ASSERT( m_entries[ i ].m_proxy != nullptr, TXT("Empty proxy in valid Entry") );
			if ( m_entries[ i ].m_proxy )
			{
				m_entries[ i ].m_proxy->OnUnregistered();
			}
		}
	}
	for ( auto entityGroup : m_characterEntities )
	{
		RED_FATAL_ASSERT( entityGroup, "Null group still registered" );
		entityGroup->SetRenderElementMap( nullptr );
	}
}

#ifdef PROFILE_CODE_ENABLED
void CRenderElementMap::UpdateStats( const SRenderElementMapEntry& entry, Int32 value )
{
	Bool isDynamic = entry.HasFlag( REMEF_Dynamic );
	Int32* counter = nullptr;
	switch ( entry.m_type )
	{
	case RPT_Particles:		counter = (Int32*)&m_stats.m_particles;																	break;
	case RPT_Apex:			counter = (Int32*)&m_stats.m_apex;																		break;
	case RPT_Flare:			counter = (Int32*)&m_stats.m_flares;																	break;
	case RPT_Fur:			counter = (Int32*)&m_stats.m_fur;																		break;
	case RPT_Stripe:		counter = isDynamic ? (Int32*)&m_stats.m_nonBakedStripes		: (Int32*)&m_stats.m_bakedStripes;		break;
	case RPT_Dimmer:		counter = isDynamic ? (Int32*)&m_stats.m_nonBakedDimmers		: (Int32*)&m_stats.m_bakedDimmers;		break;
	case RPT_SSDecal:		counter = isDynamic ? (Int32*)&m_stats.m_nonBakedDecals			: (Int32*)&m_stats.m_bakedDecals;		break;
	case RPT_Mesh:			counter = isDynamic ? (Int32*)&m_stats.m_dynamicMeshes			: (Int32*)&m_stats.m_staticMeshes;		break;
	case RPT_PointLight:	counter = isDynamic ? (Int32*)&m_stats.m_nonBakedPointLights	: (Int32*)&m_stats.m_bakedPointLights;	break;
	case RPT_SpotLight:		counter = isDynamic ? (Int32*)&m_stats.m_nonBakedSpotLights		: (Int32*)&m_stats.m_bakedSpotLights ;	break;
	}

	if ( counter )
	{
		*counter += value;
	}
}
#endif // PROFILE_CODE_ENABLED

RED_INLINE Bool IsCharacterProxy( IRenderProxyBase* proxy, CRenderEntityGroup*& entityGroup )
{
	IRenderEntityGroupProxy* groupProxy = nullptr;
	switch ( proxy->GetType() )
	{
	case RPT_Mesh:	groupProxy = static_cast< CRenderProxy_Mesh* >( proxy );	break;
#ifdef USE_APEX
	case RPT_Apex:	groupProxy = static_cast< CRenderProxy_Apex* >( proxy );	break;
#endif
	case RPT_Fur:	groupProxy = static_cast< CRenderProxy_Fur* >( proxy );		break;
	}
	if ( groupProxy )
	{
		entityGroup = groupProxy->GetEntityGroup();
		return entityGroup != nullptr;
	}
	return false;
}

RED_INLINE Bool IsProxyDynamic( IRenderProxyBase* proxy )
{
	ERenderProxyType type = proxy->GetType();
	switch ( type )
	{
	case RPT_Particles:
	case RPT_Apex:
	case RPT_Flare:
	case RPT_Fur:
	case RPT_Swarm:
		// dynamic by default
		return true;

	case RPT_Mesh:
		if ( static_cast< CRenderProxy_Mesh* >( proxy )->IsForcedDynamic() )
		{
			// mesh proxy has a parent transform (is probably skinned)
			return true;
		}
	case RPT_SSDecal:
		if ( static_cast< IRenderProxyDrawable* >( proxy )->IsDynamic() )
		{
			// mesh or decal is dynamic
			return true;
		}
		break;

	case RPT_PointLight:
	case RPT_SpotLight:
		if ( static_cast< IRenderProxyLight* >( proxy )->IsForcedDynamic() )
		{
			// light has parent attachment (is probably skinned)
			return true;
		}
		break;

	case RPT_Dimmer:
	case RPT_Stripe:
		// static by default
		break;
	default:
		RED_HALT( "Invalid proxy type" );
		break;
	}

	// static proxy
	return false;
}

Bool CRenderElementMap::Register( IRenderProxyBase* proxy )
{
	RED_FATAL_ASSERT( proxy != nullptr, "Registering invalid proxy" );
	if ( proxy->GetRegCount() > 0 )
	{
		return false;
	}
	RED_FATAL_ASSERT( proxy->GetRegCount() == 0, "Registering the same proxy more than once!" );

#ifdef PROFILE_CODE_ENABLED
	Bool skipStatsUpdate = false;
#endif // PROFILE_CODE_ENABLED

	
	SRenderElementMapEntry entry;
	entry.SetFlag( REMEF_Valid );
	entry.SetFlag( REMEF_Pass_RegularCollection );
	entry.SetFlag( IsProxyDynamic( proxy ) ? REMEF_Dynamic : REMEF_Static );
	entry.m_type = proxy->GetType();
	entry.m_typeFlag = CRenderProxyTypeFlag( entry.m_type );
	entry.m_boundingBoxMin = proxy->GetBoundingBox().Min;
	entry.m_boundingBoxMax = proxy->GetBoundingBox().Max;
	if( entry.m_type == RPT_Particles )
	{
		entry.m_refPos = proxy->GetLocalToWorld().GetTranslationRef();
		if( static_cast< CRenderProxy_Particles*>( proxy )->IsVisibleThroughWalls() )
		{
			entry.SetFlag( REMEF_SkipOcclusionTest );
		}
	}
	else if ( entry.m_type == RPT_Apex )
	{
#ifdef USE_APEX
		// HACK! HACK! HACK! to fix blinking cloth in Ciri cutscene
		if ( static_cast< CRenderProxy_Apex*>( proxy )->SkipOcclusionTest() )
		{
			entry.SetFlag( REMEF_SkipOcclusionTest );
		}
#endif
	}
	else
	{
		entry.m_refPos = proxy->GetBoundingBox().CalcCenter();
		if ( entry.m_type == RPT_Mesh && entry.HasFlag( REMEF_Dynamic ) )
		{
			// HACK HACK HACK!! for missing door in BOB cutscene: cs702_in_the_closet
			if ( static_cast< CRenderProxy_Mesh*>( proxy )->SkipOcclusionTest() )
			{
				entry.SetFlag( REMEF_SkipOcclusionTest );
			}
		}
	}

	if ( const IRenderProxyLight* lightProxy = proxy->AsLight() )
	{
		entry.m_autoHideDistanceSquared = lightProxy->GetLightHideDistance();
		entry.m_autoHideDistanceSquared *= entry.m_autoHideDistanceSquared;
	}
	else
	{
		entry.m_autoHideDistanceSquared = proxy->GetAutoHideDistanceSquared();
	}

	entry.m_proxy = proxy;
	if ( const IRenderProxyDrawable* drawableProxy = proxy->AsDrawable() )
	{
		if ( drawableProxy->CanCastShadows() || drawableProxy->IsCastingShadowsFromLocalLightsOnly() )
		{
			entry.SetFlag( REMEF_ShadowCaster );
		}
		entry.m_renderMask = drawableProxy->GetRenderMask();
	}
	CRenderEntityGroup* entityGroup = nullptr;
	if ( IsCharacterProxy( proxy, entityGroup ) )
	{
		entry.SetFlag( REMEF_CharacterProxy );
		entry.ClearFlag( REMEF_Dynamic );
		entry.ClearFlag( REMEF_Static );
		if ( entityGroup )
		{
			entityGroup->SetRenderElementMap( this );
		}
	}

	if ( entry.m_type == RPT_Mesh && static_cast<CRenderProxy_Mesh*>(entry.m_proxy)->CheckMeshProxyFlag( RMPF_IsVolumeMesh ) )
	{
		entry.SetFlag( REMEF_VolumeMesh );
	}

	Uint32 entryID = m_idAllocator.Alloc();
	RED_FATAL_ASSERT( entryID > 0, "Invalid ID, too maxy renderproxies" );

	TObjectIdType objectId = 0;
	TObjectCacheKeyType key = proxy->GetUmbraProxyId().GetKey();
#ifdef USE_ANSEL
	// UBERHACK for things not visible in Ansel mode
	if ( key == 11465474488251405214 )
	{
		entry.m_skipCollection = true;
	}
#endif // USE_ANSEL
	if ( entry.HasFlag( REMEF_Static ) )
	{
		if ( m_objectCache.Find( key, objectId ) )
		{
			const Bool insertResult = m_staticProxies.Insert( objectId, entryID );
			if ( !insertResult )
			{
				RED_LOG_ERROR( UmbraError, TXT("Failed to insert %s object [%") RED_PRIWu64 TXT("; %u]"), m_staticProxies.KeyExist( objectId ) ? TXT("DUPLICATE") : TXT(""), key, objectId );
				m_idAllocator.Release( entryID );
				return false;
			}
			RED_ASSERT( m_staticProxies.Size() < MAX_REGISTERED_STATIC_PROXIES, TXT("Too many static proxies registered (%d), expect problems."), m_staticProxies.Size() );
		}
		else
		{
#ifdef PROFILE_CODE_ENABLED
			if ( entry.m_type == RPT_Mesh )
			{
				++m_stats.m_meshesNotInObjectCache;
				skipStatsUpdate = true;
			}
#endif // PROFILE_CODE_ENABLED
			entry.ClearFlag( REMEF_Static );
			entry.SetFlag( REMEF_Dynamic );
		}
	}


	// If this is a volume mesh, still need to collect with the statics. So put it in a separate list...
	if ( entry.HasFlag( REMEF_VolumeMesh ) && !entry.HasFlag( REMEF_Static ) )
	{
		entry.ClearFlag( REMEF_Dynamic );
		entry.SetFlag( REMEF_NonStaticVolumeMesh );

		Bool didAdd = m_nonStaticVolumes.Insert( entryID );
		RED_FATAL_ASSERT( didAdd, "Didn't add Non-Static Volume entry to set, was already there?" );
	}


	if ( entry.m_type == RPT_Mesh )
	{
		static_cast< CRenderProxy_Mesh* >( entry.m_proxy )->SetStaticInRenderElementMap( entry.HasFlag( REMEF_Static ) || entry.HasFlag( REMEF_NonStaticVolumeMesh ) );
	}

	entry.m_umbraObjectId = objectId;
	proxy->SetEntryID( entryID );
	Uint32 index = 0;
	if ( m_objectIdToIndexMap.Find( objectId, index ) )
	{
		Uint32 staticsIndex = m_statics[ index ];
		SRenderElementMapEntry& oldEntry = m_entries[ staticsIndex ];
		Bool isOldEntryDynamic = oldEntry.HasFlag( REMEF_Dynamic );
		Bool isOldEntryValid = oldEntry.HasFlag( REMEF_Valid );
		RED_ASSERT( !isOldEntryDynamic, TXT("Static remap of dynamic proxy") );
		RED_ASSERT( staticsIndex == 0 || !isOldEntryValid );
		m_statics[ index ] = entryID;
	}
	m_entries[ entryID ] = entry;
	if ( entryID > m_lastEntryId )
	{
		m_lastEntryId = entryID;
	}

	// inform the proxy
	proxy->OnRegistered();

#ifdef PROFILE_CODE_ENABLED
	if ( !skipStatsUpdate )
	{
		UpdateStats( entry, 1 );
	}
#endif // PROFILE_CODE_ENABLED

	return true;
}

Bool CRenderElementMap::Unregister( IRenderProxyBase* proxy, Bool deferredUnregister /*=false*/ )
{
	RED_FATAL_ASSERT( proxy != nullptr, "Trying to unregister invalid proxy" );
	if ( proxy->GetRegCount() <= 0 )
	{
		// already unregistered
		return true;
	}

	Uint32 entryID = proxy->GetEntryID();
	SRenderElementMapEntry& entry = m_entries[ entryID ];
	RED_FATAL_ASSERT( entry.m_proxy == proxy, "Trying to unregister invalid proxy" );
	
	if ( !deferredUnregister )
	{
		if ( entry.HasFlag( REMEF_Pass_DeferredCollection ) )
		{
			if ( entry.HasFlag( REMEF_NonStaticVolumeMesh ) )
			{
				Bool didErase = m_nonStaticVolumes.Erase( entryID );
				RED_FATAL_ASSERT( didErase, "Didn't remove Non-Static Volume entry from set" );
			}

			entry.ClearFlag( REMEF_Valid );
			m_idAllocator.Release( entryID );
			proxy->OnUnregistered();
			return true;
		}
	}

#ifdef PROFILE_CODE_ENABLED
	Bool skipStatsUpdate = false;
#endif // PROFILE_CODE_ENABLED

	if ( entry.HasFlag( REMEF_Static ) )
	{
		RED_VERIFY( m_staticProxies.Erase( entry.m_umbraObjectId ) );
	}
	
	if ( deferredUnregister )
	{
		entry.SetFlag( REMEF_Pass_DeferredCollection );
	}
	else
	{
		if ( entry.HasFlag( REMEF_NonStaticVolumeMesh ) )
		{
			Bool didErase = m_nonStaticVolumes.Erase( entryID );
			RED_FATAL_ASSERT( didErase, "Didn't remove Non-Static Volume entry from set" );
		}

		entry.ClearFlag( REMEF_Valid );
		m_idAllocator.Release( entryID );
		proxy->OnUnregistered();
	}

#ifdef USE_UMBRA
	Uint32 index = 0;
	if ( m_objectIdToIndexMap.Find( entry.m_umbraObjectId, index ) )
	{
		RED_ASSERT( m_statics[ index ] == entryID );
		m_statics[ index ] = 0;
	}
#endif // USE_UMBRA

#ifdef PROFILE_CODE_ENABLED
	if ( !skipStatsUpdate )
	{
		UpdateStats( entry, -1 );
	}
#endif // PROFILE_CODE_ENABLED
	
	return true;
}

void CRenderElementMap::RegisterDynamicDecalChunk( CRenderDynamicDecalChunk* chunk )
{
	RED_FATAL_ASSERT( chunk != nullptr, "" );
	RED_FATAL_ASSERT( chunk->GetRegCount() <= 0, "" );
	chunk->OnRegistered();
	m_dynamicDecalChunks.PushBack( chunk );
}

void CRenderElementMap::UnregisterDynamicDecalChunk( CRenderDynamicDecalChunk* chunk )
{
	RED_FATAL_ASSERT( chunk != nullptr, "" );
	RED_FATAL_ASSERT( chunk->GetRegCount() > 0, "" );
	if ( m_dynamicDecalChunks.Remove( chunk ) )
	{
		chunk->OnUnregistered();
	}
}

void CRenderElementMap::UpdateProxyInfo( const Uint32 id, const Vector& referencePosition, const Box& boundingBox )
{
	RED_FATAL_ASSERT( id > 0, "Invalid id" );
	SRenderElementMapEntry& entry = m_entries[ id ];
	RED_FATAL_ASSERT( entry.HasFlag( REMEF_Valid ), "Updating invalid entry" );
	entry.m_refPos = referencePosition;
	entry.m_boundingBoxMin = boundingBox.Min;
	entry.m_boundingBoxMax = boundingBox.Max;
}

void CRenderElementMap::SetProxyFadingOut( const Uint32 id, const Bool fading )
{
	RED_FATAL_ASSERT( id > 0, "Invalid id" );
	SRenderElementMapEntry& entry = m_entries[ id ];
	RED_FATAL_ASSERT( entry.HasFlag( REMEF_Valid ), "Updating invalid entry" );
	if ( fading )
	{
		entry.SetFlag( REMEF_FadingOut );
	}
	else
	{
		entry.ClearFlag( REMEF_FadingOut );
	}
}


void CRenderElementMap::SetProxyPastAutoHide( const Uint32 id, const Bool pastAutoHide )
{
	RED_FATAL_ASSERT( id > 0, "Invalid id" );
	SRenderElementMapEntry& entry = m_entries[ id ];
	RED_FATAL_ASSERT( entry.HasFlag( REMEF_Valid ), "Updating invalid entry" );
	if ( pastAutoHide )
	{
		entry.SetFlag( REMEF_PastAutoHide );
	}
	else
	{
		entry.ClearFlag( REMEF_PastAutoHide );
	}
}


#ifdef USE_UMBRA
void CRenderElementMap::FeedVisibleObjects( TVisibleChunksIndices& visibleObjectsIndices, Int32 objectsCount )
{
	m_visibleObjectsCount = objectsCount;
	m_visibleObjectsIndices.SwapWith( visibleObjectsIndices );
}

void CRenderElementMap::BuildRemapArray( const Umbra::TomeCollection* tomeCollection, const TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIdToIndexMap )
{
	m_tomeCollection = tomeCollection;
	m_objectIdToIndexMap = Move( objectIdToIndexMap );

	CTimeCounter timer;

	m_statics.Clear();
	m_statics.Resize( remapTable.Size() );

	for ( Uint32 i = 0; i < remapTable.Size(); ++i )
	{
		m_staticProxies.Find( remapTable[ i ], m_statics[ i ] );
	}

	Double time = timer.GetTimePeriodMS();
	LOG_RENDERER( TXT("Building RemapAarray took: %1.3f ms"), time );
}

#ifndef NO_EDITOR
void CRenderElementMap::DumpVisibleMeshes( const TLoadedComponentsMap& componentsMap, const String& path )
{
	/*
	PC_SCOPE_PIX( RenderMap_DumpVisibleMeshes );

	IFile* writer = GFileManager->CreateFileWriter( path.AsChar(), FOF_Buffered|FOF_AbsolutePath );
	if ( !writer )
	{
		return;
	}

	Uint32 linesWritten = 0;

	for ( Int32 i = 0; i < m_visibleObjectsCount; ++i )
	{
		Int32 objectIndex = m_visibleObjectsIndices[ i ];
		if ( objectIndex < 0 || objectIndex >= m_statics.SizeInt() )
		{
			// TODO: figure out why it happens, this is just for crash fix
			continue;
		}
		IRenderProxyBase* proxy = m_statics[ objectIndex ];
		if ( proxy && proxy->GetType() == RPT_Mesh )
		{
			TObjectIdType umbraObjectID;
			RED_VERIFY( proxy->GetUmbraObjectId( umbraObjectID ) );
			
			ComponentDesctiprion desc;
			if ( componentsMap.Find( umbraObjectID, desc ) )
			{
				++linesWritten;
				String line = String::Printf( TXT("%d;%ls;%ls;%ls;%u\n"),	umbraObjectID,
																			desc.layerPath.AsChar(),
																			desc.entityName.AsChar(),
																			desc.meshDepoPath.AsChar(),
																			desc.chunkCount );
				writer->Serialize( UNICODE_TO_ANSI( line.AsChar() ), line.GetLength() );
			}
		}
	}

	delete writer;
	writer = nullptr;
	*/
}
#endif // NO_EDITOR
#endif // USE_UMBRA

void CRenderElementMap::Collect( const Box& box, CRenderProxyTypeFlag proxyTypeFlags, TDynArray< IRenderProxyBase* >& proxies, Uint32 flags /*=REMEF_Valid*/ ) const
{
	PC_SCOPE_PIX( CRenderElementMap_Collect_Box );
	for ( Uint32 i = 1; i <= m_lastEntryId; ++i )
	{
		const CRenderElementMap::SRenderElementMapEntry& entry = m_entries[ i ];
		if ( !entry.HasFlag( REMEF_Valid ) )
		{
			// do not collect invalid proxies
			continue;
		}
		if ( !entry.HasFlag( (ERenderElementMapEntryFlags)flags ) )
		{
			continue;
		}
		if ( ( proxyTypeFlags & entry.m_typeFlag ) == 0 )
		{
			continue;
		}
		if ( box.Touches( entry.m_boundingBoxMin, entry.m_boundingBoxMax ) )
		{
			proxies.PushBack( entry.m_proxy );
		}
	}
}

void CRenderElementMap::Collect( const CFrustum& frustum, CRenderProxyTypeFlag proxyTypeFlags, TDynArray< IRenderProxyBase* >& proxies, Uint32 flags /*=REMEF_Valid*/ ) const
{
	PC_SCOPE_PIX( CRenderElementMap_Collect_Frustum );
	for ( Uint32 i = 1; i <= m_lastEntryId; ++i )
	{
		const CRenderElementMap::SRenderElementMapEntry& entry = m_entries[ i ];
		if ( !entry.HasFlag( REMEF_Valid ) )
		{
			// do not collect invalid proxies
			continue;
		}
		if ( !entry.HasFlag( (ERenderElementMapEntryFlags)flags ) )
		{
			continue;
		}
		if ( ( proxyTypeFlags & entry.m_typeFlag ) == 0 )
		{
			continue;
		}
		if ( frustum.TestBox( entry.m_boundingBoxMin, entry.m_boundingBoxMax ) )
		{
			proxies.PushBack( entry.m_proxy );
		}
	}
}

void CRenderElementMap::Collect( IShadowmapQuery** queries, Uint32 numQueries, CRenderProxyTypeFlag proxyTypeFlags, Uint32 flags /*=REMEF_Valid*/ ) const
{
	if ( numQueries == 0 )
	{
		// early exit
		return;
	}

	PC_SCOPE_PIX( CRenderElementMap_Collect_Queries );
	for ( Uint32 i = 1; i <= m_lastEntryId; ++i )
	{
		const CRenderElementMap::SRenderElementMapEntry& entry = m_entries[ i ];
		if ( !entry.HasFlag( REMEF_Valid ) )
		{
			// do not collect invalid proxies
			continue;
		}
		if ( !entry.HasFlag( (ERenderElementMapEntryFlags)flags ) )
		{
			continue;
		}
		if ( ( proxyTypeFlags & entry.m_typeFlag ) == 0 )
		{
			continue;
		}
		RED_FATAL_ASSERT( entry.HasFlag( REMEF_Valid ), "Collecting invalid proxy" );
		for ( Uint32 j = 0; j < numQueries; ++j )
		{
			const Bool testResult = queries[ j ]->Test( entry.m_boundingBoxMin, entry.m_boundingBoxMax );
			if ( testResult )
			{
				queries[ j ]->m_proxies.PushBack( entry.m_proxy );
			}
		}
	}
}

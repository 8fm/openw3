#include "build.h"
#include "clipMap.h"
#include "entityStreamingProxy.h"
#include "streamingSectorData.h"
#include "../core/streamingGrid.h"
#include "../core/streamingGridHelpers.h"
#include "../core/configVar.h"
#include "../core/fileSystemProfilerWrapper.h"

namespace Config
{
	TConfigVar< Float, Validation::FloatRange< 0, 100, 1 > >		cvEntityStreamingStep( "Streaming/Entity", "StreamingStep", 0.2f );
	TConfigVar< Bool >												cvEntityStreamingEnabled( "Streaming/Entity", "StreamingEnabled", true );
	TConfigVar< Bool >												cvEntityForceUnstream( "Streaming/Entity", "ForceUnstream", false );
	TConfigVar< Float, Validation::FloatRange< 0, 100, 1 > >		cvEntityStreamingPrecacheDistance( "Streaming/Entity", "PrecacheDistance", 40.0f );
	TConfigVar< Int32, Validation::IntRange< 1, INT_MAX > >			cvEntityStreamingThrottleStart( "Streaming/Entity", "ThrottleMaxStreamedPerFrame", 2000 );
	TConfigVar< Int32, Validation::IntRange< 1, INT_MAX > >			cvEntityStreamingThrottleUpdate( "Streaming/Entity", "ThrottleMaxUpdatedPerFrame", 3000 );
	TConfigVar< Int32, Validation::IntRange< 4, 10 > >				cvEntityStreamingGridLevels( "Streaming/Entity", "GridLevels", 8 );
	TConfigVar< Int32, Validation::IntRange< 4096, INT_MAX > >		cvEntityStreamingGridBuckets( "Streaming/Entity", "GridBuckets", 65536 );
	TConfigVar< Float, Validation::FloatRange< 1, INT_MAX, 1 > >	cvEntityMinStreamingDistance( "Streaming/Entity", "MinStreamingDistance", 4.0f );
	TConfigVar< Float, Validation::FloatRange< 1, INT_MAX, 1 > >	cvEntityMaxStreamingDistance( "Streaming/Entity", "MaxStreamingDistance", 2048.0f );
	TConfigVar< Float, Validation::FloatRange< 1, 10, 1 > >			cvEntityMaxStreamingBudget( "Streaming/Entity", "MaxStreamingBudget", 2.0f );
}

//--------------------------------

CStreamingIgnoreList::CStreamingIgnoreList()
{
}

void CStreamingIgnoreList::IgnoreEntity( CEntity* entity )
{
	m_ignoredEntities.Insert( entity );
}

void CStreamingIgnoreList::UnignoreEntity( CEntity* entity )
{
	m_ignoredEntities.Erase( entity );
}

void CStreamingIgnoreList::ClearIgnoreList()
{
	m_ignoredEntities.Clear();
}

void CStreamingIgnoreList::GetIgnoredEntities( TDynArray< CEntity* >& entities )
{
	for ( auto it=m_ignoredEntities.Begin(); it != m_ignoredEntities.End(); ++it )
	{
		if ( (*it).IsValid() )
		{
			entities.PushBack( (*it).Get() );
		}
	}
}

//--------------------------------

CStreamingSectorData::CStreamingSectorData( class CWorld* world )
	: m_world( world )
	, m_worldSize( 64.0f )
	, m_positionValid( false )
	, m_referencePosition( 0,0,0 )
	, m_lastStreamingPosition( 0,0,0 )
	, m_queryInRangeCount( 0 )
	, m_isUpdating( false )
	, m_wasOverBudget( false )
{
	// get world extents
	if ( m_world->GetTerrain() )
	{
		m_worldSize = m_world->GetTerrain()->GetTerrainSize();
	}

	// initialize streaming grid
	const Uint32 gridLevels = Config::cvEntityStreamingGridLevels.Get();
	const Uint32 gridBuckets = Config::cvEntityStreamingGridBuckets.Get();
	m_grid = new CStreamingGrid( gridLevels, gridBuckets );

	// create quantizer
	m_quantizer = new CStreamingPositionQuantizer( m_worldSize );

	// determine expected entity count
	const Uint32 maxEntities = GIsEditor ? 400000 : 100000;

	// initialize entity masks
	m_inRangeMask = new CStreamingEntryMask(maxEntities);
	m_queryMask = new CStreamingEntryMask(maxEntities);
	m_streamingMask = new CStreamingEntryMask(maxEntities);
	m_lockedMask = new CStreamingEntryMask(maxEntities);

	// fill initial allocator
	m_proxies.Resize( maxEntities );
	m_proxyIdAllocator.Resize( maxEntities );
	m_proxyIdAllocator.Alloc();
}

CStreamingSectorData::~CStreamingSectorData()
{
	if ( m_grid )
	{
		delete m_grid;
		m_grid = nullptr;
	}

	if ( m_quantizer )
	{
		delete m_quantizer;
		m_quantizer = nullptr;
	}

	if ( m_inRangeMask )
	{
		delete m_inRangeMask;
		m_inRangeMask = nullptr;
	}

	if ( m_streamingMask )
	{
		delete m_streamingMask;
		m_streamingMask = nullptr;
	}

	if ( m_queryMask )
	{
		delete m_queryMask;
		m_queryMask = nullptr;
	}

	if ( m_lockedMask )
	{
		delete m_lockedMask;
		m_lockedMask = nullptr;
	}
}

void CStreamingSectorData::PrepareDataStructures()
{
	// make sure working tables are big enough for current data sizes
	m_queryMask->Prepare( m_proxyIdAllocator.GetCapacity() );
	m_inRangeMask->Prepare( m_proxyIdAllocator.GetCapacity() );
	m_streamingMask->Prepare( m_proxyIdAllocator.GetCapacity() );
	m_lockedMask->Prepare( m_proxyIdAllocator.GetCapacity() );
}

void CStreamingSectorData::SetReferencePosition( const Vector& position )
{
	// do a forced update on first valid position
	if ( !m_positionValid )
		m_requestFullUpdate = true;
	
	m_referencePosition = position;
	m_positionValid = true;
}

void CStreamingSectorData::RequestFullUpdate()
{
	m_requestFullUpdate = true;
}

// PS4 CERT2 HACK - faster engine time query
class FastEngineTime
{
public:
	RED_FORCE_INLINE FastEngineTime()
		: m_time( 0 )
	{}

	RED_FORCE_INLINE FastEngineTime( const Double time )
	{
		m_time = (Uint64)( time * s_freq );
	}

	RED_FORCE_INLINE FastEngineTime( Uint64 ticks )
		: m_time( ticks )
	{
	}

	RED_FORCE_INLINE static FastEngineTime GetNow()
	{
		FastEngineTime result;
		result.SetNow();
		return result;
	}

	RED_FORCE_INLINE Double operator-( const FastEngineTime& other ) const
	{
		return ((Double)m_time - (Double)other.m_time) / s_freq;
	}

	RED_FORCE_INLINE const Bool operator<( const FastEngineTime& other ) const
	{
		return m_time < other.m_time;
	}

	RED_FORCE_INLINE const Bool operator>( const FastEngineTime& other ) const
	{
		return m_time > other.m_time;
	}

	RED_FORCE_INLINE FastEngineTime operator+( const FastEngineTime& other ) const
	{
		return FastEngineTime( m_time + other.m_time );
	}

	RED_FORCE_INLINE void SetNow()
	{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
		QueryPerformanceCounter( (LARGE_INTEGER*) &m_time);
#elif defined( RED_PLATFORM_ORBIS )
		m_time = ::sceKernelReadTsc();
#endif
	}

	static void Init()
	{
		if ( !s_wasInit )
		{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
			LARGE_INTEGER freq;
			QueryPerformanceFrequency( &freq );
			s_freq  = Double( freq.QuadPart );
#elif defined( RED_PLATFORM_ORBIS )
			s_freq = ::sceKernelGetTscFrequency();
#endif
			s_wasInit = true;
		}
	}

private:
	static Bool		s_wasInit;
	static Double	s_freq;
	Uint64			m_time;
};

Double FastEngineTime::s_freq = 0.0;
Bool FastEngineTime::s_wasInit = false;

void CStreamingSectorData::UpdateStreaming( const Bool force /*= false*/, const Bool flush /*= false*/ )
{
	PC_SCOPE( UpdateStreaming );

	CTimeCounter timer;

	// streaming is disabled
	if ( !Config::cvEntityStreamingEnabled.Get() )
		return; 

	// there's no valid position
	if ( !m_positionValid )
		return;

	// do not update if we are not close enough and there are no pending updates
	if ( !m_requestFullUpdate && !force && m_streamingProxies.Empty() && (m_queryInRangeCount == m_inRangeProxies.Size()) )
	{
		const Float distToLastUpdate = m_lastStreamingPosition.DistanceTo( m_referencePosition );
		if ( distToLastUpdate <= Config::cvEntityStreamingStep.Get() )
		{
			return;
		}
	}

	// begin update
	m_isUpdating = true;

	// reset update flag
	m_lastStreamingPosition = m_referencePosition;
	m_requestFullUpdate = false;
	m_wasOverBudget = false;

	// make sure all data fit
	PrepareDataStructures();

	// quantize position
	const auto q = m_quantizer->QuantizePosition( m_lastStreamingPosition );

	// collect entries for given position
	TStreamingGridCollectorStack< 65535 > entitiesInRange;
	if ( !Config::cvEntityForceUnstream.Get() )
	{
		PC_SCOPE( CollectGrid );
		m_grid->CollectForPoint( q.x, q.y, q.z, entitiesInRange );
	}

	// build merged query
	{
		// convert the selection to bit mask
		PC_SCOPE( PrepareBitmask );
		for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
		{
			const Uint32 id = entitiesInRange[i];
			m_queryMask->Set( id );
		}

		// apply streaming lock (force select some entities)
		if ( m_hasStreamingLock )
		{
			PC_SCOPE( ApplyStreamingLock );

			for ( Uint32 i=0; i<m_lockedProxies.Size(); ++i )
			{
				const Uint32 id = m_lockedProxies[i];
				if ( !m_queryMask->Test( id ) )
				{
					entitiesInRange.Add( id, 0 );
					m_queryMask->Set( id );
				}
			}
		}
	}

	// remember the number of entities in the query
	m_queryInRangeCount = entitiesInRange.Size();

	// start streaming entities that entered the range
	{
		PC_SCOPE( StreamIn );
		Uint32 numEntitiesInRange = 0;
		for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
		{
			const Uint32 id = entitiesInRange[i];

			CEntityStreamingProxy* proxy = m_proxies[ id ];
			RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );
			RED_FATAL_ASSERT( proxy->GetId() == id, "Proxy ID does not match it's grid entry (%d!=%d)", id, proxy->GetId() );

			// proxy was not selected for streaming last frame
			if ( !m_inRangeMask->Test( id ) )
			{
				// throttle (not in the flush mode)
				numEntitiesInRange += 1;
				if ( !flush && (numEntitiesInRange > (Uint32)Config::cvEntityStreamingThrottleStart.Get()) )
					continue;

				// start streaming of the entity
				if ( proxy->StartStreaming() )
				{
					// insert to the list of proxies in range
					m_inRangeProxies.PushBack( id );
					m_inRangeMask->Set( id );

					// add to the streaming list - we will be updated until we are fully streamed
					m_streamingProxies.PushBack( id );
					m_streamingMask->Set( id );
				}
			}
		}
	}

	// update entities
	{
		PC_SCOPE( Update );

		// Streaming budget calculations
		FastEngineTime::Init();
		FastEngineTime budgetStart = FastEngineTime::GetNow();
		FastEngineTime budgetEnd = budgetStart + (Config::cvEntityMaxStreamingBudget.Get() / 1000.0f);

		Uint32 index = 0;
		Uint32 numEntitiesUpdated = 0;
		while ( index < m_streamingProxies.Size() )
		{
			const Uint32 id = m_streamingProxies[ index ];

			// get proxy for given ID
			CEntityStreamingProxy* proxy = m_proxies[ id ];
			RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );
			RED_FATAL_ASSERT( proxy->GetId() == id, "Proxy ID does not match it's grid entry (%d!=%d)", id, proxy->GetId() );

			// validate that we are still on the list
			RED_FATAL_ASSERT( m_streamingMask->Test( id ), "Proxy in the update list but the mast is not set" );

			// throttle (not in the flush mode)
			if ( !flush && (numEntitiesUpdated >= (Uint32)Config::cvEntityStreamingThrottleUpdate.Get()) )
				break;

			// throttle based on time
			if ( !GIsEditor && !flush && (FastEngineTime::GetNow() > budgetEnd) )
			{
				m_wasOverBudget = true;
				break;
			}

			// request proxy to update itself
			numEntitiesUpdated += 1;
			if ( proxy->UpdateStreaming( flush ) )
			{
				// proxy is fully updated, remove from update list
				m_streamingProxies.Remove( id );
				m_streamingMask->Clear( id );
			}
			else
			{
				// not finished, keep going
				index += 1;
			}
		}
	}

	// stream out entities that went outside the range
	{
		PC_SCOPE( StreamOut );
		Uint32 index = 0;
		while ( index < m_inRangeProxies.Size() )
		{
			const Uint32 id = m_inRangeProxies[index];

			// is this entity no longer in the set ?
			if ( m_queryMask->Test( id ) )
			{
				index += 1;
				continue;
			}

			// get proxy for given ID
			CEntityStreamingProxy* proxy = m_proxies[ id ];
			RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );
			RED_FATAL_ASSERT( proxy->GetId() == id, "Proxy ID does not match it's grid entry (%d!=%d)", id, proxy->GetId() );

			// can we unstream this entity ?
	#ifndef NO_EDITOR
			{
				CEntity* entity = proxy->GetEntity();
				if ( m_ignoreList.IsEntityIgnored(entity) )
				{
					index += 1;
					continue;
				}
			}
	#endif

			// unstream
			if ( proxy->Unstream() )
			{
				// remove from update list
				if ( m_streamingMask->Test( id ) ) 
				{
					RED_FATAL_ASSERT( m_streamingProxies.Exist(id), "Proxy not on the list even though the flag is set" );
					m_streamingProxies.Remove( id );
					m_streamingMask->Clear( id );
				}

				// entity is out of range, remove it from in-range list
				m_inRangeProxies.RemoveAtFast( index );
				m_inRangeMask->Clear( id );
			}
			else
			{
				// not unstreamed
				index += 1;
			}
		}
	}

	// Cleanup
	for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
	{
		const Uint32 id = entitiesInRange[i];
		m_queryMask->Clear( id );
	}

	// End update
	m_isUpdating = false;

	// Remove pending proxies
	for ( CEntityStreamingProxy* proxy : m_toUnregister )
		UnregisterEntity( proxy );

	// Cleanup
	m_toUnregister.ClearFast();

	// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
	{
		const Uint32 numObjectsInRange = m_queryInRangeCount;
		const Uint32 numObjectsStreaming = m_streamingProxies.Size();
		const Uint32 numObjectsStreamed = m_inRangeProxies.Size();
		const Uint32 numObjectsLocked = m_lockedProxies.Size();
		RedIOProfiler::ProfileStreamingEntity( numObjectsInRange, numObjectsStreaming, numObjectsStreamed, numObjectsLocked );
	}
#endif
}

CEntityStreamingProxy* CStreamingSectorData::RegisterEntity( CEntity* entity, const Vector& worldPosition )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// inside the update
	RED_ASSERT( !m_isUpdating, TXT("RegisterEntity called while streaming is udpated") );
	if ( m_isUpdating )
	{
		ERR_ENGINE( TXT("RegisterEntity called from witin the UpdateStreaming, entity = %ls"), entity->GetFriendlyName().AsChar() );
	}

	// allocate ID
	Uint32 entityProxyId = m_proxyIdAllocator.Alloc();
	if ( entityProxyId == 0 )
	{
		// out of ID, resize
		const Uint32 enitityLimit = m_proxyIdAllocator.GetCapacity();
		m_proxyIdAllocator.Resize( enitityLimit*2 );
		m_proxies.Resize( enitityLimit*2 );
		LOG_ENGINE( TXT("Resized ID pool in streaming sector data %d->%d"), enitityLimit, enitityLimit*2 );

		// retry
		entityProxyId = m_proxyIdAllocator.Alloc();
		RED_FATAL_ASSERT( entityProxyId != 0, "Failed to allocate entity proxy ID" );
	}

	// extract params
	const Float minRadius = Config::cvEntityMinStreamingDistance.Get();
	const Float maxRadius = Config::cvEntityMaxStreamingDistance.Get();
	const Float radius = Clamp<Float>( (Float)entity->GetStreamingDistance() + Config::cvEntityStreamingPrecacheDistance.Get(), minRadius, maxRadius );

	// quantize position and radius
	const auto q = m_quantizer->QuantizePositionAndRadius( worldPosition, radius );

	// register entity in the grid, returns the grid hash that we need to keep around
	// the data we are registering in the grid is the proxy ID
	const CStreamingGrid::TGridHash hash = m_grid->Register( q.x, q.y, q.z, q.w, entityProxyId );
	if ( hash == 0 )
	{
		ERR_ENGINE( TXT("Failed to register streamable entity '%ls' in the streaming grid. Entity will NOT be streamed."), 
			entity->GetFriendlyName().AsChar() );

		// free the entity Id
		m_proxyIdAllocator.Release(entityProxyId);
		return nullptr;
	}

	// refresh streaming if new entity is in it's streaming range
	if ( m_referencePosition.DistanceSquaredTo( worldPosition ) <= radius*radius )
	{
		RequestFullUpdate();
	}

	// create the streaming wrapper
	CEntityStreamingProxy* proxy = new CEntityStreamingProxy( entity, hash, entityProxyId );
	m_proxies[ entityProxyId ] = proxy;
	return proxy;
}

void CStreamingSectorData::UnregisterEntity( CEntityStreamingProxy* streamingProxy )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// inside the update
	RED_ASSERT( !m_isUpdating, TXT("UnegisterEntity called while streaming is udpated") );
	if ( m_isUpdating )
	{
		ERR_ENGINE( TXT("UnegisterEntity called from witin the UpdateStreaming, entity = %ls"), 
			streamingProxy->GetEntity() ? streamingProxy->GetEntity()->GetFriendlyName().AsChar() : TXT("NONE") );

		m_toUnregister.PushBack( streamingProxy );
		return; // do not unregister it now
	}

	// unregister entity from
	const Uint32 proxyID = streamingProxy->GetId();
	RED_FATAL_ASSERT( m_proxies[ proxyID ] == streamingProxy, "Unmapped proxy" );

	// proxy is in range, remove it
	if ( (proxyID < m_inRangeMask->GetCapacity()) && m_inRangeMask->Test( proxyID ) )
	{
		m_inRangeMask->Clear( proxyID );
		RED_FATAL_ASSERT( m_inRangeProxies.Exist(proxyID), "Proxy not on the list even though the flag is set" );
		m_inRangeProxies.Remove( proxyID );
	}

	// proxy is being streamed
	if ( (proxyID < m_streamingMask->GetCapacity()) && m_streamingMask->Test( proxyID ) )
	{
		m_streamingMask->Clear( proxyID );
		RED_FATAL_ASSERT( m_streamingProxies.Exist(proxyID), "Proxy not on the list even though the flag is set" );
		m_streamingProxies.Remove( proxyID );
	}

	// proxy is locked
	if ( (proxyID < m_lockedMask->GetCapacity()) && m_lockedMask->Test( proxyID ) )
	{
		m_lockedMask->Clear( proxyID );
		RED_FATAL_ASSERT( m_lockedProxies.Exist(proxyID), "Proxy not on the list even though the flag is set" );
		m_lockedProxies.Remove( proxyID );
	}

	// release the entry from the grid
	m_grid->Unregister( streamingProxy->GetGridHash(), proxyID );

	// release proxy data
	m_proxyIdAllocator.Release( proxyID );
	delete streamingProxy;
}

void CStreamingSectorData::UpdateEntity( CEntityStreamingProxy* streamingProxy, const Vector& worldPosition )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// unregister entity from
	const Uint32 proxyID = streamingProxy->GetId();
	RED_FATAL_ASSERT( m_proxies[ proxyID ] == streamingProxy, "Unmapped proxy" );

	// quantize position
	const auto q = m_quantizer->QuantizePosition( worldPosition );

	
	// move proxy in the grid
	const auto hash = streamingProxy->GetGridHash();
	const auto newHash = m_grid->Move( hash, q.x, q.y, q.z, proxyID );
	if ( newHash != hash )
	{
		streamingProxy->m_gridHash = newHash;
	}
}

void CStreamingSectorData::GetDebugInfo( SStreamingDebugData& outInfo ) const
{
	outInfo.m_referencePosition = m_referencePosition;
	outInfo.m_lastStreamingPosition = m_lastStreamingPosition;
	outInfo.m_numGridLevels = m_grid->GetNumLevels();
	outInfo.m_numGridBucketsMax = m_grid->GetMaxBuckets();
	outInfo.m_numGridBucketsUsed = m_grid->GetNumBuckets();
	outInfo.m_numProxiesInRange = m_queryInRangeCount;
	outInfo.m_numProxiesStreamed = m_inRangeProxies.Size();
	outInfo.m_numProxiesStreaming = m_streamingProxies.Size();
	outInfo.m_numProxiesRegistered = m_proxyIdAllocator.GetNumAllocated() - 1;
	outInfo.m_numProxiesLocked = m_lockedProxies.Size();
	outInfo.m_wasOverBudgetLastFrame = m_wasOverBudget;
	outInfo.m_worldSize = m_worldSize;
}

void CStreamingSectorData::GetGridDebugInfo( const Uint32 level, struct SStreamingGridDebugData& outData ) const
{
	m_grid->GetDebugInfo( level, outData );
}

void CStreamingSectorData::ForceStreamForPoint( const Vector& point )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// finish current streaming
	UpdateStreaming( /*force*/ true, /* flush */ true );

	// make sure all data fit
	PrepareDataStructures();

	// quantize position
	const auto q = m_quantizer->QuantizePosition( point );

	// collect entries for given position
#ifdef NO_EDITOR
	TStreamingGridCollectorStack< 50000 > entitiesInRange;
#else
	TStreamingGridCollectorStack< 150000 > entitiesInRange;
#endif
	m_grid->CollectForPoint( q.x, q.y, q.z, entitiesInRange );

	// stream in the entities in range
	// NOTE: do not unstream entities outside this range
	Uint32 numEntitiesStreamed = 0;
	for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
	{
		const Uint32 id = entitiesInRange[i];

		CEntityStreamingProxy* proxy = m_proxies[ id ];
		RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );

		// proxy was not selected for streaming last frame
		if ( !m_inRangeMask->Test( id ) )
		{
			// start streaming of the entity
			proxy->StartStreaming();
			proxy->UpdateStreaming( /* flush */ true );
			numEntitiesStreamed += 1;

			// insert to the list of proxies in range
			m_inRangeProxies.PushBack( id );
			m_inRangeMask->Set( id );
		}
	}

	// force stream in the locked entities
	for ( Uint32 i=0; i<m_lockedProxies.Size(); ++i )
	{
		const Uint32 id = m_lockedProxies[i];

		CEntityStreamingProxy* proxy = m_proxies[ id ];
		RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );

		// proxy was not selected for streaming last frame
		if ( !m_inRangeMask->Test( id ) )
		{
			// start streaming of the entity
			proxy->StartStreaming();
			proxy->UpdateStreaming( /* flush */ true );
			numEntitiesStreamed += 1;

			// insert to the list of proxies in range
			m_inRangeProxies.PushBack( id );
			m_inRangeMask->Set( id );
		}
	}

	// we cannot be over budget with forced stuff
	m_wasOverBudget = false;
}

void CStreamingSectorData::ForceStreamForArea( const Box& area )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// finish current streaming
	UpdateStreaming( /*force*/ true, /* flush */ true );

	// make sure all data fit
	PrepareDataStructures();

	// quantize position
	const auto qMin = m_quantizer->QuantizePosition( area.Min );
	const auto qMax = m_quantizer->QuantizePosition( area.Max );

	// collect entries for given position
#ifdef NO_EDITOR
	TStreamingGridCollectorStack< 50000 > entitiesInRange;
#else
	TStreamingGridCollectorStack< 150000 > entitiesInRange;
#endif
	m_grid->CollectForArea( qMin.x, qMin.y, qMax.x, qMax.y, entitiesInRange );

	// stream in the entities in range
	// NOTE: do not unstream entities outside this range
	Uint32 numEntitiesStreamed = 0;
	for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
	{
		const Uint32 id = entitiesInRange[i];

		CEntityStreamingProxy* proxy = m_proxies[ id ];
		if ( proxy )
		{
			// proxy was not selected for streaming last frame
			if ( !m_inRangeMask->Test( id ) )
			{
				// start streaming of the entity
				proxy->StartStreaming();
				proxy->UpdateStreaming( /* flush */ true );
				numEntitiesStreamed += 1;

				// insert to the list of proxies in range
				m_inRangeProxies.PushBack( id );
				m_inRangeMask->Set( id );
			}
		}
	}

	// force stream in the locked entities
	for ( Uint32 i=0; i<m_lockedProxies.Size(); ++i )
	{
		const Uint32 id = m_lockedProxies[i];

		CEntityStreamingProxy* proxy = m_proxies[ id ];
		RED_FATAL_ASSERT( proxy != nullptr, "No proxy found for collected ID%d, proxy is no longer in gird", id );

		// proxy was not selected for streaming last frame
		if ( !m_inRangeMask->Test( id ) )
		{
			// start streaming of the entity
			proxy->StartStreaming();
			proxy->UpdateStreaming( /* flush */ true );
			numEntitiesStreamed += 1;

			// insert to the list of proxies in range
			m_inRangeProxies.PushBack( id );
			m_inRangeMask->Set( id );
		}
	}

	// we cannot be over budget with forced stuff
	m_wasOverBudget = false;
}

void CStreamingSectorData::SetStreamingLock( const Box* bounds, const Uint32 numBounds )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// clear current lock
	m_lockedMask->ClearAll();
	m_lockedProxies.ClearFast();

	// make sure all data fit
	PrepareDataStructures();

	// collect objects from all areas and create one big merged list
	// we can use less space in case of the cooked data
#ifdef NO_EDITOR
	TStreamingGridCollectorStack< 50000 > entitiesInRange;
#else
	TStreamingGridCollectorStack< 150000 > entitiesInRange;
#endif
	for ( Uint32 i=0; i<numBounds; ++i )
	{
		// quantize position
		const Box area = bounds[i];
		const auto qMin = m_quantizer->QuantizePosition( area.Min );
		const auto qMax = m_quantizer->QuantizePosition( area.Max );

		// collect entries for given position
		m_grid->CollectForArea( qMin.x, qMin.y, qMax.x, qMax.y, entitiesInRange );
	}

	// add the entries to locked list
	for ( Uint32 i=0; i<entitiesInRange.Size(); ++i )
	{
		const Uint32 id = entitiesInRange[i];
		if ( !m_lockedMask->Test( id ) )
		{
			m_lockedMask->Set( id );
			m_lockedProxies.PushBack( id );
		}
	}

	// set streaming lock
	m_hasStreamingLock = ( entitiesInRange.Size() > 0 );

	// refresh on next update
	m_requestFullUpdate = true;
}
/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataStreaming.h"
#include "sectorDataStreamingContext.h"
#include "sectorDataStreamingThread.h"

#include "../core/configVar.h"
#include "../core/profiler.h"

// W3 hack
#include "renderer.h"
#include "../core/fileSystemProfilerWrapper.h"

namespace Config
{
	TConfigVar< Bool >					cvSectorStreamingEnabled( "Streaming/Sectors", "StreamingEnabled", true );
	TConfigVar< Float >					cvSectorResourcePrefetchRadius( "Streaming/Sectors", "ResourcePrefetchRadius", 150.0f );
}

//-----

CSectorDataStreaming::CSectorDataStreaming( const Float worldSize, class IRenderScene* renderScene, class CPhysicsWorld* physicsScene, class CDynamicLayer* dynamicLayer, const Bool hasPrefetchData, class CClipMap* terrainClipMap )
	: m_positionValid( false )
	, m_context( nullptr )
	, m_threadData( nullptr )
	, m_thread( nullptr )
{
	//// create streaming context (data)
	m_context = new CSectorDataStreamingContext( worldSize, renderScene, physicsScene, dynamicLayer, hasPrefetchData, terrainClipMap );

	// create streaming thread (could be a task if we had more stack space)
	m_threadData = new CSectorDataStreamingContextThreadData();
	m_thread = new CSectorDataStreamingThread( m_threadData );

	// start thread
	m_thread->InitThread();
#if defined( RED_PLATFORM_ORBIS )
	m_thread->SetAffinityMask( (1 << 0) | (1 << 1) | (1 << 4) | (1 << 5) );
	m_thread->SetPriority( Red::Threads::TP_BelowNormal );
#elif defined( RED_PLATFORM_DURANGO )
	m_thread->SetAffinityMask( (1 << 4) | (1 << 5) | (1 << 6) );
	m_thread->SetPriority( Red::Threads::TP_BelowNormal );
#endif
}

CSectorDataStreaming::~CSectorDataStreaming()
{
	delete m_thread;
	m_thread = nullptr;

	delete m_threadData;
	m_threadData = nullptr;

	delete m_context;
	m_context = nullptr;
}

void CSectorDataStreaming::GetDebugInfo( SSectorStreamingDebugData& outInfo )
{
	outInfo.m_numObjectsRegistered = m_context->GetNumTotalObjects();
	outInfo.m_numObjectsInRange = m_context->GetNumInrangeObjects();
	outInfo.m_numObjectsStreaming = m_context->GetNumStreamingObjects();
	outInfo.m_numObjectsStreamed = m_context->GetNumStreamedObjects();
	outInfo.m_numObjectsLocked = m_context->GetNumLockedObjects();
	outInfo.m_wasOverBudgetLastFrame = false;
}

void CSectorDataStreaming::SetReferencePosition( const Vector& position )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This function must be called from main thread" );

	m_referencePosition = position;
	m_positionValid = true;
}

void CSectorDataStreaming::BeginStreaming()
{
	PC_SCOPE_PIX( BeginStreaming );

	RED_FATAL_ASSERT( ::SIsMainThread(), "This function must be called from main thread" );

	// streaming is disabled
	if ( !Config::cvSectorStreamingEnabled.Get() )
		return; 

	// there's no valid position
	if ( !m_positionValid )
		return;

	// already requested
	if ( m_isStreaming )
		return;

	// start streaming, lock the data
	m_accessLock.Acquire();
	m_isStreaming = true;

	// start the streaming job (on the thread)
	m_thread->Start( *m_context, m_referencePosition );
}

void CSectorDataStreaming::FinishStreaming()
{
	PC_SCOPE_PIX( FinishStreaming );

	RED_FATAL_ASSERT( ::SIsMainThread(), "This function must be called from main thread" );

	// not streaming
	if ( !m_isStreaming )
		return;

	// finish streaming
	m_thread->Finish();

	// no longer streaming
	m_isStreaming = false;
	m_accessLock.Release();

	// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
	{
		const Uint32 numObjectsInRange = m_context->GetNumInrangeObjects();
		const Uint32 numObjectsStreaming = m_context->GetNumStreamingObjects();
		const Uint32 numObjectsStreamed = m_context->GetNumStreamedObjects();
		const Uint32 numObjectsLocked = m_context->GetNumLockedObjects();
		RedIOProfiler::ProfileStreamingSector( numObjectsInRange, numObjectsStreaming, numObjectsStreamed, numObjectsLocked );
	}
#endif
}

Uint32 CSectorDataStreaming::AttachSectorData( const Uint64 contentHash, const CSectorData* sectorData, const Bool isVisible )
{
	PC_SCOPE( AttachSectorData );

	if ( sectorData && sectorData->HasData() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );
		return m_context->AttachSectorData( contentHash, sectorData, isVisible );
	}
	else
	{
		return 0;
	}
}

void CSectorDataStreaming::RemoveSectorData( const Uint32 sectorId )
{
	PC_SCOPE( RemoveSectorData );

	// remove from data
	if ( sectorId )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );
		m_context->RemoveSectorData( sectorId );
	}
}

void CSectorDataStreaming::ToggleSectorDataVisibility( const Uint32 sectorId, const Bool isVisible )
{
	// remove from data
	m_context->ToggleSectorDataVisibility( sectorId, isVisible );
}

void CSectorDataStreaming::ForceStreamForPoint( const Vector& point )
{
	PC_SCOPE_PIX( ForceStreamForPoint );

	static const Bool forceStream = true;

	CTimeCounter timerGlobal;
	
	LOG_ENGINE( TXT("Starting prefetch for position: [%f,%f,%f]"), 
		point.X, point.Y, point.Z );

	// finish current streaming (if any)
	{
		CTimeCounter timer;
 		FinishStreaming();
		LOG_ENGINE( TXT(" 1) Finish streaming: %1.2fms"), timer.GetTimePeriodMS() );
	}

	// force load objects for given location
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );

		// get current states
		const Uint32 numStreamedObjectsStart = m_context->GetNumStreamedObjects();

		// unstream for given location, this DOES NOT triggers resource loading
		{
			PC_SCOPE_PIX( Unstream );

			CTimeCounter timer;
			m_context->SetLoadingLock( true );
			m_context->ProcessAsync( *m_threadData, point, true, forceStream );
			m_context->ProcessSync( *m_threadData, true, forceStream );
			m_context->SetLoadingLock( false );
			LOG_ENGINE( TXT(" 2) Unstream: %1.2fms"), timer.GetTimePeriodMS() );
		}

		// we need to flush rendering to allow shit to be released
		{
			PC_SCOPE_PIX( FlushRender );

			CTimeCounter timer;
			GRender->Flush();
			LOG_ENGINE( TXT(" 3) Flush: %1.2fms"), timer.GetTimePeriodMS() );
		}

		// get current states
		const Uint32 numStreamedObjectsMid = m_context->GetNumStreamedObjects();

		// stream for given location, this triggers resource loading
		{
			PC_SCOPE_PIX( Prefetch );

			CTimeCounter timer;
			m_context->ProcessAsync( *m_threadData, point, true, forceStream );
			m_context->ProcessSync( *m_threadData, true, forceStream );
			LOG_ENGINE( TXT(" 4) Prefetch: %1.2fms"), timer.GetTimePeriodMS() );
		}

		// flush resource loading
		{
			PC_SCOPE_PIX( Flush );

			CTimeCounter timer;
			m_context->FlushResourceLoading();
			LOG_ENGINE( TXT(" 5) Flush: %1.2fms"), timer.GetTimePeriodMS() );
		}

		// finalize streaming - all stuff should be loaded
		{
			PC_SCOPE_PIX( Stream );

			CTimeCounter timer;
			m_context->ProcessAsync( *m_threadData, point, true, forceStream );
			m_context->ProcessSync( *m_threadData, true, forceStream );

			LOG_ENGINE( TXT(" 6) Stream: %1.2fms"), timer.GetTimePeriodMS() );
		}

		// get current states
		const Uint32 numStreamedObjectsAfter = m_context->GetNumStreamedObjects();

		// total stats
		LOG_ENGINE( TXT("Object count: %d old -> %d common -> %d new"),
			numStreamedObjectsStart, numStreamedObjectsMid, numStreamedObjectsAfter );
	}

	// global stats
	LOG_ENGINE( TXT("Total streaming time: %1.2fms"), 
		timerGlobal.GetTimePeriodMS() );
}

void CSectorDataStreaming::SetStreamingLock( const Box* bounds, const Uint32 numBounds )
{
	PC_SCOPE_PIX( SetStreamingLock );
	m_context->SetStreamingLock( bounds, numBounds );
}


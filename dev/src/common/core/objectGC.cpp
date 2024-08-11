/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "configVar.h"
#include "object.h"
#include "objectDiscardList.h"
#include "objectGC.h"
#include "objectMap.h"
#include "objectRootSet.h"
#include "objectReachability.h"
#include "loadingJobManager.h"
#include "messagePump.h"
#include "..\redMemoryFramework\redMemorySystemMemoryStats.h"
#include "objectGC_Legacy.h"
#include "objectGC_MarkAndSweep.h"
#include "fileSystemProfilerWrapper.h"
#include "..\gpuApiUtils\gpuApiMemory.h"
#include "configVar.h"

#ifdef RED_USE_NEW_MEMORY_SYSTEM
#include "../redMemory/include/metricsUtils.h"
#include "../redMemory/src/systemAllocatorType.h"
#endif

//---

#if defined( RED_PLATFORM_DURANGO ) && defined( USE_PIX )
#define RED_DO_NOT_GC_DURING_CAPTURE
#include <pix.h>
#endif


CObjectGC* GObjectGC = NULL;

//---

namespace Config
{
	// general GC config
	TConfigVar<Bool> cvUseLegacyGC( "Memory/GC", "Legacy", true, Config::eConsoleVarFlag_ReadOnly ); // cannot be changed at runtime
	TConfigVar<Int32, Validation::IntRange<0,INT_MAX> > cvIdleFrameCount( "Memory/GC", "IdleFrameCount", 30 );
	TConfigVar<Int32, Validation::IntRange<0,INT_MAX> > cvDefaultPoolTrigger( "Memory/GC", "DefaultPoolMemoryTrigger", 2208 );
	TConfigVar<Int32, Validation::IntRange<0,INT_MAX> > cvMeshPoolTrigger( "Memory/GC", "MeshPoolMemoryTrigger", 10 );

	// runtime GC hacks
	TConfigVar<Bool> cvSuppress( "Memory/GC/Hacks", "SuppressAll", false ); // suppress all GC
	TConfigVar<Bool> cvReportLeaks( "Memory/GC/Hacks", "DumpLeaks", false );  // report memory leaks into the system
	TConfigVar<Int32> cvForceGCForFrames( "Memory/GC/Hacks", "ForceGCForFrames", 0 ); // force GC for n consecutive frames
}

//---- 

CObjectGC::CObjectGC()
	: m_totalCount( 0 )
	, m_lock( 1, 1 ) // binary semaphore
	, m_nextAllowedGCFrame( 0 )
	, m_frameIndex( 0 )
	, m_strategy( nullptr )
{
	// create GC strategy	
	if ( Config::cvUseLegacyGC.Get() )
	{
		m_strategy.Reset( new CObjectGC_Legacy() );
	}
	else
	{
		m_strategy.Reset( new CObjectsGC_MarkAndSweep() );
	}
}

CObjectGC::~CObjectGC()
{
}

Bool CObjectGC::ShouldPerformGC() const
{
#ifdef RED_DO_NOT_GC_DURING_CAPTURE
	if(  PIXGetCaptureState( ) != 0 )
	{
		return false;
	}
#endif

	// do not perform the emergency GC to often
	if ( m_frameIndex < m_nextAllowedGCFrame )
		return false;

	// GC is suppressed
	if ( m_suppress )
		return false;

#ifndef RED_USE_NEW_MEMORY_SYSTEM

#ifdef NO_EDITOR	// Disable this check in the editor, it has plenty of memory anyway
	// limit the memory used by object
#ifdef RED_PLATFORM_CONSOLE
	const Int64 memorySizeMB = Memory::GetPoolTotalBytesAllocated<  MemoryPool_CObjects  >() >> 20;

	if ( memorySizeMB > (Uint32)Config::cvObjectMemoryTrigger.Get() )
	{
		LOG_CORE( TXT("Reason for emergency GC: used object memory is too high (%d/%d MB)"), memorySizeMB, Config::cvObjectMemoryTrigger.Get() );
		return true;
	}
#endif
#endif

	// call GC if the default pool gets low on memory
	const Uint64 freeMemoryInDefaultPool = ( Memory::GetPoolBudget< MemoryPool_Default >() - Memory::GetPoolTotalBytesAllocated< MemoryPool_Default >() ) >> 20;
	if( freeMemoryInDefaultPool < Config::cvDefaultPoolTrigger.Get() )
	{
		LOG_CORE( TXT("Reason for emergency GC: free memory in default pool is too low (%d/%d MB)"), freeMemoryInDefaultPool, Config::cvDefaultPoolTrigger.Get());
		return true;
	}

#else

#ifdef RED_PLATFORM_CONSOLE

	const Int64 memoryConsumed = red::memory::GetTotalBytesAllocated() - red::memory::GetPoolTotalBytesAllocated< MemoryPool_Audio >();	// Audio is handled differently on durango and orbis. Discard as condition.
	const Int64 memoryConsumedTrigger = static_cast< red::memory::u64 >( Config::cvDefaultPoolTrigger.Get() ) << 20;

	if( memoryConsumedTrigger - memoryConsumed < 0 )
	{
		LOG_CORE( TXT("Reason for emergency GC: Memory Consumed too high (%lld/%lld bytes)"), memoryConsumed, memoryConsumedTrigger );
		
#ifdef RED_PLATFORM_ORBIS
		const Uint64 physicalMemory = red::memory::AcquireSystemAllocator().GetCurrentPhysicalMemoryAvailable() 
			- Red::MemoryFramework::PageAllocator::GetInstance().GetPlatformImplementation().GetDirectMemoryAllocated();
#else
		const Uint64 physicalMemory = red::memory::AcquireSystemAllocator().GetCurrentPhysicalMemoryAvailable();
#endif

		LOG_CORE( TXT("Physical Memory Available: %lld bytes"), physicalMemory );
		return true;
	}

#endif

#endif

	// Call GC if the mesh pool gets low on memory (since objects in GC may have references to render stuff)
#ifdef RED_PLATFORM_CONSOLE
	Red::MemoryFramework::PoolLabel meshPoolLabel = (Red::MemoryFramework::PoolLabel)-1;
#ifdef RED_PLATFORM_DURANGO
	meshPoolLabel = GpuApi::GpuMemoryPool_InPlaceMeshBuffers;
#elif defined( RED_PLATFORM_ORBIS )
	meshPoolLabel = GpuApi::GpuMemoryPool_InPlaceRenderData;
#endif
	Red::MemoryFramework::AllocatorInfo meshPoolInfo;
	GpuApi::GpuApiMemory::GetInstance()->GetPool( meshPoolLabel )->RequestAllocatorInfo( meshPoolInfo );
	const Uint64 freeMemoryInMeshPool = ( meshPoolInfo.GetBudget() - GpuApi::GpuApiMemory::GetInstance()->GetMetricsCollector().GetMetricsForPool( meshPoolLabel ).m_totalBytesAllocated ) >> 20;
	if( freeMemoryInMeshPool < Config::cvMeshPoolTrigger.Get() )
	{
		LOG_CORE( TXT("Reason for emergency GC: free memory in mesh pool is too low (%d/%d MB)"), freeMemoryInMeshPool, Config::cvMeshPoolTrigger.Get());
		return true;
	}
#endif

	// forced GC
	if ( Config::cvForceGCForFrames.Get() > 0 )
	{
		LOG_CORE( TXT("Reason for emergency GC: user request") );
		Config::cvForceGCForFrames.Set( Config::cvForceGCForFrames.Get() - 1 );
		return true;
	}

	// execute GC
	return false;
}

void CObjectGC::Tick()
{
	// internal frame counter
	m_frameIndex += 1;

	// execute GC
	if ( ShouldPerformGC() )
	{
		CollectNow(); 
	}
}

void CObjectGC::RegisterHelper( IObjectGCHelper* helper )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_helpersLock );

	RED_FATAL_ASSERT( helper != nullptr, "No helper provided" );
	RED_FATAL_ASSERT( !m_helpers.Exist(helper), "Helper already registered" );

	m_helpers.PushBack( helper );
}

void CObjectGC::UnregisterHelper( IObjectGCHelper* helper )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_helpersLock );

	RED_FATAL_ASSERT( helper != nullptr, "No helper provided" );
	RED_FATAL_ASSERT( m_helpers.Exist(helper), "Helper not registered" );

	m_helpers.Remove( helper );
}

Bool CObjectGC::LockJobSystem( const Bool force )
{
	PC_SCOPE( SyncWithJobSystem );

	// LEGACY: synchronize GC with the work done on the loading threads
	// TODO: we can use generation counter for object to filter out new objects from GC
	// figure out if we can skip GC even though it's requested
	if ( !force )
	{	
		if ( m_suppress )
		{
			LOG_CORE( TXT("GC skipped: suppressed") );
			return false;
		}

		// Lock the job manager - we won't issue any new jobs during GC
		SJobManager::GetInstance().Lock();

		// wait if we are doing something important
		if ( SJobManager::GetInstance().IsBlockingGC() )
		{
			SJobManager::GetInstance().Unlock();
			LOG_CORE( TXT("GC skipped: got GC blocking jobs") );
			return false;
		}
	}
	else
	{
		// Lock the job manager - we won't issue any new jobs during GC
		SJobManager::GetInstance().Lock();

		// wait for the GC blocking jobs to finish
		if ( SJobManager::GetInstance().IsBlockingGC() )
		{
			CTimeCounter syncTime;

			while ( SJobManager::GetInstance().IsBlockingGC() )
			{
				Red::Threads::SleepOnCurrentThread( 0 );
			}

			LOG_CORE( TXT("GC sync: %1.2fms"), syncTime.GetTimePeriodMS() );
		}
	}

	// allowed
	return true;
}

void CObjectGC::Collect()
{
	Perform( false );
}

void CObjectGC::CollectNow()
{
	Perform( true );
}

void CObjectGC::Perform( const Bool force /*= false*/ )
{
	PC_SCOPE( GC );
	RED_FATAL_ASSERT( ::SIsMainThread(), "GC should be called only from main thread" );

	RedIOProfilerBlock ioBlock( TXT("GC") );

	// Totally no GC
	if ( Config::cvSuppress.Get() )
	{
		ERR_CORE( TXT("GC IS SUPPRESSED - NOT CALLING - OOM CAN HAPPEN") );
		return;
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();
	// Inform helpers that the GC is about to start
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_helpersLock );
		for ( auto* helper : m_helpers )
		{
			helper->OnGCStarting();
		}
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();
	// Do not accumulate non-discarded stuff
	GObjectsDiscardList->ProcessList( force );
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Lock GC (binary semaphore - code is not thread safe nor reentrant)
	Red::Threads::CScopedLock< Red::Threads::CSemaphore > lock( m_lock );

	// Lock the job system
	if ( !LockJobSystem( force ) )
	{
		// Inform helpers that the GC has finished
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_helpersLock );
			for ( auto* helper : m_helpers )
			{
				helper->OnGCFinished();
			}
		}
		return;
	}

	// Perform the GC using best possible option
	m_strategy->CollectGarbage( Config::cvReportLeaks.Get() );

	// Update GC frame index
	m_nextAllowedGCFrame = m_frameIndex + Config::cvIdleFrameCount.Get();
	m_totalCount += 1;

	// Unlock job system
	SJobManager::GetInstance().Unlock();

	PUMP_MESSAGES_DURANGO_CERTHACK();
	// In forced GC process the discard list also to cleanup objects
	GObjectsDiscardList->ProcessList( force );
	PUMP_MESSAGES_DURANGO_CERTHACK();

	// Inform helpers that the GC has finished
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_helpersLock );
		for ( auto* helper : m_helpers )
		{
			helper->OnGCFinished();
		}
	}
	PUMP_MESSAGES_DURANGO_CERTHACK();
}

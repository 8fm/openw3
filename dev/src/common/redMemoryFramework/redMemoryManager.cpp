/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryManager.h"
#include "redMemoryFrameworkPlatform.h"
#include "redMemoryAllocator.h"
#include "redMemoryLog.h"
#include "redMemoryCrashReporter.h"
#include "redMemoryAssert.h"
#include "redMemoryFillHook.h"

namespace Red { namespace MemoryFramework {

////////////////////////////////////////////////////////////////////
// Platform-specific flags for default pools
#if defined( RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API )
	const Red::System::Uint32 c_DefaultStaticPoolFlags = Allocator_NoPhysicalReserved | Allocator_StaticSize;
	const Red::System::Uint32 c_DefaultOverflowPoolFlags = Allocator_NoPhysicalReserved | Allocator_StaticSize;
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
	const Red::System::Uint32 c_DefaultStaticPoolFlags = Allocator_FlexibleMemory | Allocator_UseOnionBus | Allocator_AccessCpuReadWrite | Allocator_StaticSize;
	const Red::System::Uint32 c_DefaultOverflowPoolFlags = Allocator_DirectMemory | Allocator_UseOnionBus | Allocator_AccessCpuReadWrite | Allocator_StaticSize;
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
	const Red::System::Uint32 c_DefaultStaticPoolFlags = Allocator_StaticSize | Allocator_AccessCpu | Allocator_Cached | RED_MEMORY_DURANGO_STATIC_ALLOCID;
	const Red::System::Uint32 c_DefaultOverflowPoolFlags = Allocator_StaticSize | Allocator_UseDebugMemory | RED_MEMORY_DURANGO_OVERFLOW_ALLOCID;
#endif

////////////////////////////////////////////////////////////////////
// CTor
//
MemoryManager::MemoryManager( Red::System::MemSize staticPoolSize, Red::System::MemSize overflowPoolSize )
	: m_staticPool(nullptr)
	, m_overflowPool(nullptr)
	, m_defaultHookList(nullptr)
	, m_userPoolHookList(nullptr)
{
#ifndef RED_FINAL_BUILD
	// Initialise the crash reporter
	CrashReporter::GetInstance().Initialise();
#endif

	// Default pools are created as soon as possible
	InitialiseDefaultPools( staticPoolSize, overflowPoolSize );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Register pool name / class name lookup table pointers with the metrics collector
	m_metricsCollector.SetMetricsNameLookupTables( &m_memoryPoolNames, &m_memoryClassNames, &m_memoryClassGroups );
#endif
}

////////////////////////////////////////////////////////////////////
// DTor
//	Be very careful about shutting down the manager as static allocations
//  from CRT / external libs can outlive the static scope of the game itself
//	This is particularly obvious with setlocale() as it can allocate into static / named
//	pools
MemoryManager::~MemoryManager()
{
	ReleaseDefaultPools();
	DestroyAllNamedPools();
}

////////////////////////////////////////////////////////////////////
// OnFrameStart
//
void MemoryManager::OnFrameStart( )
{
	m_metricsCollector.OnFrameStart();
}

////////////////////////////////////////////////////////////////////
// ResetMetrics
//
void MemoryManager::ResetMetrics( )
{
	m_metricsCollector.ResetMetrics();
}


////////////////////////////////////////////////////////////////////
// OnFrameEnd
//
void MemoryManager::OnFrameEnd( )
{
	m_metricsCollector.OnFrameEnd();
}

////////////////////////////////////////////////////////////////////////
// GetRegisteredPoolCount
//	How many pools are registered with this manager
Red::System::Uint16 MemoryManager::GetRegisteredPoolCount()
{
	return m_allocators.GetAllocatorCount();
}

////////////////////////////////////////////////////////////////////////
// GetPoolLabelForIndex
//	Get a the pool label for a particular registered pool
PoolLabel MemoryManager::GetPoolLabelForIndex( Red::System::Uint16 poolIndex )
{
	return m_allocators.GetLabelForIndex( poolIndex );
}

////////////////////////////////////////////////////////////////////
// AnnotateSystemMemoryAreas
//	Labels mapped memory areas with pool names on platforms that support it
void MemoryManager::AnnotateSystemMemoryAreas()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	class MemoryAddressAnnotator : public AllocatorWalker
	{
	public:
		MemoryAddressAnnotator( const System::AnsiChar* poolName )
			: m_poolName( poolName )
		{ 
		}			
		virtual void OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size )
		{
			PageAllocator::GetInstance().AnnotateMemoryRegion( address, size, m_poolName );
		}
	private:
		const System::AnsiChar* m_poolName;
	};

	for( Red::System::Uint16 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		PoolLabel theLabel = m_allocators.GetLabelForIndex( poolIndex );
		IAllocator* allocator = m_allocators.GetAllocatorByLabel( theLabel );
		if( allocator )
		{
			System::AnsiChar poolName[32] = { '\0' };
			m_memoryPoolNames.GetNameByLabel( theLabel, poolName, 32 );

			MemoryAddressAnnotator annotator( poolName );
			allocator->WalkAllocator( &annotator );
		}
	}
#endif
}

////////////////////////////////////////////////////////////////////
// GetMetricsCollector
//
AllocationMetricsCollector& MemoryManager::GetMetricsCollector()
{
	return m_metricsCollector;
}

////////////////////////////////////////////////////////////////////
// BeginMetricsDump
//
void MemoryManager::BeginMetricsDump( const Red::System::Char* filename )
{
	m_metricsCollector.BeginMetricsDump( filename );
	
	for( Red::System::Uint16 allocIndex = 0; allocIndex < k_MaximumPools; ++allocIndex )
	{
		PoolLabel theLabel = m_allocators.GetLabelForIndex( allocIndex );
		if( m_allocators.GetAllocatorByLabel( theLabel ) )
		{
			m_metricsCollector.WriteAllocatorHeader( theLabel, m_allocators.GetAllocatorByLabel( theLabel ) );
		}
	}

	m_metricsCollector.EndMetricsDumpHeader();
}

////////////////////////////////////////////////////////////////////
// EndMetricsDump
//
void MemoryManager::EndMetricsDump( )
{
	m_metricsCollector.EndMetricsDump();
}

////////////////////////////////////////////////////////////////////
// EndMetricsDump
//
Red::System::Bool MemoryManager::IsDumpingMetrics()
{
	return m_metricsCollector.IsDumpingMetrics();
}

////////////////////////////////////////////////////////////////////
// RegisterUserPoolHook
//	Adds a hook that runs on user pools only
void MemoryManager::RegisterUserPoolHook( MemoryHook* theHook )
{
	MemoryHook* lastHook = m_userPoolHookList;
	while( lastHook && lastHook->GetNextHook() != nullptr )
	{
		lastHook = lastHook->GetNextHook();
	}

	if( !lastHook )
	{
		m_userPoolHookList = theHook;
	}
	else
	{
		lastHook->SetNextHook( theHook );
	}
}

////////////////////////////////////////////////////////////////////
// AddDefaultPoolHook
//	
void MemoryManager::AddDefaultPoolHook( MemoryHook* hook )
{
	MemoryHook* lastHook = m_defaultHookList;
	while( lastHook && lastHook->GetNextHook() != nullptr )
	{
		lastHook = lastHook->GetNextHook();
	}

	if( !lastHook )
	{
		m_defaultHookList = hook;
	}
	else
	{
		lastHook->SetNextHook( hook );
	}
}

////////////////////////////////////////////////////////////////////
// ShouldCreateOverflowPool
//	Returns true if an overflow pool should be created (platform / config dependent)
Bool MemoryManager::ShouldCreateOverflowPool()
{
#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL
	return PageAllocator::GetInstance().IsExtraDebugMemoryAvailable();
#else
	return false;		// Never create overflow pool in final builds
#endif
}

////////////////////////////////////////////////////////////////////
// InitialiseDefaultPools
//	Create any pools required on startup.
//	Unfortunately we can't do much if this goes wrong as it is called
//	from the constructor
void MemoryManager::InitialiseDefaultPools( Red::System::MemSize staticPoolSize, Red::System::MemSize overflowPoolSize )
{
	// A bit nasty, but we can't make these global since globals aren't initialised by CRT yet
	typedef MemoryFiller<> FillDefaultPoolsWithZero;
	static FillDefaultPoolsWithZero s_MemFiller( FillDefaultPoolsWithZero::ZeroOnAllocate | FillDefaultPoolsWithZero::ZeroOnReallocate );
	AddDefaultPoolHook( &s_MemFiller );

	// Static Pool
	if( staticPoolSize > 0 )
	{
		m_staticPool = m_allocatorCreator.CreateAllocator< RedMemoryStaticPoolAllocator >();
		RED_MEMORY_ASSERT( m_staticPool, "Failed to create static memory pool. Out of system memory?" );

		EAllocatorInitResults result = m_staticPool->Initialise( RedMemoryStaticPoolAllocator::CreationParameters( staticPoolSize, staticPoolSize, staticPoolSize ), c_DefaultStaticPoolFlags );
		RED_UNUSED( result );
		RED_MEMORY_ASSERT( result == AllocInit_OK, "Failed to initialise static allocator" );
	}

	if( overflowPoolSize > 0 && ShouldCreateOverflowPool() )
	{
		// Overflow Pool
		m_overflowPool = m_allocatorCreator.CreateAllocator< RedMemoryOverflowAllocator >();
		RED_MEMORY_ASSERT( m_overflowPool, "Failed to create overflow memory pool. Out of system memory?" );

		EAllocatorInitResults result = m_overflowPool->Initialise( RedMemoryOverflowAllocator::CreationParameters( overflowPoolSize, overflowPoolSize, overflowPoolSize ), c_DefaultOverflowPoolFlags );
		RED_UNUSED( result );
		RED_MEMORY_ASSERT( result == AllocInit_OK, "Failed to initialise overflow allocator" );
	}
}

////////////////////////////////////////////////////////////////////
// ReleaseDefaultPools
//	Destroy any pools created on startup
void MemoryManager::ReleaseDefaultPools()
{
	if( m_staticPool != nullptr )
	{
		m_staticPool->Release();
		m_allocatorCreator.DestroyAllocator< RedMemoryStaticPoolAllocator >( m_staticPool );
	}

	if( m_overflowPool != nullptr )
	{
		m_overflowPool->Release();
		m_allocatorCreator.DestroyAllocator< RedMemoryOverflowAllocator >( m_overflowPool );
	}
}

////////////////////////////////////////////////////////////////////
// DestroyAllNamedPools
//	Release all pool allocators
void MemoryManager::DestroyAllNamedPools()
{
	for( Red::System::Int16 poolIndex = k_MaximumPools; poolIndex >= 0; --poolIndex )
	{
		PoolLabel theLabel = m_allocators.GetLabelForIndex( poolIndex );
		IAllocator* toDestroy = m_allocators.GetAllocatorByLabel( theLabel );
		if( toDestroy )
		{
			toDestroy->Release();
			m_allocators.RemoveAllocatorByLabel( theLabel );
		}
	}
}

////////////////////////////////////////////////////////////////////
// DestroyNamedPool
//	Destroy a pool. Note that the destructor of the pool is not called
void MemoryManager::DestroyNamedPool( PoolLabel label )
{
	IAllocator* allocToDestroy = m_allocators.GetAllocatorByLabel( label );
	RED_MEMORY_ASSERT( allocToDestroy != nullptr, "Allocator not found" );

	if( allocToDestroy != nullptr )
	{
		allocToDestroy->Release();
		m_allocators.RemoveAllocatorByLabel( label );
	}
}

///////////////////////////////////////////////////////////////////
// OnAllocateRegionFailed
//	Called when a region couldn't be allocated.
MemoryRegionHandle MemoryManager::OnAllocateRegionFailed( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, RegionLifetimeHint lifetimeHint )
{
	// First, try to allocate to the overflow pool
	RED_MEMORY_LOG_ONCE( TXT( "Allocate region failed (Size %d, alignment %d, lifetime=%hs" ), (Uint32)allocSize, (Uint32)allocAlignment, lifetimeHint == Region_Shortlived ? "short" : "long" );

	OutOfMemory( label, memoryClass, allocSize, allocAlignment );

	return nullptr;
}

///////////////////////////////////////////////////////////////////
// Region free failed
//	Called when a region could not be free'd. Check the label - may be using the wrong pool
EAllocatorFreeResults MemoryManager::OnFreeRegionFailed( PoolLabel label, MemoryClass memoryClass, MemoryRegionHandle& handle )
{
	// First, try to allocate to the overflow pool
	RED_MEMORY_ASSERT( false, "Free region failed." );

	return Free_NotOwned;
}

///////////////////////////////////////////////////////////////////
// OnFreeFailed
//	Called when a free fails. This usually means a dangling pointer to a destroyed pool
void MemoryManager::OnFreeFailed( PoolLabel label, MemoryClass memoryClass, const void* ptr, EAllocatorFreeResults reason )
{
	const Red::System::AnsiChar* poolName = GetMemoryPoolName( label );
	const Red::System::AnsiChar* className = GetMemoryClassName( memoryClass );

	Red::System::AnsiChar crashBufferAnsi[ 1024 * 4 ] = {'\0'};
	Red::System::Int32 charsWritten = 0;

	charsWritten += Red::System::SNPrintF( crashBufferAnsi, ARRAY_COUNT( crashBufferAnsi ) - charsWritten,  "*** A call to Free() failed in Pool %hs (MemClass %hs)!\n", poolName, className );
	charsWritten += Red::System::SNPrintF( crashBufferAnsi + charsWritten, ARRAY_COUNT( crashBufferAnsi ) - charsWritten,  "Result: 0x%x, Address: %p\n" RED_PRIsize_t, reason, ptr );
	
	MetricsCallstack callStack( 1 );
	for( Red::System::Int32 csIndex = 0; csIndex < callStack.GetCallstackDepth(); ++csIndex )
	{
		if( ARRAY_COUNT( crashBufferAnsi ) - charsWritten > 0 )
		{
			Red::System::Char callStackTxt[128] = {'\0'};
			callStack.GetAsString( csIndex, callStackTxt, 128 );
			Red::System::AnsiChar callStackTxtAnsi[128] = {'\0'};
			Red::System::StringConvert( callStackTxtAnsi, callStackTxt, 128 );
			charsWritten += Red::System::SNPrintF( crashBufferAnsi + charsWritten, ARRAY_COUNT( crashBufferAnsi ) - charsWritten,  "\t%hs\n", callStackTxtAnsi );
		}
	}

	Red::System::Char crashBuffer[ 1024 * 4 ] = { 0 };
	Red::System::StringConvert( crashBuffer, crashBufferAnsi, charsWritten );
	CrashReporter::GetInstance().WriteCrashData( crashBuffer, charsWritten * sizeof( Red::System::Char ) );

	RED_MEMORY_HALT( "A call to MemoryManager::Free has failed. Possible dangling pointer!" );
}

///////////////////////////////////////////////////////////////////
// ReleaseFreeMemoryToSystem
//	Attempt to free memory back to the OS.
Red::System::MemSize MemoryManager::ReleaseFreeMemoryToSystem( PoolLabel label )
{
	IAllocator* theAllocator = m_allocators.GetAllocatorByLabel( label );
	if( theAllocator && !( theAllocator->GetFlags() & Allocator_StaticSize ) )
	{
		return theAllocator->ReleaseFreeMemoryToSystem( m_metricsCollector.GetAllocatorAreaCallback( label ) );
	}

	return 0;
}

///////////////////////////////////////////////////////////////////
// Attempt to free memory back to the OS. This should be called in low-memory situations
//	No guarantees to how much (if any) it will manage to release
Red::System::MemSize MemoryManager::ReleaseFreeMemoryToSystem()
{
	Red::System::MemSize memoryFreed = 0u;
	for( Red::System::Uint16 allocIndex = 0; allocIndex < k_MaximumPools; ++allocIndex )
	{
		PoolLabel thePoolLabel = m_allocators.GetLabelForIndex( allocIndex );
		IAllocator* theAllocator = m_allocators.GetAllocatorByLabel( thePoolLabel );
		if( theAllocator && !( theAllocator->GetFlags() & Allocator_StaticSize ) )
		{
			memoryFreed += theAllocator->ReleaseFreeMemoryToSystem( m_metricsCollector.GetAllocatorAreaCallback( thePoolLabel ) );
		}
	}

	if( memoryFreed > 0 )
	{
		RED_MEMORY_LOG( TXT( "%" ) RED_PRIWsize_t TXT( " bytes freed back to the system" ), memoryFreed );
	}

	return memoryFreed;
}

///////////////////////////////////////////////////////////////////
// OutOfMemory
//	We could not fulfill an allocation request, throw out some errors!
void MemoryManager::OutOfMemory( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocSize, Red::System::MemSize alignment )
{
	// Write to the crash reporter first
	const Red::System::MemSize c_crashReportMaxLength = 1024 * 24;
	Red::System::Char crashReportDataBuffer[ c_crashReportMaxLength ] = {'\0'};
	Red::System::Char* reportBuffer = crashReportDataBuffer;
	Red::System::Int32 charsWritten = 0;

#define WRITE_OUT_OF_MEMORY_CRASH_BUFFER(fmt,...)										\
	charsWritten = Red::System::SNPrintF( reportBuffer,									\
    reinterpret_cast< Red::System::MemUint >( crashReportDataBuffer ) + c_crashReportMaxLength - reinterpret_cast< Red::System::MemUint >( reportBuffer ), \
	fmt, ##__VA_ARGS__ );	reportBuffer += charsWritten;

	// Have to do some fiddling here as we can only get internal names as AnsiChar
	Red::System::AnsiChar errorText[256] = {'\0'};
	const Red::System::AnsiChar* poolName = GetMemoryPoolName( label );
	const Red::System::AnsiChar* className = GetMemoryClassName( memoryClass );

	const Red::System::AnsiChar* c_errorFormat = "***** OUT OF MEMORY! *****\n\nFailed to allocate %" RED_PRIsize_t " bytes from pool '%hs' (Memory class '%hs', alignment %" RED_PRIsize_t ")\n\n";
	Red::System::Int32 charsPrinted = Red::System::SNPrintF( errorText, ARRAY_COUNT( errorText ), c_errorFormat, allocSize, poolName, className, alignment );
	Red::System::StringConvert( reportBuffer, errorText, charsPrinted );
	reportBuffer += charsPrinted - 1;	// Ignore the EOF

	MetricsCallstack callStack( 1 );
	for( Red::System::Int32 csIndex = 0; csIndex < callStack.GetCallstackDepth(); ++csIndex )
	{
		Red::System::Char callStackTxt[128] = {'\0'};
		callStack.GetAsString( csIndex, callStackTxt, 128 );
		WRITE_OUT_OF_MEMORY_CRASH_BUFFER( TXT( "\t%ls\n" ), callStackTxt );
	}
	CrashReporter::GetInstance().WriteCrashData( crashReportDataBuffer, 
												( reinterpret_cast< Red::System::MemUint >( reportBuffer ) - reinterpret_cast< Red::System::MemUint >( crashReportDataBuffer ) ) * sizeof( Red::System::Char ) );

	// Output to the log (reuse crash report buffer for ansi->unicode conversion)
	Red::System::StringConvert( crashReportDataBuffer, errorText, charsPrinted );
	RED_MEMORY_LOG( TXT( "%ls" ), crashReportDataBuffer );

	// Allow the pool to output any addition info that may be useful for debugging
	RED_MEMORY_LOG( TXT( "***********************************************************************" ) );
	RED_MEMORY_LOG( TXT( "Extended Pool Information:" ) );
	IAllocator* thePool = m_allocators.GetAllocatorByLabel( label );
	if( thePool )
	{
		thePool->OnOutOfMemory();
	}
	RED_MEMORY_LOG( TXT( "***********************************************************************" ) );

	// Output manager-wide usage stats
	AllocationMetricsCollector::LogToMemoryLog logDevice;
	m_metricsCollector.DumpPoolMemoryReport< AllocationMetricsCollector::LogToMemoryLog >( logDevice, "OutOfMemory" );
	m_metricsCollector.DumpClassMemoryReport< AllocationMetricsCollector::LogToMemoryLog >( logDevice, "OutOfMemory" );

	m_metricsCollector.EndMetricsDump();
	RED_LOG_FLUSH();

	if( ShouldBreakOnOOM( GetPool( label ) ) )
	{
		RED_MEMORY_HALT( "Out of Memory!" );

#ifdef RED_FINAL_BUILD
		// We force a crash in final builds, just so we can see this is a OOM situation.
		// On shipping, we should most likely disable this, unless we will be getting crash dumps from live machines
		int* forceCrashNow = nullptr;
		*forceCrashNow = 0xF0000000;
#endif
	}
}

///////////////////////////////////////////////////////////////////
// RegisterPoolName
//	Register a pool name (for metrics)
void MemoryManager::RegisterPoolName( PoolLabel label, const Red::System::AnsiChar* poolName )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_memoryPoolNames.RegisterName( label, poolName );
#endif
}

///////////////////////////////////////////////////////////////////
// RegisterClassName
//	Register a class name (for metrics)
void MemoryManager::RegisterClassName( MemoryClass memoryClass, const Red::System::AnsiChar* className )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_memoryClassNames.RegisterName( memoryClass, className );
#endif
}

///////////////////////////////////////////////////////////////////
// RegisterClassGroup
//	Register a memory class group for metrics. memClasses array is copied internally
void MemoryManager::RegisterClassGroup( const AnsiChar* groupName, MemoryClass* memClasses, Uint32 count )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_memoryClassGroups.AddGroupDefinition( groupName, memClasses, count );
#endif
}

///////////////////////////////////////////////////////////////////
// GetMemoryClassName
const Red::System::AnsiChar* MemoryManager::GetMemoryClassName( const MemoryClass memoryClass ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	static Red::System::AnsiChar name[ 128 ];
	if ( m_memoryClassNames.GetNameByIndex( memoryClass, name, ARRAY_COUNT(name) ) )
	{
		return name;		
	}
	else
#endif
	{
		return "Unknown";
	}	
}

///////////////////////////////////////////////////////////////////
// GetMemoryPoolName
const Red::System::AnsiChar* MemoryManager::GetMemoryPoolName( const PoolLabel memoryPool ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	static Red::System::AnsiChar name[ 128 ];
	if ( m_memoryPoolNames.GetNameByIndex( memoryPool, name, ARRAY_COUNT(name) ) )
	{
		return name;
	}
	else
#endif
	{
		return "Unknown";
	}
}

} }

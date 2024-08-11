#include "build.h"
#include "memory.h"
#include "../redMemoryFramework/redMemoryFillHook.h"
#include "memoryHooks.h"
#include "atomicSharedPtr.h"

// Define this to catch uninitialised data after alloc
//#define ENABLE_UNINITIALISED_DATA_CATCHER

// Define this to add sentinels to the end of all allocations + test for overruns
//#define ENABLE_MEMORY_OVERRUN_CHECKS

// Memory fill-on-free. Puts 0xcdcdcd... in free memory
//#define ENABLE_MEMORY_FILL_ON_FREE

namespace CoreMemory {

	using namespace ::Red::MemoryFramework;

	//////////////////////////////////////////////////////////////////////
	// Memory dump debug text helper

#ifndef RED_FINAL_BUILD
	const Uint32 c_memoryDumpDebugStringLength = 256;
	struct TLSDebugStringData 
	{
		AnsiChar m_debugString[ c_memoryDumpDebugStringLength ];
		MemSize m_debugStringLength;
	};
	RED_TLS static TLSDebugStringData s_memoryDumpTLSData;		// Store one string per thread so we dont get conflicts

	Uint32 ScopedMemoryDebugStringCallback( AnsiChar* targetString, Uint32 targetStrLength )
	{
		if( s_memoryDumpTLSData.m_debugStringLength > 0 )
		{
			Red::System::StringConcatenate( targetString, s_memoryDumpTLSData.m_debugString, targetStrLength );
		}
		return static_cast< Uint32 >( s_memoryDumpTLSData.m_debugStringLength );
	}
#endif

	ScopedMemoryDebugString::ScopedMemoryDebugString( const AnsiChar* dbgText )
	{
#ifndef RED_FINAL_BUILD
		Red::System::StringCopy( s_memoryDumpTLSData.m_debugString, dbgText, c_memoryDumpDebugStringLength );
		s_memoryDumpTLSData.m_debugStringLength = Red::System::StringLength( dbgText );
#endif
	}

	ScopedMemoryDebugString::~ScopedMemoryDebugString()
	{
#ifndef RED_FINAL_BUILD
		s_memoryDumpTLSData.m_debugStringLength = 0;
#endif
	}

	//////////////////////////////////////////////////////////////////////
	// Core Memory Hooks

	struct CoreMemoryFillSelector
	{
		// Used to filter on pool / memory class for zero-fill on allocate
		RED_INLINE static Bool ShouldFillMemory( PoolLabel label, MemoryClass memoryClass ) 
		{ 

#ifndef NO_EDITOR
			if ( memoryClass == MC_PhysX )
				return true;
#endif
			RED_UNUSED( label );
			return memoryClass < MC_No_Memset_DO_NOT_USE_MOVE_OR_REMOVE; // see comment in memoryclasses.h
		}
	};

	// Note! The order of hooks is important to ensure things work correctly
	void InitialiseCoreMemoryHooks()
	{
#ifndef RED_USE_NEW_MEMORY_SYSTEM
		// Memory-fill hook. Fills allocated memory with 0
		typedef Red::MemoryFramework::MemoryFiller< CoreMemoryFillSelector > SelectiveMemoryFiller;
		static SelectiveMemoryFiller s_memoryFiller( SelectiveMemoryFiller::ZeroOnAllocate | SelectiveMemoryFiller::ZeroOnReallocate );
		SRedMemory::GetInstance().RegisterUserPoolHook( &s_memoryFiller );

#ifdef ENABLE_UNINITIALISED_DATA_CATCHER
		// Uninitialise data hook. Fills allocated memory with 0xcdcdcdcd
		static UninitialiseDataCatcher s_uninitializeDataCatcher;
		SRedMemory::GetInstance().RegisterUserPoolHook( &s_uninitializeDataCatcher );
#endif

#ifdef ENABLE_MEMORY_OVERRUN_CHECKS
		// Stomp catcher. Adds sentinel data to end of allocations
		static MemoryStompHook s_memoryStompCatcher;
		SRedMemory::GetInstance().RegisterUserPoolHook( &s_memoryStompCatcher );
#endif

#ifdef ENABLE_MEMORY_FILL_ON_FREE
		// Debug fill-on-free. Puts 0xcdcdcd into deleted data
		typedef Red::MemoryFramework::MemoryFiller<> MemoryFillDebug;
		static MemoryFillDebug s_memoryFillCatcher( MemoryFillDebug::DebugOnFree );
		SRedMemory::GetInstance().RegisterUserPoolHook( &s_memoryFillCatcher );
#endif

#endif
	}

	//////////////////////////////////////////////////////////////////////
	// Scoped memory reporter
	CScopedMemoryDump::CScopedMemoryDump( const Red::System::Char* titleText, Red::MemoryFramework::PoolLabel pool )
		: m_poolId( pool )
	{
		Red::System::StringCopy( m_reportTitle, titleText, 256 );
		PopulateMetrics( m_startMetrics );
	}

	CScopedMemoryDump::~CScopedMemoryDump()
	{
#ifndef RED_USE_NEW_MEMORY_SYSTEM
		Red::MemoryFramework::RuntimePoolMetrics endMetrics;
		PopulateMetrics( endMetrics );

		LOG_CORE( TXT( "Memory Diff Report for '%ls'" ), m_reportTitle );
		SRedMemory::GetInstance().GetMetricsCollector().DumpRuntimeMemoryDifference( m_startMetrics, endMetrics );
#endif
	}

	void CScopedMemoryDump::PopulateMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics )
	{
#ifndef RED_USE_NEW_MEMORY_SYSTEM
		if( m_poolId != -1 )
		{
			metrics = SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( m_poolId );
		}
		else
		{
			SRedMemory::GetInstance().GetMetricsCollector().PopulateAllMetrics( metrics );
		}
#endif
	}

	/////////////////////////////////////////////////////////////////////
	// Memory pool parameters interface
	CMemoryPoolParameters::CMemoryPoolParameters()
	{
		for( Uint32 poolIndex = 0; poolIndex < Red::MemoryFramework::k_MaximumPools; poolIndex++ )
		{
			m_poolCreationParameters[ poolIndex ] = nullptr;
		}
	}

	const Red::MemoryFramework::IAllocatorCreationParameters* CMemoryPoolParameters::GetPoolParameters( Red::MemoryFramework::PoolLabel poolLabel )
	{
		return m_poolCreationParameters[ poolLabel ];
	}

	void CMemoryPoolParameters::SetPoolParameters( Red::MemoryFramework::PoolLabel poolLabel, const Red::MemoryFramework::IAllocatorCreationParameters* params )
	{
		m_poolCreationParameters[ poolLabel ] = params;
	}

	void CMemoryPoolParameters::ForEachPoolParameter( std::function< void(Red::MemoryFramework::PoolLabel, const Red::MemoryFramework::IAllocatorCreationParameters*) > callback )
	{
		for( Uint32 poolIndex = 0; poolIndex < Red::MemoryFramework::k_MaximumPools; poolIndex++ )
		{
			if( m_poolCreationParameters[ poolIndex ] != nullptr )
			{
				callback( poolIndex, m_poolCreationParameters[ poolIndex ] );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Auto-generate pool name / memory class name lookups for the metrics system
	void RegisterMemoryMetricsNames()
	{
#ifndef RED_USE_NEW_MEMORY_SYSTEM
		// Pool Names
		#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	Memory::RegisterPoolName( PoolName, #PoolName );
			#include "memoryPools.h"
		#undef DECLARE_MEMORY_POOL
#endif

		// Class Names
		#define DECLARE_MEMORY_CLASS( ClassName )	Memory::RegisterClassName( ClassName, #ClassName );
			#include "memoryClasses.h"
		#undef DECLARE_MEMORY_CLASS

		// Class Groups
		#define BEGIN_MEMORY_CLASS_GROUP(grpName)	{	\
			const AnsiChar* groupNamePtr = grpName;		\
			Red::MemoryFramework::MemoryClass memClassArray[ Red::MemoryFramework::k_MaximumMemoryClasses ] = { 0 };	\
			Uint32 classCount = 0;
		#define DECLARE_MEMORY_CLASS( ClassName )	memClassArray[ classCount++ ] = ClassName;
		#define END_MEMORY_CLASS_GROUP	Memory::RegisterClassGroup( groupNamePtr, memClassArray, classCount );	}
			#include "memoryClasses.h"
		#undef BEGIN_MEMORY_CLASS_GROUP
		#undef DECLARE_MEMORY_CLASS
		#undef END_MEMORY_CLASS_GROUP
	}

	//////////////////////////////////////////////////////////////////////
	// Auto-generate pool creation and name lookups 
	EMemoryInitialisationResult Initialise( CMemoryPoolParameters* poolParameters )
	{
		InitialiseCoreMemoryHooks();
		RegisterMemoryMetricsNames();

		RED_FATAL_ASSERT( poolParameters, "Core Initialise called with NULL parameters" );

#ifndef RED_USE_NEW_MEMORY_SYSTEM

#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )																											\
		{																																							\
			const Red::MemoryFramework::IAllocatorCreationParameters* creationParams = poolParameters->GetPoolParameters( PoolName );								\
			if( !creationParams )																																	\
			{																																						\
				LOG_CORE( TXT( "No creation parameters set for pool '%ls'. Skipping pool creation" ), TXT(#PoolName) );													\
			}																																						\
			else																																					\
			{																																						\
				if( INTERNAL_RED_MEMORY_CREATE_POOL( SRedMemory::GetInstance(), CoreMemory, PoolName, *creationParams, Flags ) != MemoryManager_OK )				\
					return MemInit_OutOfMemory;																														\
			}																																						\
		}
		#include "memoryPools.h"
#undef DECLARE_MEMORY_POOL

#ifndef RED_FINAL_BUILD
		SRedMemory::GetInstance().GetMetricsCollector().RegisterAllocationStringCallback( ScopedMemoryDebugStringCallback );
#endif

#endif

		return MemInit_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Use this function to filter any unwanted allocations from the debug allocator
	// This will only be called when CORE_USES_DEBUG_ALLOCATOR is defined
	Bool ShouldUseDebugAllocator( Red::MemoryFramework::PoolLabel pool, Red::MemoryFramework::MemoryClass memClass )
	{
		return true;
	}
}

namespace Red
{
	void DeleteRefCount( AtomicRefCount * refCount )
	{
		delete refCount;
	}
}

//////////////////////////////////////////////////////////////////////////
// Microsoft platforms allow us to define global new / delete in Core.
// PS4 will only let us define them in object files explicitly linked to main
// Therefore, we only define them on MS compilers here
#ifdef RED_COMPILER_MSC
	#define OP_NEW			operator new
	#define OP_NEW_ARRAY	operator new[]
	#define OP_DELETE		operator delete
	#define OP_DELETE_ARRAY operator delete[]
	#define THROWS_BAD_ALLOC throw()
	#include "operatorNewDelete.inl"
#endif

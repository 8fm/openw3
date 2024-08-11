/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "memoryHelpers.h"

#ifdef RED_USE_NEW_MEMORY_SYSTEM
#include "../redMemory/include/metricsUtils.h"
#include "../redMemory/src/oomHandlerIgnore.h"

#endif

#include "../redMemoryFramework/redMemoryFrameworkTypes.h"


namespace CoreMemory
{
	extern void RegisterMemoryMetricsNames();
}

namespace Memory
{
	void RegisterOOMCallback( OutOfMemoryCallback )
	{}

	void UnregisterOOMCallback( OutOfMemoryCallback )
	{}

	void DumpClassMemoryReport( const Red::System::AnsiChar* title )
	{
		Red::MemoryFramework::AllocationMetricsCollector::LogToMemoryLog logDevice;
		GetMetricsCollector().DumpClassMemoryReport( logDevice, title );
	}
	
	void DumpPoolMemoryReport( const Red::System::AnsiChar* title )
	{
		Red::MemoryFramework::AllocationMetricsCollector::LogToMemoryLog logDevice;
		GetMetricsCollector().DumpPoolMemoryReport( logDevice, title );
	}

#ifndef RED_USE_NEW_MEMORY_SYSTEM

	Uint64 GetTotalBytesAllocated()
	{
		return SRedMemory::GetInstance().GetMetricsCollector().GetTotalBytesAllocated();
	}

	Uint32 GetPoolCount()
	{
		return SRedMemory::GetInstance().GetRegisteredPoolCount();
	}

	Uint64 GetTotalAllocations( Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_totalAllocations;
#else
		return 0;
#endif
	}

	Uint64 GetPoolTotalBytesAllocated( Uint32 poolLabel )
	{
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_totalBytesAllocated;
	}
	
	Uint64 GetPoolTotalBytesAllocatedPeak( Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_totalBytesAllocatedPeak;
#else 
		return 0;
#endif
	}

	Uint64 GetPoolTotalAllocations( Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_totalAllocations;
#else 
		return 0;
#endif
	}


	const AnsiChar * GetPoolName( Uint32 poolLabel )
	{
		return SRedMemory::GetInstance().GetMemoryPoolName( poolLabel );
	}

	const AnsiChar * GetMemoryClassName( Uint32 memClass )
	{
		return SRedMemory::GetInstance().GetMemoryClassName( memClass );
	}

	void GetMemoryClassName( Uint32 memClass, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters )
	{
		SRedMemory::GetInstance().GetMetricsCollector().GetMemoryClassName( memClass, buffer, maxCharacters );
	}

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass, Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_allocatedBytesPerMemoryClass[ memClass ];
#else
		return 0;
#endif
	}

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass )
	{
		return SRedMemory::GetInstance().GetMetricsCollector().GetTotalBytesAllocatedForClass( memClass );
	}

	Uint64 GetAllocationPerMemoryClass( Uint32 memClass, Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( poolLabel ).m_allocationsPerMemoryClass[ memClass ];
#else
		return 0;
#endif
	}

	Uint64 ReleaseFreeMemoryToSystem()
	{
		return SRedMemory::GetInstance().ReleaseFreeMemoryToSystem();
	}

	void OnFrameStart()
	{
		SRedMemory::GetInstance().OnFrameStart();
	}
	
	void OnFrameEnd()
	{
		SRedMemory::GetInstance().OnFrameEnd();
	}

	void RegisterCurrentThread()
	{
		
	}

	void Initialize( CoreMemory::CMemoryPoolParameters * param )
	{
		CoreMemory::Initialise( param );
		SRedMemory::GetInstance().AnnotateSystemMemoryAreas();
	}

	Red::MemoryFramework::AllocationMetricsCollector & GetMetricsCollector()
	{
		return SRedMemory::GetInstance().GetMetricsCollector();
	}

	void RegisterPoolName( Uint32 label, const AnsiChar* poolName )
	{
		SRedMemory::GetInstance().RegisterPoolName( label, poolName );
	}

	void RegisterClassName( Uint32 memoryClass, const AnsiChar* className )
	{
		SRedMemory::GetInstance().RegisterClassName( memoryClass, className );
	}

	void RegisterClassGroup( const AnsiChar* groupName, Uint32* memClasses, Uint32 count )
	{
		SRedMemory::GetInstance().RegisterClassGroup( groupName, memClasses, count );
	}

#else

	Uint64 GetPoolBudget( Uint32 label )
	{
		return red::memory::GetPoolBudget( label );
	}

	Uint64 GetTotalBytesAllocated()
	{
		return red::memory::GetTotalBytesAllocated();
	}

	Uint32 GetPoolCount()
	{
		return red::memory::GetPoolCount(); // TODO
	}

	Uint64 GetTotalAllocations( Uint32 /*poolLabel*/ )
	{
		return 0; // TODO
	}

	Uint64 GetPoolTotalBytesAllocated( Uint32 /*poolLabel*/ )
	{
		return 0; // TODO
	}
	
	Uint64 GetPoolTotalBytesAllocatedPeak( Uint32 /*poolLabel*/ )
	{
		return 0; // TODO
	}

	Uint64 GetPoolTotalAllocations( Uint32 /*poolLabel*/ )
	{
		return 0; // TODO
	}

	const AnsiChar * GetPoolName( Uint32 poolLabel )
	{
		return red::memory::GetPoolName( poolLabel );  
	}

	const AnsiChar * GetMemoryClassName( Uint32 memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		static Red::System::AnsiChar name[ 128 ];
		if ( GetMetricsCollector().GetMemoryClassName( memClass, name, ARRAY_COUNT(name) ) )
		{
			return name;		
		}
		else
#endif
		{
			return "Unknown";
		}	
	}

	void GetMemoryClassName( Uint32 memClass, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		GetMetricsCollector().GetMemoryClassName( memClass, buffer, maxCharacters );
#endif	
	}

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass, Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return GetMetricsCollector().GetMetricsForPool( poolLabel ).m_allocatedBytesPerMemoryClass[ memClass ];
#else
		return 0;
#endif
	}

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return GetMetricsCollector().GetTotalBytesAllocatedForClass( memClass );
#else
		return 0;
#endif
	}

	Uint64 GetAllocationPerMemoryClass( Uint32 memClass, Uint32 poolLabel )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return GetMetricsCollector().GetMetricsForPool( poolLabel ).m_allocationsPerMemoryClass[ memClass ];
#else
		return 0;
#endif
	}

	Uint64 ReleaseFreeMemoryToSystem()
	{
		return 0; // TODO
	}

	void OnFrameStart()
	{
		GetMetricsCollector().OnFrameStart();
	}
	
	void OnFrameEnd()
	{
		GetMetricsCollector().OnFrameEnd();
	}

	struct AllocatorDepot
	{
		red::memory::LockingDynamicTLSFAllocator ioAllocator;
		red::memory::LockingDynamicTLSFAllocator umbraAllocator;
		red::memory::DynamicFixedSizeAllocator umbraTCAllocator;
		red::memory::LockingDynamicTLSFAllocator umbraTempAllocator;

#ifdef RED_PLATFORM_ORBIS
		red::memory::LockingDynamicTLSFAllocator audioAllocator;
#endif

		red::memory::OOMHandlerIgnore ignoreOOmHandler;
	};

	static AllocatorDepot s_allocatorDepot;


#ifndef RED_FINAL_BUILD

#define RED_INIT_CORE_MEMORY_POOL( poolType, poolParentType, allocator, budget ) \
	RED_INITIALIZE_MEMORY_POOL( poolType, poolParentType, allocator, budget ); \
	RegisterPoolName( poolType::GetHandle(), #poolType );

#else
#define RED_INIT_CORE_MEMORY_POOL( poolType, poolParentType, allocator, budget ) \
	RED_INITIALIZE_MEMORY_POOL( poolType, poolParentType, allocator, budget ) 
#endif

	void CoreInitializeMemoryPools()
	{
		red::memory::InitializeRootPools();
		red::memory::RegisterCurrentThread();

		CoreMemory::RegisterMemoryMetricsNames();

#ifndef RED_FINAL_BUILD
		RegisterPoolName( MemoryPool_Default::GetHandle(), "MemoryPool_Default" );
#endif

		red::memory::DefaultAllocator & defaultAllocator = red::memory::AcquireDefaultAllocator();

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_CObjects, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 300 )  );

		const Uint64 ioPoolBudget = RED_MEGA_BYTE( 36 );
		const Uint64 umbraPoolBudget = RED_MEGA_BYTE( 128 );

#ifdef RED_PLATFORM_CONSOLE
		const Uint64 ioPoolmaxSize = RED_MEGA_BYTE( 36 );
		const Uint64 umbraMaxSize =  umbraPoolBudget;
#else
		const Uint64 ioPoolmaxSize = RED_MEGA_BYTE( 300 );
		const Uint64 umbraMaxSize = RED_GIGA_BYTE( 2 );
#endif
		const red::memory::DynamicTLSFAllocatorParameter ioPoolParam = 
		{
			&red::memory::AcquireSystemAllocator(),
			ioPoolmaxSize,
			ioPoolmaxSize,
			ioPoolmaxSize,
			red::memory::Flags_CPU_Read_Write
		};

		s_allocatorDepot.ioAllocator.Initialize( ioPoolParam );

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_IO, MemoryPool_Default,	s_allocatorDepot.ioAllocator, ioPoolBudget );
		red::memory::SetPoolOOMHandler< MemoryPool_IO >( &s_allocatorDepot.ignoreOOmHandler );

		const red::memory::DynamicTLSFAllocatorParameter umbraPoolParam = 
		{
			&red::memory::AcquireSystemAllocator(),
			umbraMaxSize,
			umbraPoolBudget,
			RED_MEGA_BYTE( 16 ),
			red::memory::Flags_CPU_Read_Write
		};

		s_allocatorDepot.umbraAllocator.Initialize( umbraPoolParam );

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Umbra, MemoryPool_Default, s_allocatorDepot.umbraAllocator, umbraPoolBudget );

		const Uint64 umbraTomeCollectionBlockSize = RED_MEGA_BYTE( 20 );
		const red::memory::DynamicFixedSizeAllocatorParameter umbraTCPoolParam = 
		{
			&red::memory::AcquireSystemAllocator(),
			static_cast< Uint32 >( umbraTomeCollectionBlockSize ),
			16,
			2,
			2,
			red::memory::Flags_CPU_Read_Write
		};

		s_allocatorDepot.umbraTCAllocator.Initialize( umbraTCPoolParam );

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_UmbraTC, MemoryPool_Default, s_allocatorDepot.umbraTCAllocator, 2 * umbraTomeCollectionBlockSize );

#if defined( RED_PLATFORM_ORBIS )

		const Uint64 audioPoolmaxSize = RED_MEGA_BYTE( 240 );
		const red::memory::DynamicTLSFAllocatorParameter audioPoolParam = 
		{
			&red::memory::AcquireFlexibleSystemAllocator(),
			audioPoolmaxSize,
			audioPoolmaxSize,
			audioPoolmaxSize,
			red::memory::Flags_CPU_Read_Write
		};

		s_allocatorDepot.audioAllocator.Initialize( audioPoolParam );

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Audio, MemoryPool_Default, s_allocatorDepot.audioAllocator, audioPoolmaxSize );

#else

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Audio, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 240 ) );

#endif

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Physics, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 150 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Animation, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 300 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_GameSave, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 15 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Strings, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 50 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Task, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 8 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_FoliageData, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 50 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_SpeedTree, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 128 ) );

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_Debug, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 256 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_RedGui, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 256 ));

		RED_INIT_CORE_MEMORY_POOL( MemoryPool_TerrainEditor, MemoryPool_Default, defaultAllocator, RED_MEGA_BYTE( 256 ) );
		RED_INIT_CORE_MEMORY_POOL( MemoryPool_ScriptCompilation, MemoryPool_Default, defaultAllocator , RED_MEGA_BYTE( 256 ));
	}

	void Initialize( CoreMemory::CMemoryPoolParameters * )
	{
		CoreInitializeMemoryPools();
		red::memory::RegisterCurrentThread();
	}

	void RegisterCurrentThread()
	{
		red::memory::RegisterCurrentThread();
	}

	struct MetricsProxy
	{
		MetricsProxy()
		{
			instance.SetMetricsNameLookupTables( &poolNames, &classNames, &groupNames );
		}

		Red::MemoryFramework::AllocationMetricsCollector instance;
		Red::MemoryFramework::PoolNamesList poolNames;
		Red::MemoryFramework::MemoryClassNamesList classNames;
		Red::MemoryFramework::MemoryClassGroups< Red::MemoryFramework::k_MaximumMemoryClassGroups > groupNames;
	};

	MetricsProxy & GetProxy()
	{
		static MetricsProxy s_proxy;
		return s_proxy;
	}

	Red::MemoryFramework::AllocationMetricsCollector & GetMetricsCollector()
	{
		return GetProxy().instance;
	}

	void RegisterPoolName( Uint32 label, const AnsiChar* poolName )
	{
		GetProxy().poolNames.RegisterName( label, poolName );
	}

	void RegisterClassName( Uint32 memoryClass, const AnsiChar* className )
	{
		GetProxy().classNames.RegisterName( memoryClass, className );
	}

	void RegisterClassGroup( const AnsiChar* groupName, Uint32* memClasses, Uint32 count )
	{
		GetProxy().groupNames.AddGroupDefinition( groupName, memClasses, count );
	}

#endif
}

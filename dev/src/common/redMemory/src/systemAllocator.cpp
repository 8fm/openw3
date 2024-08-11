/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocator.h"
#include "pageAllocator.h"
#include "log.h"

namespace red
{
namespace memory
{
	SystemAllocator::SystemAllocator()
		:	m_commitedMemoryInBytes( 0 ),
			m_pageAllocator( nullptr )
	{}

	SystemAllocator::~SystemAllocator()
	{}

	void SystemAllocator::Initialize( PageAllocator * pageAllocator )
	{
		RED_MEMORY_ASSERT( pageAllocator, "INTERNAL ERROR. PageAllocator is null." );
		m_pageAllocator = pageAllocator;
		OnInitialize();
	}

	VirtualRange SystemAllocator::ReserveVirtualRange( u64 size, u32 flags )
	{
		RED_MEMORY_ASSERT( m_pageAllocator, "INTERNAL ERROR. PageAllocator is null." );
		RED_MEMORY_ASSERT( size, "Cannot reserve range of size 0." );
		return m_pageAllocator->ReserveRange( size, flags );
	}

	void SystemAllocator::ReleaseVirtualRange( const VirtualRange & block )
	{
		RED_MEMORY_ASSERT( m_pageAllocator, "INTERNAL ERROR. PageAllocator is null." );

		if( block != NullVirtualRange() )
		{
			const u64 memoryDecommited = OnReleaseVirtualRange( block );
			atomic::ExchangeAdd64( &m_commitedMemoryInBytes, -static_cast< i64 >( memoryDecommited ) );
			m_pageAllocator->ReleaseRange( block );
		}
	}

	SystemBlock SystemAllocator::Commit( const SystemBlock & block, u32 flags )
	{
		const SystemBlock result = OnCommit( block, flags );
		atomic::ExchangeAdd64( &m_commitedMemoryInBytes, result.size );
		return result;
	}

	void SystemAllocator::Decommit( const SystemBlock & block )
	{
		OnDecommit( block );
		atomic::ExchangeAdd64( &m_commitedMemoryInBytes, -static_cast< i64 >( block.size ) );
	}

	void SystemAllocator::WriteReportToLog() const
	{
#ifdef RED_MEMORY_ENABLE_LOGGING
		RED_MEMORY_LOG( "System Allocator Informations" );
		RED_MEMORY_LOG(  "\tMemory Consumed: %10.02f KB", m_commitedMemoryInBytes / 1024.0f );
#endif
	}

	u64 SystemAllocator::GetTotalPhysicalMemoryAvailable() const
	{
		return OnGetTotalPhysicalMemoryAvailable();
	}

	u64 SystemAllocator::GetCurrentPhysicalMemoryAvailable() const
	{
#ifdef RED_PLATFORM_DURANGO

		TITLEMEMORYSTATUS titleMemoryStatus;
		std::memset( &titleMemoryStatus, 0, sizeof( TITLEMEMORYSTATUS ) );
		titleMemoryStatus.dwLength = sizeof( titleMemoryStatus );
		BOOL result = TitleMemoryStatus( &titleMemoryStatus );

		RED_FATAL_ASSERT( result != 0, "SYSTEM ERROR cannot fetch Durango available memory." );
		RED_UNUSED( result );

		return titleMemoryStatus.ullAvailMem;

#else
		return GetTotalPhysicalMemoryAvailable() - m_commitedMemoryInBytes;
#endif

	}
}
}

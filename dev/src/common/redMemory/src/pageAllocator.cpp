/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pageAllocator.h"
#include "assert.h"
#include "utils.h"

namespace red
{
namespace memory
{
	PageAllocator::PageAllocator()
		:	m_platformPageSize( 0 ),
			m_pageReservedCount( 0 )
	{}

	PageAllocator::~PageAllocator()
	{}

	void PageAllocator::Initialize()
	{
		OnInitialize();
	}

	VirtualRange PageAllocator::ReserveRange( u64 size, u32 flags )
	{
		RED_MEMORY_ASSERT( m_platformPageSize, "INTERNAL ERROR. Page size is 0." );
		RED_MEMORY_ASSERT( IsPowerOf2( m_platformPageSize ), "INTERNAL ERROR. Page size are not power of 2." );

		const VirtualRange range = OnReserveRange( size, m_platformPageSize, flags );
		const atomic::TAtomic32 pageCount = static_cast< atomic::TAtomic32 >( GetVirtualRangeSize( range ) / m_platformPageSize );
		atomic::ExchangeAdd32( &m_pageReservedCount, pageCount ); 
		return range;
	}

	void PageAllocator::ReleaseRange( const VirtualRange & range )
	{
		RED_MEMORY_ASSERT( m_platformPageSize, "INTERNAL ERROR. Page size is 0." );
		RED_MEMORY_ASSERT( IsPowerOf2( m_platformPageSize ), "INTERNAL ERROR. Page size are not power of 2." );

		if( range != NullVirtualRange() )
		{
			OnReleaseRange( range );
			const atomic::TAtomic32 pageCount = static_cast< atomic::TAtomic32 >( GetVirtualRangeSize( range ) / m_platformPageSize );
			atomic::ExchangeAdd32( &m_pageReservedCount, -pageCount ); 
		}
	}

	u32 PageAllocator::GetPageSize() const
	{
		return m_platformPageSize;
	}

	u32 PageAllocator::GetReservedPageCount() const
	{
		return m_pageReservedCount;
	}

	void PageAllocator::SetPageSize( u32 pageSize )
	{
		m_platformPageSize = pageSize;
	}
}
}

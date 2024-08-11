/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_H_
#define _RED_MEMORY_PAGE_ALLOCATOR_H_

#include "virtualRange.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API PageAllocator
	{
	public:
		
		void Initialize();

		VirtualRange ReserveRange( u64 size, u32 flags );
		void ReleaseRange( const VirtualRange & range );

		u32 GetPageSize() const;
		u32 GetReservedPageCount() const;

	protected:

		PageAllocator();
		virtual ~PageAllocator();

		void SetPageSize( u32 pageSize );

	private:

		PageAllocator( const PageAllocator & );
		const PageAllocator & operator=( const PageAllocator & );

		virtual void OnInitialize() = 0;
		virtual VirtualRange OnReserveRange( u64 size, u32 pageSize, u32 flags ) = 0; 
		virtual void OnReleaseRange( const VirtualRange & range ) = 0;
	
		u32 m_platformPageSize;
		atomic::TAtomic32 m_pageReservedCount;
	};
}
}

#endif

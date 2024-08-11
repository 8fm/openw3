/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SYSTEM_ALLOCATOR_H_
#define _RED_MEMORY_SYSTEM_ALLOCATOR_H_

#include "systemBlock.h"
#include "virtualRange.h"

namespace red
{
namespace memory
{
	class PageAllocator;

	class RED_MEMORY_API SystemAllocator
	{
	public:

		SystemAllocator();
		virtual ~SystemAllocator();

		void Initialize( PageAllocator * pageAllocator );

		RED_MOCKABLE VirtualRange ReserveVirtualRange( u64 size, u32 flags );
		RED_MOCKABLE void ReleaseVirtualRange( const VirtualRange & range );
		
		RED_MOCKABLE SystemBlock Commit( const SystemBlock & block, u32 flags );
		RED_MOCKABLE void Decommit( const SystemBlock & block );

		u64 GetTotalPhysicalMemoryAvailable() const;
		u64 GetCurrentPhysicalMemoryAvailable() const;

		void WriteReportToLog() const;

	private:

		SystemAllocator( const SystemAllocator& );
		SystemAllocator& operator=( const SystemAllocator& );

		virtual void OnInitialize() = 0;
		virtual u64 OnReleaseVirtualRange( const VirtualRange & block ) = 0;
		virtual SystemBlock OnCommit( const SystemBlock & block, u32 flags ) = 0;
		virtual void OnDecommit( const SystemBlock & block ) = 0;
		virtual u64 OnGetTotalPhysicalMemoryAvailable() const = 0;

		atomic::TAtomic64 m_commitedMemoryInBytes;
		PageAllocator * m_pageAllocator;
	};
}
}

#endif

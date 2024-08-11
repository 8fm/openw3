/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_THREAD_ID_PROVIDER_HPP_
#define _RED_MEMORY_THREAD_ID_PROVIDER_HPP_

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE ThreadId ThreadIdProvider::GetCurrentId() const
	{

#ifdef RED_PLATFORM_ORBIS

		const ThreadId id = static_cast< ThreadId >( ::scePthreadGetthreadid() );

#else
		// Don't ask ... 
		// I simply manually inlined ::GetCurrentThreadId().
		i64 value = __readgsqword( 0x30 ); // Thread Information Block start here. 
		const u32 id = *reinterpret_cast< u32* >( value + 0x48 ); // Thread Id is offseted by 0x48.

#endif

		return id;
	}
}
}

#endif

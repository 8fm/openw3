/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_SYSTEM_THREADS_H_
#define _RED_SYSTEM_THREADS_H_

#include "os.h"

#if defined( RED_COMPILER_MSC )
#	define RED_TLS __declspec(thread)
#elif defined( RED_COMPILER_CLANG )
#	define RED_TLS __thread
#else
#	error Compiler not supported
#endif

namespace Red
{
	namespace System
	{
		namespace Internal
		{
			// This structure is dangerous and requires careful use, hence part of the internal namespace.
			// Contained within will be platform specific data to identify a thread so that,
			// for instance, a call stack for that particular thread can be retrieved
			struct ThreadId
			{
//FIXME>: Need to better encapsulate than using ::GetCurrentThreadId() everywhere!
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
				ThreadId() : id( 0 ) {}
				ThreadId( DWORD tId ) : id( tId ) {}
				DWORD id;
#elif defined( RED_PLATFORM_ORBIS )
				Uint32 id;
				ThreadId() : id( static_cast< Uint32 >( ::scePthreadGetthreadid() ) ) {}
#else
# error Unsupported platform!
#endif
			public:
				RED_INLINE Bool operator==( const ThreadId& other ) const
				{
					return id == other.id;
				}

				RED_INLINE Bool operator!=( const ThreadId& other ) const
				{
					return !( *this == other );
				}

				RED_INLINE Uint32 AsNumber() const
				{
					return (Uint32) id;
				}

				void InitWithCurrentThread()
				{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
					id = ::GetCurrentThreadId();
#elif defined( RED_PLATFORM_ORBIS )
					id = static_cast< Uint32 >( ::scePthreadGetthreadid() );
#else
# error Unsupported platform!
#endif
				}

				RED_INLINE Bool IsValid() const 
				{
					return id != 0;
				}

				static ThreadId CurrentThread()
				{
					ThreadId threadId;
					threadId.InitWithCurrentThread();
					return threadId;
				}
			};
		}
	}
}

#endif //_RED_SYSTEM_THREADS_H_

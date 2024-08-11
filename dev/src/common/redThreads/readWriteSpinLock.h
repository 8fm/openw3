/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_THREADS_READ_WRITE_SPIN_LOCK_H_
#define _RED_THREADS_READ_WRITE_SPIN_LOCK_H_

#include "redThreadsAtomic.h"

namespace Red
{
namespace Threads
{
	class CRWSpinLock
	{
	public:

		CRWSpinLock();

		void AcquireShared();
		void Acquire();

		void ReleaseShared();
		void Release();

	private:

		void EndTryWrite();

		volatile AtomicOps::TAtomic32 m_lock;
	};
}
}

#include "readWriteSpinLock.inl"

#endif

#include "build.h"

#include "../../common/redThreads/redThreadsThread.h"
#include "../../common/redThreads/redThreadsAtomic.h"

#if 0

using namespace Red::System;
using namespace Red::Threads;

TEST( RWLock, AcquireShared)
{
	CRWLock lock;
	
	EXPECT_NO_FATAL_FAILURE( lock.AcquireReadShared() );
	EXPECT_NO_FATAL_FAILURE( lock.ReleaseReadShared() );
}

TEST( RWLock, AcquireExclusive )
{
	CRWLock lock;
	EXPECT_NO_FATAL_FAILURE( lock.AcquireWriteExclusive() );
	EXPECT_NO_FATAL_FAILURE( lock.ReleaseWriteExclusive() );
}

TEST( RWLock, AcquireSharedRecursive )
{
	CRWLock lock;
	const int lockCount = 1024;
	for ( int i = 0; i < lockCount; ++i )
	{
		EXPECT_NO_FATAL_FAILURE( lock.AcquireReadShared() );
	}

	for ( int i = 0; i < lockCount; ++i )
	{
		EXPECT_NO_FATAL_FAILURE( lock.ReleaseReadShared() );
	}
}

TEST( RWLock, LockWrite )
{
	class CWriterThread : public CThread
	{
	private:
		CRWLock&			m_rwlock;
		CAtomic< Bool >&	m_lockResult;

	public:
		CWriterThread( CRWLock& rwlock, CAtomic< Bool >& lockResult )
			: m_rwlock( rwlock )
			, m_lockResult( lockResult )
		{
		}

		virtual void ThreadFunc() override
		{
			const Bool result = m_rwlock.TryAcquireWriteExclusive();

			if ( result )
			{
				// Should not have been able to lock it, but release it anyway
				m_rwlock.ReleaseWriteExclusive();
			}

			m_lockResult.SetValue( result );
		}
	};

	CRWLock rwlock;
	const Bool expectedWriteLockResult = true; // Successfully acquire a write lock while there's no contention on it
	CAtomic< Bool > writeLockResult( ! expectedWriteLockResult ); // Init to failure value in case the writer thread doesn't change this value

	CThread* writerThread = new CWriterThread( rwlock, writeLockResult );
	writerThread->InitThread();
	writerThread->JoinThread();
	delete writerThread;

	EXPECT_EQ( expectedWriteLockResult, writeLockResult.GetValue() );
}

TEST( RWLock, LockedWrite_FailLockWrite )
{
	class CWriterThread : public CThread
	{
	private:
		CRWLock&			m_rwlock;
		CAtomic< Int32 >&	m_barrierCounter;
		CAtomic< Bool >&	m_lockResult;

	public:
		CWriterThread( CRWLock& rwlock, CAtomic< Int32 >& barrierCounter, CAtomic< Bool >& lockResult )
			: m_rwlock( rwlock )
			, m_barrierCounter( barrierCounter )
			, m_lockResult( lockResult )
		{
		}

		virtual void ThreadFunc() override
		{
			m_barrierCounter.Decrement();

			// Wait for other threads to try contending for the lock, just because
			while ( m_barrierCounter.GetValue() > 0 )
			{
				continue;
			}

			const Bool result = m_rwlock.TryAcquireWriteExclusive();

			if ( result )
			{
				// Should not have been able to lock it, but release it anyway
				m_rwlock.ReleaseWriteExclusive();
			}

			m_lockResult.SetValue( result  );
		}
	};

	const int numThreads = 10;

	CRWLock rwlock;
	CAtomic< Int32 > barrierCounter( numThreads );
	const Bool expectedWriteLockResult = false; // Failed to acquire a write lock while there is still a write lock
	CAtomic< Bool > writeLockResult[ numThreads ];
	for ( int i = 0; i < numThreads; ++i )
	{
		writeLockResult[ i ].SetValue( ! expectedWriteLockResult ); // Init to failure value in case the writer thread doesn't change this value
	}

	// Lock exclusively here so all the writer threads are expected to fail
	EXPECT_NO_FATAL_FAILURE( rwlock.AcquireWriteExclusive() );

	CThread* threads[ numThreads ];
	for ( int i = 0 ; i < numThreads; ++i )
	{
		threads[i] = new CWriterThread( rwlock, barrierCounter, writeLockResult[ i ] );
	}

	for ( int i = 0 ; i < numThreads; ++i )
	{
		threads[i]->InitThread();
	}

	for ( int i = 0 ; i < numThreads; ++i )
	{
		threads[i]->JoinThread();
	}

	for ( int i = 0 ; i < numThreads; ++i )
	{
		delete threads[i];
		threads[i] = nullptr;
	}

	EXPECT_NO_FATAL_FAILURE( rwlock.ReleaseWriteExclusive() );

	Bool finalWriteLockResult = false;
	for ( int i = 0; i < numThreads; ++i )
	{
		// Make sure no thread could lock
		finalWriteLockResult |= writeLockResult[ i ].GetValue();
	}

	EXPECT_EQ( expectedWriteLockResult, finalWriteLockResult );
}

TEST( RWLock, LockedRead_FailLockWrite )
{
	class CReaderThread : public CThread
	{
	private:
		CRWLock&			m_rwlock;
		CAtomic< Int32 >&	m_barrierCounter;

	public:
		CReaderThread( CRWLock& rwlock, CAtomic< Int32 >& barrierCounter )
			: m_rwlock( rwlock )
			, m_barrierCounter( barrierCounter )
		{
		}

		virtual void ThreadFunc() override
		{
			m_rwlock.AcquireReadShared();
			m_barrierCounter.Decrement();
			while ( m_barrierCounter.GetValue() > 0 )
			{
				continue;
			}
			m_rwlock.ReleaseReadShared();
		}
	};

	class CWriterThread : public CThread
	{
	private:
		CRWLock&			m_rwlock;
		CAtomic< Int32 >&	m_barrierCounter;
		CAtomic< Bool >&	m_lockResult;
		
	public:
		CWriterThread( CRWLock& rwlock, CAtomic< Int32 >& barrierCounter, CAtomic< Bool >& lockResult )
			: m_rwlock( rwlock )
			, m_barrierCounter( barrierCounter )
			, m_lockResult( lockResult )
		{
		}

		virtual void ThreadFunc() override
		{
			
			// Wait for the reader threads to lock first
			while ( m_barrierCounter.GetValue() > 1 )
			{
				continue;
			}

			const Bool result = m_rwlock.TryAcquireWriteExclusive();
			m_barrierCounter.Decrement();

			if ( result )
			{
				// Should not have been able to lock it, but release it anyway
				m_rwlock.ReleaseWriteExclusive();
			}

			m_lockResult.SetValue( result );
		}
	};

	const int numReaderThreads = 1;
	const int totalNumThreads = numReaderThreads + 1;
	const int writerThreadIndex = totalNumThreads - 1;

	CRWLock rwlock;
	CAtomic< Int32 > barrierCounter( totalNumThreads );
	const Bool expectedWriteLockResult = false; // Failed to acquire a write lock while there were read locks
	CAtomic< Bool > writeLockResult( ! expectedWriteLockResult ); // Init to failure value in case the writer thread doesn't change this value

	CThread* threads[ totalNumThreads ];
	for ( int i = 0 ; i < numReaderThreads; ++i )
	{
		threads[i] = new CReaderThread( rwlock, barrierCounter );
	}
	for ( int i = 0 ; i < numReaderThreads; ++i )
	{
		threads[i]->InitThread();
	}

	threads[ writerThreadIndex ] = new CWriterThread( rwlock, barrierCounter, writeLockResult );
	threads[ writerThreadIndex ]->InitThread();

	for ( int i = 0 ; i < totalNumThreads; ++i )
	{
		threads[i]->JoinThread();
	}
	for ( int i = 0 ; i < totalNumThreads; ++i )
	{
		delete threads[i];
		threads[i] = nullptr;
	}

	EXPECT_EQ( expectedWriteLockResult, writeLockResult.GetValue() );
}

// Note: apparently run out of being able to make threads (209 of them ) before hitting any max number of concurrent readers
// More a test for the PS4 which supposedly can fail with EBUSY for too many concurrent readers
#if 0
TEST( RWLock, MaxConcurrentReaderThreads )
{
	class CTestThread : public CThread
	{
	private:
		CRWLock&			m_rwlock;
		CAtomic< Int32 >&	m_barrierCounter;

	public:
		CTestThread( CRWLock& rwlock, CAtomic< Int32 >& barrierCounter )
			: m_rwlock( rwlock )
			, m_barrierCounter( barrierCounter )
		{
		}

		virtual void ThreadFunc() override
		{
			const int numRecursiveLocks = 1024;

			for ( int i = 0 ; i < numRecursiveLocks; ++i )
			{
				m_rwlock.AcquireReadShared();
			}

			m_barrierCounter.Decrement();
			while ( m_barrierCounter.GetValue() > 0 )
			{
				continue;
			}

			for ( int i = 0; i < numRecursiveLocks; ++i )
			{
				m_rwlock.ReleaseReadShared();
			}
		}
	};

	const int numThreads = 200;//209; // Orbis
	CRWLock rwlock;
	CAtomic< Int32 > barrierCounter( numThreads );
	CTestThread* threads[ numThreads ];
	for ( int i = 0 ; i < numThreads; ++i )
	{
		threads[i] = new CTestThread( rwlock, barrierCounter );
	}
	for ( int i = 0 ; i < numThreads; ++i )
	{
		threads[i]->InitThread();
	}

 	for ( int i = 0 ; i < numThreads; ++i )
 	{
 		threads[i]->JoinThread();
 	}

 	for ( int i = 0 ; i < numThreads; ++i )
 	{
 		delete threads[i];
		threads[i] = nullptr;
 	}	

	// And see if went into some infinite wait
	EXPECT_TRUE( true );
}
#endif // #if 0

#endif // #if 0
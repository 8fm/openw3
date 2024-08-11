/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redThreads/redThreadsThread.h"
#include "../redThreads/redThreadsAtomic.h"

#include "redIOPlatform.h"
#include "redIOCommon.h"
#include "redIOAsyncFileHandleCache.h"

class CDebugPageFios2;
class CDebugPageAsyncFileHandleCache;

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
REDIO_NAMESPACE_BEGIN
struct SAsyncReadToken;
REDIO_NAMESPACE_END

REDIO_GENERICAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CIOProactor;

//////////////////////////////////////////////////////////////////////////
// CIOProactor
//////////////////////////////////////////////////////////////////////////
class CIOProactor : private Red::Threads::CThread
{
	REDIO_NOCOPY_CLASS( CIOProactor );

public:
	static const Uint32 INVALID_FILE_HANDLE = CAsyncFileHandleCache::INVALID_FILE_HANDLE;
	typedef CAsyncFileHandleCache::TFileHandle TFileHandle;

private:
	typedef Red::Threads::CConditionVariable CConditionVariable;
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	static const Uint32 MAX_ASYNC_OPS = 512;

private:
	virtual void			ThreadFunc() override;

private:
	CAsyncFileHandleCache	m_asyncFileHandleCache;


	mutable CMutex			m_queueMutex;
	typedef Red::Threads::CLightMutex	 TQueueLock;
	
private:
	struct SAsyncOp
	{
		CAsyncFile*			m_asyncFile;
		SAsyncReadToken*	m_pAsyncReadToken;
		Uint32				m_cacheFileHandle;
		Uint32				m_operationId;
		Uint32				m_ioTag;
		Uint32				m_startTick;
	};

	struct SAsyncQueue
	{
		//TQueueLock			m_lock;
		Uint32					m_count;
		SAsyncOp				m_list[ MAX_ASYNC_OPS ];

		SAsyncQueue();

		RED_INLINE Bool HasData() const
		{
			return m_count > 0;
		}

		void Put( const SAsyncOp& put );
		SAsyncOp GetBest( CAsyncFile* lastFile, const Uint64 lastOffset, const Uint32 currentTick );
	};

	SAsyncQueue				m_queues[ eAsyncPriority_Count ];
	Uint32					m_totalOpsInQueue;

	CConditionVariable		m_condQueueWait;

	CAsyncFile*				m_lastRequestFile;
	Uint64					m_lastRequestOffset;

	Uint32					m_currentTick;

private:
	Bool					m_shutdownFlag;

public:
							CIOProactor();
							~CIOProactor();
	Bool					Init();
	void					Shutdown();

public:
	TFileHandle				OpenFile( const Char* filePath, Uint32 asyncFlags = eAsyncFlag_None  );	//!< Returns a file handle, or INVALID_FILE_HANDLE on error.
	void					ReleaseFile( TFileHandle fh );			//!< Releases the file handle. See BeginRead for its lifetime.
	Uint64					GetFileSize( TFileHandle fh );			//!< Returns the file size, or 0 on error.
	const Char*				GetFileName( TFileHandle fh ) const;	//!< Returns the file name

public:
	//! Starts an asynchronous read operation.
	//! The asyncReadToken must remain valid while any reads are pending.
	//! The caller may close the file handle after the BeginRead call and it will remain valid while any reads are pending.
	//! It's an error to call this function again from the asyncReadToken callback if not also returning eCallbackRequest_Finish: 
	//! instead update the asyncReadToken and return the appropriate ECallbackRequest. If you return eCallbackRequest_Finish you
	//! can chain reading from another file by calling BeginRead from the callback.
	//!
	//! @param fh				the file handle to read from
	//! @param asyncReadToken	the initialized asyncReadToken that controls the read operation through its callback
	//! @param priority			the priority of the read operation
	void					BeginRead( TFileHandle fh, SAsyncReadToken& asyncReadToken, EAsyncPriority priority = eAsyncPriority_Normal, Uint32 ioTag = 0 );

private:
	void					DoLoop();
	void					ProcessBeginRead( SAsyncOp& asyncOp );
	void					GetBestOp( SAsyncOp& asyncOp );

private:
	friend class ::CDebugPageFios2;
	friend class ::CDebugPageAsyncFileHandleCache;
	CAsyncFileHandleCache&			GetAsyncFileHandleCacheForDebug() { return m_asyncFileHandleCache; }
	const CAsyncFileHandleCache&	GetAsyncFileHandleCacheForDebug() const { return m_asyncFileHandleCache; }
};

REDIO_GENERICAPI_NAMESPACE_END

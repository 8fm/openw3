/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/utility.h"

#include "redIOPlatform.h"
#include "redIOCommon.h"

#include "redIOFiosFwd.h"
#include "redIOAsyncFileHandleCache.h"

class CDebugPageFios2;
class CDebugPageAsyncFileHandleCache;

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
REDIO_NAMESPACE_BEGIN
struct SAsyncReadToken;
REDIO_NAMESPACE_END

#ifdef RED_PLATFORM_ORBIS

REDIO_ORBISAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// CIndexAllocator
//////////////////////////////////////////////////////////////////////////
class CIndexAllocator
{
private:
	int64_t m_freeIndexMask;

public:
	CIndexAllocator();

	Int32	AllocateIndex();
	void	FreeIndex( Int32 index );
};

//////////////////////////////////////////////////////////////////////////
// CIOProactor
//////////////////////////////////////////////////////////////////////////
class CIOProactor
{
	REDIO_NOCOPY_CLASS( CIOProactor );

public:
	static const Uint32 INVALID_FILE_HANDLE = CAsyncFileHandleCache::INVALID_FILE_HANDLE;
	typedef CAsyncFileHandleCache::TFileHandle TFileHandle;

private:
	static const Uint32		MAX_ASYNC_OPS = 64;

private:
	struct SAsyncOp
	{
		TFileHandle			m_fh;
		SceFiosFH			m_fiosFH;
		SAsyncReadToken*	m_asyncReadToken;
		EAsyncResult		m_asyncResult;
		Uint32				m_numberOfBytesRead;
		Uint32				m_operationId;
		Int8				m_fiosPriority;
		Uint32				m_ioTag;
	};

private:
	CIndexAllocator			m_indexAllocator;
	CAsyncFileHandleCache	m_asyncFileHandleCache;

private:
	static CIOProactor*		s_instance;

private:
	SAsyncOp				m_asyncOps[ MAX_ASYNC_OPS ];

public:
	CIOProactor();
	~CIOProactor();
	Bool					Init();
	void					Shutdown();

public:
	TFileHandle				OpenFile( const Char* filePath, Uint32 asyncFlags = eAsyncFlag_None );		//!< Returns a file handle, or INVALID_FILE_HANDLE on error.
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
	static Int32			OnAsyncOp( void* context, SceFiosOp fiosOp, SceFiosOpEvent event, Int32 err );

private:
	void					ProcessAsyncOp( SAsyncOp& asyncOp );
	void					ReadAsync( SceFiosFH, SAsyncOp& asyncOp );

private:
	friend class ::CDebugPageFios2;
	friend class ::CDebugPageAsyncFileHandleCache;
	CAsyncFileHandleCache&			GetAsyncFileHandleCacheForDebug() { return m_asyncFileHandleCache; }
	const CAsyncFileHandleCache&	GetAsyncFileHandleCacheForDebug() const { return m_asyncFileHandleCache; }
};

REDIO_ORBISAPI_NAMESPACE_END

#endif // RED_PLATFORM_ORBIS

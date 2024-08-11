/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "../redSystem/error.h"
#include "redIOAsyncReadToken.h"
#include "redIOProactorOrbisAPI.h"
#include "../core/fileSystemProfilerWrapper.h"

#ifdef RED_PLATFORM_ORBIS

#include <fios2.h>
#include <sce_atomic.h>
#include <x86intrin.h>
#include "redIOCommon.h"

#define REDIO_DEBUG_PROACTOR 0

REDIO_ORBISAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// CIndexAllocator
//////////////////////////////////////////////////////////////////////////
CIndexAllocator::CIndexAllocator()
{
	m_freeIndexMask = ~0ULL;
}

Int32 CIndexAllocator::AllocateIndex()
{
	for (;;)
	{
		const int64_t index = __tzcnt_u64( m_freeIndexMask );
		if ( index == 64 )
		{
			return -1;
		}
	
		const int64_t indexMask = 1ULL << index;
		if ( ::sceAtomicAnd64( &m_freeIndexMask, ~indexMask) & indexMask )
		{
			return index;
		}
	}
}

void CIndexAllocator::FreeIndex( Int32 index )
{
	RED_FATAL_ASSERT( index >= 0 && index < 64, "Invalid index %u", index );
	const int64_t indexMask = 1ULL << index;
	const int64_t oldValue = ::sceAtomicOr64( &m_freeIndexMask, indexMask );
	RED_FATAL_ASSERT( (oldValue & indexMask ) == 0, "Free on unallocated index %d", index );
	RED_UNUSED(oldValue);
}

//////////////////////////////////////////////////////////////////////////
// FMapSceErrToResult
//////////////////////////////////////////////////////////////////////////
static EAsyncResult FMapSceErrToResult( SceInt32 sceErr )
{
	EAsyncResult asyncResult = eAsyncResult_Error;
	switch (sceErr)
	{
	case SCE_FIOS_OK:
		asyncResult = eAsyncResult_Success;
		break;
	case SCE_FIOS_IN_PROGRESS:
		asyncResult = eAsyncResult_Pending;
		break;
	case SCE_FIOS_ERROR_CANCELLED:
		asyncResult = eAsyncResult_Canceled;
		break;
	default:
		break;
	}

	return asyncResult;
}

static Int8 HIGHEST_FIOS_PRIORITY = 2;
static Int8 LOWEST_FIOS_PRIORITY = -1;

static Int8 MapPriorityToFiosPriority( EAsyncPriority priority )
{
	Int8 fiosPriority = 0;
	switch (priority)
	{
	case eAsyncPriority_Critical:
		break;
		fiosPriority = HIGHEST_FIOS_PRIORITY;
	case eAsyncPriority_High:
		fiosPriority = HIGHEST_FIOS_PRIORITY-1;
		break;
	case eAsyncPriority_Normal:
		fiosPriority = 0;
		break;
		break;
	case eAsyncPriority_Low:
		fiosPriority = LOWEST_FIOS_PRIORITY;
		break;
	default:
		break;
	}

	return fiosPriority;
}

CIOProactor* CIOProactor::s_instance = nullptr;

CIOProactor::CIOProactor()
	:  m_asyncFileHandleCache( REDIO_MAX_FILE_HANDLES, REDIO_MAX_FILE_HANDLES )
{
	RED_FATAL_ASSERT( !s_instance, "Sorry, only one instance allowed at a time!");
	s_instance = this;
}

CIOProactor::~CIOProactor()
{
	s_instance = nullptr;
}

Bool CIOProactor::Init()
{
	return true;
}

void CIOProactor::Shutdown()
{
	//sceFiosCancelAllOps 
}

RED_TLS Int32 GAsyncOpStackDepth = 0;

#ifndef RED_FINAL_BUILD
static RED_TLS Int32 GDebugMaxAsyncOpStackDepth = 0;
#endif

struct SScopedCounter
{
	REDIO_NOCOPY_STRUCT( SScopedCounter );

	Int32& m_counter;

	explicit SScopedCounter( Int32& counter )
		: m_counter( counter )
	{
		++m_counter;
	}

	~SScopedCounter()
	{
		--m_counter;
	}

	operator Int32() const { return m_counter; }
};

Int32 CIOProactor::OnAsyncOp( void* context, SceFiosOp fiosOp, SceFiosOpEvent event, Int32 err )
{
	// FIXME: Need to defer
	SScopedCounter scopedCounter( GAsyncOpStackDepth );
	RED_FATAL_ASSERT( scopedCounter < 64, "Too high recursion level");

#ifndef RED_FINAL_BUILD
	if ( GAsyncOpStackDepth > GDebugMaxAsyncOpStackDepth )
	{
		GDebugMaxAsyncOpStackDepth = GAsyncOpStackDepth;
	}
#endif

	REDIO_ASSERT( fiosOp != SCE_FIOS_OP_INVALID );

	switch ( event )
	{
		// SDK docs on SceFiosOpCallback: "A callback can be called from the caller's thread or from a FIOS2 thread."
	case SCE_FIOS_OPEVENT_COMPLETE:
		{
			Uint32 amountTransferred = 0;
			const EAsyncResult asyncResult = FMapSceErrToResult( err );
			if ( asyncResult == eAsyncResult_Success )
			{
				const SceFiosSize sceActualCount = ::sceFiosOpGetActualCount( fiosOp );
				REDIO_ASSERT( sceActualCount >= 0 && sceActualCount <= ~Uint32(0) );
				amountTransferred = static_cast< Uint32 >( sceActualCount );
			}

			// Free op before user callback, which may request another async read. Note: Will trigger reentrant SCE_FIOS_OPEVENT_DELETE.
			::sceFiosOpDelete( fiosOp );

			SAsyncOp& asyncOp = *reinterpret_cast< SAsyncOp* >( context );
			asyncOp.m_asyncResult = asyncResult;
			asyncOp.m_numberOfBytesRead = amountTransferred;

#ifdef RED_PROFILE_FILE_SYSTEM
			if ( asyncOp.m_operationId )
			{
				RedIOProfiler::ProfileAsyncIOReadEnd( asyncOp.m_operationId );
			}
#endif

			RED_FATAL_ASSERT( s_instance, "Null instance!");
			s_instance->ProcessAsyncOp( asyncOp );
		}
		break;
	case SCE_FIOS_OPEVENT_DELETE:
		break;
	default:
		break;
	}

	return SCE_FIOS_OK;
}

void CIOProactor::ReadAsync( SceFiosFH fiosFH, SAsyncOp& asyncOp )
{
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileAsyncIOReadStart( asyncOp.m_operationId );
#endif

	SceFiosOpAttr attr = SCE_FIOS_OPATTR_INITIALIZER;
	attr.pCallback = &CIOProactor::OnAsyncOp;
	attr.pCallbackContext = &asyncOp;
	attr.priority = asyncOp.m_fiosPriority;
	if ( asyncOp.m_fiosPriority == HIGHEST_FIOS_PRIORITY )
	{
		attr.deadline = SCE_FIOS_TIME_EARLIEST;
	}
	SAsyncReadToken& asyncReadToken = *asyncOp.m_asyncReadToken;
	const SceFiosOp op = ::sceFiosFHPread( &attr, fiosFH, asyncReadToken.m_buffer, asyncReadToken.m_numberOfBytesToRead, asyncReadToken.m_offset );
	RED_FATAL_ASSERT( op != SCE_FIOS_OP_INVALID, "Out of async ops!");
	RED_UNUSED(op);
}

CIOProactor::TFileHandle CIOProactor::OpenFile( const Char* filePath, Uint32 asyncFlags /*= eAsyncFlag_None*/ )
{
	return m_asyncFileHandleCache.Open( filePath, asyncFlags );
}

void CIOProactor::ReleaseFile( TFileHandle fh )
{
	m_asyncFileHandleCache.Release( fh );
}

Uint64 CIOProactor::GetFileSize( TFileHandle fh )
{
	// FIXME: Should cache it, or have a way of knowing it ahead of time
	CAsyncFile* asyncFile = m_asyncFileHandleCache.GetAsyncFile_AddRef( fh );
	if ( ! asyncFile )
	{
		return 0;
	}

	const Uint64 size = asyncFile->GetFileSize();
	m_asyncFileHandleCache.Release( fh );

	return size;
}

const Char* CIOProactor::GetFileName( TFileHandle fh ) const
{
	return m_asyncFileHandleCache.GetFileName( fh );
}

void CIOProactor::BeginRead( TFileHandle fh, SAsyncReadToken& asyncReadToken, EAsyncPriority priority /*= eAsyncPriority_Normal */, Uint32 ioTag /*=0*/ )
{
	CAsyncFile* asyncFile = m_asyncFileHandleCache.GetAsyncFile_AddRef( fh );
	if ( ! asyncFile )
	{
		const FAsyncOpCallback callback = asyncReadToken.m_callback;
		if ( callback )
		{
			callback( asyncReadToken, eAsyncResult_Error, 0 );
		}
		return;
	}

#ifdef RED_PROFILE_FILE_SYSTEM
	const Uint32 asyncOpId = RedIOProfiler::ProfileAllocRequestId();
	RedIOProfiler::ProfileAsyncIOReadScheduled( asyncFile->GetFileHandle(), asyncOpId, asyncReadToken.m_offset, asyncReadToken.m_numberOfBytesToRead, ioTag );
#endif

	const SceFiosFH fiosFH = asyncFile->GetPlatformHandle();

	const Int32 index = m_indexAllocator.AllocateIndex();
	RED_FATAL_ASSERT( index >= 0, "Failed to allocate another asyncOp index!" );
	RED_FATAL_ASSERT( index < ARRAY_COUNT_U32( m_asyncOps ), "Invalid asyncOp index %u", index );
	SAsyncOp& asyncOp = m_asyncOps[ index ];
	asyncOp.m_fh = fh;
	asyncOp.m_fiosFH = fiosFH;
	asyncOp.m_asyncReadToken = &asyncReadToken;
	asyncOp.m_fiosPriority = MapPriorityToFiosPriority( priority );
	asyncOp.m_ioTag = ioTag;

#ifdef RED_PROFILE_FILE_SYSTEM
	asyncOp.m_operationId = asyncOpId;
#else
	asyncOp.m_operationId = 0;
#endif

	ReadAsync( fiosFH, asyncOp );
}

extern RED_TLS Bool GIsFiosCallbackThread;

void CIOProactor::ProcessAsyncOp( SAsyncOp& asyncOp )
{
	// When entering this function, the asyncOp state was not pending but it could leave pending.
	RED_FATAL_ASSERT( asyncOp.m_asyncResult != eAsyncResult_Pending, "AsyncOp pending. Could be asynchronously modified!" );

	const SceFiosFH fiosFH = asyncOp.m_fiosFH;
	SAsyncReadToken& asyncReadToken = *asyncOp.m_asyncReadToken;

	// We don't update the asyncOp itself. The async select updates it when the op was finished pending.
	// Use these new variables exclusively to avoid accidentally using asyncOp after it's again in a pending state
	EAsyncResult newAsyncResult = asyncOp.m_asyncResult;
	Uint32 newNumberOfBytesRead = asyncOp.m_numberOfBytesRead;

	if ( newAsyncResult == eAsyncResult_Success )
	{
		FAsyncOpCallback callback = asyncReadToken.m_callback;
		ECallbackRequest callbackResult = callback ? callback( asyncReadToken, eAsyncResult_Success, newNumberOfBytesRead )
			: eCallbackRequest_Finish;

		if ( callbackResult == eCallbackRequest_More )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			const Uint32 asyncOpId = RedIOProfiler::ProfileAllocRequestId();
			asyncOp.m_operationId = asyncOpId;
#endif

			// Assumes the asyncReadToken was updated by the callback. Otherwise we're just rereading the same thing.
			ReadAsync( fiosFH, asyncOp );
			return; // ProcessAsyncOp will be called recursively, get out now
		}
	}
	else
	{
		// Get callback again, since the previous callback may have changed
		FAsyncOpCallback callback = asyncReadToken.m_callback;
		if ( callback )
		{
			// Ignore callback result. Either failed or too late to read more now.
			(void)callback( asyncReadToken, newAsyncResult, newAsyncResult != eAsyncResult_Error ? newNumberOfBytesRead : 0 );
		}
	}

	// Done. Clean up the asyncOp.

	// SDK docs on SceFiosOpCallback: "A callback can be called from the caller's thread or from a FIOS2 thread."
	// So set it back to false in case it was on the calling thread: worst case is the handle will then be released asyncly vs deadlock
	GIsFiosCallbackThread = true;
	m_asyncFileHandleCache.Release( asyncOp.m_fh );
	GIsFiosCallbackThread = false;

	const ptrdiff_t diff = &asyncOp - &m_asyncOps[0];
	RED_FATAL_ASSERT( diff >= 0 && diff < (ptrdiff_t)ARRAY_COUNT_U32(m_asyncOps), "Invalid asyncOp %p", &asyncOp );

#if REDIO_DEBUG_PROACTOR
	asyncOp.m_fh = INVALID_FILE_HANDLE;
	asyncOp.m_fiosFH = nullptr;
	asyncOp.m_asyncReadToken = nullptr;
#endif

	const Uint32 index = static_cast< Uint32 >( diff );
	m_indexAllocator.FreeIndex( index );
}

REDIO_ORBISAPI_NAMESPACE_END

#endif // RED_PLATFORM_ORBIS

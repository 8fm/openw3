/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "..\redIO\redIOAsyncIO.h" // async
#include "..\redIO\redIOAsyncFileHandleCache.h" // async
#include "..\redIO\redIOAsyncReadToken.h"

/// Data prefetcher for sectorized shit
/// TEMPORARY SOLUTION FOR W3 ONLY
class CSectorPrefetchDataLoader
{
public:
	CSectorPrefetchDataLoader( const String& absolutePath );
	~CSectorPrefetchDataLoader();

	/// Are we loading something
	RED_FORCE_INLINE const Bool IsLoading() const { return m_numLoadingBuffers.GetValue() > 0; }

	/// Create new load request, can fail
	typedef std::function< void () > CallbackFunction;
	const Bool RequestLoad( const Uint64 absoluteOffset, const Uint32 size, void* targetMem, CallbackFunction callback );

private:
	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	AsyncFileHandle;

	typedef Red::Threads::CMutex					TLock; 
	typedef Red::Threads::CScopedLock< TLock >		TScopedLock;

	struct LoadingJob
	{
		DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_Engine, __alignof( LoadingJob ) );

		Red::IO::SAsyncReadToken		m_token;
		CSectorPrefetchDataLoader*		m_loader;
		CallbackFunction				m_callback;
	};

	Red::Threads::CAtomic< Int32 >		m_numLoadingBuffers;
	Red::Threads::CAtomic< Int32 >		m_numLoadingBytes;

	AsyncFileHandle						m_asyncFileHandle;

	TLock								m_lock;
	TDynArray< LoadingJob* >			m_pendingJobs;

	static Red::IO::ECallbackRequest OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
};
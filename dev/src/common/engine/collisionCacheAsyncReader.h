/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"
#include "../physics/compiledCollision.h"

/// Helper class used to asynchronously read data from collision cache
/// NOTE: thread safe but only one token can be read at a time
/// TODO: due to crappy threading system this is using it's own threads (mostly to make sure stall will not happen)
class CCollsionCacheAsyncReader
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

public:
	typedef void (*TKickstartFunction)();

	CCollsionCacheAsyncReader();
	~CCollsionCacheAsyncReader();

	// result
	enum EResult
	{
		eResult_Busy,					//!< Loader is busy - we cannot start the loading yet
		eResult_StartedAndWaiting,		//!< We started async loading on the thread - we will have to wait for the results
		eResult_StartedAndFinished,		//!< We started and finished the loading so fast that the result is already here
		eResult_Failed,					//!< We failed to start the load of given data
	};

	// loading task for async reader
	struct SLoadingTask
	{
		Red::IO::CAsyncFileHandleCache::TFileHandle			m_file;					//!< File to load from (in case of disk loading)
		const void*											m_memory;				//!< Memory to load from (in case of memory loading)
		Uint32												m_loadBufferSize;		//!< Size of the requested load buffer
		Uint32												m_readBufferSize;		//!< Size of the requested read buffer
		Uint32												m_offset;				//!< Offset in file
		Uint32												m_sizeOnDisk;			//!< Size of the data on disk (what we load)
		Uint32												m_sizeInMemory;			//!< Size of the data in memory (what we deserialize)
		Uint64												m_dataCRC;				//!< CRC of the data on disk
		std::function< void(CompiledCollisionPtr) >			m_callback;				//!< Callback that return loaded CompiledCollison
		TKickstartFunction									m_kickstart;			//!< Kickstart function to call after work is done (for prefetching)


		RED_INLINE SLoadingTask()
			: m_file( Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
			, m_memory( nullptr )
			, m_offset( 0 )
			, m_sizeOnDisk( 0 )
			, m_sizeInMemory( 0 )
			, m_dataCRC( 0 )
			, m_loadBufferSize( 0 )
			, m_readBufferSize( 0 )
		{}
	};

	// request loading - NOTE: we can only process one request at a time
	EResult StartLoading( const SLoadingTask& loadingTask );

	Bool IsBusy() const;

private:
	typedef TDynArray< Uint8, MC_PhysicalCollision, MemoryPool_Physics >		TInternalBuffer;

	// internal decompression&deserialization thread
	class CDecompressionThread : public Red::Threads::CThread
	{
	public:
		CDecompressionThread();
		~CDecompressionThread();

		enum ESendResult
		{
			eSendResult_Pending,		// decompression task was scheduled
			eSendResult_Finished,		// decompression task finished right away
		};

		ESendResult SendTask( SLoadingTask& task, void* readBuffer, void* loadBuffer, volatile Bool* isLoading );

		void SendKillSignal();

	private:
		volatile Bool						m_exit;
		Red::Threads::CSemaphore			m_start;

		SLoadingTask*						m_task;
		void*								m_readBuffer;
		void*								m_loadBuffer;
		volatile Bool*						m_isLoading;

		virtual void ThreadFunc();

		// decompress and deserialize collision data
		static CompiledCollisionPtr DecompressData( const SLoadingTask& task, void* loadData, void* readData );
	};

	Red::IO::SAsyncReadToken				m_readToken;		//!< Reading token
	SLoadingTask							m_task;				//!< Current task
	volatile Bool							m_isLoading;		//!< Are we loading ?

	Red::Threads::CMutex					m_lock;				//!< Internal access lock

	CDecompressionThread*					m_thread;			//!< Decompression thread

	TInternalBuffer							m_readBuffer;		//!< Internal read buffer - used for reading compressed data
	TInternalBuffer							m_loadBuffer;		//!< Internal load buffer - used for decompression and loading uncompressed data

	// ensure we will support given collision cache
	void EnsureSupport( const SLoadingTask& loadingTask );

	// loading callbacks
	static Red::IO::ECallbackRequest OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
};


/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheReadOnly.h"
#include "collisionCacheAsyncReader.h"

#include "../core/configVar.h"

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >		cvSmallCollisionDataPreloadTreshold( "Collision", "SmallCollisionDataPreloadTreshold", 12000, eConsoleVarFlag_Developer );
	TConfigVar< Bool  >											cvUsePreloadedCollisionData( "Collision", "UsePreloadedCollisionData", true, eConsoleVarFlag_Developer );
	TConfigVar< Bool  >											cvHackDisableAPEXResourceLoading( "Collision", "HackDisableAPEXResourceLoading", false, eConsoleVarFlag_Developer );
}

namespace Helper
{
	class CCollsionCacheAsyncSharedReader
	{
	public:
		CCollsionCacheAsyncSharedReader()
			: m_reader( nullptr )
			, m_useCount( 0 )
		{};

		static CCollsionCacheAsyncSharedReader& GetInstance()
		{
			static CCollsionCacheAsyncSharedReader theInstance;
			return theInstance;
		}

		CCollsionCacheAsyncReader* GetReader( CCollisionCacheReadOnly* owner )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

			if ( !m_reader )
			{
				RED_FATAL_ASSERT( m_useCount == 0, "Invalid internal reference count" );
				m_reader = new CCollsionCacheAsyncReader();
			}

			m_owners.PushBack( owner );
			m_useCount += 1;
			return m_reader;
		}

		void ReleaseReader( CCollsionCacheAsyncReader* reader, CCollisionCacheReadOnly* owner )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

			RED_FATAL_ASSERT( m_useCount > 0, "Invalid internal reference count" );
			RED_FATAL_ASSERT( m_reader == reader, "Trying to relase invalid reader" );
			RED_FATAL_ASSERT( m_owners.Exist( owner ), "Trying to relase invalid reader" );

			m_owners.Remove( owner );

			if ( 0 == m_useCount-- )
			{
				delete m_reader;
				m_reader = nullptr;
			}			
		}

		void KickstartMorework()
		{
			// get best work
			CCollisionCacheReadOnly* kickStartCache = nullptr;
			Uint32 kickStartIndex = 0;
			if ( PopFromKickstartList( kickStartCache, kickStartIndex ) )
			{
				// start it
				kickStartCache->KickstartToken( kickStartIndex );
			}
		}

		Bool AddToKickstartList( CCollisionCacheReadOnly* cache, const Uint32 tokenIndex, EColllisionTokenType tokenType )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_kickstartQueueLock );

			KickStartToken token( cache, tokenIndex, tokenType );
			if ( !m_kickstartQueue.Full() )
			{
				// added
				m_kickstartQueue.PushBack( token );
				return true;
			}

			// not added
			return false;
		}

		Bool RemoveFromKickstartList( CCollisionCacheReadOnly* cache, const Uint32 tokenIndex )
		{	
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_kickstartQueueLock );

			// find
			for ( Uint32 i=0; i<m_kickstartQueue.Size(); ++i )
			{
				const auto& item = m_kickstartQueue[i];
				if ( item.m_cache == cache && item.m_index == tokenIndex )
				{
					m_kickstartQueue.RemoveAtFast( i );
					return true;
				}
			}

			// not removed
			return false;
		}

	private:
		CCollsionCacheAsyncReader*				m_reader;
		Uint32									m_useCount;
		TDynArray< CCollisionCacheReadOnly* >	m_owners;

		struct KickStartToken
		{
			CCollisionCacheReadOnly*							m_cache;
			Uint32												m_index;
			EColllisionTokenType		m_type;

			KickStartToken( CCollisionCacheReadOnly* cache, const Uint32 index, EColllisionTokenType tokenType )
				: m_cache( cache )
				, m_index( index )
				, m_type( tokenType )
			{}
		};

		static const Uint32 KICKSTART_QUEUE_SIZE	= 2056;				// how many tokens we can restart
		typedef TStaticArray< KickStartToken, KICKSTART_QUEUE_SIZE >	TKickstartQueue;

		TKickstartQueue					m_kickstartQueue;				//!< Queue of requested token IDs that we could not load because we were busy - a good candididates for prefetch
		Red::Threads::CMutex			m_kickstartQueueLock;			//!< Access lock for the queue

		Red::Threads::CMutex			m_lock;

		Bool PopFromKickstartList( CCollisionCacheReadOnly*& outCache, Uint32 &outTokenIndex )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_kickstartQueueLock );

			// find best - that is not apex
			Int32 bestIndex = -1;
			Int32 bestType = 100;
			for ( Uint32 i=0; i<m_kickstartQueue.Size(); ++i )
			{
				const auto& item = m_kickstartQueue[i];
				if ( bestIndex == -1 || item.m_type < bestType )
				{
					bestType = item.m_type;
					bestIndex = i;
					break;
				}
			}

			// nothing
			if ( bestIndex == -1 )
				return false;

			// pop it
			outCache = m_kickstartQueue[ bestIndex ].m_cache;
			outTokenIndex = m_kickstartQueue[ bestIndex ].m_index;
			m_kickstartQueue.RemoveAtFast( bestIndex );
			return true;
		}
	};
}

Red::Threads::CMutex			GCollisionCacheReadOnlyLock; // global lock

CCollisionCacheReadOnly::CCollisionCacheReadOnly()
	: m_file( Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	, m_mounter( m_staticData, m_preloadedData )
	, m_isLoaded( false )
	, m_isReady( false )
	, m_isFailed( false )
{
	// create shared reading interface
	// NOTE: the reader is shared between all async cache instances to save memory and resources
	m_reader = Helper::CCollsionCacheAsyncSharedReader::GetInstance().GetReader( this );
}

CCollisionCacheReadOnly::~CCollisionCacheReadOnly()
{
	// release shared reader handle
	if ( nullptr != m_reader )
	{
		Helper::CCollsionCacheAsyncSharedReader::GetInstance().ReleaseReader( m_reader, this );
		m_reader = nullptr;
	}

	// close file handle
	if ( m_file != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		Red::IO::GAsyncIO.ReleaseFile( m_file );
		m_file = Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE;
	}
}

Bool CCollisionCacheReadOnly::Initialize( const String& absoluteFilePath )
{
	// open file
	m_file = Red::IO::GAsyncIO.OpenFile( absoluteFilePath.AsChar() );
	if ( m_file == Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		ERR_ENGINE( TXT("Collision cache IO error: unable to open collisio.cache file from '%ls'"), absoluteFilePath.AsChar() );
		return false;
	}

	// start reading the cache data
	m_loadingTimer.ResetTimer();
	m_mounter.StartLoading( m_file, Config::cvSmallCollisionDataPreloadTreshold.Get(), &m_isLoaded );
	return true;
}

ICollisionCache::EResult CCollisionCacheReadOnly::HasCollision( const String& name, Red::System::DateTime time ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GCollisionCacheReadOnlyLock );

	// make sure the access is ready
	if ( !const_cast< CCollisionCacheReadOnly* >( this )->PrepareAccess() )
		return eResult_NotReady;

	// cache is invalid
	if ( m_isFailed )
		return eResult_Invalid;

	// check in the list
	Uint32 tokenIndex = 0;
	const Uint32 fileHash = CalcStringHash( name );
	if ( m_map.Find( fileHash, tokenIndex ) )
	{
		// just a pro-forma check
		const auto& token = m_staticData.m_tokens[ tokenIndex ];
		if ( token.m_dataSizeInMemory > 0 )
		{
			return eResult_Valid;
		}
	}

	// no collision
	return eResult_Invalid;
}

void CCollisionCacheReadOnly::KickstartToken( const Uint32 tokenIndex )
{
	FindCompiled( nullptr, tokenIndex, nullptr );
}

ICollisionCache::EResult CCollisionCacheReadOnly::FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds )
{
	// make sure the file headers are fully loaded and file is ready for access
	if ( !PrepareAccess() )
		return eResult_NotReady;

	// cache is invalid
	if ( m_isFailed )
		return eResult_Invalid;

	// lookup the token data for given file
	Uint32 tokenIndex = 0;
	const Uint32 fileHash = CalcStringHash( name );
	if ( !m_map.Find( fileHash, tokenIndex ) )
		return eResult_Invalid; // file not in cache

	// hack: disable APEX loading
	if ( Config::cvHackDisableAPEXResourceLoading.Get() )
	{
		const auto type = m_staticData.m_tokens[ tokenIndex ].m_collisionType;
		if ( type == RTT_ApexCloth || type == RTT_ApexDestruction )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GCollisionCacheReadOnlyLock );

			RuntimeData& runtimeData = m_runtimeData[ tokenIndex ];
			runtimeData.m_hasFailed = true;

			RemoveFromKickStartQueue( tokenIndex );
			return eResult_Invalid;
		}
	}

	// lock before the access
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GCollisionCacheReadOnlyLock );

	// load using internal function
	return FindCompiled( &result, tokenIndex, bounds );
}

ICollisionCache::EResult CCollisionCacheReadOnly::FindCompiled( CompiledCollisionPtr* result, const Uint32 tokenIndex, Box2* bounds )
{
	// get tokens - both static (from cache) and runtime (loading state)
	const CCollisionCacheData::CacheToken& staticData = m_staticData.m_tokens[ tokenIndex ];
	if( bounds ) *bounds = staticData.m_boundingArea;
	RuntimeData& runtimeData = m_runtimeData[ tokenIndex ];

	// entry has failed loading
	if ( runtimeData.m_hasFailed )
	{
		RemoveFromKickStartQueue( tokenIndex );
		return eResult_Invalid;
	}

	// use the loaded data we have
	CompiledCollisionPtr compiledMesh = runtimeData.m_compiledMesh.Lock();
	if ( compiledMesh )
	{
		// if we are sucessful remove ourselves from the kickstarter queue
		if ( result )
		{
			RemoveFromKickStartQueue( tokenIndex );
			m_collisionReadyContainer.Erase( compiledMesh );
			*result = compiledMesh;
		}

		return eResult_Valid;
	}

	// we are busy doing our shit
	if ( m_reader->IsBusy() )
	{
		AddToKickStartQueue( tokenIndex );
		return eResult_NotReady;
	}

	// prepare loading task
	CCollsionCacheAsyncReader::SLoadingTask loadingTask;
	loadingTask.m_sizeInMemory = staticData.m_dataSizeInMemory;
	loadingTask.m_sizeOnDisk = staticData.m_dataSizeOnDisk;
	loadingTask.m_loadBufferSize = m_staticData.m_header.m_loadBufferSize; // use maximum for the whole file -> less allocations
	loadingTask.m_readBufferSize = m_staticData.m_header.m_readBufferSize; // use maximum for the whole file -> less allocations
	loadingTask.m_dataCRC = staticData.m_diskCRC;
	loadingTask.m_callback = [this,tokenIndex]( CompiledCollisionPtr ptr ){ this->OnCollisionReady( tokenIndex, ptr ); };
	loadingTask.m_kickstart = &OnKickstartMoreWork;

	// try to use the local data from the cache
	const void* preloadedData = m_preloadedData.GetPreloadedData( staticData.m_dataOffset, staticData.m_dataSizeOnDisk );
	if ( preloadedData && Config::cvUsePreloadedCollisionData.Get() )
	{
		// we are loading from the memory
		loadingTask.m_memory = preloadedData;
		loadingTask.m_offset = 0; // we have the right data directly at the "preloadedData" pointer
	}
	else
	{
		// we are loading from a file
		loadingTask.m_file = m_file;
		loadingTask.m_offset = staticData.m_dataOffset;
	}

	// start loading, note that the reader may be busy with other task
	const auto ret = m_reader->StartLoading( loadingTask );
	if ( CCollsionCacheAsyncReader::eResult_Busy == ret )
	{
		// add to the kickstart queue if not full - this way we may be able to prefetch it
		AddToKickStartQueue( tokenIndex );

		// we can't do anything in here
		return eResult_NotReady;
	}
	else if ( ret == CCollsionCacheAsyncReader::eResult_StartedAndWaiting )
	{
		// --- we entered the loading state,
		// after this call the internal state (runtimeState) is no longer safe to look at
		// especially the isLoading flag may already be false, just return eNotReady and hope we will visit the stuff soon
		return eResult_NotReady;
	}
	else if ( ret == CCollsionCacheAsyncReader::eResult_StartedAndFinished )
	{
		compiledMesh = runtimeData.m_compiledMesh.Lock();

		if ( result )
		{
			m_collisionReadyContainer.Erase( compiledMesh );
			*result = compiledMesh;
		}

		// token should not be kickstarted any more
		RemoveFromKickStartQueue( tokenIndex );

		// we finished loading in zero time - possible for small preloaded data that does not require decompression on a thread
		// in here the m_isLoaded should be valid
		RED_FATAL_ASSERT( runtimeData.m_hasFailed == false, "Invalid state encountered even though success was reported" );
		RED_FATAL_ASSERT( compiledMesh, "Invalid state encountered even though success was reported" );
		return eResult_Valid;
	}
	else
	{
		// we have failed to load the data
		return eResult_Invalid;
	}
}

void CCollisionCacheReadOnly::OnKickstartMoreWork()
{
	Helper::CCollsionCacheAsyncSharedReader::GetInstance().KickstartMorework();
}

void CCollisionCacheReadOnly::AddToKickStartQueue( const Uint32 tokenIndex )
{
	if ( !m_kickstartSet.Get( tokenIndex ) )
	{
		const auto tokenType = m_staticData.m_tokens[ tokenIndex ].m_collisionType;
		if ( Helper::CCollsionCacheAsyncSharedReader::GetInstance().AddToKickstartList( this, tokenIndex, tokenType ) )
		{
			// only add it to the kick started list if it was successfully added
			m_kickstartSet.Set( tokenIndex );
		}
	}
}

void CCollisionCacheReadOnly::RemoveFromKickStartQueue( const Uint32 tokenIndex )
{
	if ( m_kickstartSet.Get( tokenIndex ) )
	{
		m_kickstartSet.Clear( tokenIndex );

		if ( !Helper::CCollsionCacheAsyncSharedReader::GetInstance().RemoveFromKickstartList( this, tokenIndex ) )
		{
			ERR_ENGINE( TXT("Token %d was not registered in the kick off list"), tokenIndex );
		}
	}
}

ICollisionCache::EResult CCollisionCacheReadOnly::Compile( CompiledCollisionPtr& result, const ICollisionContent*, const String& name, Red::System::DateTime time, CObject* sourceObject )
{
	// do not compile - use the found result only
	return FindCompiled( result, name, time );
}

Bool CCollisionCacheReadOnly::PrepareAccess()
{
	// already done
	if ( m_isReady )
	{
		return true;
	}

	// lock
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GCollisionCacheReadOnlyLock );

	// already done
	RED_THREADS_MEMORY_BARRIER();
	if ( m_isReady )
	{
		return true;
	}

	// not loaded yet
	if ( !m_isLoaded )
	{
		// is it loading for to long ?
		const Float loadingTimeLimit = 20.0f;
		if ( m_loadingTimer.GetTimePeriod() > loadingTimeLimit )
		{
			m_isFailed = true;
			m_isReady = true;

			LOG_ENGINE( TXT("Collision cache AsyncIO: not ready after %1.2fs, assuming cache is corrupted"), m_loadingTimer.GetTimePeriod() );
			RED_FATAL_ASSERT( false, "Collision cache corrupted" );
			return true;
		}
		else
		{
			LOG_ENGINE( TXT("Collision cache AsyncIO: trying to access collision cache while it's still loading") );
			return false;
		}
	}

	// time the finalization 
	CTimeCounter timer;

	// create runtime data table
	const Uint32 numTokens = m_staticData.m_tokens.Size();
	m_runtimeData.Resize( numTokens );


	// prepare the token map
	m_kickstartSet.Resize( numTokens );
	m_kickstartSet.ClearAll();

	// build string->ID mapping
	m_map.Reserve( numTokens );
	for ( Uint32 index = 0; index < m_staticData.m_tokens.Size(); ++index )
	{
		const CCollisionCacheData::CacheToken& token = m_staticData.m_tokens[ index ];

		// calculate hash
		const AnsiChar* fileName = &m_staticData.m_strings[ token.m_name ];
		const FileKey key = CalcStringHash( fileName );

		// add to map
		if ( !m_map.Insert( key, index ) )
		{
			Uint32 existingIndex = 0;
			if ( m_map.Find( key, existingIndex ) )
			{
				const AnsiChar* otherFileName = &m_staticData.m_strings[ m_staticData.m_tokens[ existingIndex ].m_name ];

				RED_HALT( "Collsion cache key collision between key '%ls' and '%ls'. Entry will not be added.", 
					fileName, otherFileName );
			}
		}
	}

	// stats
	LOG_ENGINE( TXT("Collision cache AsyncIO: finalized loading in %1.2fms"), timer.GetTimePeriodMS() );

	// we are ready now to service requests
	m_isReady = true;
	return true;
}

//---

CCollisionCacheReadOnly::FileKey CCollisionCacheReadOnly::CalcStringHash( const String& str )
{
	return Red::System::CalculateAnsiHash32LowerCase( str.AsChar() );
}

CCollisionCacheReadOnly::FileKey CCollisionCacheReadOnly::CalcStringHash( const StringAnsi& str )
{
	return Red::System::CalculateAnsiHash32LowerCase( str.AsChar() );
}

CCollisionCacheReadOnly::FileKey CCollisionCacheReadOnly::CalcStringHash( const AnsiChar* str )
{
	return Red::System::CalculateAnsiHash32LowerCase( str );
}

Uint32 CCollisionCacheReadOnly::GetNumEntries_Debug()
{
	// make sure the file headers are fully loaded and file is ready for access
	if ( !PrepareAccess() )
		return 0;

	return m_runtimeData.Size();
}

String CCollisionCacheReadOnly::GetEntryPath_Debug( const Uint32 index )
{
	// make sure the file headers are fully loaded and file is ready for access
	if ( !PrepareAccess() )
		return String::EMPTY;

	if ( index < m_runtimeData.Size() )
	{
		const AnsiChar* path = &m_staticData.m_strings[ m_staticData.m_tokens[ index ].m_name ];
		return ANSI_TO_UNICODE( path );
	}

	return String::EMPTY;
}

EColllisionTokenType CCollisionCacheReadOnly::GetEntryType_Debug( const Uint32 index )
{
	// make sure the file headers are fully loaded and file is ready for access
	if ( !PrepareAccess() )
		return RTT_Unknown;

	if ( index < m_staticData.m_tokens.Size() )
	{
		return m_staticData.m_tokens[ index ].m_collisionType;
	}

	return RTT_Unknown;
}

void CCollisionCacheReadOnly::OnCollisionReady( Uint32 token, CompiledCollisionPtr collision )
{
	RED_FATAL_ASSERT( token < m_runtimeData.Size(), "Invalid token." );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GCollisionCacheReadOnlyLock );
	
	RuntimeData& runtimeData = m_runtimeData[ token ];
	runtimeData.m_hasFailed = !collision;
	if( collision )
	{
		runtimeData.m_compiledMesh = collision;
		// ctremblay Those collision cache using polling instead of callback. 
		// I need to keep result alive until next polling... sucks but that's how it is. and its a bit late for W3 to do heavy refactor
		m_collisionReadyContainer.Insert( collision );	 
	}
}

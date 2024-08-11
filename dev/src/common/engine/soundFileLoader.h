/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "../core/loadingJob.h"
#include "../core/contentListener.h"
#include "../core/lazyCacheChain.h"
#include "soundCacheDataFormat.h"

class CDirectory;

// Enum for different types of sound compression
enum ESoundCompression
{
	SOUND_COMPRESSION_PCM = 1,//AKCODECID_PCM,
	SOUND_COMPRESSION_ADPCM = 2,//AKCODECID_ADPCM,
	SOUND_COMPRESSION_OGG = 4,//AKCODECID_VORBIS,
	SOUND_COMPRESSION_XMA = 3,//AKCODECID_XMA,
	SOUND_COMPRESSION_MP2 = 0,//AKCODECID_EXTERNAL_SOURCE,
	SOUND_COMPRESSION_MP3 = 0,//AKCODECID_EXTERNAL_SOURCE,
	SOUND_COMPRESSION_XWMA = 9,//AKCODECID_XWMA
	SOUND_COMPRESSION_ATRAC9 = 12	///< ATRAC-9 encoding

};

struct SSoundCachedResurce
{
	String fileName;
	Uint32 fileSize;
	Uint32 cacheOffset;
	void* temporaryDataBufferHandle;

	SSoundCachedResurce() : fileSize( 0 ), cacheOffset( 0 ), temporaryDataBufferHandle( 0 ) {}
	SSoundCachedResurce( String _fileName, Uint32 _fileSize ) : fileName( _fileName ), fileSize( _fileSize ), cacheOffset( 0 ), temporaryDataBufferHandle( 0 ) {}
	SSoundCachedResurce( String _fileName, EJobPriority _priority, Uint32 _fileSize ) : fileName( _fileName ), fileSize( _fileSize ), cacheOffset( 0 ), temporaryDataBufferHandle( 0 ) {}

	friend IFile& operator<<( IFile& ar, SSoundCachedResurce& val )
	{
		ar << val.fileName;
		ar << val.fileSize;
		return ar;
	}
};

class CSoundBank
{
	friend class CSoundSystem;
	friend class CSoundFileLoader;
	friend class CDebugWindowSound;
	friend class CSoundBanksWhileInitTask;

protected:
	static TDynArray< CSoundBank > m_soundBanks;
	static Red::Threads::CAtomic< Int32 > m_memoryAllocated;
	static Int32 m_memoryAllocationLimit;

	Red::Threads::CAtomic< Uint32 > m_refCount;
	Uint32 m_bankId;
	Uint32 m_size;
	Bool m_loadedAlways;

	CName m_fileName;

	Red::Threads::CAtomic< Int32 > m_isLoaded;
	
	static CSoundBank s_initBank;

public:
	static String s_initBankPath;

public:
	CSoundBank() : m_refCount( 0 ), m_bankId( 0 ), m_isLoaded( -1 ), m_loadedAlways(false) {}
	CSoundBank(const CSoundBank & bank) : m_refCount( bank.m_refCount.GetValue() ), m_isLoaded( bank.m_isLoaded.GetValue() ), m_bankId( bank.m_bankId ), m_loadedAlways( bank.m_loadedAlways ), m_fileName( bank.m_fileName ), m_size( bank.m_size )
	{}
	CSoundBank( const CName& fileName, Uint32 size ) : m_fileName( fileName ), m_bankId( 0 ), m_refCount( 0 ), m_isLoaded( -1 ), m_loadedAlways(false), m_size( size ) {}
	
	CSoundBank& operator=( const CSoundBank& bank )
	{
		m_refCount.SetValue( bank.m_refCount.GetValue() );
		m_bankId = bank.m_bankId;
		m_size = bank.m_size;
		m_loadedAlways = bank.m_loadedAlways;
		m_fileName = bank.m_fileName;
		m_isLoaded.SetValue( bank.m_isLoaded.GetValue() );

		return *this;
	}

	static void ClearBanks();
	static void ReloadSoundbanks();
	static void ShutDown();
	static CSoundBank* FindSoundBank( const CName& fileName );
	static TDynArray< String > GetAvaibleBanks();

	static Uint32 GetMemoryAllocatedSoundbanks() { return m_memoryAllocated.GetValue(); }
	static Uint32 GetSoundBanksCount();
	static const CSoundBank& GetSoundBank(Uint32 index);

	const CName& GetFileName() const { return m_fileName; }
	Bool IsLoaded();
	Bool IsLoadingFinished() { return m_isLoaded.GetValue() != -1; }
	String GetLoadingResultString();
	Bool QueueLoading();
	Bool Unload();
	Bool Reload();

	Uint32 GetRefCount() const;
	Uint32 GetSize() const;
};

//////////////////////////////////////////////////////////////////////////

class CSoundCache
{
public:
	CSoundCache();
	~CSoundCache();

	//! Always check if cache is ready to read (that means, if async loader finished loading cache description)
	Bool IsReady() const;

	//! Get async file handle
	RED_FORCE_INLINE const Red::IO::CAsyncFileHandleCache::TFileHandle GetFile() const { return m_cacheFile; }

	//! Find sound resource in cache
	const CSoundCacheData::CacheToken* Find( const String& fileNameWithPath ) const;

	//! Check if cache has any resource
	RED_FORCE_INLINE Bool Empty() const { return m_soundCacheData.m_header.m_tokenTableCount == 0; }

	//! List all sound banks in this cache
	void ListAllSoundBanks( TDynArray<StringAnsi>& soundBanks );

	//! Start async cache description loading
	static CSoundCache* ProcessCacheAsync( Red::IO::CAsyncFileHandleCache::TFileHandle sourceFile );

private:
	Red::IO::CAsyncFileHandleCache::TFileHandle m_cacheFile;		//!< Cache file
	CSoundCacheData								m_soundCacheData;	//!< Cache description
	volatile Bool								m_isReady;			//!< Is cache loaded by async loader
	CSoundCacheDataAsyncLoader*					m_asyncLoader;		//!< Async loader, created when needs to load, destroyed after

};

//////////////////////////////////////////////////////////////////////////

class CSoundFileLoader
{
	friend class CSoundSystem;

private:
	class CSoundFileLoaderHook* m_hook;
	static Bool m_refreshSounbanks;
	class CSoundBanksRefreshTask* m_task;

	Red::Threads::CAtomic< Bool > m_whileInit;

public:
	CSoundFileLoader() : m_hook( 0 ), m_whileInit( false ), m_task( nullptr ) {}

	void Reset();
	void PreInit( const String& rootPath );
	void Init();
	void Shutdown();

	static void SetRefreshSoundBanksFlag();
	void RefreshSoundBanks();

private:
	void FillFileList( CDirectory* dir, TDynArray< String >& fileList, const String& extension );

};

//////////////////////////////////////////////////////////////////////////

// Helper structure for finding sound resource in cache chain
struct CacheTokenToSoundCachePair
{
	CacheTokenToSoundCachePair()
		: m_token( nullptr )
		, m_cache( nullptr )
	{
		/* Intentionally Empty */
	}

	CacheTokenToSoundCachePair( const CSoundCacheData::CacheToken* token, const CSoundCache* cache )
		: m_token( token )
		, m_cache( cache )
	{
		/* Intentionally Empty */
	}

	CacheTokenToSoundCachePair( const CacheTokenToSoundCachePair& other )
		: m_token( other.m_token )
		, m_cache( other.m_cache )
	{
		/* Intentionally Empty */
	}

	Bool IsSet()
	{
		return m_token != nullptr && m_cache != nullptr;
	}

	const CSoundCacheData::CacheToken* m_token;
	const CSoundCache* m_cache;

};

class CSoundCacheResolver : public IContentListener
{
public:
	CSoundCacheResolver() : m_isEmpty( true ) {};
	static const Uint32 MAX_CACHE_CHAIN_LENGTH = 64;
	typedef Helper::CLazyCacheChain< CSoundCache, MAX_CACHE_CHAIN_LENGTH > CacheChain;

	typedef THashMap< String, CacheTokenToSoundCachePair > NameToSoundResourceHashMap;

public:
	//! Returns cache finding result, so resource can be asynchronously loaded
	CacheTokenToSoundCachePair GetSoundResource( const String& fileNameWithPath );

	//! List all sound banks stored in caches
	Bool ListAllSoundBanks( TDynArray<StringAnsi>& soundBanks );

	//! Get resolver name
	virtual const Char* GetName() const { return TXT("CSoundCacheResolver"); }

	//! If new sound cache is available, add it to CacheChain and start description async loading
	virtual void OnContentAvailable(const SContentInfo& contentInfo );

	//! Check if any cache is loaded
	Bool IsInitialized();

	Bool IsEmpty();
private:
	CacheChain					m_soundCacheChain;			//!< All loaded (or pending for load) caches
	NameToSoundResourceHashMap	m_soundResourceHashMap;		//!< Hash map to speed up sound resource searching
	Bool						m_isEmpty;

};

typedef TSingleton< CSoundCacheResolver > GSoundCacheResolver;

#pragma once

class CSectorPrefetchDataLoader;
class CSectorPrefetchGrid;
class CSectorPrefetchMemoryBuffer;

/// Runtime cache for data for sector prefetch
/// TEMPORARY SOLUTION FOR W3 ONLY
class CSectorPrefetchRuntimeCache
{
public:
	CSectorPrefetchRuntimeCache( const CSectorPrefetchMemoryBuffer& memoryBuffer );
	~CSectorPrefetchRuntimeCache();

	/// Initialize from disk file
	const Bool Initialize( const String& absoluteFilePath );

	/// Fetch new set of data
	const Bool PrecacheCells( const Vector& worldPosition );

	/// Request a data for given hash, data will be decompressed and copied out to specified memory, returns false if there's no data
	/// Returns false if the decompressed data size DOES NOT match the expected value
	typedef std::function< void* ( Uint32 dataSize, Uint16 alignment ) > DataAllocFunction;
	typedef std::function< void ( void* memory ) > DataFreeFunction;
	void* GetCachedData( const Uint64 resourceHash, const Uint32 expectedDataSize, DataAllocFunction allocator, DataFreeFunction deallocator ) const;

	/// is resource with hash exists in cache
	const Bool Exist(const Uint64 resourceHash) const;

private:
	static const Uint32 MAX_ENTRIES			= 4096; // top limit, actual for W3= ~800

	enum CellState
	{
		eCell_NotLoaded = 0,
		eCell_Loading = 1,
		eCell_Ready = 2,
	};

	struct CachedCell
	{
		CellState			m_state;			// state of the cell data
		Uint32				m_dataSize;			// size of the data in the big buffer
		Uint32				m_memoryDataOffset;	// offset to the data in the big buffer in the memory (can be moved around)
		Uint64				m_diskDataOffset;	// offset to the data in the big buffer on disk
		mutable Uint32		m_lastTickUsed;		// last time this cell was used

		RED_FORCE_INLINE CachedCell()
			: m_state( eCell_NotLoaded )
			, m_dataSize( 0 )
			, m_diskDataOffset( 0 )
			, m_memoryDataOffset( 0 )
			, m_lastTickUsed( 0 )
		{}
	};

	typedef Red::Threads::CMutex TLock;
	typedef Red::Threads::CScopedLock< TLock > TScopeLock;

	mutable TLock								m_lock;
	mutable Uint32								m_tickCounter;

	TDynArray< CachedCell >						m_cachedCells;
	THashMap< Uint64, Uint32  >					m_cachedEntries; // remapping of AVAIABLE cache entries to the static entries from cache

	THashSet< Uint64 >							m_allKnownEntries; // debug hash table for all known resources, helps to determine if something should be already cached but it's not

	Uint32										m_memoryUsed;
	Uint32										m_memoryAllocatorPos;

	const CSectorPrefetchMemoryBuffer*			m_memoryBuffer;
	CSectorPrefetchDataLoader*					m_loader;
	CSectorPrefetchGrid*						m_grid;

	THashSet< Uint64 >							m_ignoredBuffers;

	const Bool FreeUnusedCells( const Uint32 amountOfMemoryToFree ); // note: can fail (if we are loading to much data)
	const Bool DefragMemory();
};

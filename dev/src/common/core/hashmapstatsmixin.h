#pragma once

//#include <ctime>

#ifdef EXPOSE_HASH_MAP_STATISTICS_API

#define NUM_ENTRIES 100

struct HashMapStatsData
{
	const char * m_name;

	SYSTEMTIME	m_timeStampStart;
	SYSTEMTIME	m_timeStampEnd;

	Uint32	m_singleEntrySize;

	Uint32	m_threadID;
	Uint32	m_numCreated;

	Uint32	m_numInserts;
	Uint32	m_numRemoves;
	Uint32	m_numRehash;
	Uint32	m_numCopies;
	Uint32	m_numErases;
	Uint32	m_numReads;
	Uint32	m_numAssigns;
	Uint64	m_numHashCalcs;
	Uint32	m_maxElts;

	// Logged on destruction
	Uint32	m_numBuckets;
	Uint32	m_numElts;
	Uint32	m_maxBucketCount;
	Uint32	m_minBucketCount;
	Uint32	m_zeroBucketCount;
};

class CHashMapStatsMixin
{
private:

	static Uint32 m_globalEmptyCounter;
	static Uint32 m_globalCounter;
	static Uint32 m_numInstances;
	static Uint32 m_numEntries;
	static HashMapStatsData m_hashMapStats[ NUM_ENTRIES ];

	mutable HashMapStatsData m_data;

public:

	CHashMapStatsMixin()
	{
		++m_numInstances;
		m_data.m_numCreated = m_globalCounter++;

		m_data.m_threadID		= GetCurrentThreadId();
		GetSystemTime( &m_data.m_timeStampStart );
		m_data.m_numInserts		= 0;
		m_data.m_numRemoves		= 0;
		m_data.m_numRehash		= 0;
		m_data.m_numCopies		= 0;
		m_data.m_numErases		= 0;
		m_data.m_numReads		= 0;
		m_data.m_numAssigns		= 0;
		m_data.m_numHashCalcs	= 0;
		m_data.m_numBuckets		= 0;
		m_data.m_numElts		= 0;
		m_data.m_maxBucketCount	= 0;
		m_data.m_minBucketCount	= 0;
		m_data.m_zeroBucketCount= 0;
		m_data.m_maxElts		= 0;
	}

	template< typename HashMap >
	FORCE_INLINE void OnInsert	( const HashMap & map ) const
	{
		++m_data.m_numInserts;

		if( map.Size() > m_data.m_maxElts )
			m_data.m_maxElts = map.Size();
	}

	FORCE_INLINE void OnRemove	() const
	{
		++m_data.m_numRemoves;
	}

	FORCE_INLINE void OnRehash	() const
	{
		++m_data.m_numRehash;
	}

	FORCE_INLINE void OnCopy	() const
	{
		++m_data.m_numCopies;
	}

	FORCE_INLINE void OnErase	() const
	{
		++m_data.m_numErases;
	}

	FORCE_INLINE void OnRead	() const
	{
		++m_data.m_numReads;
	}

	FORCE_INLINE void OnAssign	() const
	{
		++m_data.m_numAssigns;
	}

	FORCE_INLINE void OnHash	() const
	{
		++m_data.m_numHashCalcs;
	}

	template< typename HashMap >
	FORCE_INLINE void OnDestroy	( const HashMap & map, Uint32 singleEntrySize )
	{
		m_data.m_name				= typeid( Map ).name();
		m_data.m_singleEntrySize	= singleEntrySize;
		m_data.m_numBuckets			= map.NumBuckets();
		m_data.m_numElts			= map.Size();
		m_data.m_maxBucketCount		= map.BucketMaxCount();
		m_data.m_minBucketCount		= map.BucketMinCount();
		m_data.m_zeroBucketCount	= map.BucketZeroCount();

		if( m_data.m_maxElts == 0 )
			++m_globalEmptyCounter;
	}

	~CHashMapStatsMixin()
	{
		ASSERT( m_numInstances > 0 );

		--m_numInstances;

		if( m_numEntries >= NUM_ENTRIES )
		{
			LogStats();

			m_numEntries = 0;
		}

		GetSystemTime( &m_data.m_timeStampEnd );

		if( m_data.m_timeStampEnd.wMinute != m_data.m_timeStampStart.wMinute )
		{
			m_hashMapStats[ m_numEntries++ ] = m_data;
		}

		if( 0 == m_numInstances )
		{
			LogStats( true );
		}
	}

private:

	static void LogStats( Bool bLast = false );
	static void LogEntry( const HashMapStatsData & entry, Uint32 numEntry );

};

class CMapStatsMixin
{
private:

	static Uint32 m_globalEmptyCounter;
	static Uint32 m_globalCounter;
	static Uint32 m_numInstances;
	static Uint32 m_numEntries;
	static HashMapStatsData m_mapStats[ NUM_ENTRIES ];

	mutable HashMapStatsData m_data;

public:

	CMapStatsMixin()
	{
		++m_numInstances;
		m_data.m_numCreated = m_globalCounter++;

		m_data.m_threadID		= GetCurrentThreadId();
		GetSystemTime( &m_data.m_timeStampStart );
		m_data.m_numInserts		= 0;
		m_data.m_numRemoves		= 0;
		m_data.m_numRehash		= 0;
		m_data.m_numCopies		= 0;
		m_data.m_numErases		= 0;
		m_data.m_numReads		= 0;
		m_data.m_numAssigns		= 0;
		m_data.m_numHashCalcs	= 0;
		m_data.m_numBuckets		= 0;
		m_data.m_numElts		= 0;
		m_data.m_maxBucketCount	= 0;
		m_data.m_minBucketCount	= 0;
		m_data.m_zeroBucketCount= 0;
		m_data.m_maxElts		= 0;
	}

	template< typename Map >
	FORCE_INLINE void OnInsert	( const Map & map ) const
	{
		++m_data.m_numInserts;

		if( map.Size() > m_data.m_maxElts )
			m_data.m_maxElts = map.Size();
	}

	FORCE_INLINE void OnRemove	() const
	{
		++m_data.m_numRemoves;
	}

	FORCE_INLINE void OnRehash	() const
	{
		++m_data.m_numRehash;
	}

	FORCE_INLINE void OnCopy	() const
	{
		++m_data.m_numCopies;
	}

	FORCE_INLINE void OnErase	() const
	{
		++m_data.m_numErases;
	}

	FORCE_INLINE void OnRead	() const
	{
		++m_data.m_numReads;
	}

	FORCE_INLINE void OnAssign	() const
	{
		++m_data.m_numAssigns;
	}

	template< typename Map >
	FORCE_INLINE void OnDestroy	( const Map & map )
	{
		m_data.m_name				= typeid( Map ).name();
		m_data.m_singleEntrySize	= 0;
		m_data.m_numBuckets			= 0;
		m_data.m_numElts			= map.Size();
		m_data.m_maxBucketCount		= 0;
		m_data.m_minBucketCount		= 0;
		m_data.m_zeroBucketCount	= 0;

		if( m_data.m_maxElts == 0 )
			++m_globalEmptyCounter;
	}

	~CMapStatsMixin()
	{
		ASSERT( m_numInstances > 0 );

		--m_numInstances;

		if( m_numEntries >= NUM_ENTRIES )
		{
			LogMapStats();

			m_numEntries = 0;
		}

		GetSystemTime( &m_data.m_timeStampEnd );

		if( m_data.m_timeStampEnd.wMinute != m_data.m_timeStampStart.wMinute )
		{
			m_mapStats[ m_numEntries++ ] = m_data;
		}

		if( 0 == m_numInstances )
		{
			LogMapStats( true );
		}
	}

private:

	static void LogMapStats( Bool bLast = false );
	static void LogMapEntry( const HashMapStatsData & entry, Uint32 indEntry );

};

#else

class CHashMapStatsMixin
{
public:

	template< typename HashMap >
	RED_INLINE void OnInsert	( const HashMap& /*map*/ ) const
	{
	}

	RED_INLINE void OnRemove	() const
	{
	}

	RED_INLINE void OnRehash	() const
	{
	}

	RED_INLINE void OnCopy	() const
	{
	}

	RED_INLINE void OnErase	() const
	{
	}

	RED_INLINE void OnRead	() const
	{
	}

	RED_INLINE void OnAssign	() const
	{
	}

	RED_INLINE void OnHash	() const
	{
	}

	template< typename HashMap >
	RED_INLINE void OnDestroy	( const HashMap& /*map*/, Uint32 /*singleEntrySize*/ )
	{
	}
};

#endif //EXPOSE_HASH_MAP_STATISTICS_API


#include "build.h"
#include "hashmapstatsmixin.h"

#ifdef EXPOSE_HASH_MAP_STATISTICS_API

namespace
{
	HANDLE getLogFileHandle()
	{
		static HANDLE hFile = INVALID_HANDLE_VALUE;

		if( hFile == INVALID_HANDLE_VALUE )
		{
			hFile = CreateFile( TEXT( "hash_logger.log" ),
								FILE_APPEND_DATA,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL );

			DWORD lErr = GetLastError();
			if( hFile == INVALID_HANDLE_VALUE )
				ASSERT( false );
		}

		return hFile;
	}

	void Log( const char * msg )
	{
		HANDLE hFile = getLogFileHandle();

		DWORD dummy;

		WriteFile( hFile, msg, Red::System::StringLength( msg ) - 1, &dummy, NULL );
	}

	void flushLogFile()
	{
		FlushFileBuffers( getLogFileHandle() );
	}

	void closeLogFile()
	{
		CloseHandle( getLogFileHandle() );
	}



	HANDLE getMapLogFileHandle()
	{
		static HANDLE hFile = INVALID_HANDLE_VALUE;

		if( hFile == INVALID_HANDLE_VALUE )
		{
			hFile = CreateFile( TEXT( "map_logger.log" ),
								FILE_APPEND_DATA,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL );

			DWORD lErr = GetLastError();
			if( hFile == INVALID_HANDLE_VALUE )
				ASSERT( false );
		}

		return hFile;
	}

	void LogMap( const char * msg )
	{
		HANDLE hFile = getMapLogFileHandle();

		DWORD dummy;

		WriteFile( hFile, msg, Red::System::StringLength( msg ) - 1, &dummy, NULL );
	}

	void flushMapLogFile()
	{
		FlushFileBuffers( getMapLogFileHandle() );
	}

	void closeMapLogFile()
	{
		CloseHandle( getMapLogFileHandle );
	}

}

Uint32 CHashMapStatsMixin::m_globalEmptyCounter = 0;
Uint32 CHashMapStatsMixin::m_numInstances = 0;
Uint32 CHashMapStatsMixin::m_globalCounter = 0;
Uint32 CHashMapStatsMixin::m_numEntries = 0;
HashMapStatsData CHashMapStatsMixin::m_hashMapStats[ NUM_ENTRIES ];

Uint32 CMapStatsMixin::m_globalEmptyCounter = 0;
Uint32 CMapStatsMixin::m_globalCounter = 0;
Uint32 CMapStatsMixin::m_numInstances = 0;
Uint32 CMapStatsMixin::m_numEntries = 0;
HashMapStatsData CMapStatsMixin::m_mapStats[ NUM_ENTRIES ];

//** **********************************
//
void CHashMapStatsMixin::LogStats( Bool bLast )
{
	char buffer[ 2048 ];
	const char * format = "\n\nHash map stats: %d hash maps have been created till now. %d empty entires have been processed till now.\n";
	Red::System::SNPrintF( buffer, 2048, format, m_globalCounter, m_globalEmptyCounter );
	Log( buffer );

	for( Uint32 i = 0; i < m_numEntries; ++i )
	{
		LogEntry( m_hashMapStats[ i ], i );
	}

	flushLogFile();

	if( bLast )
	{
		closeLogFile();
	}
}

//** **********************************
//
void CHashMapStatsMixin::LogEntry( const HashMapStatsData & entry, Uint32 indEntry )
{
	const char * name	= entry.m_name;

	SYSTEMTIME tms	= entry.m_timeStampStart;
	SYSTEMTIME tme	= entry.m_timeStampEnd;

	Uint32 threadID	= entry.m_threadID;

	Uint32 singleEntrySize = entry.m_singleEntrySize;
	Uint32 numInserts	= entry.m_numInserts;
	Uint32 numRemoves	= entry.m_numRemoves;
	Uint32 numRehash	= entry.m_numRehash;
	Uint32 numCopies	= entry.m_numCopies;
	Uint32 numErases	= entry.m_numErases;
	Uint32 numReads	= entry.m_numReads;
	Uint32 numAssigns	= entry.m_numAssigns;
	Uint64 numHash	= entry.m_numHashCalcs;

	Uint32 maxElts	= entry.m_maxElts;
	Uint32 numElts	= entry.m_numElts;
	Uint32 numBuckets	= entry.m_numBuckets;
	Uint32 maxBucket	= entry.m_maxBucketCount;
	Uint32 minBucket	= entry.m_minBucketCount;
	Uint32 zeroBucket	= entry.m_zeroBucketCount;
	Uint32 numEntry	= entry.m_numCreated;

	char buffer[ 2048 ];
	const char * format = "\n\n%02d:%02d:%02d.%03d  | [Ind: %d] [Num: %d] [Single entry size: %d] %s\n%02d:%02d:%02d.%03d  | MaxEntries Entries   Buckets   Empty MaxCnt MinCnt   Insert Remove   Rehash   HashCalls \n%012X  | %10d %7d   %7d   %5d %6d %6d   %6d %6d   %6d   %8ld";
	Red::System::SNPrintF( buffer, 2048, format, tms.wHour, tms.wMinute, tms.wSecond, tms.wMilliseconds, indEntry, numEntry, singleEntrySize, name, tme.wHour, tme.wMinute, tme.wSecond, tme.wMilliseconds, threadID, maxElts, numElts, numBuckets, zeroBucket, maxBucket, minBucket, numInserts, numRemoves, numRehash, numHash );

	if( maxElts == 0 )
	{
		char buf1[ 2048 ];
		Red::System::SNPrintF( buf1, 2048, "%s   #########", buffer );
		Log( buf1 );
	}
	else
	{
		Log( buffer );
	}
}

//** **********************************
//
void CMapStatsMixin::LogMapStats( Bool bLast )
{
	char buffer[ 2048 ];
	const char * format = "\n\nMap stats: %d maps have been created till now. %d empty entires have been processed till now.\n";
	Red::System::SNPrintF( buffer, 2048, format, m_globalCounter, m_globalEmptyCounter );
	LogMap( buffer );

	for( Uint32 i = 0; i < m_numEntries; ++i )
	{
		LogMapEntry( m_mapStats[ i ], i );
	}

	flushMapLogFile();

	if( bLast )
	{
		closeMapLogFile();
	}
}

//** **********************************
//
void CMapStatsMixin::LogMapEntry( const HashMapStatsData & entry, Uint32 indEntry )
{
	const char * name	= entry.m_name;

	SYSTEMTIME tms	= entry.m_timeStampStart;
	SYSTEMTIME tme	= entry.m_timeStampEnd;

	Uint32 threadID	= entry.m_threadID;

	Uint32 singleEntrySize = entry.m_singleEntrySize;
	Uint32 numInserts	= entry.m_numInserts;
	Uint32 numRemoves	= entry.m_numRemoves;
	Uint32 numCopies	= entry.m_numCopies;
	Uint32 numErases	= entry.m_numErases;
	Uint32 numReads	= entry.m_numReads;
	Uint32 numAssigns	= entry.m_numAssigns;

	Uint32 maxElts	= entry.m_maxElts;
	Uint32 numElts	= entry.m_numElts;
	Uint32 numEntry	= entry.m_numCreated;

	char buffer[ 2048 ];
	const char * format = "\n\n%02d:%02d:%02d.%03d  | [Ind: %d] [Num: %d] [Single entry size: %d] %s\n%02d:%02d:%02d.%03d  | MaxEntries Entries   Insert Remove   \n%012X  | %10d %7d   %6d %6d  ";
	Red::System::SNPrintF( buffer, 2048, format, tms.wHour, tms.wMinute, tms.wSecond, tms.wMilliseconds, indEntry, numEntry, singleEntrySize, name, tme.wHour, tme.wMinute, tme.wSecond, tme.wMilliseconds, threadID, maxElts, numElts, numInserts, numRemoves );

	if( maxElts == 0 )
	{
		char buf1[ 2048 ];
		Red::System::SNPrintF( buf1, 2048, "%s   #########", buffer );
		LogMap( buf1 );
	}
	else
	{
		LogMap( buffer );
	}
}

#endif //EXPOSE_HASH_MAP_STATISTICS_API

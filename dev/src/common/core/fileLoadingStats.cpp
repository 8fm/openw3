/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fileLoadingStats.h"
#include "class.h"

CFileLoadingStats::CFileLoadingStats()
{

}

CFileLoadingStats::~CFileLoadingStats()
{
	Clear();
}

void CFileLoadingStats::Clear()
{
	// Delete file stats
	m_files.ClearPtr();

	// Delete class stats
	m_classes.ClearPtr();
}

CFileLoadingStats::FileStats::FileStats( const String& filePath )
	: m_filePath( filePath )
	, m_openTime( 0.0f )
	, m_tableLoadingTime( 0.0f )
	, m_objectCreationTime( 0.0f )
	, m_deserializationTime( 0.0f )
	, m_postLoadTime( 0.0f )
	, m_numBlocks( 0 )
	, m_bytesRead( 0 )
{
}

CFileLoadingStats::ClassStats::ClassStats( const CClass* objectClass )
	: m_class( objectClass )
	, m_numObjects( 0 )
	, m_creationTime( 0.0f )
	, m_deserializationTime( 0.0f )
	, m_postLoadTime( 0.0f )
	, m_numBlocks( 0 )
	, m_bytesRead( 0 )
{
}

CFileLoadingStats::FileStats* CFileLoadingStats::GetFileStats( const String& file, Bool create /*=true*/ )
{
	CScopedLock scopedLock( m_mutex );

	// Find in the list
	FileStats* stats = NULL;
	if ( !m_files.Find( file, stats ) )
	{
		// Create new if allowed
		if ( create )
		{
			stats = new FileStats( file );
			m_files.Insert( file, stats );
		}
	}

	// Return stats structure
	return stats;
}

CFileLoadingStats::ClassStats* CFileLoadingStats::GetClassStats( const CClass* objectClass, Bool create /*=true*/ )
{
	CScopedLock scopedLock( m_mutex );

	// Find in the list
	ClassStats* stats = NULL;
	if ( !m_classes.Find( objectClass, stats ) )
	{
		// Create new if allowed
		if ( create )
		{
			stats = new ClassStats( objectClass );
			m_classes.Insert( objectClass, stats );
		}
	}

	// Return stats structure
	return stats;
}

THashMap< String, CFileLoadingStats::FileStats* > CFileLoadingStats::GetFileStatsCopy() const
{
	return m_files;
}

THashMap< const CClass*, CFileLoadingStats::ClassStats* > CFileLoadingStats::GetClassStatsCopy() const
{
	return m_classes;
}

static int FileStatsCompare( const void *arg1, const void *arg2 )
{
	CFileLoadingStats::FileStats* a = * ( CFileLoadingStats::FileStats** ) arg1;
	CFileLoadingStats::FileStats* b = * ( CFileLoadingStats::FileStats** ) arg2;
	Float totalA = a->m_deserializationTime + a->m_objectCreationTime + a->m_postLoadTime + a->m_tableLoadingTime;
	Float totalB = b->m_deserializationTime + b->m_objectCreationTime + b->m_postLoadTime + b->m_tableLoadingTime;
	if ( totalA < totalB ) return 1;
	if ( totalA > totalB ) return -1;
	return 0;
}

static int ClassStatsCompare( const void *arg1, const void *arg2 )
{
	CFileLoadingStats::ClassStats* a = * ( CFileLoadingStats::ClassStats** ) arg1;
	CFileLoadingStats::ClassStats* b = * ( CFileLoadingStats::ClassStats** ) arg2;
	Float totalA = a->m_deserializationTime;
	Float totalB = b->m_deserializationTime;
	if ( totalA < totalB ) return 1;
	if ( totalA > totalB ) return -1;
	return 0;
}

void CFileLoadingStats::Dump() const
{
	CScopedLock scopedLock( m_mutex );

	// Header
	LOG_CORE( TXT("====================================================================================") );
	LOG_CORE( TXT("Loading stats for %i files and %i classes"), m_files.Size(), m_classes.Size() );
	LOG_CORE( TXT("====================================================================================") );
	LOG_CORE( TXT("") );

	// File list
	LOG_CORE( TXT("=================================== FILE LIST =============================================") );
	LOG_CORE( TXT("TotalLoad | LoadTables | CreatExports | LoadExports | PostLoad |  Blocks  |   Data   | Name") );
	LOG_CORE( TXT("   ms     |     ms     |      ms      |      ms     |    ms    |    N     |    KB    | ") );
	LOG_CORE( TXT("=========================================================================================") );

	// Grab elements
	TDynArray< FileStats* > sortedStats;
	for ( THashMap< String, FileStats* >::const_iterator i=m_files.Begin(); i!=m_files.End(); ++i )
	{
		sortedStats.PushBack( i->m_second );
	}

	// Sort the elements
	qsort( sortedStats.TypedData(), sortedStats.Size(), sizeof( FileStats* ), &FileStatsCompare );

	// Dump elements
	Float totalFileTotal = 0.0f;
	Float totalFileLoadingTables = 0.0f;
	Float totalFileObjectCreation = 0.0f;
	Float totalFileDeserialization = 0.0f;
	Float totalFilePostLoadTime = 0.0f;
	Uint64 totalNumBlocks = 0;
	Uint64 totalFileBytes = 0;
	for ( Uint32 i=0; i<sortedStats.Size(); i++ )
	{
		// Calculate the total time spent loading
		const FileStats* stats = sortedStats[i];;
		Float totalTime = stats->m_deserializationTime;
		totalTime += stats->m_objectCreationTime;
		totalTime += stats->m_tableLoadingTime;
		totalTime += stats->m_postLoadTime;

		// Accumulate totals
		totalFileTotal += totalTime;
		totalFileLoadingTables += stats->m_tableLoadingTime;
		totalFileObjectCreation += stats->m_objectCreationTime;
		totalFileDeserialization += stats->m_deserializationTime;
		totalFilePostLoadTime += stats->m_postLoadTime;
		totalFileBytes += ( Uint64 ) stats->m_bytesRead;
		totalNumBlocks += stats->m_numBlocks;

		// Dump the info
		LOG_CORE( TXT("  %7.1f |    %7.1f |     %8.1f |    %8.1f | %8.1f | %8i | %8.1f | %s"), 
			totalTime * 1000.0f,
			stats->m_tableLoadingTime * 1000.0f,// ms
			stats->m_objectCreationTime * 1000.0f,// ms
			stats->m_deserializationTime * 1000.0f,// ms
			stats->m_postLoadTime * 1000.0f, // ms
			stats->m_numBlocks,
			stats->m_bytesRead / 1024.0f,
			stats->m_filePath.AsChar() );
	}

	// Dump the file summary
	LOG_CORE( TXT("= TOTAL =================================================================================") );
	LOG_CORE( TXT("  %7.1f |    %7.1f |     %8.1f |    %8.1f | %8.1f | %6.1f | %8.1f"), 
		totalFileTotal * 1000.0f,
		totalFileLoadingTables * 1000.0f,// ms
		totalFileObjectCreation * 1000.0f,// ms
		totalFileDeserialization * 1000.0f,// ms
		totalFilePostLoadTime * 1000.0f, // ms
		totalNumBlocks,
		totalFileBytes / 1024.0f );
	LOG_CORE( TXT("") );

	// Class list
	LOG_CORE( TXT("=================================== CLASS LIST ==========================================") );
	LOG_CORE( TXT("TotalLoad |   Create   |  Serialize  | PostLoad |  Blocks  |   Data   | Instances | Name") );
	LOG_CORE( TXT("   ms     |      ms    |      ms     |    ms    |    N     |    KB    |    int    | ") );
	LOG_CORE( TXT("=========================================================================================") );

	// Grab elements
	TDynArray< ClassStats* > sortedClasses;
	for ( THashMap< const CClass*, ClassStats* >::const_iterator i=m_classes.Begin(); i!=m_classes.End(); ++i )
	{
		sortedClasses.PushBack( i->m_second );
	}

	// Sort the elements
	qsort( sortedClasses.TypedData(), sortedClasses.Size(), sizeof( ClassStats* ), &ClassStatsCompare );

	// Dump elements
	for ( Uint32 i=0; i<sortedClasses.Size(); i++ )
	{
		// Calculate the total time spent loading
		const ClassStats* stats = sortedClasses[i];;
		Float totalTime = stats->m_deserializationTime;
		totalTime += stats->m_creationTime;
		totalTime += stats->m_postLoadTime;

		// Dump the info
		LOG_CORE
		(
			TXT("  %7.1f |    %7.1f |    %8.1f | %8.1f | %8i | %9.1f | %9i | %s"), 
			totalTime * 1000.0f,
			stats->m_creationTime * 1000.0f,// ms
			stats->m_deserializationTime * 1000.0f,// ms
			stats->m_postLoadTime * 1000.0f, // ms
			stats->m_numBlocks,	// us
			stats->m_bytesRead / 1024.0f,
			stats->m_numObjects,
			stats->m_class->GetName().AsString().AsChar()
		);
	}
}

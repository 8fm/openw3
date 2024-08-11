/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "hashset.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"
#include "httpResponseData.h"
#include "fileHandleCache.h"

class CDebugPageAsyncFileHandleCache : public IDebugPageHandlerHTML
{
public:
	CDebugPageAsyncFileHandleCache()
		: IDebugPageHandlerHTML( "/asyncfiles/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Async file handle cache"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	// information about import
	class NativeHandleInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		NativeHandleInfo( const Red::IO::CAsyncFileHandleCache::SDebugCacheEntry& init )
			: m_path( UNICODE_TO_ANSI(init.m_filePath) )
			, m_fileHandle( init.m_fh )
			, m_useCount( init.m_useCount )
			, m_lru( init.m_lru )
			, m_hashTableIndex( init.m_hashTableIndex )
			, m_calcHashTableIndex( init.m_calcHashTableIndex )
			, m_hash( init.m_hash )
			, m_closeWhenNotUsed( init.m_data.CloseWhenNotUsed() )
		{
		}

	private:
		enum EColumn
		{
			eColumn_FileHandle = 1,
			eColumn_UseCount = 2,
			eColumn_CloseWhenNotUsed = 3,
			eColumn_LRU = 4,
			eColumn_HashTableIndex = 5,
			eColumn_CalcHashTableIndex = 6,
			eColumn_Hash = 7,
			eColumn_Path = 8,
		};

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const NativeHandleInfo*)other);

			switch ( columnID )
			{
			case eColumn_FileHandle:			return m_fileHandle < b->m_fileHandle;
			case eColumn_UseCount:				return m_useCount < b->m_useCount;
			case eColumn_CloseWhenNotUsed:		return m_closeWhenNotUsed < b->m_closeWhenNotUsed;
			case eColumn_LRU:					return m_lru < b->m_lru;
			case eColumn_HashTableIndex:		return m_hashTableIndex < b->m_hashTableIndex;
			case eColumn_CalcHashTableIndex:	return m_calcHashTableIndex < b->m_calcHashTableIndex;
			case eColumn_Hash:					return m_hash < b->m_hash;
			case eColumn_Path:					return m_path < b->m_path;
			}
			return m_path < b->m_path;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case eColumn_FileHandle:			doc.Writef("0x%08X", m_fileHandle ); break;
			case eColumn_UseCount:				doc.Writef("%d", m_useCount ); break;
			case eColumn_CloseWhenNotUsed:		doc.Writef("%d", m_closeWhenNotUsed ); break;
			case eColumn_LRU:					doc.Writef("%u", m_lru ); break;
			case eColumn_HashTableIndex:		doc.Writef("%u", m_hashTableIndex ); break;
			case eColumn_CalcHashTableIndex:	doc.Writef("%u", m_calcHashTableIndex ); break;
			case eColumn_Hash:					doc.Writef("0x%08X", m_hash ); break;
			case eColumn_Path:					doc.Write(m_path.AsChar()); break;
			}
		}

	private:
		StringAnsi		m_path;
		Uint32			m_fileHandle;
		Uint32			m_useCount;
		Uint16			m_lru;
		Uint16			m_hashTableIndex;
		Uint16			m_calcHashTableIndex;
		Uint32			m_hash;
		Bool			m_closeWhenNotUsed;
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{

		const Uint32 MAX_DEBUG_ENTRIES = 512;
		Red::IO::CAsyncFileHandleCache::SDebugCacheEntry debugEntries[MAX_DEBUG_ENTRIES];
		Uint32 numEntries = 0;
		Red::IO::GAsyncIO.GetAsyncFileHandleCacheForDebug().GetCacheEntriesForDebug( debugEntries, MAX_DEBUG_ENTRIES, &numEntries );
		Uint32 numEntriesInUse = 0;
		for ( Uint32 i = 0; i < numEntries; ++i )
		{
			if ( debugEntries[i].m_useCount > 0 )
				++numEntriesInUse;
		}

		// get general memory statistics
		{
			Red::IO::CAsyncFileHandleCache::SDebugStats debugStats;
			Red::IO::GAsyncIO.GetAsyncFileHandleCacheForDebug().GetStatsForDebug( debugStats );
			CDebugPageHTMLInfoBlock info( doc, "General information" );
			info.Info( "Entries: ").Writef( "%u", debugStats.m_numEntries );
			info.Info( "Peak: ").Writef( "%u",  debugStats.m_peakEntries );
			info.Info( "In use: ").Writef( "%u", numEntriesInUse );
			info.Info( "Soft limit: ").Writef( "%u", debugStats.m_softLimit );
			info.Info( "Hard limit: ").Writef( "%u", debugStats.m_hardLimit );
			info.Info( "Evicted: ").Writef( "%u", debugStats.m_numEvictions );
		}

		// handles
		{
			CDebugPageHTMLInfoBlock info( doc, "File handles" );

			CDebugPageHTMLTable table( doc, "handles" );
			table.AddColumn( "File Handle", 60, true );
			table.AddColumn( "Use Count", 60, true );
			table.AddColumn( "Auto Close", 60, true );
			table.AddColumn( "LRU", 60, true );
			table.AddColumn( "Hash Index", 80, true );
			table.AddColumn( "Calc Hash Index", 80, true );
			table.AddColumn( "Hash", 80, true );
			table.AddColumn( "Path", 1.0f, true );

			// add entries
			for ( Uint32 i = 0; i < numEntries; ++i )
			{
				const Red::IO::CAsyncFileHandleCache::SDebugCacheEntry& debugEntry = debugEntries[i];
				const Char* fileName = Red::IO::GAsyncIO.GetFileName( debugEntry.m_fh );
				if ( !fileName )
					continue;

				table.AddRow( new NativeHandleInfo( debugEntry ) );
			}

			// render table
			table.Render( 900, "generic", fullURL );
		}

		return true;
	}
};

class CDebugPageFileHandleCache : public IDebugPageHandlerHTML
{
public:
	CDebugPageFileHandleCache()
		: IDebugPageHandlerHTML( "/syncfiles/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "File handle cache"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	// information about import
	class NativeHandleInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		StringAnsi		m_path;
		Uint32			m_numReaders;
		Uint32			m_lru;
		Uint32			m_threadID;
		Bool			m_isInvalid;

	public:
		NativeHandleInfo( const StringAnsi& path, const Uint32 numReaders, const Uint32 lru, const Bool isInvalid, const Uint32 threadID )
			: m_path( path )
			, m_numReaders( numReaders )
			, m_lru( lru )
			, m_isInvalid( isInvalid )
			, m_threadID( threadID )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const NativeHandleInfo*)other);

			switch ( columnID )
			{
				case 1: return m_threadID < b->m_threadID;
				case 2: return m_numReaders < b->m_numReaders;
				case 3: return m_isInvalid < b->m_isInvalid;
				case 4: return m_lru < b->m_lru;
			}
			return m_path < b->m_path; // 5
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Writef("%d", m_threadID); break;
				case 2: doc.Writef("%d", m_numReaders); break;
				case 3: doc.Write(m_isInvalid ? "INVALID" : (m_numReaders ? "ACTIVE" : "NOT USED")); break;
				case 4: doc.Writef("%d", m_lru); break;
				case 5: doc.Write(m_path.AsChar()); break;
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CNativeFileHandleWrapper::st_lock );
		const auto& handles = CNativeFileHandleWrapper::st_openedHandles;

		// get general memory statistics
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );
			info.Info( "Total physical handles: " ).Writef( "%d", handles.Size() );

			Uint32 numHandlesInUse = 0;
			Uint32 numInvalidHandles = 0;
			THashSet< String > uniquePaths;

			for ( auto it = handles.Begin(); it != handles.End(); ++it )
			{
				uniquePaths.Insert( (*it)->GetAbsolutePath() );
				if ( (*it)->IsInvalid() )
				{
					numInvalidHandles += 1;
				}
				if ( (*it)->GetReaderCount() > 0 )
				{
					numHandlesInUse += 1;
				}
			}

			if ( numHandlesInUse )
				info.Info( "Total handles in use: " ).Writef( "%d", numHandlesInUse );

			if ( uniquePaths.Size() )
				info.Info( "Total unique files: " ).Writef( "%d", uniquePaths.Size() );

			if ( numInvalidHandles )
				info.Info( "Total invalid handles: " ).Writef( "%d", numInvalidHandles );
		}

		// handles
		{
			CDebugPageHTMLInfoBlock info( doc, "File handles" );

			CDebugPageHTMLTable table( doc, "handles" );
			table.AddColumn( "ThreadID", 60, true );
			table.AddColumn( "Readers", 80, true );
			table.AddColumn( "Status", 120, true );
			table.AddColumn( "LRU", 80, true );
			table.AddColumn( "Path", 1.0f, true );

			// add entries
			for ( auto it = handles.Begin(); it != handles.End(); ++it )
			{
				const auto& h = **it;
				table.AddRow( new NativeHandleInfo(
					UNICODE_TO_ANSI( h.GetAbsolutePath().AsChar() ), 
					h.GetReaderCount(), h.GetLRUMarker(), h.IsInvalid(), h.GetOwningThread() ) );
			}
					
			// render table
			table.Render( 900, "generic", fullURL );
		}

		return true;
	}
};

void InitFileHandleCacheDebugPages()
{
	new CDebugPageFileHandleCache(); // autoregister
	new CDebugPageAsyncFileHandleCache();
}
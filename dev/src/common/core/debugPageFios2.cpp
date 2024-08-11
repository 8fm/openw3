/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../redSystem/crt.h"
#include "../redIO/redIO.h"
#include "hashmap.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"
#include "httpResponseData.h"
#include "fileHandleCache.h"
#include "stringConversion.h"
#include "fileHandleCache.h"

#ifdef RED_PLATFORM_ORBIS

namespace DebugPageHelpers
{
	static const AnsiChar* GetFiosOpenFlagsTxt( SceFiosOpenFlags flag )
	{
		switch( flag )
		{
		case SCE_FIOS_O_READ:		return "SCE_FIOS_O_READ";
		case SCE_FIOS_O_WRITE:		return "SCE_FIOS_O_WRITE";
		case SCE_FIOS_O_APPEND:		return "SCE_FIOS_O_APPEND";
		case SCE_FIOS_O_CREAT:		return "SCE_FIOS_O_CREAT";
		case SCE_FIOS_O_TRUNC:		return "SCE_FIOS_O_TRUNC";
		default:
			break;
		}

		return "<Unknown>";
	}

	StringAnsi GetFiosOpenFlagsString( Uint32 flags )
	{
		if ( flags == 0 )
			return StringAnsi::EMPTY;

		Red::System::StackStringWriter< AnsiChar, 256 > writer;
		const SceFiosOpenFlags ar[] = { SCE_FIOS_O_READ, SCE_FIOS_O_WRITE, SCE_FIOS_O_APPEND, SCE_FIOS_O_CREAT, 
								 SCE_FIOS_O_TRUNC };

		Uint32 flagsLeft = flags;
		const Uint32 len = ARRAY_COUNT_U32(ar);
		for ( Uint32 i = 0; i < len; ++i )
		{
			const SceFiosOpenFlags fl = ar[i];
			if ( (flagsLeft & fl) != 0 )
			{
				const AnsiChar* txt = GetFiosOpenFlagsTxt(fl);
				writer.Append(txt);

				flagsLeft &= ~fl;
				if ( flagsLeft != 0 )
					writer.Append("|");
			}
		}

		return writer.AsChar();
	}
}

class CDebugPageFios2FH : public IDebugPageHandlerHTML
{
public:
	CDebugPageFios2FH()
		: IDebugPageHandlerHTML( "/fios2fh/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Fios2 FH"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		SceFiosFH fh = SCE_FIOS_FH_INVALID;

		Red::System::StringToInt( fh, relativeURL.GetPath().AsChar(), nullptr, Red::System::BaseTen );

		// A bit of a race to check if valid, but best that can be done here
		if ( fh == SCE_FIOS_FH_INVALID || !sceFiosIsValidHandle(fh) )
		{
			// display a message instead of 404
			doc << "<span class=\"error\">Invalid FH</span>";
			return true;
		}

		// TBD: path, open params, size etc sceFiosFHGet*, stat, permissions, sceFiosStatusFlags
		const Uint32 BUFSZ = 512;
		AnsiChar dbgTxt[BUFSZ];
		::sceFiosDebugDumpFH( fh, dbgTxt, BUFSZ );

		{
			CDebugPageHTMLInfoBlock info( doc, "Fios2" );
			doc.Link( "/fios2/" ).Write( "Back" );
		}

		// Raw file handle dump
		{
			CDebugPageHTMLInfoBlock info( doc, "SceFiosFH debug dump" );
			info.Info("").Writef("%hs", dbgTxt);
		}

		return true;
	}
};

class CDebugPageFios2 : public IDebugPageHandlerHTML
{
public:
	CDebugPageFios2()
		: IDebugPageHandlerHTML( "/fios2/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Fios2"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	// information about import
	class FileHandleInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		StringAnsi			m_path;
		FileHandleInfo*		m_nextDuplicate;
		Uint64				m_accessTimeNanosec;
		Int64				m_size;
		Int32				m_fiosFH;
		Uint32				m_fiosOpenFlags;
		Int32				m_duplicateFH;

	public:
		FileHandleInfo( const StringAnsi& path, Uint64 accessTimeNanosec, Int64 size, Int32 fiosFH, Uint32 fiosOpenFlags )
			: m_path( path )
			, m_nextDuplicate(nullptr)
			, m_accessTimeNanosec( accessTimeNanosec )
			, m_size( size )
			, m_fiosFH( fiosFH )
			, m_fiosOpenFlags( fiosOpenFlags )
			, m_duplicateFH( fiosFH )
		{
		}

		const StringAnsi& GetPath() const { return m_path; }

		void SetDuplicateFH( Int32 fh ) { m_duplicateFH = fh; }

	private:
		enum EColumn
		{
			eColumn_FH			= 1,
			eColumn_DuplicateFH	= 2,
			eColumn_Size		= 3,
			eColumn_OpenFlags	= 4,
			eColumn_Path		= 5,
			eColumn_AccessTime	= 6,
		};

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const FileHandleInfo*)other);

			switch ( columnID )
			{
			case eColumn_FH:			return m_fiosFH < b->m_fiosFH;
			case eColumn_DuplicateFH:	return m_duplicateFH < b->m_duplicateFH;
			case eColumn_Size:			return m_size < b->m_size;
			case eColumn_OpenFlags:		return m_fiosOpenFlags < b->m_fiosOpenFlags;
			case eColumn_Path:			return m_path < b->m_path;
			case eColumn_AccessTime:	return m_accessTimeNanosec < b->m_accessTimeNanosec;
			default:
				break;
			}
			return m_path < b->m_path;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case eColumn_FH:			doc.Link( "/fios2fh/%d", m_fiosFH ).Writef( "0x08%X", m_fiosFH ); break;
			case eColumn_DuplicateFH:	doc.Writef( (m_duplicateFH != m_fiosFH ) ? "0x08%X" : "-", m_duplicateFH ); break;
			case eColumn_Size:			doc.Writef("%lld", m_size ); break;
			case eColumn_OpenFlags:		doc.Writef("0x%04X (%hs)", m_fiosOpenFlags, DebugPageHelpers::GetFiosOpenFlagsString(m_fiosOpenFlags).AsChar() ); break;
			case eColumn_Path:			doc.Writef("%hs", m_path.AsChar()); break;
			case eColumn_AccessTime:
				{
					const Uint32 BUFSZ = 48;
					AnsiChar dateBuf[BUFSZ];
					::sceFiosDebugDumpDate( m_accessTimeNanosec, dateBuf, BUFSZ );
					doc.Writef("%hs", dateBuf );
				}
				break;
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		const Uint32 MAX_NUM_FH = 256;
		SceFiosFH allFHs[MAX_NUM_FH];
		const Uint32 numFHs = ::sceFiosGetAllFHs( allFHs, MAX_NUM_FH );
	
		// overview
		{
			CDebugPageHTMLInfoBlock info( doc, "Fios2 Info" );

			info.Info("System-wide open FHs: ").Writef("%u", numFHs);

			Uint32 numSyncCacheHandles = 0;
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( CNativeFileHandleWrapper::st_lock );
				numSyncCacheHandles = CNativeFileHandleWrapper::st_openedHandles.Size();
			}

			Red::IO::CAsyncFileHandleCache::SDebugStats debugStats;
			Red::IO::GAsyncIO.GetAsyncFileHandleCacheForDebug().GetStatsForDebug( debugStats );
			info.Info("...of which owned by sync FH cache: ").Link( "/syncfiles/").Writef("%u", numSyncCacheHandles );
			info.Info("...of which owned by async FH cache: ").Link( "/asyncfiles/").Writef("%u", debugStats.m_numEntries );
		}

		// file handles
		{
			CDebugPageHTMLInfoBlock info( doc, "File handles" );

			CDebugPageHTMLTable table( doc, "handles" );

			table.AddColumn( "SceFiosFH", 60, true );
			table.AddColumn( "Dup FH", 10, true );
			table.AddColumn( "Size", 80, true );
			table.AddColumn( "SceFiosOpenFlags", 120, true );
			table.AddColumn( "Path", .8f, true );
			table.AddColumn( "Access Timestamp", .1f, true );

			THashMap< StringAnsi, Int32 > fileHandleMap;

			// add entries
			for ( Uint32 i = 0; i < numFHs; ++i )
			{
				SceFiosFH fh = allFHs[i];

				if ( !::sceFiosIsValidHandle(fh) )
					continue;

				const AnsiChar* path = ::sceFiosFHGetPath( fh );
				if ( !path )
					continue;

				SceFiosStat stat;
				if ( ::sceFiosFHStatSync( nullptr, fh, &stat ) != SCE_FIOS_OK )
					continue;

				const SceFiosOpenParams* params = ::sceFiosFHGetOpenParams(fh);
				if ( !params )
					continue;

				FileHandleInfo* info = new FileHandleInfo( path, stat.accessDate, stat.fileSize, fh, params->openFlags );
				if ( !fileHandleMap.Insert( info->GetPath(), fh ) )
				{
					const Int32 dupFH = fileHandleMap[ info->GetPath().AsChar() ];
					info->SetDuplicateFH( dupFH );
				}

				table.AddRow( info );
			}

			// render table
			table.Render( 900, "generic", fullURL );
		}

		return true;
	}
};

#endif // RED_PLATFORM_ORBIS

void InitFios2DebugPages()
{
#ifdef RED_PLATFORM_ORBIS
	new CDebugPageFios2(); // autoregister
	new CDebugPageFios2FH();
#endif
}
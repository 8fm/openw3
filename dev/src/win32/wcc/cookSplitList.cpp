/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/algorithms.h"
#include "../../common/core/analyzer.h"
#include "cookSplitList.h"

//----

CCookerSplitFile::FileEntry::FileEntry(const StringAnsi& depotPath, const CCookerSplitFileEntry::TChunkID chunkID )	
{
	CFilePath::GetConformedPath( depotPath, m_filePath );
	m_fileId = TResourceID( m_filePath );
	m_chunkId = chunkID;
}

CCookerSplitFile::FileEntry* CCookerSplitFile::FileEntry::Parse( const JSONValue& val )
{
	const StringAnsi filePath = GetAttrStr(val, "path");
	if ( filePath.Empty() )
		return nullptr; // TODO: add error checking ?

	// missing id
	if ( !val["id"].IsString() )
	{
		ERR_WCC( TXT("Resource '%hs' has no ID specified" ), filePath.AsChar() );
		return nullptr;
	}

	// missing bundle name
	if ( !val["chunk"].IsString() )
	{
		ERR_WCC( TXT("Resource '%hs' has no chunk name specified" ), filePath.AsChar() );
		return nullptr;
	}

	// parse the ID and compare
	const AnsiChar* txt = val["id"].GetString();
	Red::Core::ResourceManagement::CResourceId resId;
	if ( !resId.FromString( txt ) )
	{
		ERR_WCC( TXT("Resource '%hs' has invalid ID specified" ), filePath.AsChar() );
		return nullptr;
	}

	// create entry
	const TChunkID chunk = TChunkID( ANSI_TO_UNICODE( val["chunk"].GetString() ) );
	Red::TUniquePtr<FileEntry> entry( new FileEntry( filePath, chunk ) );

	// make sure the ID matched
	if ( !(resId == entry->m_fileId) )
	{
		ERR_WCC( TXT("Resource '%hs' has different calculated ID than the saved one!" ), filePath.AsChar() );
		return nullptr;
	}

	// valid
	return entry.Release();
}

void CCookerSplitFile::FileEntry::Write(JSONWriter& writer) const
{
	writer.StartObject();

	writer.String( "path" );
	writer.String( m_filePath.AsChar() );

	writer.String( "chunk" );
	writer.String( UNICODE_TO_ANSI( m_chunkId.AsChar() ) );

	AnsiChar idStr[64];
	if ( m_fileId.ToString(idStr, ARRAY_COUNT(idStr)) )
	{
		writer.String( "id" );
		writer.String( idStr );
	}

	writer.EndObject();
}

//----

CCookerSplitFile::CCookerSplitFile()
{
}

CCookerSplitFile::~CCookerSplitFile()
{
	Reset();
}

void CCookerSplitFile::Reset()
{
	m_entries.ClearPtr();
}

Bool CCookerSplitFile::LoadFromFile( const String& absoluteFilePath )
{
	CTimeCounter timer;

	Reset();

	// load the file data
	LOG_WCC( TXT("Loading split list...") );
	TDynArray< Uint8 > fileData;
	if ( !GFileManager->LoadFileToBuffer( absoluteFilePath, fileData, true ) ) // JSON parser needs the NULL at the end
		return false;

	// parse JSON file
	LOG_WCC( TXT("Parsing split list...") );
	JSONDocument d;
	d.Parse<0>((AnsiChar*) fileData.Data());

	// errors, cooker data base is invalid, manual deletion is required
	if ( d.HasParseError() )
	{
		ERR_WCC( TXT("JSON parsing error: '%s'"), d.GetParseError() );
		ERR_WCC( TXT("Cooker split list data is corrupted!") );
		return false;
	}

	// parse the resource entries
	const JSONValue& db = d["files"];
	const rapidjson::SizeType numFiles = db.Size();
	for( rapidjson::SizeType i = 0; i<numFiles; ++i )
	{
		const JSONValue& fileEntry = db[ i ];

		FileEntry* entry = FileEntry::Parse( fileEntry );
		if ( entry )
		{
			m_entriesMap[ entry->GetFileId() ] = entry;
			m_entries.PushBack( entry );
		}
	}

	// stats
	LOG_WCC( TXT("Parsed %d file entries from cooker split list in %1.2fs"), 
		m_entries.Size(), timer.GetTimePeriod() );
	return true;
}

Bool CCookerSplitFile::SaveToFile( const String& absoluteFilePath )
{
	CTimeCounter timer;

	// open the output file
	IFile* outputFile = GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath );
	if ( !outputFile )
		return false;

	// build the output JSON file
	{
		typedef JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer(fileStream);

		writer.StartObject();
		{
			writer.String("files");
			writer.StartArray();

			// store the entries in the same order
			for ( Uint32 i=0; i<m_entries.Size(); ++i )
			{
				const FileEntry* entry = m_entries[i];
				entry->Write( writer );
			}

			writer.EndArray();
		}
		writer.EndObject();
	}

	// stats
	LOG_WCC( TXT("Cooker split list saved in %1.2fs (%d entries, %1.2f MB)"),
		timer.GetTimePeriod(), m_entries.Size(), outputFile->GetSize() / (1024.0f*1024.0f) );

	delete outputFile;
	return true;
}

Bool CCookerSplitFile::AddEntry( const StringAnsi& unsafeDepotPath, const CCookerSplitFileEntry::TChunkID fileChunkID )
{
	StringAnsi temp1;
	const StringAnsi depotPath( CFilePath::ConformPath(unsafeDepotPath, temp1) );

	const Red::Core::ResourceManagement::CResourceId resId( depotPath );

	// find existing entry
	FileEntry* entry = nullptr;
	if ( m_entriesMap.Find( resId, entry ) )
	{
		if ( fileChunkID != entry->GetFileChunkID() )
		{
			WARN_WCC( TXT("File '%ls' is already registered with chunk '%ls'. Unable to add to chunk '%ls."),
				ANSI_TO_UNICODE( depotPath.AsChar() ),
				ANSI_TO_UNICODE( entry->GetFileChunkID().AsChar() ),
				ANSI_TO_UNICODE( fileChunkID.AsChar() ) );

			return false;
		}
	}
	else
	{
		// create new entry
		entry = new FileEntry( depotPath, fileChunkID );

		// add to internal map
		m_entriesMap[ resId ] = entry;
		m_entries.PushBack( entry );
	}

	return true;
}

const Uint32 CCookerSplitFile::GetNumEntries() const
{
	return m_entries.Size();
}

const CCookerSplitFileEntry* CCookerSplitFile::GetEntry( const Uint32 index ) const
{
	if ( index < m_entries.Size() )
		return m_entries[ index ];

	return nullptr;
}

const CCookerSplitFileEntry* CCookerSplitFile::GetEntry( const Red::Core::ResourceManagement::CResourceId& id ) const
{
	FileEntry* ret = nullptr;
	m_entriesMap.Find( id, ret );
	return ret;
}

const CCookerSplitFileEntry* CCookerSplitFile::GetEntry( const StringAnsi& deptotPath ) const
{
	FileEntry* ret = nullptr;
	Red::Core::ResourceManagement::CResourceId id( deptotPath );
	m_entriesMap.Find( id, ret );
	return ret;
}
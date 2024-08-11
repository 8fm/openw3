/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/algorithms.h"
#include "../../common/core/analyzer.h"
#include "cookSeedFile.h"

//----

CCookerSeedFile::FileEntry::FileEntry(const StringAnsi& depotPath, const StringAnsi& bundleName, const CCookerSeedFileEntry::TChunkIDs* chunkIDs )
{
	CFilePath::GetConformedPath( depotPath, m_filePath );
	CFilePath::GetConformedPath( bundleName, m_bundleName );

	if ( chunkIDs != nullptr )
	{
		m_chunkIDs = *chunkIDs;
	}
}

CCookerSeedFile::FileEntry* CCookerSeedFile::FileEntry::Parse( const JSONValue& val )
{
	const StringAnsi filePath = GetAttrStr(val, "path");
	if ( filePath.Empty() )
		return nullptr; // TODO: add error checking ?

	// missing bundle name
	if ( !val["bundle"].IsString() )
	{
		ERR_WCC( TXT("Resource '%hs' has no bundle name specified" ), filePath.AsChar() );
		return nullptr;
	}

	// parse the bundle name
	const AnsiChar* bundleName = val["bundle"].GetString();
	
	TChunkIDs localChunkIDs;
	const JSONValue& chunksArray = val["chunks"];
	if( chunksArray.IsArray() )
	{
		const Uint32 chunksArraySize = chunksArray.Size();
		for( rapidjson::SizeType i = 0; i < chunksArraySize; ++i )
		{
			localChunkIDs.PushBack( CName( ANSI_TO_UNICODE( chunksArray[ i ].GetString( ) ) ) );
		}
	}

	// return loaded entry
	return new FileEntry( filePath, bundleName, &localChunkIDs );
}

void CCookerSeedFile::FileEntry::Write(JSONWriter& writer) const
{
	writer.StartObject();

	writer.String( "path" );
	writer.String( m_filePath.AsChar() );

	writer.String( "bundle" );
	writer.String( m_bundleName.AsChar() );

	if( !m_chunkIDs.Empty() )
	{
		writer.String("chunks");
		writer.StartArray();
		for( auto chunkID : m_chunkIDs )
		{
			writer.String( chunkID.AsAnsiChar( ) );
		}
		writer.EndArray();
	}

	writer.EndObject();
}

//----

CCookerSeedFile::CCookerSeedFile()
{
}

CCookerSeedFile::~CCookerSeedFile()
{
	Reset();
}

void CCookerSeedFile::Reset()
{
	m_entries.ClearPtr();
	m_entriesMap.Clear();
}

Bool CCookerSeedFile::Initialize( const class CAnalyzerOutputList& fileList )
{
	const Uint32 numBundles = fileList.GetNumBundles();
	for ( Uint32 i=0; i<numBundles; ++i )
	{
		const StringAnsi& bundleName = fileList.GetBundleName(i);
		const Uint32 numFiles = fileList.GetBundleFileCount(i);

		for ( Uint32 j=0; j<numFiles; ++j )
		{
			const StringAnsi& fileName = fileList.GetBundleFileName(i,j);
			const CAnalyzerOutputList::TChunkIDs* fileChunkIDs = fileList.GetBundleFileChunkIDs(i,j);

			if ( !AddEntry( fileName, bundleName, fileChunkIDs ) )
				return false;
		}
	}

	// all files added
	return true;
}

Bool CCookerSeedFile::LoadFromFile( const String& absoluteFilePath )
{
	CTimeCounter timer;

	Reset();

	// load the file data
	LOG_WCC( TXT("Loading seed file...") );
	TDynArray< Uint8 > fileData;
	if ( !GFileManager->LoadFileToBuffer( absoluteFilePath, fileData, true ) ) // JSON parser needs the NULL at the end
		return false;

	// parse JSON file
	LOG_WCC( TXT("Parsing seed file...") );
	JSONDocument d;
	d.Parse<0>((AnsiChar*) fileData.Data());

	// errors, cooker data base is invalid, manual deletion is required
	if ( d.HasParseError() )
	{
		ERR_WCC( TXT("JSON parsing error: '%s'"), d.GetParseError() );
		ERR_WCC( TXT("Cooker seed file data is corrupted!") );
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
			// Kinda hacky way of making the entries unique in the hash-map
			const Red::Core::ResourceManagement::CResourceId entryId( entry->GetFilePath() + entry->GetBundleName() );
			m_entriesMap[ entryId ] = entry;
			m_entries.PushBack( entry );
		}
	}

	// stats
	LOG_WCC( TXT("Parsed %d file entries from cooker seed file in %1.2fs"), 
		m_entries.Size(), timer.GetTimePeriod() );
	return true;
}

Bool CCookerSeedFile::SaveToFile( const String& absoluteFilePath )
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
	LOG_WCC( TXT("Cooker seed file saved in %1.2fs (%d entries, %1.2f MB)"),
		timer.GetTimePeriod(), m_entries.Size(), outputFile->GetSize() / (1024.0f*1024.0f) );

	delete outputFile;
	return true;
}

Bool CCookerSeedFile::AddEntry( const StringAnsi& unsafeDepotPath, const StringAnsi& unsafeBundleName, const CCookerSeedFileEntry::TChunkIDs* fileChunkIDs )
{
	StringAnsi temp1, temp2;
	const StringAnsi depotPath( CFilePath::ConformPath(unsafeDepotPath, temp1) );
	const StringAnsi bundleName( CFilePath::ConformPath(unsafeBundleName, temp2) );

	const Red::Core::ResourceManagement::CResourceId resId( depotPath + bundleName );
	// find existing entry
	FileEntry* entry = nullptr;
	if ( m_entriesMap.Find( resId, entry ) )
	{
		// it's illegal to have the same file defined twice in the same bundle within the same seed list
		if ( entry->GetBundleName() == bundleName )
		{
			WARN_WCC( TXT("File '%ls' is already registered in bundle '%ls'. Unable to add to bundle '%ls' withing the same seed list."),
				ANSI_TO_UNICODE( depotPath.AsChar() ),
				ANSI_TO_UNICODE( entry->GetBundleName().AsChar() ),
				ANSI_TO_UNICODE( bundleName.AsChar() ) );

			return false;
		}
	}
	else
	{
		// create new entry
		entry = new FileEntry( depotPath, bundleName, fileChunkIDs );

		m_entriesMap[ resId ] = entry;
		m_entries.PushBack( entry );
	}

	return true;
}

Bool CCookerSeedFile::RemoveEntry( const StringAnsi& unsafeDepotPath, const StringAnsi& unsafeBundleName )
{
	StringAnsi temp1, temp2;
	const StringAnsi depotPath( CFilePath::ConformPath(unsafeDepotPath, temp1) );
	const StringAnsi bundleName( CFilePath::ConformPath(unsafeBundleName, temp2) );

	const Red::Core::ResourceManagement::CResourceId resId( depotPath + bundleName );

	// find existing entry
	FileEntry* entry = nullptr;
	if ( m_entriesMap.Find( resId, entry ) )
	{
		if ( entry->GetBundleName() != bundleName )
		{
			WARN_WCC( TXT("File '%ls' is not already registered in bundle '%ls' not in bundle '%ls'. Unable to remove it."),
				ANSI_TO_UNICODE( depotPath.AsChar() ),
				ANSI_TO_UNICODE( entry->GetBundleName().AsChar() ),
				ANSI_TO_UNICODE( bundleName.AsChar() ) );

			return false;
		}

		m_entriesMap.Erase(resId);
		m_entries.Remove(entry);
		delete entry;

		return true;
	}

	// not found (silent)
	return false;
}

const Uint32 CCookerSeedFile::GetNumEntries() const
{
	return m_entries.Size();
}

const CCookerSeedFileEntry* CCookerSeedFile::GetEntry( const Uint32 index ) const
{
	RED_ASSERT( index < m_entries.Size() );

	if ( index < m_entries.Size() )
		return m_entries[ index ];

	return nullptr;
}

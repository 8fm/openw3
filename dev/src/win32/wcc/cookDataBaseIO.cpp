/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/algorithms.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/engine/layerInfo.h"
#include "cookDataBase.h"

//---

void CCookerDataBase::UsageEntry::Write( JSONWriter& writer ) const
{
	writer.StartObject();

	writer.String( "dist" );
	writer.Double( m_visibilityDistance );  

	if ( m_visibilityDistance >= 0.0f )
	{
		writer.String("x"); writer.Double(m_position.X);
		writer.String("y");	writer.Double(m_position.Y);
		writer.String("z");	writer.Double(m_position.Z);
	}

	if ( m_componentName )
	{
		writer.String( "component" );
		writer.String( m_componentName.AsAnsiChar() );
	}

	if ( m_componentClass )
	{
		writer.String( "componentClass" );
		writer.String( m_componentClass.AsAnsiChar() );
	}

	if ( m_entityName )
	{
		writer.String( "entity" );
		writer.String( m_entityName.AsAnsiChar() );
	}

	if ( m_entityClass )
	{
		writer.String( "entityClass" );
		writer.String( m_entityClass.AsAnsiChar() );
	}

	if ( m_layerName )
	{
		writer.String( "layer" );
		writer.String( m_layerName.AsAnsiChar() );
	}

	if ( m_worldName )
	{
		writer.String( "world" );
		writer.String( m_worldName.AsAnsiChar() );
	}

	writer.EndObject();
}

CCookerDataBase::UsageEntry* CCookerDataBase::UsageEntry::Parse( const JSONValue& val )
{
	if ( !val.IsObject() )
		return nullptr;

	Red::TUniquePtr<UsageEntry> entry( new UsageEntry() );

	// parse the position
	entry->m_visibilityDistance = (float)GetAttrDouble( val, "dist" );
	if ( entry->m_visibilityDistance > 0.0f)
	{
		entry->m_position.X = (float)GetAttrDouble( val, "x" );
		entry->m_position.Y = (float)GetAttrDouble( val, "y" );
		entry->m_position.Z = (float)GetAttrDouble( val, "z" );
	}

	// parse the world name
	entry->m_worldName = GetAttrName( val, "world" );
	if ( entry->m_worldName )
	{
		entry->m_layerName = GetAttrName( val, "layer" );
		if ( entry->m_layerName )
		{
			entry->m_entityName = GetAttrName( val, "entity" );
			entry->m_entityClass = GetAttrName( val, "entityClass" );

			if ( entry->m_entityName )
			{
				entry->m_componentName = GetAttrName( val, "component" );
				entry->m_componentClass = GetAttrName( val, "componentClass" );
			}
		}
	}

	return entry.Release();
}

//---

void CCookerDataBase::FileEntry::Write(JSONWriter& writer) const
{
	writer.StartObject();

	writer.String( "path" );
	writer.String( m_filePath.AsChar() );

	writer.String( "level" );
	writer.Int( m_dependencyLevel );

	if ( m_className )
	{
		writer.String("class");
		writer.String( UNICODE_TO_ANSI( m_className.AsChar() ) );
	}

	if ( m_flagIsSeedFile )
	{
		writer.String( "seed" );
		writer.Int( 1 );
	}

	if ( m_flagIsAdditional )
	{
		writer.String( "additional" );
		writer.Int( 1 );
	}

	if ( m_flagIsBufferFile )
	{
		writer.String( "buffer" );
		writer.Int( 1 );
	}

	if ( m_flagIsConsumed )
	{
		writer.String( "consumed" );
		writer.Int( 1 );
	}

	AnsiChar idStr[64];
	if ( m_fileId.ToString(idStr, ARRAY_COUNT(idStr)) )
	{
		writer.String( "id" );
		writer.String( idStr );
	}

	if ( m_memoryCPU )
	{
		writer.String("memCpu");
		writer.Int( m_memoryCPU );
	}

	if ( m_memoryGPU )
	{
		writer.String("memGpu");
		writer.Int( m_memoryGPU );
	}

	if ( m_diskSize )
	{
		writer.String("diskSize");
		writer.Int( m_diskSize );
	}

	if ( m_numImportedAsHardDependency > 0 )
	{
		writer.String("hardImports");
		writer.Int( m_numImportedAsHardDependency );
	}

	if ( m_numImportedAsSoftDependency > 0 )
	{
		writer.String("softImports");
		writer.Int( m_numImportedAsSoftDependency );
	}

	if ( m_numImportedAsInplaceDependency > 0 )
	{
		writer.String("inplaceImports");
		writer.Int( m_numImportedAsInplaceDependency );
	}

	if ( !m_hardDependencies.Empty() )
	{
		writer.String( "hard" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_hardDependencies.Size(); ++i )
		{
			const FileEntry* dep = m_hardDependencies[i];

			AnsiChar idStr[64];
			if ( dep->m_fileId.ToString(idStr, ARRAY_COUNT(idStr)) )
			{
				writer.String( idStr );
			}
		}
		writer.EndArray();
	}

	if ( !m_softDependencies.Empty() )
	{
		writer.String( "soft" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_softDependencies.Size(); ++i )
		{
			const FileEntry* dep = m_softDependencies[i];

			AnsiChar idStr[64];
			if ( dep->m_fileId.ToString(idStr, ARRAY_COUNT(idStr)) )
			{
				writer.String( idStr );
			}
		}
		writer.EndArray();
	}

	if ( !m_inplaceDependencies.Empty() )
	{
		writer.String( "inplace" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_inplaceDependencies.Size(); ++i )
		{
			const FileEntry* dep = m_inplaceDependencies[i];

			AnsiChar idStr[64];
			if ( dep->m_fileId.ToString(idStr, ARRAY_COUNT(idStr)) )
			{
				writer.String( idStr );
			}
		}
		writer.EndArray();
	}

	if ( !m_usage.Empty() )
	{
		writer.String( "usage" );

		writer.StartArray();
		for ( auto it = m_usage.Begin(); it != m_usage.End(); ++it )
		{
			const UsageEntry* usage = (*it).m_second;
			usage->Write( writer );
		}
		writer.EndArray();
	}

	writer.EndObject();
}

CCookerDataBase::FileEntry* CCookerDataBase::FileEntry::Parse( const JSONValue& val )
{
	const StringAnsi filePath = GetAttrStr(val, "path");
	if ( filePath.Empty() ) return nullptr; // TODO: add error checking ?

	// missing id
	if ( !val["id"].IsString() )
	{
		ERR_WCC( TXT("Resource '%ls' has no ID specified" ), ANSI_TO_UNICODE( filePath.AsChar() ) );
		return nullptr;
	}

	// parse the ID and compare
	const AnsiChar* txt = val["id"].GetString();
	Red::Core::ResourceManagement::CResourceId resId;
	if ( !resId.FromString( txt ) )
	{
		ERR_WCC( TXT("Resource '%ls' has invalid ID specified" ), ANSI_TO_UNICODE( filePath.AsChar() ) );
		return nullptr;
	}

	Red::TUniquePtr<FileEntry> entry( new FileEntry( filePath ) );

	// make sure the ID matched
	if ( !(resId == entry->GetFileId()) )
	{
		ERR_WCC( TXT("Resource '%ls' has different calculated ID than the saved one!" ), ANSI_TO_UNICODE( filePath.AsChar() ) );
		return nullptr;
	}

	// parse generic stuff
	entry->m_flagIsSeedFile = GetAttrInt(val, "seed", 0) != 0;
	entry->m_flagIsBufferFile = GetAttrInt(val, "buffer", 0) != 0;
	entry->m_flagIsConsumed = GetAttrInt(val, "consumed", 0) != 0;
	entry->m_flagIsAdditional = GetAttrInt(val, "additional", 0) != 0;
	entry->m_memoryCPU = GetAttrInt(val, "cpuMem");
	entry->m_memoryGPU = GetAttrInt(val, "gpuMem");
	entry->m_diskSize = GetAttrInt(val, "diskSize");
	entry->m_className = GetAttrName(val, "class");

	// process dependencies
	ParseDependencies( val[ "soft" ], entry->m_unresolvedSoftDependencies );
	ParseDependencies( val[ "hard" ], entry->m_unresolvedHardDependencies );
	ParseDependencies( val[ "inplace" ], entry->m_unresolvedInplaceDependencies );

	// parse the usage list
	const JSONValue& usageList = val["usage"];
	if (usageList.IsArray())
	{
		const Uint32 numEntries = usageList.Size();
		for ( Uint32 i=0; i<numEntries; ++i )
		{
			UsageEntry* usage = UsageEntry::Parse(usageList[i]);
			if ( usage )
			{
				const Uint64 hash = usage->CalcRuntimeHash();
				entry->m_usage.Insert( hash, usage );
			}
		}
	}

	return entry.Release();
}

void CCookerDataBase::FileEntry::ParseDependencies( const JSONValue& val, TUnresolvedDependencies& outDependencies )
{
	if ( !val.IsArray() )
		return;

	const rapidjson::SizeType numFiles = val.Size();
	for( rapidjson::SizeType i = 0; i<numFiles; ++i )
	{
		const JSONValue& fileEntry = val[ i ];
		if ( !fileEntry.IsString() )
			continue;

		// parse the hex back to the resource ID
		const AnsiChar* txt = fileEntry.GetString();
		Red::Core::ResourceManagement::CResourceId resId;
		if ( !resId.FromString( txt ) )
		{
			WARN_WCC( TXT("Invalid dependency resource id" ) );
			continue;;
		}

		// add to list
		outDependencies.PushBack( resId );
	}
}

//----

Bool CCookerDataBase::LoadFromFile( const String& absoluteFilePath, const Bool includeAdditionalFiles/*=false*/ )
{
	CTimeCounter timer;

	Reset();

	// load the file data
	LOG_WCC( TXT("Loading cooker data base...") );
	TDynArray< Uint8 > fileData;
	if ( !GFileManager->LoadFileToBuffer( absoluteFilePath, fileData, true ) ) // JSON parser needs the NULL at the end
		return false;

	// parse JSON file
	LOG_WCC( TXT("Parsing cooker data base...") );
	JSONDocument d;
	d.Parse<0>((AnsiChar*) fileData.Data());

	// errors, cooker data base is invalid, manual deletion is required
	if ( d.HasParseError() )
	{
		ERR_WCC( TXT("JSON parsing error: '%s'"), d.GetParseError() );
		ERR_WCC( TXT("Cooker data base is corrupted!") );
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
			if ( entry->m_flagIsAdditional && !includeAdditionalFiles )
			{
				delete entry;
				continue;
			}

			m_entriesMap[ entry->GetFileId() ] = entry;
			m_entries.PushBack( entry );
		}
	}

	// resolve file dependencies
	for ( FileEntry* file : m_entries )
		file->ResolveUnresolvedDependencies( m_entriesMap );

	// stats
	LOG_WCC( TXT("Parsed %d existing entries from cooker data base in %1.2fs"), 
		m_entries.Size(), timer.GetTimePeriod() );
	return true;
}

Bool CCookerDataBase::LoadFromFile( const TList<String>& cookDBFilePaths, const Bool includeAdditionalFiles /*= false*/ )
{
	CTimeCounter timer;

	Reset();

	Uint32 groupIndex = 0;
	for( const String& absoluteFilePath : cookDBFilePaths)
	{
		// load the file data
		LOG_WCC( TXT("Loading cooker data base '%ls'..."), absoluteFilePath );
		TDynArray< Uint8 > fileData;
		if ( !GFileManager->LoadFileToBuffer( absoluteFilePath, fileData, true ) ) // JSON parser needs the NULL at the end
			return false;

		// parse JSON file
		LOG_WCC( TXT("Parsing cooker data base '%ls'..."), absoluteFilePath );
		JSONDocument d;
		d.Parse<0>((AnsiChar*) fileData.Data());

		// errors, cooker data base is invalid, manual deletion is required
		if ( d.HasParseError() )
		{
			ERR_WCC( TXT("JSON parsing error: '%s'"), d.GetParseError() );
			ERR_WCC( TXT("Cooker data base '%ls' is corrupted!"), absoluteFilePath );
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
				if ( entry->m_flagIsAdditional && !includeAdditionalFiles )
				{
					delete entry;
					continue;
				}

				entry->m_groupIndex = groupIndex; // different DBs will have different group indices

				FileEntry* existingEntry = nullptr;
				m_entriesMap.Find( entry->GetFileId(), existingEntry );
				if ( !existingEntry )
				{
					m_entriesMap[ entry->GetFileId() ] = entry;
					m_entries.PushBack( entry );
				}
				else
				{
					LOG_CORE( TXT("Entry '%hs' found again in second DB"), entry->GetFilePath().AsChar() );
					existingEntry->m_groupIndex = groupIndex;

					existingEntry->m_unresolvedHardDependencies.PushBackUnique( entry->m_unresolvedHardDependencies );
					existingEntry->m_unresolvedSoftDependencies.PushBackUnique( entry->m_unresolvedSoftDependencies );
					existingEntry->m_unresolvedInplaceDependencies.PushBackUnique( entry->m_unresolvedInplaceDependencies );

					delete entry;
				}
			}
		}

		// numerate
		groupIndex += 1;
	}


	// resolve file dependencies
	for ( FileEntry* file : m_entries )
		file->ResolveUnresolvedDependencies( m_entriesMap );

	// stats
	LOG_WCC( TXT("Parsed %d existing entries from cooker data base in %1.2fs"), 
		m_entries.Size(), timer.GetTimePeriod() );
	return true;
}

Bool CCookerDataBase::SaveToFile( const String& absoluteFilePath, const Uint32 groupIndexToSave )
{
	CTimeCounter timer;

	// resolve dependency order
	// we want to sort the dependencies in their loading order (deeper first)
	TDynArray< CCookerDataBase::FileEntry* > orderedEntries;
	orderedEntries.Reserve( m_entries.Size() );
	for ( auto it = m_entriesMap.Begin(); it != m_entriesMap.End(); ++it )
	{
		CCookerDataBase::FileEntry* entry = (*it).m_second;
		entry->m_dependencyLevel = -1;
		orderedEntries.PushBack(entry);
	}

	// walk the dependency graph
	for ( auto it = m_entriesMap.Begin(); it != m_entriesMap.End(); ++it )
	{
		CCookerDataBase::FileEntry* entry = (*it).m_second;
		entry->ResolveDepenencyOrder(0);
	}

	struct CompareEntryDepth
	{
		bool operator()( const FileEntry* lh, const FileEntry* rh ) const
		{
			// sort by decreasing dependency level
			if ( lh->m_dependencyLevel > rh->m_dependencyLevel )
				return true;
			if ( lh->m_dependencyLevel < rh->m_dependencyLevel )
				return false;

			// sort by path name at the same level
			if ( lh->GetFilePath() < rh->GetFilePath() )
				return true;

			return false;
		}
	};

	// sort the entries so they are stored in the proper loading order
	if ( !orderedEntries.Empty() )
	{
		CompareEntryDepth pred;
		SortUtils::IntroSort( &orderedEntries[0], orderedEntries.Size(), pred );
	}

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

			for ( Uint32 i=0; i<orderedEntries.Size(); ++i )
			{
				const CCookerDataBase::FileEntry* entry = orderedEntries[i];
				if ( entry->m_groupIndex == groupIndexToSave )
				{
					entry->Write( writer );
				}
			}

			writer.EndArray();
		}
		writer.EndObject();
	}

	// stats
	LOG_WCC( TXT("Cooker data base saved in %1.2fs (%d entries, %1.2f MB)"),
		timer.GetTimePeriod(), m_entries.Size(), outputFile->GetSize() / (1024.0f*1024.0f) );

	delete outputFile;

	return true;
}
/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/bundledefinition.h"
#include "../../common/core/stringConversion.h"
#include "../../common/engine/world.h"
#include "../../common/game/gameworld.h"
#include "../../common/engine/umbraTile.h"
#include "../../common/engine/terrainTile.h"
#include "splitFilesCommandlet.h"
#include "cookDataBase.h"
#include "cookSeedFile.h"
#include "cookSplitList.h"
#include "playgoHelper.h"

IMPLEMENT_ENGINE_CLASS( CSplitFilesCommandlet )

//---------------------------

Int32 CSplitFilesCommandlet::ChunkMask::GetChunkIndex( const CName& chunkName )
{
	Int32 index = -1;

	const AnsiChar* chunkNameStr = chunkName.AsAnsiChar();
	if ( 0 == Red::StringCompareNoCase( chunkNameStr, "content", 7 ) )
	{
		index = atoi( chunkNameStr+7 );
	}

	return index;
}

CName CSplitFilesCommandlet::ChunkMask::GetChunkdName( const Uint32 chunkIndex )
{
	switch ( chunkIndex )
	{
		case 0: return CNAME( content0 );
		case 1: return CNAME( content1 );
		case 2: return CNAME( content2 );
		case 3: return CNAME( content3 );
		case 4: return CNAME( content4 );
		case 5: return CNAME( content5 );
		case 6: return CNAME( content6 );
		case 7: return CNAME( content7 );
		case 8: return CNAME( content8 );
		case 9: return CNAME( content9 );
		case 10: return CNAME( content10 );
		case 11: return CNAME( content11 );
		case 12: return CNAME( content12 );
		case 13: return CNAME( content13 );
		case 14: return CNAME( content14 );
		case 15: return CNAME( content15 );
		case 16: return CNAME( content16 );
	}

	return CName::NONE;
}

//---------------------------

CSplitFilesCommandlet::CSplitFilesCommandlet()
{
	m_commandletName = CName( TXT("split") );
}

CSplitFilesCommandlet::~CSplitFilesCommandlet()
{
}

Bool CSplitFilesCommandlet::Execute( const CommandletOptions& options )
{
	// get cook DB path
	String cookDBPath;
	if ( !options.GetSingleOptionValue( TXT("db"), cookDBPath ) )
	{
		ERR_WCC( TXT("Expected path to cook.db") );
		return false;
	}

	// check if we have the seed files
	if ( !options.HasOption( TXT("seed") ) )
	{
		ERR_WCC( TXT("Expected at least one seed file") );
		return false;
	}

	// output path
	String outputPath;
	if ( !options.GetSingleOptionValue( TXT("out"), outputPath ) )
	{
		ERR_WCC( TXT("Expected output path") );
		return false;
	}

	// custom override for fallback chunk
	String fallbackChunkName;
	options.GetSingleOptionValue( TXT("fallback"), fallbackChunkName );

	// import needed stuff from cook.db
	if ( !LoadCookDB( cookDBPath ) )
		return false;

	// import needed stuff from seed files
	const auto seedFileList = options.GetOptionValues( TXT("seed") );
	if ( !LoadSeedFiles( seedFileList ) )
		return false;

	// distribute the files to proper chunks
	if ( !ResolveFinalChunks() )
		return false;

	// validate splitting - all hard dependencies and soft dependencies of every file must be accessible
	if ( !ValidateSplit() )
		return false;

	// save output
	if ( !WriteOutputFile( outputPath ))
		return false;

	// done
	return true;
}

Bool CSplitFilesCommandlet::LoadCookDB( const String& absolutePath )
{
	LOG_WCC( TXT("Loading cooker data base...") );

	if ( !m_dataBase.LoadFromFile( absolutePath ) )
	{
		ERR_WCC( TXT("Failed to load cook.db from '%ls'"), absolutePath.AsChar() );
		return false;
	}

	LOG_WCC( TXT("Extracting cooker data base...") );

	TDynArray< CCookerResourceEntry > dataBaseFiles;
	m_dataBase.GetFileEntries(dataBaseFiles);
	Uint32 numSoftDeps = 0, numHardDeps = 0;
	for ( Uint32 fileIndex = 0; fileIndex<dataBaseFiles.Size(); ++fileIndex )
	{
		const CCookerResourceEntry& entry = dataBaseFiles[fileIndex];

		// create file info
		FileInfo* file = GetFile( entry.GetFileId(), entry.GetFilePath() );
		if ( !file )
			return false; // DB error

		// get file class (used for filtering dependencies)
		m_dataBase.GetFileResourceClass( file->m_dbId, file->m_fileClass );

		// get file size
		m_dataBase.GetFileDiskSize( file->m_dbId, file->m_fileSize );

		// get file dependencies (from DB)
		TDynArray< CCookerResourceEntry > softDeps, hardDeps, inplaceDeps;
		m_dataBase.GetFileDependencies( file->m_dbId, hardDeps, softDeps, inplaceDeps );

		// process dependencies
		file->m_hardDependencies.Reserve( hardDeps.Size() );
		for ( const CCookerResourceEntry& dep : hardDeps )
		{
			FileInfo* depFile = GetFile( dep.GetFileId(), dep.GetFilePath() );
			if ( !depFile )
				return false; // DB error

			file->m_hardDependencies.PushBack( depFile );
			numHardDeps += 1;
		}

		// process soft dependencies
		file->m_softDependencies.Reserve( hardDeps.Size() );
		for ( const CCookerResourceEntry& dep : softDeps )
		{
			FileInfo* depFile = GetFile( dep.GetFileId(), dep.GetFilePath() );
			if ( !depFile )
				return false; // DB error

			file->m_softDependencies.PushBack( depFile );
			numSoftDeps += 1;
		}
	}

	LOG_WCC( TXT("Extracted %d files from cooker data base (%d hard dependencies, %d soft dependencies)"), 
		m_allFiles.Size(), numHardDeps, numSoftDeps );
	return true;
}

Bool CSplitFilesCommandlet::LoadSeedFiles( const TList< String >& seedFileList )
{
	Uint32 numValidFiles = 0;
	Uint32 numMissingFiles = 0;
	Uint32 numFilesWithChunks = 0;

	// load the seed files and extract chunk information
	LOG_WCC( TXT("Extracting seed files...") );
	for ( auto it = seedFileList.Begin(); it != seedFileList.End(); ++it )
	{
		// load the seed file
		const String& seedFilePath = *it;
		CCookerSeedFile seedFile;
		if ( !seedFile.LoadFromFile( seedFilePath ) )
		{
			ERR_WCC( TXT("Unable to load seed file '%ls'. Generation of output split list will stop."), seedFilePath.AsChar() );
			return false;
		}

		// process entries
		const Uint32 numEntries = seedFile.GetNumEntries();
		for ( Uint32 i=0; i<numEntries; ++i )
		{
			const CCookerSeedFileEntry* entry = seedFile.GetEntry(i);

			// find existing file
			const Red::Core::ResourceManagement::CResourceId fileId( entry->GetFilePath() );
			FileInfo* file = FindFileInfo( fileId );
			if ( !file )
			{
				WARN_WCC( TXT("File '%ls' referenced in seed file '%ls' not found in cook.db"), 
					ANSI_TO_UNICODE( entry->GetFilePath().AsChar() ), CFilePath( seedFilePath ).GetFileName().AsChar() );

				numMissingFiles += 1;
				continue;
			}

			// setup stuff
			file->m_isSeedFile = true;

			// setup inital chunks
			for ( const CName& chunkId : entry->GetFileChunkIDs() )
			{
				file->m_chunks.Merge( ChunkMask( chunkId ) );
			}

			// count stuff
			numValidFiles += 1;
			numFilesWithChunks += entry->GetFileChunkIDs().Empty() ? 0 : 1;
		}
	}

	//  stats
	LOG_WCC( TXT("Extracted %d files from %d seed files (%d missing, %d with assigned chunks)"), 
		numValidFiles, seedFileList.Size(), numMissingFiles, numFilesWithChunks );
	return true;
}

Bool CSplitFilesCommandlet::FileInfo::ShouldPropagateChunkDependency( CName fromResourceType, CName toResourceType, const Bool secondPass )
{
	// do not propagate dependencies
	// NOTE: we can only decide not to propagate dependencies that are SOFT
	if ( fromResourceType == GetTypeName< CQuest >() || fromResourceType == GetTypeName< CQuestPhase >() )
	{
		if ( toResourceType == GetTypeName< CStoryScene >() || toResourceType == GetTypeName< CCommunity >() )
		{
			return false;
		}
	}

	// do not allow red games to pull in the world files
	if ( fromResourceType == TXT("CWitcherGameResource") )
	{
		if ( toResourceType == GetTypeName< CWorld >() || toResourceType == GetTypeName< CGameWorld >() )
		{
			return false;
		}
	}

	return true;
}

void CSplitFilesCommandlet::FileInfo::PropagateChunks( const FileInfo* parentFile, ChunkMask chunkMask, const Bool isSoftDependency, const Bool secondPass )
{
	// legal ?
	if ( isSoftDependency && parentFile )
	{
		if ( !ShouldPropagateChunkDependency( parentFile->m_fileClass, m_fileClass, secondPass ) )
		{
			// ignore some of the cases, only for soft dependencies
			return;
		}
	}

	// already visited ? if so than it's nothing new to explore here
	if ( !m_tempChunks.Merge( chunkMask ))
		return;

	// add chunk dependency
	const Int32 prevChunk = m_chunks.GetFinalChunk();
	m_chunks.Merge( chunkMask );
	const Int32 curChunk = m_chunks.GetFinalChunk();

	// report movement in chunks
	if ( secondPass && prevChunk != -1 && curChunk < prevChunk )
	{
		LOG_WCC( TXT("Advanced '%hs' %d->%d because of %hs dep from '%hs'"), 
			m_filePath.AsChar(), prevChunk, curChunk, 
			isSoftDependency ? "soft" : "hard",
			parentFile ? parentFile->m_filePath.AsChar() : "null" );
	}

	// recurse via hard dependencies (no excuses)
	for ( FileInfo* file : m_hardDependencies )
	{
		file->PropagateChunks( this, chunkMask, false, secondPass );
	}

	// recurse via hard dependencies (some hacks allowed)
	for ( FileInfo* file : m_softDependencies )
	{
		file->PropagateChunks( this, chunkMask, true, secondPass );
	}
}

Bool CSplitFilesCommandlet::ResolveFinalChunks()
{
	// resolve logic:
	//  propagate chunks through dependencies
	//  resolve generated chunk list to leave one final chunk in each entry
	{
		CTimeCounter timer;

		for ( FileInfo* info : m_allFiles )
		{
			info->m_tempChunks.m_chunks.ClearAll();
		}

		for ( FileInfo* info : m_allFiles )
		{
			if ( info->m_chunks.m_chunks.IsAnySet() )
			{
				info->PropagateChunks( nullptr, info->m_chunks, false, false );
			}
		}

		LOG_WCC( TXT("Chunks propagated in %1.3fs"), timer.GetTimePeriod() );
	}

	// put all unmarked resources in the fallback chunk
	{
		Uint32 numOrphanedFiles = 0;
		for ( FileInfo* info : m_allFiles )
		{
			if ( info->m_chunks.m_chunks.IsNoneSet() )
			{
				LOG_WCC( TXT("Orphande file '%hs' placed in startup chunk"), 
					info->m_filePath.AsChar() );

				info->m_chunks.m_chunks.Set(0); // put in content0
				numOrphanedFiles += 1;
			}
		}

		if ( numOrphanedFiles )
		{
			LOG_WCC( TXT("Moved %d orphaned files"), numOrphanedFiles );
		}
	}

	// propagate the dependencies again
	{
		CTimeCounter timer;

		for ( FileInfo* info : m_allFiles )
		{
			info->m_tempChunks.m_chunks.ClearAll();
		}

		for ( FileInfo* info : m_allFiles )
		{
			if ( info->m_chunks.m_chunks.IsAnySet() )
			{
				info->PropagateChunks( nullptr, info->m_chunks, false, true );
			}
		}

		LOG_WCC( TXT("Chunks propagated (phase two) in %1.3fs"), timer.GetTimePeriod() );
	}

	// resolve chunk at each file
	{
		Uint32 numFallbackFiles = 0;

		CTimeCounter timer;
		for ( FileInfo* info : m_allFiles )
		{
			// resolve using the first set bit
			const Int32 installChunk = info->m_chunks.GetFinalChunk();
			if ( installChunk == -1 )
			{
				ERR_WCC( TXT("File '%hs' still not resolved@"), info->m_filePath.AsChar() );
				return false;
			}

			// assign
			info->m_resolvedChunkIndex = installChunk;
			info->m_resolvedChunk = ChunkMask::GetChunkdName( installChunk );
		}

		LOG_WCC( TXT("Final chunks resolved in %1.3fms (%d files used fallback)"), timer.GetTimePeriod(), numFallbackFiles );
	}

	// done
	return true;
}

Bool CSplitFilesCommandlet::ValidateSplit()
{
	CTimeCounter timer;

	// sizes of the data in chunks
	Uint64 chunkSizes[ ChunkMask::MAX_CHUNKS ];
	Uint32 chunkCounts[ ChunkMask::MAX_CHUNKS ];
	Red::MemoryZero( chunkSizes, sizeof(chunkSizes) );
	Red::MemoryZero( chunkCounts, sizeof(chunkCounts) );	

	// make sure that all dependencies of a given file are installed before the file
	Bool status = true;
	for ( FileInfo* info : m_allFiles )
	{
		// count sizes of the data in each chunk
		chunkSizes[ info->m_resolvedChunkIndex ] += info->m_fileSize;
		chunkCounts[ info->m_resolvedChunkIndex ] += 1;

		// check hard deps
		for ( FileInfo* depInfo : info->m_hardDependencies )
		{
			if ( depInfo->m_resolvedChunkIndex > info->m_resolvedChunkIndex )
			{
				ERR_WCC( TXT("File '%hs' (hard dep of '%hs') is installed in '%hs' which is after '%hs'"),
					depInfo->m_filePath.AsChar(), info->m_filePath.AsChar(),
					depInfo->m_resolvedChunk.AsAnsiChar(), info->m_resolvedChunk.AsAnsiChar() );
				status = false;
			}
		}

		// check soft deps
		for ( FileInfo* depInfo : info->m_softDependencies )
		{
			if ( depInfo->m_resolvedChunkIndex > info->m_resolvedChunkIndex )
			{				
				WARN_WCC( TXT("File '%hs' (soft dep of '%hs') is installed in '%hs' which is after '%hs'"),
					depInfo->m_filePath.AsChar(), info->m_filePath.AsChar(),
					depInfo->m_resolvedChunk.AsAnsiChar(), info->m_resolvedChunk.AsAnsiChar() );
			}
		}
	}

	LOG_WCC( TXT("Split checked in %1.3fms"), timer.GetTimePeriodMS() );

	for ( Uint32 i=0; i<ChunkMask::MAX_CHUNKS; ++i )
	{
		if ( chunkSizes[i] > 0 )
		{
			LOG_WCC( TXT(" content%d: %d files, %1.2fMB" ), 
				i, chunkCounts[i], chunkSizes[i] / (1024.0f*1024.0f) );
		}
	}

	if ( !status )
	{
		ERR_WCC( TXT("!!! SPLITING IS BROKEN !!! - the split alogorithm has failed to split the content. Please debug - Ask Dex/DavidB") );
		//return false;
	}

	// done
	return true;
}

Bool CSplitFilesCommandlet::WriteOutputFile( const String& outputFilePath )
{
	CTimeCounter timer;

	// prepare the file data
	CCookerSplitFile splitFile;
	for ( FileInfo* info : m_allFiles )
	{
		// just to make sure...
		if ( !info->m_resolvedChunk )
		{
			ERR_WCC( TXT("File '%ls' does not have a proper content chunk assigned"), ANSI_TO_UNICODE( info->m_filePath.AsChar() ) );
			return false;
		}

		// emit data to output file
		splitFile.AddEntry( info->m_filePath, info->m_resolvedChunk );
	}

	// save the file
	if ( !splitFile.SaveToFile( outputFilePath ) )
	{
		ERR_WCC( TXT("Failed to save output split file into '%ls'"), outputFilePath.AsChar() );
		return false;
	}

	// saved
	LOG_WCC( TXT("File split list saved in %1.3fms (%d entries)"), timer.GetTimePeriod(), splitFile.GetNumEntries() );
	return true;
}

CSplitFilesCommandlet::FileInfo* CSplitFilesCommandlet::GetFile( const ResourceId& id, const StringAnsi& depotPath )
{
	FileInfo* ret = nullptr;
	if ( !m_allFilesMap.Find( id, ret ) )
	{
		// look file up in the DB - it's a fatal error if it's not there
		const DbID dbId = m_dataBase.GetFileEntry( depotPath );
		if ( dbId == CCookerDataBase::NO_ENTRY )
		{
			ERR_WCC( TXT("No DB entry for file '%ls'. File was not cooked."), ANSI_TO_UNICODE( depotPath.AsChar() ) );
			return nullptr;
		}

		// create entry
		ret = new FileInfo( dbId, id, depotPath );
		m_allFilesMap[ id ] = ret;
		m_allFiles.PushBack( ret );
	}

	return ret;
}

CSplitFilesCommandlet::FileInfo* CSplitFilesCommandlet::FindFileInfo( const ResourceId& id ) const
{
	FileInfo* ret = nullptr;
	m_allFilesMap.Find( id, ret );
	return ret;
}

void CSplitFilesCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Required usage:") );
	LOG_WCC( TXT("  split -db=<cook.db> [-seed=<seedfile.seed>]* -out=<path>") );
	LOG_WCC( TXT("Arguments:") );
	LOG_WCC( TXT("  -db=<cook.db>			- Path to the cook.db file") );
	LOG_WCC( TXT("  -seed=<seedfile.seed>   - Path to the a seed file (multiple files supported)") );
	LOG_WCC( TXT("  -out=<path>				- Output file name") );
	LOG_WCC( TXT("  -fallback=<name>		- Specify custom fallback chunk for unassigned resources") );
}

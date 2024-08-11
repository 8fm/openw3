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
#include "exportBundlesCommandlet.h"
#include "cookSeedFile.h"

IMPLEMENT_ENGINE_CLASS( CExportBundlesCommandlet )

//------------------

void CExportBundlesCommandlet::FileInfo::AddToBundle( BundleInfo* bundle )
{
	if ( bundle && !m_bundles.Exist( bundle ) )
	{
		m_bundles.PushBack( bundle );
		RED_ASSERT( !bundle->m_files.Exist(this), TXT("File already in bundle") );
		bundle->m_files.PushBack( this );
	}
}

void CExportBundlesCommandlet::FileInfo::RemoveFromBundle( BundleInfo* bundle )
{
	if ( m_bundles.Exist( bundle ) )
	{
		RED_ASSERT( bundle->m_files.Exist(this), TXT("File not in bundle") );
		bundle->m_files.Remove( this );
		m_bundles.Remove( bundle );
	}
}

void CExportBundlesCommandlet::FileInfo::RemoveFromAllBundles()
{
	for ( BundleInfo* bundle : m_bundles )
	{
		RED_ASSERT( bundle->m_files.Exist(this), TXT("File not in bundle") );
		bundle->m_files.Remove( this );
	}

	m_bundles.Clear();
}

const Bool CExportBundlesCommandlet::FileInfo::IsInBundles() const
{
	return !m_bundles.Empty();
}

const Bool CExportBundlesCommandlet::FileInfo::IsInSomeAlwaysAccessibleBundle() const
{
	for ( const BundleInfo* bundle : m_bundles )
	{
		if ( bundle->IsAlwaysAccessible() )
			return true;
	}

	return false;
}

const Bool CExportBundlesCommandlet::FileInfo::IsInSeedBundles() const
{
	return !m_seedBundles.Empty();
}

//------------------

CExportBundlesCommandlet::CExportBundlesCommandlet()
{
	m_fastCompression = false;
	m_commandletName = CName( TXT("exportbundles") );
}

CExportBundlesCommandlet::~CExportBundlesCommandlet()
{
}

Bool CExportBundlesCommandlet::Execute( const CommandletOptions& options )
{
	// get the path to the cooker data base
	String cookDataBasePath;
	if ( !options.GetSingleOptionValue( TXT("db"), cookDataBasePath ) )
	{
		ERR_WCC( TXT("Cooker data base file is not specified") );
		return false;
	}

	// fast compression
	if ( options.HasOption( TXT("extrafast") ) )
	{
		LOG_WCC( TXT("No bundle compression will be used") );
		m_fastCompression = true;
	}

	// load cooker data base
	if ( !m_dataBase.LoadFromFile( cookDataBasePath ) )
	{
		ERR_WCC( TXT("Failed to load cooker data base from '%ls'"), cookDataBasePath.AsChar() );
		return false;
	}

	// get the output file name
	String outputFilePath;
	if ( !options.GetSingleOptionValue( TXT("out"), outputFilePath ) )
	{
		ERR_WCC( TXT("Output JSON path not specified") );
		return false;
	}

	// no seed files
	if ( !options.HasOption( TXT("seed") ) )
	{
		ERR_WCC( TXT("Seed files not specified") );
		return false;
	}

	// convert cooker DB files into our local representation for further processing
	{
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

			// set file index
			RED_ASSERT( file->m_fileIndex == -1 );
			file->m_fileIndex = fileIndex;

			// get file size
			m_dataBase.GetFileDiskSize( file->m_dbId, file->m_fileSize );
			m_dataBase.GetFileConsumedFlag( file->m_dbId, file->m_isConsumed );

			// some file types are exclusive - can only be added to seed bundles
			const StringAnsi ext = StringHelpers::GetFileExtension( file->m_filePath );
			if ( ext == "redgame" )
				file->m_isNotPropagating = true;

			// get file dependencies (from DB)
			TDynArray< CCookerResourceEntry > softDeps, hardDeps, inplaceDeps;
			m_dataBase.GetFileDependencies( file->m_dbId, hardDeps, softDeps, inplaceDeps );

			// process hard dependencies
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
			file->m_softDependencies.Reserve( softDeps.Size() );
			for ( const CCookerResourceEntry& dep : softDeps )
			{
				FileInfo* depFile = GetFile( dep.GetFileId(), dep.GetFilePath() );
				if ( !depFile )
					return false; // DB error

				if(  StringHelpers::GetFileExtension( depFile->m_filePath ) == "buffer" )
				{
					file->m_bufferDependencies.PushBack( depFile );
				}
				else
				{
					file->m_softDependencies.PushBack( depFile );
				}
		
				numSoftDeps += 1;
			}
		}

		LOG_WCC( TXT("Extracted %d files from cooker data base (%d hard dependencies, %d soft dependencies)"), 
			m_allFiles.Size(), numHardDeps, numSoftDeps );
	}

	// get the path to the split list
	String splitFilePath;
	CCookerSplitFile splitFile;
	if ( options.GetSingleOptionValue( TXT("split"), splitFilePath ) )
	{
		LOG_WCC( TXT("Bundle export will use file splitting data from '%ls'"), splitFilePath.AsChar() );

		// load split data
		if ( !splitFile.LoadFromFile( splitFilePath ) )
		{
			ERR_WCC( TXT("Failed to load split list from '%ls'"), splitFilePath.AsChar() );
			return false;
		}
	}

	// process the initial seed files
	auto seedFiles = options.GetOptionValues( TXT("seed") );
	for ( auto it = seedFiles.Begin(); it != seedFiles.End(); ++it )
	{
		const String& filePath = *it;

		CCookerSeedFile seedFile;
		if ( !seedFile.LoadFromFile(filePath) )
		{
			ERR_WCC( TXT("Failed to load seed file from '%ls'"), filePath.AsChar() );
			continue;
		}

		// process seed file - for every file entry add the seeded file to the bundle
		// NOTE: we can have files in the seed list that did not get cooked, it's kind of OK
		const Uint32 numSeedFiles = seedFile.GetNumEntries();
		for ( Uint32 i=0; i<numSeedFiles; ++i )
		{
			const CCookerSeedFileEntry* fileEntry = seedFile.GetEntry(i);

			// find file
			FileInfo* fileInfo = nullptr;
			Red::Core::ResourceManagement::CResourceId fileId( fileEntry->GetFilePath() );
			if ( !m_allFilesMap.Find( fileId, fileInfo ) )
			{
				WARN_WCC( TXT("File '%hs' does not exist in cook and cannot be added to bundle '%hs'"),
					fileEntry->GetFilePath().AsChar(), fileEntry->GetBundleName().AsChar() );
				continue;
			}

			if( m_seedFiles.Exist( fileInfo ) )
			{
				BundleInfo* bundle = GetBundle( fileEntry->GetBundleName() );
				if ( !bundle )
				{
					continue;
				}
				fileInfo->m_seedBundles.PushBack( bundle );
			}
			else
			{
				// add to seed file list
				m_seedFiles.PushBack( fileInfo );

				// get/create the matching bundle
				BundleInfo* bundle = GetBundle( fileEntry->GetBundleName() );
				if ( !bundle )
				{
					continue;
				}

				// add the file<-> bundle mapping
				fileInfo->m_seedBundles.PushBack(bundle);
			}

		}
	}

	// stats
	LOG_WCC( TXT("Processed %d seed files, %d bundles created, %d seed files in bundles"),
		seedFiles.Size(), m_allBundles.Size(), m_seedFiles.Size() );

	// find and mark bundles that should not be split
	if ( !MarkBundleFlags() )
		return false;

	// mark all used files 
	if ( !MarkUsedFiles() )
		return false;

	// create the buffers bundle
	if ( !FillBuffersBundle() )
		return false;

	// this may create data duplication
	if ( !FillBundles() )
		return false;

	// optimize bundle structure - move stuff into the startup bundle
	if ( !OptimizeBundles() )
		return false;

	// split data
	if ( (splitFile.GetNumEntries() > 0) && !SplitBundlesIntoChunks( splitFile ) )
		return false;

	// validate the bundles
	if ( (splitFile.GetNumEntries() > 0) && !ValidateBundles() )
		return false;

	// make sure we will not have bundles that to large
	if ( !SplitLargeBundles() )
		return false;

	// write the output file
	if ( !WriteOutputFile( outputFilePath ))
		return false;

	return true;

}

void CExportBundlesCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  exportbundles -db=[cookerdb] -seed=[seedfiles] -spatial=[seedfiles] -out=[outputfile]") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Params:") );
	LOG_WCC( TXT("  -db=<cookerdb>       - Cooker data base file") );
	LOG_WCC( TXT("  -seed=<seedfiles>    - Initial list of seed files") );
	LOG_WCC( TXT("  -spatial=<seedfiles> - Additional list of seed files that represent spatial configuration (they override the existing bundles)") );
	LOG_WCC( TXT("  -out=<outputfile>    - Absolute path to the final output file (JSON bundle list)") );
	LOG_WCC( TXT("  -split=<splitfile>	 - Split bundles into chunks using given split file"));
	LOG_WCC( TXT("  -extrafast           - Use fast compression for bunles (much faster, larged output bundles)") );
}

CExportBundlesCommandlet::BundleInfo* CExportBundlesCommandlet::GetBundle( const StringAnsi& bundleName, const Bool createIfNotFound /*= true */ )
{
	BundleInfo* ret = nullptr;
	if ( !m_allBundlesMap.Find( bundleName, ret ) )
	{
		if ( createIfNotFound )
		{
			ret = new BundleInfo( bundleName );
			m_allBundlesMap[ bundleName ] = ret;
			m_allBundles.PushBack( ret );
		}
	}

	return ret;
}

CExportBundlesCommandlet::FileInfo* CExportBundlesCommandlet::GetFile( const ResourceId& id, const StringAnsi& depotPath )
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

Bool CExportBundlesCommandlet::MarkBundleFlags()
{
	// HACK LIST !!!!!
	THashMap< StringAnsi, CName > forcedContentMapping;

	// some bundles are always at content0
	forcedContentMapping.Insert( "startup", CName( TXT("content0" ) ) );
	forcedContentMapping.Insert( "common", CName( TXT("content0" ) ) );
	forcedContentMapping.Insert( "xml", CName( TXT("content0" ) ) );
	forcedContentMapping.Insert( "r4items", CName( TXT("content0" ) ) );	
	forcedContentMapping.Insert( "r4gui", CName( TXT("content0" ) ) );	

	// levels
	forcedContentMapping.Insert( "world_kaer_morhen", CName( TXT("content1" ) ) );
	forcedContentMapping.Insert( "world_prolog_village", CName( TXT("content2" ) ) );
	forcedContentMapping.Insert( "world_wyzima_castle", CName( TXT("content3" ) ) );
	forcedContentMapping.Insert( "world_novigrad", CName( TXT("content4" ) ) );
	forcedContentMapping.Insert( "world_skellige", CName( TXT("content5" ) ) );
	forcedContentMapping.Insert( "world_island_of_mist", CName( TXT("content7" ) ) );
	forcedContentMapping.Insert( "world_the_spiral", CName( TXT("content10" ) ) );
	forcedContentMapping.Insert( "world_prolog_village_winter", CName( TXT("content12" ) ) );
	
	for ( BundleInfo* bundle : m_allBundles )
	{
		const StringAnsi& bundleName = bundle->m_name;

		for ( const auto& pairs : forcedContentMapping )
		{
			const StringAnsi& contentPattern = pairs.m_first;
			if ( bundleName.BeginsWith( contentPattern ) )
			{
				bundle->m_forcedChunk = pairs.m_second;

				LOG_WCC( TXT("Snapped bundle '%hs' to content chunk '%ls'"), 
					bundleName.AsChar(), bundle->m_forcedChunk.AsChar() );

				break;
			}
		}
	}

	return true;
}

Bool CExportBundlesCommandlet::MarkUsedFiles()
{
	// mark all accessible files
	Uint32 numUsedFiles = 0;
	for ( FileInfo* seedFile : m_seedFiles )
	{
		seedFile->Visit<1,1,1>(
			[&numUsedFiles](FileInfo* file)
			{
				if ( file->m_isUsed )
					return false;

				file->m_isUsed = true;
				numUsedFiles += 1;
				return true;
			}
		);
	}

	// report unused files (cooked but not reachable through seed files)
	if ( numUsedFiles < m_allFiles.Size() )
	{
		LOG_WCC( TXT("Only %d files out of %d are used (%d files not used)"), 
		numUsedFiles, m_allFiles.Size(), m_allFiles.Size() - numUsedFiles );

		for ( FileInfo* file : m_allFiles )
		{
			if ( !file->m_isUsed )
			{
				LOG_WCC( TXT("Not used file '%hs'"), file->m_filePath.AsChar());
			}
		}
	}
	else
	{
		LOG_WCC( TXT("All %d files are used"), m_allFiles.Size() );
	}

	// any way, it's not an error to have unused files
	return true;
}

Bool CExportBundlesCommandlet::FillBuffersBundle()
{
	BundleInfo* buffers = GetBundle( "buffers" );

	// process all of the buffers
	for ( FileInfo* file : m_allFiles )
	{
		if ( file->m_filePath.EndsWith( ".buffer" ) )
		{
			file->RemoveFromAllBundles();
			file->AddToBundle( buffers );
		}
	}

	// done
	return true;
}

Bool CExportBundlesCommandlet::FillBundles()
{
	// get blob bundle
	BundleInfo* blob = GetBundle( "blob" );

	THashMap<StringAnsi, FileInfo*> censoredFiles;

	// HACK: force remove w2w files from the blob bundle
	for ( FileInfo* seedFile : m_allFiles )
	{
		if ( seedFile->m_filePath.EndsWith( ".w2w") )
		{
			ERR_WCC( TXT("World file '%hs' is in the blob bundle, removing"), seedFile->m_filePath.AsChar() );

			seedFile->RemoveFromBundle( blob );
			seedFile->m_seedBundles.Remove(blob);
		}
		else
		{
			size_t lastDotIndex = 0;
			if( seedFile->m_filePath.FindCharacter('.', lastDotIndex, true) )
			{
				StringAnsi filePathWithoutExtension = seedFile->m_filePath.LeftString(lastDotIndex);
				static const StringAnsi censoredPostfix("_censored");
				if(filePathWithoutExtension.EndsWith(censoredPostfix))
				{
					StringAnsi filePathExtension = seedFile->m_filePath.RightString(seedFile->m_filePath.GetLength() - (lastDotIndex+1));
					StringAnsi filePathNotCensored = filePathWithoutExtension.LeftString(filePathWithoutExtension.GetLength() - censoredPostfix.GetLength());
					filePathNotCensored += "." + filePathExtension;
					
					censoredFiles.Insert(filePathNotCensored, seedFile);

					seedFile->RemoveFromBundle( blob );
					seedFile->m_seedBundles.Remove(blob);
				}
			}
		}
	}

	// process the seed files, propagate the bundle assignment through the hard dependencies
	for ( FileInfo* seedFile : m_seedFiles )
	{
		for ( BundleInfo* seedBundle : seedFile->m_seedBundles )
		{
			LOG_WCC( TXT("SeedFile '%hs', seed bundle '%hs'"), seedFile->m_filePath.AsChar(), seedBundle->m_name.AsChar() );

			seedFile->Visit<1,0,0>(
				[seedBundle, seedFile, censoredFiles](FileInfo* file)
				{
					// already in bundle
					if ( file->m_bundles.Exist( seedBundle ) )
						return false;

					// file is not propagating it's dependencies
					//if ( file->m_isNotPropagating )
						//return false;

					// add to seed bundle
					if ( file != seedFile )
					{
						LOG_WCC( TXT("Propagated '%hs' from '%hs' to '%hs'"), file->m_filePath.AsChar(), seedFile->m_filePath.AsChar(), seedBundle->m_name.AsChar() );
					}

					file->AddToBundle( seedBundle );

					//!========================================================================================================================
					//! Search for censored version
					//! Censored and original version have to be in the same bundle
					//!
					//! During patching process we copy whole atomic bundles and if original and censored version are not in the same bundle
					//! we do not have full information about that is original resource have censored version
					//!
					//! ex. t_02__q701_dagonet_giant_headless_corpse.w2mesh is in world_bob_startup.bundle
					//!     t_02__q701_dagonet_giant_headless_corpse_censored.w2mesh is in blob.bundle
					//!     during patch metadata.store generation we have only world_bob_startup.bundle so metadata generator can not find
					//!     "_censored" version of t_02__q701_dagonet_giant_headless_corpse.w2mesh and as entry in metadata.store is 
					//!     original t_02__q701_dagonet_giant_headless_corpse.w2mesh
					//!=======================================================================================================================
					FileInfo* fileCensored = nullptr;
					if( censoredFiles.Find(file->m_filePath, fileCensored) )
					{
						fileCensored->AddToBundle( seedBundle );
					}

					return true;
				}
			);
		}
	}

	// add unassigned files to the blob bundle
	Uint32 numOrphanedFiles = 0;
	for ( FileInfo* file : m_allFiles )
	{
		if ( !file->IsInSomeAlwaysAccessibleBundle() && !file->IsInSeedBundles() )
		{
			// some extensions are excluded from this bull shit
			const StringAnsi ext = StringHelpers::GetFileExtension( file->m_filePath );
			if ( ext == "w2ter" || ext == "w3occlusion" || ext.BeginsWith( "nav") )
				continue;

			// make sure it's always accessible
			file->AddToBundle( blob );
			numOrphanedFiles += 1;
		}
	}
	LOG_WCC( TXT("Added %d orphaned files to the blob bundle"), numOrphanedFiles );

	// no error
	return true;
}

Bool CExportBundlesCommandlet::MakeBaseBundle( BundleInfo* bundle, const BundleInfo* withRespectToBundle )
{
	TDynArray< BundleInfo* > bundlesToRemoveFrom;
	bundlesToRemoveFrom.Reserve( 30 );

	// analyze every file in a bundle
	Uint32 numFilesRemoved = 0;
	for ( FileInfo* file : bundle->m_files )
	{
		// remove file duplicates from other bundles
		bundlesToRemoveFrom.ClearFast();
		for ( BundleInfo* fileBundle : file->m_bundles )
		{
			if ( fileBundle != bundle )
			{
				if ( !withRespectToBundle || (fileBundle == withRespectToBundle) )
				{
					bundlesToRemoveFrom.PushBack( fileBundle );
				}
			}
		}

		// do we need to be removed from the bundles
		if ( bundlesToRemoveFrom.Empty() )
			continue;

		// stats
		numFilesRemoved += 1;

		// perform removal of the bundles
		for ( BundleInfo* fileBundle : bundlesToRemoveFrom )
		{
			file->RemoveFromBundle( fileBundle );
		}
	}

	// ok
	LOG_WCC( TXT("%d files made unique when make a base out of bundle '%hs'"), numFilesRemoved, bundle->m_name.AsChar() );
	return true;
}

Bool CExportBundlesCommandlet::OptimizeBundles()
{
	// make anything that is in the startup bundle exclusive
	if ( !MakeBaseBundle( GetBundle( "startup"), nullptr ) ) // startup it the top most exclusive
		return false;

	// make each _startup bundle for the worlds a base for the _matching runtime bundle
	for ( BundleInfo* bundle : m_allBundles )
	{
		// is this a generic world startup bundle ?
		if ( !bundle->m_name.EndsWith( "_startup") )
			continue;

		// get the generic bundle name
		const StringAnsi genericName = bundle->m_name.StringBefore( "_startup" );
		const StringAnsi runtimeName = genericName + "_runtime";

		// find the runtime bundle
		BundleInfo* runtimeBundle = GetBundle( runtimeName, false /*create*/ );
		if ( !runtimeBundle )
		{
			ERR_WCC( TXT("No runtime bundle for startup bundle '%hs'"), bundle->m_name.AsChar() );
			continue; // this is not a fatal error
		}

		// do filtering - remove everything from the runtime bundle that also appears in the startup bundle
		if ( !MakeBaseBundle( bundle, runtimeBundle ) )
			return false;
	}

	// make sure that anything that is in the r4items bundle is not duplicated in the blob bundles
	if ( !MakeBaseBundle( GetBundle( "r4items"), GetBundle( "blob" ) ) )
		return false;

	// filtering done
	return true;
}

Bool CExportBundlesCommandlet::SplitBundle( BundleInfo* originalBundle, const CCookerSplitFile& splitDB )
{
	// For each file, put it in the right split bundle, preserving original relative file order
	auto originalFiles = originalBundle->m_files; // m_files is modified by the iteration
	for ( FileInfo* file : originalFiles )
	{
		// find info about this file in the split map
		const CCookerSplitFileEntry* splitEntry = splitDB.GetEntry( file->m_fileId );
		if ( !splitEntry )
		{
			ERR_WCC( TXT("No split data for file '%ls' in the split map. Unable to split cook into separate chunks."),
				ANSI_TO_UNICODE( file->m_filePath.AsChar() ) );
			return false;
		}

		// use the forced chunk if we have one
		const CName resolvedChunkID = (originalBundle->m_forcedChunk) 
			? originalBundle->m_forcedChunk
			: splitEntry->GetFileChunkID();

		// assemble path
		const StringAnsi splitBundleName = 
			StringAnsi::Printf( "content\\%ls\\bundles\\%hs", resolvedChunkID.AsString().AsChar(), originalBundle->m_name.AsChar() );

		// get the final split bundle
		BundleInfo* splitBundle = GetBundle( splitBundleName );
		splitBundle->m_splitTag = resolvedChunkID;
		originalBundle->m_splitBundles.PushBackUnique( splitBundle );

		// move the file to the split bundle
		file->RemoveFromBundle( originalBundle );
		file->AddToBundle( splitBundle );
		//LOG_WCC( TXT("File '%hs' split '%hs' -> '%hs'"), file->m_filePath.AsChar(), originalBundle->m_name.AsChar(), splitBundle->m_name.AsChar() );
	}

	// no errors
	return true;
}

Bool CExportBundlesCommandlet::SplitBundlesIntoChunks( const CCookerSplitFile& splitDB )
{
	auto originalBundles = m_allBundles;

	// cleanup current bundles - we will NOT use theme
	m_allBundles.ClearFast();
	m_allBundlesMap.ClearFast();

	// do the splitting
	for ( BundleInfo* bundle : originalBundles )
	{
		if ( !SplitBundle( bundle, splitDB ) )
		{
			return false;
		}
	}

	// print split table
	{
		LOG_WCC( TXT("-----------------------------------------------------------------") );
		static const Double OneMB = 1024.0*1024.0;
		::Sort( m_allBundles.Begin(), m_allBundles.End(), []( const BundleInfo* a, const BundleInfo* b ) { return a->m_name < b->m_name; } );
		for ( BundleInfo* bundle : m_allBundles )
		{
			LOG_WCC( TXT("Splits map for bundle '%ls' (%d files, %1.2f MB):"), 
				ANSI_TO_UNICODE( bundle->m_name.AsChar() ), bundle->m_files.Size(), (Double)bundle->CalcDataSize() / OneMB );

			::Sort( bundle->m_splitBundles.Begin(), bundle->m_splitBundles.End(), []( const BundleInfo* a, const BundleInfo* b ) { return a->m_name < b->m_name; } );
			for ( BundleInfo* splitBundle : bundle->m_splitBundles )
			{
				LOG_WCC( TXT("   '%ls' (%d files, %1.2f MB)"), 
					ANSI_TO_UNICODE( splitBundle->m_name.AsChar() ), splitBundle->m_files.Size(), (Double)splitBundle->CalcDataSize() / OneMB );
			}
		}
	}

	// print per content chunk stats
	{
		// collect all split tags used for actual splitting
		TDynArray< CName > splitTags;
		for ( BundleInfo* bundle : m_allBundles )
		{
			if ( bundle->m_splitTag )
			{
				splitTags.PushBackUnique( bundle->m_splitTag );
			}
		}


		LOG_WCC( TXT("-----------------------------------------------------------------") );
		static const Double OneMB = 1024.0*1024.0;
		for ( CName splitTag : splitTags )
		{
			Uint32 numBundles = 0;
			Uint32 numFiles = 0;
			Uint64 fileSize = 0;

			for ( BundleInfo* bundle : m_allBundles )
			{
				if ( bundle->m_splitTag == splitTag )
				{
					numFiles += bundle->m_files.Size();
					fileSize += bundle->CalcDataSize();
					numBundles += 1;
				}
			}

			LOG_WCC( TXT("Content chunk '%ls': %d bundles, %d files, %1.2f MB"), 
					splitTag.AsChar(), numBundles, numFiles, (Double)fileSize / OneMB );
		}
	}

	// done
	return true;
}

Bool CExportBundlesCommandlet::SplitLargeBundle( BundleInfo* sourceBundleInfo, const Uint64 bundleSizeLimit )
{
	// do we even care ?
	const Uint64 initialBundleSize = sourceBundleInfo->CalcDataSize();
	if ( initialBundleSize < bundleSizeLimit )
	{
		// bundle does not have to be split
		m_allBundles.PushBack( sourceBundleInfo );
		m_allBundlesMap.Insert( sourceBundleInfo->m_name, sourceBundleInfo );
		return true;
	}

	WARN_WCC( TXT("Bundle '%ls' will be split because there's risk it will grow over 4GB"), 
		ANSI_TO_UNICODE( sourceBundleInfo->m_name.AsChar() ) );

	// detach file links
	for ( FileInfo* file : sourceBundleInfo->m_files )
	{
		file->m_seedBundles.Clear();
		file->m_bundles.Clear();
	}

	// start splitting
	Uint32 partIndex = 0;
	Uint32 fileIndex = 0;
	while ( fileIndex < sourceBundleInfo->m_files.Size() )
	{
		Uint64 partSize = 0;

		CFilePath bundlePath( ANSI_TO_UNICODE( sourceBundleInfo->m_name.AsChar() ) );

		// assemble new bundle path
		const String originalBundleName = bundlePath.GetFileName();
		const String bundleName = String::Printf( TXT("%s%d"), originalBundleName.AsChar(), partIndex );
		bundlePath.SetFileName( bundleName );

		// create new bundle
		BundleInfo* bundle = GetBundle( UNICODE_TO_ANSI( bundlePath.ToString().AsChar() ) );

		// keep adding the files
		const Uint32 startFile = fileIndex;
		while ( fileIndex < sourceBundleInfo->m_files.Size() )		
		{
			FileInfo* info = sourceBundleInfo->m_files[ fileIndex ];

			// will this file fit into current bundle ?
			if ( partSize + info->m_fileSize > bundleSizeLimit )
				break;

			// add it to current bundle
			bundle->m_files.PushBack( info );
			info->m_bundles.PushBack( bundle );

			// advance
			partSize += info->m_fileSize;
			fileIndex += 1;
		}

		// stats
		LOG_WCC( TXT(" Part %d: %d files, %1.2fMB"), 
			partIndex, fileIndex - startFile, 
			(Double)partSize / (1024.0*1024.0) );

		// we need a new part
		partIndex += 1;		
	}

	return true;
}

Bool CExportBundlesCommandlet::SplitLargeBundles()
{
	static const Uint64 largeBundleLimit = (Uint64)0xFFFFFFFF - (50 << 20);

	// filter out big bundles for splitting
	TDynArray< BundleInfo* > sourceBundles = Move( m_allBundles );
	m_allBundlesMap.Clear();
	for ( BundleInfo* bundle : sourceBundles )
	{
		if ( !SplitLargeBundle( bundle, largeBundleLimit ) )
			return false;
	}

	// done
	return true;
}

CExportBundlesCommandlet::BundleListHelper::BundleListHelper( const TAllBundles& bundles )
{
	// reset
	m_allBundles.Reserve( bundles.Size() );
	m_scenarioBundles.Reserve( bundles.Size() );

	// add normal bundles
	for ( auto* bundle : bundles )
		m_allBundles.PushBack( bundle );
}

void CExportBundlesCommandlet::BundleListHelper::ResetScenario()
{
	m_scenarioBundles.ClearFast();
}

void CExportBundlesCommandlet::BundleListHelper::AddContentDir( const StringAnsi& name )
{
	for ( const BundleInfo* bundle : m_allBundles )
	{
		// bundle not from this content part
		if ( !bundle->m_name.ContainsSubstring( name ) )
			continue;

		// skip world bundles unless explicitly named
		if ( bundle->m_name.ContainsSubstring( "world_") )
			continue;

		m_scenarioBundles.PushBackUnique( bundle );
	}
}

void CExportBundlesCommandlet::BundleListHelper::AddWorld( const StringAnsi& worldPrefixName )
{
	for ( const BundleInfo* bundle : m_allBundles )
	{
		const StringAnsi part = bundle->m_name.StringAfter( worldPrefixName );
		if ( part == "_startup" || part == "_runtime" )
		{
			m_scenarioBundles.PushBackUnique( bundle );
		}
	}
}

Bool CExportBundlesCommandlet::ValidateBundles()
{
	BundleListHelper scenario(m_allBundles);

	// content chunk list (HACKED)
	TDynArray< StringAnsi > contentChunkNames;
	contentChunkNames.PushBack( "content0\\" );
	contentChunkNames.PushBack( "content1\\" );
	contentChunkNames.PushBack( "content2\\" );
	contentChunkNames.PushBack( "content3\\" );
	contentChunkNames.PushBack( "content4\\" );
	contentChunkNames.PushBack( "content5\\" );
	contentChunkNames.PushBack( "content6\\" );
	contentChunkNames.PushBack( "content7\\" );
	contentChunkNames.PushBack( "content8\\" );
	contentChunkNames.PushBack( "content9\\" );
	contentChunkNames.PushBack( "content10\\" );
	contentChunkNames.PushBack( "content11\\" );
	contentChunkNames.PushBack( "content12\\" );
	contentChunkNames.PushBack( "content13\\" );
	contentChunkNames.PushBack( "content14\\" );
	contentChunkNames.PushBack( "content15\\" );

	// world <-> chunk mapping
	TSortedMap< StringAnsi, StringAnsi > worldMapping;
	worldMapping.Insert( "content1\\", "world_kaer_morhen" );
	worldMapping.Insert( "content2\\", "world_prolog_village" );
	worldMapping.Insert( "content3\\", "world_wyzima_castle" );
	worldMapping.Insert( "content4\\", "world_novigrad" );
	worldMapping.Insert( "content5\\", "world_skellige" );
	worldMapping.Insert( "content7\\", "world_island_of_mist" );
	worldMapping.Insert( "content10\\", "world_the_spiral" );
	worldMapping.Insert( "content12\\", "world_prolog_village_winter" );

	// validate general scenario - all bundles with no worlds - this should be loadable
	Uint32 numScenariosFailed = 0;
	{
		scenario.ResetScenario();
		for ( Uint32 i=0; i<contentChunkNames.Size(); ++i )
		{
			const StringAnsi& contentName = contentChunkNames[i];
			scenario.AddContentDir( contentName );

			// validate scenario
			if ( !ValidateBundleScenario( contentName, scenario.GetScenarioBundles() ) )
			{
				numScenariosFailed += 1;
			}
		}
	}

	// test scenarios with world
	{
		for ( Uint32 i=0; i<contentChunkNames.Size(); ++i )
		{
			scenario.ResetScenario();

			// add previously installed chunks + current chunk
			for ( Uint32 j=0; j<=i; ++j )
				scenario.AddContentDir( contentChunkNames[j] );

			// add world that goes with that chunk, we may have no world, then skip
			StringAnsi worldName;
			if ( !worldMapping.Find( contentChunkNames[i], worldName ) )
				continue;

			scenario.AddWorld( worldName );

			// validate scenario
			if ( !ValidateBundleScenario( worldName, scenario.GetScenarioBundles() ) )
			{
				numScenariosFailed += 1;
			}
		}
	}

	// we have failed scenarios
	if ( numScenariosFailed )
	{
		ERR_WCC( TXT("!!! FATAL ERROR !!! There were some installation scenarios that failed checks. Game will not work correctly in PlayGO/StreamInstall. Tell Dex/DavidB ASAP. Stopping cook.") );
		//return false;
	}

	// ok
	return true;
}

Bool CExportBundlesCommandlet::ValidateBundleScenario( const StringAnsi& scenarioName, const TDynArray< const BundleInfo* >& mountedBundles )
{
	// create hash set of accessible files
	THashSet< const FileInfo* > accessibleFiles;
	for ( const BundleInfo* bundle : mountedBundles )
	{
		for ( const FileInfo* file : bundle->m_files )
		{
			accessibleFiles.Insert(file);
		}
	}

	// stats
	LOG_WCC( TXT("Checking scenario %hs, %d bundles, %d accessible files"), scenarioName.AsChar(), mountedBundles.Size(), accessibleFiles.Size() );

	// make sure that all of the file soft, hard and buffer dependencies will be accessible
	Uint32 numMissingHardDeps = 0;
	Uint32 numMissingSoftDeps = 0;
	Uint32 numMissingBufferDeps = 0;
	Uint32 numFilesWithMissingDeps = 0;
	for ( const FileInfo* file : accessibleFiles )
	{
		Bool hasMissingDeps = false;

		// ignore some special cases
		if ( file->m_filePath.EndsWith( "redgame") || file->m_filePath.EndsWith( "w2phase" ) )
			continue;

		// check hard dependencies
		for ( const FileInfo* depFile : file->m_hardDependencies )
		{
			if ( !accessibleFiles.Exist(depFile) )
			{
				ERR_WCC( TXT("File '%hs' hard dependency to '%hs' is not accessible"),
					file->m_filePath.AsChar(), depFile->m_filePath.AsChar() );

				numMissingHardDeps += 1;
				hasMissingDeps = true;
			}
		}

		// check soft dependencies
		for ( const FileInfo* depFile : file->m_softDependencies )
		{
			if ( !accessibleFiles.Exist(depFile) )
			{
				ERR_WCC( TXT("File '%hs' soft dependency to '%hs' is not accessible"),
					file->m_filePath.AsChar(), depFile->m_filePath.AsChar() );

				numMissingSoftDeps += 1;
				hasMissingDeps = true;
			}
		}

		// check buffer dependencies
		for ( const FileInfo* depFile : file->m_bufferDependencies )
		{
			if ( !accessibleFiles.Exist(depFile) )
			{
				ERR_WCC( TXT("File '%hs' buffer dependency to '%hs' is not accessible"),
					file->m_filePath.AsChar(), depFile->m_filePath.AsChar() );

				numMissingBufferDeps += 1;
				hasMissingDeps = true;
			}
		}

		// count files with missing deps
		if ( hasMissingDeps )
			numFilesWithMissingDeps += 1;
	}

	// stats
	if ( numFilesWithMissingDeps )
	{
		ERR_WCC( TXT("Streaming scenario result: FAILED") );
		ERR_WCC( TXT("  Files with missing dependencies: %d"), numFilesWithMissingDeps );
		ERR_WCC( TXT("  Including %d missing hard dependencies"), numMissingHardDeps );
		ERR_WCC( TXT("  Including %d missing soft dependencies"), numMissingSoftDeps );
		ERR_WCC( TXT("  Including %d missing buffers"), numMissingBufferDeps );
		return false;
	}
	else
	{
		ERR_WCC( TXT("Streaming scenario result: SUCCESS") );
	}

	// valid
	return true;
}

Bool CExportBundlesCommandlet::WriteOutputFile( const String& outputFilePath )
{
	Red::Core::BundleDefinition::CBundleDefinitionWriter writer( UNICODE_TO_ANSI(outputFilePath.AsChar()) );

	// make sure target path is valid 
	if ( !GFileManager->CreatePath( outputFilePath.AsChar() ) )
	{
		ERR_WCC( TXT("Failed to create output path for '%ls'"), outputFilePath.AsChar() );
		return false;
	}

	// count input data size (all cooked files)
	Uint64 inputGameDataSize = 0;
	for ( const FileInfo* file : m_allFiles )
	{
		// skip so called "consumed" files (for example dds textures that were added to the texture cache)
		if ( file->m_isConsumed )
			continue;

		// track file size
		Uint32 fileDiskSize = 0;
		if ( m_dataBase.GetFileDiskSize( file->m_dbId, fileDiskSize ) )
		{
			inputGameDataSize += fileDiskSize;
		}
	}

	// write bundle size stats
	LOG_WCC( TXT("Input data size: %1.2f MB"),
		(Double)inputGameDataSize / (1024.0*1024.0) );

	// define the bundles and file lists
	// NOTE: we keep the ordering of files put in the bundles THE SAME as the dependency order (so we can burst read them properly)
	Uint64 totalGameDataSize = 0;
	for ( BundleInfo* bundle : m_allBundles )
	{
		// check if we have any actual files in the bundle
		Bool hasFiles = false;
		for ( const FileInfo* file : bundle->m_files )
		{
			// skip so called "consumed" files (for example dds textures that were added to the texture cache)
			if ( !file->m_isConsumed )
			{
				hasFiles = true;
				break;
			}			
		}

		// no files in
		if( !hasFiles )
		{
			WARN_WCC( TXT( "Bundle: %hs had no files! and won't be created." ), bundle->m_name.AsChar() );
			continue;
		}

		const StringAnsi bundleFileName = bundle->m_name + ".bundle";
		writer.AddBundle( bundleFileName );

		// measure amount of data in a single bundle
		Uint64 bundleDataSize = 0;

		// preserve the order of the files that we got in the cook.db
		Sort( bundle->m_files.Begin(), bundle->m_files.End(), FileInfo::CompareFileIndex );
		for ( const FileInfo* file : bundle->m_files )
		{
			// skip so called "consumed" files (for example dds textures that were added to the texture cache)
			if ( file->m_isConsumed )
				continue;

			// track file size
			Uint32 fileDiskSize = 0;
			if ( m_dataBase.GetFileDiskSize( file->m_dbId, fileDiskSize ) )
			{
				// file may not exist :(
				if ( fileDiskSize == 0 )
				{
					//WARN_WCC( TXT("File '%hs' has zero disk size but is requested to be added to the bundle '%hs'. It may not work."), file->m_filePath.AsChar(), bundle->m_name.AsChar() );
				}

				bundleDataSize += fileDiskSize;
			}

			// setup file description
			Red::Core::BundleDefinition::SBundleFileDesc fileDesc;
			fileDesc.Populate( file->m_filePath );

			// determine compression
			if ( file->m_filePath.EndsWith( "usm" ) )
			{
				// hack for scaleform movies
				fileDesc.m_compressionType = Red::Core::Bundle::CT_Uncompressed;
			}
			else if ( m_fastCompression )
			{
				// use faster compression
				fileDesc.m_compressionType = Red::Core::Bundle::CT_LZ4HC;
			}
			else
			{
				// use auto compression
				fileDesc.m_compressionType = Red::Core::Bundle::CT_Auto;
			}

			// add to the bundle definition
			writer.AddBundleFileDesc( bundleFileName, fileDesc );
		}

		// write bundle size stats
		LOG_WCC( TXT("Bundle '%ls': %1.2f MB"), 
			ANSI_TO_UNICODE( bundle->m_name.AsChar() ),
			(Double)bundleDataSize / (1024.0*1024.0) );
		totalGameDataSize += bundleDataSize;
	}

	// write total size stats
	if ( inputGameDataSize )
	{
		LOG_WCC( TXT("Total data size: %1.2f MB (%1.2f%% of original size)"), 
			(Double)totalGameDataSize / (1024.0*1024.0),
			((Double)totalGameDataSize / (Double)inputGameDataSize) * 100.0 );

		// write DATA DUPLICATION FACTOR regardless of the "silent" setting
		{
			fprintf( stdout, "Total data size (for all bundles): %1.2f MB\n", 
				(Double)totalGameDataSize / (1024.0*1024.0) );

			fprintf( stdout, "Data duplication factor: %1.2f\n", 
				((Double)totalGameDataSize / (Double)inputGameDataSize) );
		}
	}

	// flush output to file
	if ( !writer.Write() )
	{
		ERR_WCC( TXT("Failed to save the generated JSON file to '%ls'"), outputFilePath.AsChar() );
		return false;
	}

	LOG_WCC( TXT("Saved output JSON file to '%ls'"), outputFilePath.AsChar() );
	return true;
}

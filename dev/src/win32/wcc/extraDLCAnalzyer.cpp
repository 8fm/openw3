/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/2darray.h"
#include "cookDataBase.h"
#include "../../common/core/depot.h"

#ifndef NO_EDITOR

/// Analyzer for additional content from base game used by DLCs
class CExtraDLCAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CExtraDLCAnalyzer)

public:
	CExtraDLCAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4extradlc"); }
	virtual const Char* GetDescription() const { return TXT("Analyze DLS for additional base game content"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	static const AnsiChar* BUNDLE_NAME_DLC;

	void ListFiles( CDirectory* dir, TDynArray< CDiskFile* >& outFiles ) const;
};

BEGIN_CLASS_RTTI(CExtraDLCAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CExtraDLCAnalyzer);

const AnsiChar* CExtraDLCAnalyzer::BUNDLE_NAME_DLC = "blob"; // blob vs DLC so we don't create new file handles

CExtraDLCAnalyzer::CExtraDLCAnalyzer()
{
}

bool CExtraDLCAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// we need the cook.db from last patch
	String lastPatchCookDBPath;
	CCookerDataBase db;
	if ( options.GetSingleOptionValue( TXT("db"), lastPatchCookDBPath ) )
	{
		// load cook.cb
		if ( !db.LoadFromFile( lastPatchCookDBPath ) )
		{
			ERR_GAME( TXT("Unable to load cook.db from last patch. Expecting path to be valid.") );
			return false;
		}
	}

	// scan cook DB to get known files
	THashSet< String > mainGameFiles;
	{
		TDynArray< CCookerResourceEntry > entries;
		db.GetFileEntries( entries );

		for ( const auto& entry : entries )
		{
			const String filePath = ANSI_TO_UNICODE( entry.GetFilePath().AsChar() );

			// skip DLC files
			if ( filePath.BeginsWith( TXT("dlc\\") ) )
				continue;

			// add to ref
			mainGameFiles.Insert( filePath );
		}

		RED_LOG( WCC, TXT("Found %d files in existing cook.db"), mainGameFiles.Size() );
	}

	// DLC table file (dir name + content )
	String dlcTableDepotPath;
	if ( !options.GetSingleOptionValue( TXT("dlcs"), dlcTableDepotPath ) )
	{
		ERR_GAME( TXT("Expecting depot path to dlcs.csv table") );
		return false;
	}

	// load game definition file
	THandle< C2dArray > dlcTable = LoadResource< C2dArray >( dlcTableDepotPath );
	if ( !dlcTable )
	{
		ERR_GAME( TXT("Failed to load DLC table form '%ls'"), dlcTableDepotPath.AsChar() );
		return false;
	}

	// add the game definition file to the game bundle, no content chunk ID
	outputList.SetBundleName( BUNDLE_NAME_DLC );
	outputList.SetContentChunk( CName( TXT("content0") ) );
	outputList.AddFile( UNICODE_TO_ANSI( dlcTableDepotPath .AsChar() ) ); // make sure it's included (for reference)

	// crap
	dlcTable->AddToRootSet();

	// scan DLCs
	Uint32 numInterDLCDeps = 0;
	THashSet< String > dlcUsedFiles;
	const Uint32 numDLCs = dlcTable->GetNumberOfRows();
	RED_LOG( WCC, TXT("Found information about %d DLCs"), numDLCs );
	for ( Uint32 i=0; i<numDLCs; ++i )
	{
		// get row content
		const String dlcDirName = dlcTable->GetValue( TXT("Dir"), i );
		const String contentName = dlcTable->GetValue( TXT("Content"), i );

		// blah...
		if ( dlcDirName.Empty() || contentName.Empty() )
		{
			ERR_GAME( TXT("Invalid data in DLC table at row %d"), i );
			return false;
		}

		// validate content name
		if ( !contentName.BeginsWith( TXT("content") ) )
		{
			ERR_GAME( TXT("Content chunk '%ls' is not a valid name"), contentName.AsChar() );
			return false;
		}

		// setup content chunk for all of the files
		outputList.SetContentChunk( CName( contentName.AsChar() ) );

		// get DLC directory
		CDirectory* dlcDir = GDepot->FindPath( dlcDirName.AsChar() );
		if ( !dlcDir )
		{
			ERR_GAME( TXT("Directory '%ls' does not pepresent a valid DLC directory in depot. Skipping."), dlcDirName.AsChar() );
			continue;
		}

		// get files for DLC
		TDynArray< CDiskFile* > dlcFiles;
		ListFiles( dlcDir, dlcFiles );
		if ( dlcFiles.Empty() )
		{
			ERR_GAME( TXT("DLC directory '%ls' does not contain valid files. Skipping."), dlcDirName.AsChar() );
			continue;
		}

		// analyze file dependencies
		Uint32 numNewFiles = 0;
		Uint32 numNewStreamingFiles = 0;
		Uint32 gcCollect = 0;
		for ( auto* ptr : dlcFiles )
		{
			// load file deps
			Red::TScopedPtr< IFile > reader( ptr->CreateReader() );
			if ( !reader )
			{
				ERR_GAME( TXT("Unable to open file '%ls' for reading"), ptr->GetDepotPath().AsChar() );
				continue;
			}

			// load the deps
			CDependencyLoader loader( *reader, nullptr );
			if ( !loader.LoadTables() )
			{
				ERR_GAME( TXT("Unable to load content of file '%ls'"), ptr->GetDepotPath().AsChar() );
				continue;
			}
			else
			{
				reader->Seek(0);

				// get the imported files
				TDynArray< FileDependency > deps;
				loader.LoadDependencies( deps, true );
				
				// add imported files to the dep list
				for ( const auto& dep : deps )
				{
					// Ignore w2ent files since they are flattened
					/*if ( dep.m_depotPath.EndsWith( TXT("w2ent") ) )
						continue;*/

					// DLC file
					if ( dep.m_depotPath.BeginsWith( dlcDirName ) )
						continue;

					// ANOTHER DLC FILE
					if ( dep.m_depotPath.BeginsWith( TXT("dlc\\") ) )
					{
						ERR_GAME( TXT("!!! FATAL ERROR !!!" ) );
						ERR_GAME( TXT("DLC has dependency on another DLC: '%ls' uses '%ls'. Stoppin cook."),
							ptr->GetDepotPath().AsChar(), dep.m_depotPath.AsChar() );
						numInterDLCDeps += 1;
						continue;
					}

					// each file only once...
					if ( dlcUsedFiles.Exist( dep.m_depotPath ) ) 
						continue;
					dlcUsedFiles.Insert( dep.m_depotPath );
					
					// is in the DB ?
					if ( mainGameFiles.Exist( dep.m_depotPath ) )
					{
						//RED_LOG( WCC, TXT("DLC dependency '%ls' ignored because it's already cooked"), dep.m_depotPath.AsChar() );
						continue;
					}

					// a new file
					RED_LOG( WCC, TXT("File '%ls' has depencency on '%ls' from main game"), ptr->GetDepotPath().AsChar(), dep.m_depotPath.AsChar() );
					outputList.AddFile( UNICODE_TO_ANSI( dep.m_depotPath.AsChar() ) );
					numNewFiles += 1;
				}
			}

			// Uber extra shit - streaming dependencies
			{
				TDynArray< FileDependency > deps;

				// special hacks for entity templates
				if ( ptr->GetDepotPath().EndsWith( TXT("w2ent") ) )
				{
					THandle< CEntityTemplate > et = Cast< CEntityTemplate >( ptr->Load() );
					if ( et && et->GetEntityObject() )
					{
						const auto& streamingData = et->GetEntityObject()->GetLocalStreamedComponentDataBuffer();
						if ( streamingData.GetSize() > 0 )
						{
							CMemoryFileReader memReader( (const Uint8*)streamingData.GetData(), streamingData.GetSize(), 0 );
							CDependencyLoader memLoader( memReader, nullptr );
							if ( memLoader.LoadTables() )
							{
								memReader.Seek(0);
								memLoader.LoadDependencies( deps, true );
							}
						}
					}

					// unload
					et->Discard();

					// cleanup - very important
					if ( gcCollect++ > 100 )
					{
						gcCollect = 0;
						GObjectGC->CollectNow();
					}
				}

				// special hacks for entity templates
				else if ( ptr->GetDepotPath().EndsWith( TXT("w2l") ) )
				{
					THandle< CLayer > l = Cast< CLayer >( ptr->Load() );
					for ( auto ent : l->GetEntities() )
					{
						if ( ent )
						{
							const auto& streamingData = ent->GetLocalStreamedComponentDataBuffer();
							if ( streamingData.GetSize() > 0 )
							{
								CMemoryFileReader memReader( (const Uint8*)streamingData.GetData(), streamingData.GetSize(), 0 );
								CDependencyLoader memLoader( memReader, nullptr );
								if ( memLoader.LoadTables() )
								{
									memReader.Seek(0);
									memLoader.LoadDependencies( deps, true );
								}
							}
						}
					}

					// unload
					l->Discard();

					// cleanup - very important
					if ( gcCollect++ > 100 )
					{
						gcCollect = 0;
						GObjectGC->CollectNow();
					}
				}

				// add imported files to the dep list
				for ( const auto& dep : deps )
				{
					// Ignore w2ent files since they are flattened
					/*if ( dep.m_depotPath.EndsWith( TXT("w2ent") ) )
						continue;*/

					// DLC file
					if ( dep.m_depotPath.BeginsWith( dlcDirName ) )
						continue;

					// ANOTHER DLC FILE
					if ( dep.m_depotPath.BeginsWith( TXT("dlc\\") ) )
					{
						ERR_GAME( TXT("!!! FATAL ERROR !!!" ) );
						ERR_GAME( TXT("DLC has streaming dependency on another DLC: '%ls' uses '%ls'. Stoppin cook."),
							ptr->GetDepotPath().AsChar(), dep.m_depotPath.AsChar() );
						numInterDLCDeps += 1;
						continue;
					}

					// each file only once...
					if ( dlcUsedFiles.Exist( dep.m_depotPath ) ) 
						continue;
					dlcUsedFiles.Insert( dep.m_depotPath );
					
					// is in the DB ?
					if ( mainGameFiles.Exist( dep.m_depotPath ) )
					{
						//RED_LOG( WCC, TXT("DLC dependency '%ls' ignored because it's already cooked"), dep.m_depotPath.AsChar() );
						continue;
					}

					// a new file
					RED_LOG( WCC, TXT("File '%ls' has streaming depencency on '%ls' from main game"), ptr->GetDepotPath().AsChar(), dep.m_depotPath.AsChar() );
					outputList.AddFile( UNICODE_TO_ANSI( dep.m_depotPath.AsChar() ) );
					numNewFiles += 1;
					numNewStreamingFiles += 1;
				}
			}
		}

		// final log
		if ( numNewFiles > 0 )
		{
			RED_LOG( WCC, TXT("DLC '%ls' has %d main game dependencies (%d streaming)"), dlcDirName.AsChar(), numNewFiles, numNewStreamingFiles );
		}

		// Full cleanup
		GObjectGC->CollectNow();
	}

	// num inter-DLC deps
	if ( numInterDLCDeps > 0 )
	{
		ERR_GAME( TXT("Found %d FATAL DLC-DLC cross dependencies. Stopping cook."), numInterDLCDeps );
		return false;
	}

	// unload
	dlcTable->RemoveFromRootSet();

	// done
	return true;
}

void CExtraDLCAnalyzer::ListFiles( CDirectory* dir, TDynArray< CDiskFile* >& outFiles ) const
{
	// collect files from directory
	for ( auto* ptr : dir->GetFiles() )
	{
		// skip XML and CSV files here 
		if ( ptr->GetFileName().EndsWith( TXT("csv") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("xml") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("link") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("usm") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("srt") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("dds") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("png") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("resw") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("txt") ) )
			continue;		
		else if ( ptr->GetFileName().EndsWith( TXT("resfiles") ) )
			continue;
		else if ( ptr->GetFileName().EndsWith( TXT("sav") ) )
			continue;

		outFiles.PushBack( ptr );
	}

	// recurse
	for ( auto* childDir : dir->GetDirectories() )
	{
		ListFiles( childDir, outFiles );
	}
}

#endif

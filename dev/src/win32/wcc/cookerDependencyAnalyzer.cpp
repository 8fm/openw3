/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "../../common/core/depot.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/game/attitude.h"
#include "../../common/core/feedback.h"

#include "cookerDependencyAnalyzer.h"
#include "../../common/core/gatheredResource.h"

CCookerDependencyAnalyzer::CCookerDependencyAnalyzer( size_t allocBudget /*=0*/, Bool useSoftHandles /*=false*/ )
	: m_allocBudget( allocBudget )
	, m_useSoftHandles( useSoftHandles )
{
}

CCookerDependencyAnalyzer::~CCookerDependencyAnalyzer()
{
	ClearCache();
}

Bool CCookerDependencyAnalyzer::GetSingleFileDependencies( const String& depotPath, TDynArray< String >& referencedFiles, const THashSet< String >& excludedExtensions /*= THashSet<String>()*/ )
{
	// Create a list with one file
	TDynArray< String > fileList;
	fileList.PushBack( depotPath );

	// Use the file group dependency grabber
	return GetFilesDependencies( fileList, referencedFiles, excludedExtensions );
}

Bool CCookerDependencyAnalyzer::GetFilesDependencies( const TDynArray< String > depotPaths, TDynArray< String >& referencedFiles, const THashSet< String >& excludedExtensions /*= THashSet<String>()*/)
{
	// Prepare, clear "visit" flag for all files
	for ( Uint32 i=0; i<m_dependencies.Size(); ++i )
	{
		File* file = m_dependencies[i];
		file->m_visited = false;
		file->m_recursed = false;
	}

	// Find the depot file for the starting file
	for ( Uint32 i=0; i<depotPaths.Size(); i++ )
	{
		// Update progress
		GFeedback->UpdateTaskProgress( i, depotPaths.Size() );

		// Canceled
		if ( GFeedback->IsTaskCanceled() )
		{
			return false;
		}

		// Get the dependency file object
		File* file = GetDependencyFile( depotPaths[i] );

		Bool isFileExcluded = false;
		if ( file )
		{
			CFilePath filePath( file->m_depotPath );
			isFileExcluded = excludedExtensions.Exist( filePath.GetExtension() );
		}

		if ( file && ! isFileExcluded )
		{
			// Start the reference grabbing
			TDynArray< String > tempDependencies;
			GetRecursiveDependencies( file, 0, tempDependencies, excludedExtensions );

			// Add to final list in reversed order
			for ( Uint32 i=0; i<tempDependencies.Size(); ++i )
			{
				// Add to final list
				const String& depotPath = tempDependencies[i];
				referencedFiles.PushBackUnique( depotPath.ToLower() );
			}
		}
	}

	// Done
	return true; 
}

void CCookerDependencyAnalyzer::GetRecursiveDependencies( File* file, Int32 level, TDynArray< String >& referencedFiles, const THashSet< String >& excludedExtensions )
{
	// Recurse
	if ( !file->m_recursed )
	{
		// We have checked the dependencies
		file->m_recursed = true;

		// Get file dependencies
		TDynArray< File* > fileDependencies;
		GetFileDependencies( file, fileDependencies, excludedExtensions );

		// Recurse to references files
		for ( Uint32 i=0; i<fileDependencies.Size(); i++ )
		{
			// Recurse only to not visited files
			File* referencedFile = fileDependencies[i];
			if ( !referencedFile->m_visited )
			{
				GetRecursiveDependencies( referencedFile, level+1, referencedFiles, excludedExtensions );
			}
		}
	}

	// Add file to the list after local dependencies
	if ( !file->m_visited )
	{
		file->m_visited = true;
		referencedFiles.PushBack( file->m_depotPath );
		//LOG_CORE( TXT("Res[%i], level %i: '%s'"), referencedFiles.Size()-1, level, file->m_depotPath.AsChar() );
	}

	// Collect garbage if too much is allocated
	if( Memory::GetTotalBytesAllocated() > m_allocBudget )
	{
		SGarbageCollector::GetInstance().CollectNow();

		// Release empty memory pools to recover some memory
		Memory::ReleaseFreeMemoryToSystem();
	}
}

void CCookerDependencyAnalyzer::ClearCache()
{
	// Delete dependencies
	m_dependencies.ClearPtr();

	// Clear the file map
	m_fileMap.Clear();
}

Bool CCookerDependencyAnalyzer::GetFileDependencies( File* file, TDynArray< File* >& referencedFiles, const THashSet< String >& excludedExtensions )
{
	RED_ASSERT( file );

	// Generate file absolute path
	CDiskFile* diskFile = GDepot->FindFileUseLinks( file->m_depotPath, 0 );
	if ( diskFile )
	{
		// If file did not changed since last dependencies were generated the use the ones found in cache
		CDateTime time( diskFile->GetFileTime() );
		if ( time.IsValid() && file->m_fileTime == time )
		{
			// Use cached dependencies
			for ( Uint32 i=0; i<file->m_dependencies.Size(); i++ )
			{
				File* targetFile = file->m_dependencies[i];
				referencedFiles.PushBack( targetFile );
			}

			// Dependencies generated
			return true;
		}

		// Clear file dependency list because it will be regenerated
		file->m_dependencies.Clear();

		// Load the file
		IFile* targetFile = diskFile->CreateReader();
		if ( targetFile )
		{
			// Create dependency loader
			IDependencyLoader* linker = SDependencyLinkerFactory::GetInstance().GetLoader( diskFile, *targetFile );
			if ( linker )
			{
				// Load dependencies
				TDynArray< FileDependency > loadedDependencies;
				if ( linker->LoadDependencies( loadedDependencies, m_useSoftHandles ) )
				{
					// Generate dependencies :)
					for ( Uint32 i=0; i<loadedDependencies.Size(); i++ )
					{
						// Get the target file
						const FileDependency& dependecy = loadedDependencies[i];

						// Create the file token for dependency file
						File* targetDependency = GetDependencyFile( dependecy.m_depotPath );
						if ( targetDependency  )
						{
							CFilePath filePath( targetDependency->m_depotPath );
							if ( ! excludedExtensions.Exist( filePath.GetExtension() ) )
							{
								file->m_dependencies.PushBack( targetDependency );
							}
						}
					}
				}

				// Delete linker file
				delete linker;
			}
 
			// Close the file
			delete targetFile;
		}

		// Dependencies were updated
		file->m_fileTime = time;

		// Extract final dependencies
		for ( Uint32 i=0; i<file->m_dependencies.Size(); i++ )
		{
			File* targetFile = file->m_dependencies[i];
			referencedFiles.PushBack( targetFile );
		}

		// Valid
		return true;
	}

	// Not valid
	return false;
}

CCookerDependencyAnalyzer::File* CCookerDependencyAnalyzer::GetDependencyFile( const String& depotPath )
{
	// Find in the map
	CCookerDependencyAnalyzer::File* found = NULL;
	if ( !m_fileMap.Find( depotPath, found ) )
	{
		// Get the real file
		CDiskFile* file = GDepot->FindFileUseLinks( depotPath, 0 );
		if ( !file )
		{
			// No file exists in the depot under that name
			return nullptr;
		}

		// Find under the internal path ( may be different )
		String realDepotPath = file->GetDepotPath();
		if ( !m_fileMap.Find( realDepotPath, found ) )
		{
			// Allocate new entry
			found = new CCookerDependencyAnalyzer::File;
			found->m_depotPath = realDepotPath;	// Use valid file name !
			found->m_fileTime = CDateTime::INVALID; // This will force dependencies to be read from the file
			found->m_visited = false;
			found->m_recursed = false;

			// Register
			m_dependencies.PushBack( found );
			m_fileMap.Insert( realDepotPath, found );
		}

		// Register with different name
		if ( realDepotPath != depotPath )
		{
			//LOG_CORE( TXT("Aliased dependency: '%s'-> '%s'"), depotPath.AsChar(), realDepotPath.AsChar() );
			m_fileMap.Insert( depotPath, found );
		}
	}

	// Return token
	RED_ASSERT( found );
	return found;
}

//////////////////////////////////////////////////////////////////////////

void CCookerDependencyAnalyzer::ScanForBaseFiles( ECookingPlatform cookingPlatform, TDynArray< String >& filesWithDependencies, TDynArray< String >& rawFiles )
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////
	///// HACK : Hard-coded list of files to collect kinda sucks! Some of these are okay, with the definitions manager and
	///// stuff. The others that just look for all files of some type (or even worse ALL files in a directory) are terrible!
	/////
	///// TODO : This is probably picking up a lot more files than are really needed.
	/////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LOG_WCC( TXT("Finding base files:") );

	// Add as raw resources to avoid dependency analysis potentially blowing up, and should be self contained
	LOG_WCC( TXT("  Engine resources") );
	{
		TDynArray < String > paths;
		GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT("engine\\"), TXT( "*.*" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

	// Add all behavior trees because their resource paths can be set from scripts
	// Adding to "raw" files to avoid dependency analysis which could pull in more than wanted just for base dependencies
	// gathered in such a hackish way
	LOG_WCC( TXT("  Behavior") );
	{
		const String behTreeDirectory = GFileManager->GetDataDirectory() + TXT("gameplay\\trees\\");
		const String behDirectory = GFileManager->GetDataDirectory() + TXT("gameplay\\behaviors\\");

		TDynArray < String > paths;
		GFileManager->FindFiles( behTreeDirectory, TXT( "*.w2behtree" ), paths, true );
		GFileManager->FindFiles( behDirectory, TXT( "*.*" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

	// Add attitudes XML file
	rawFiles.PushBack( ATTITUDES_XML );

	// Always add all item xml files
	LOG_WCC( TXT("  Item definitions") );
	{
		String itemDirectory = GFileManager->GetDataDirectory() + CDefinitionsManager::GetItemsDefinitionsDir();
		TDynArray < String > paths;
		GFileManager->FindFiles( itemDirectory, TXT( "*.xml" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

	// Add all ability xml files
	LOG_WCC( TXT("  Ability definitions") );
	{
		String itemDirectory = GFileManager->GetDataDirectory() + CDefinitionsManager::GetAbilitiesDir();
		TDynArray < String > paths;
		GFileManager->FindFiles( itemDirectory, TXT( "*.xml" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

#if 0
	// Add all global cutscenes
	LOG_WCC( TXT("  Cutscenes") );
	{
		String cutscenesCSV = GFileManager->GetDataDirectory() + TXT("gameplay\\globals\\cutscene.csv");
		C2dArray *arr = C2dArray::CreateFromString( cutscenesCSV );

		Uint32 colSize, rowSize;
		arr->GetSize( colSize, rowSize );
		for ( Uint32 rowNum = 0; rowNum < rowSize; ++rowNum )
		{
			filesWithDependencies.PushBack( arr->GetValue( 1, rowNum ).ToLower() );
		}
	}
#endif

	// Add all resource definitions xml files
	LOG_WCC( TXT("  Resource definitions") );
	{
		String resdefDirectory = GFileManager->GetDataDirectory() + CResourceDefManager::DIRECTORY;
		TDynArray < String > paths;
		GFileManager->FindFiles( resdefDirectory, TXT( "*.xml" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

	// Add all resources from definitions
	LOG_WCC( TXT("  Resource map") );
	{
		SResourceDefManager::GetInstance().LoadAllDefinitions();
		const CResourceDefManager::TResourceMap& resourceMap = SResourceDefManager::GetInstance().GetResourceMap();

		CResourceDefManager::TResourceMap::const_iterator iter;

		for( iter = resourceMap.Begin(); iter != resourceMap.End(); ++iter )
		{
			filesWithDependencies.PushBack( iter->m_second.GetPath() );
		}
	}

	// Add all item definitions
	LOG_WCC( TXT("  Item templates") );
	{
		CDefinitionsManager definitionManager;
		TDynArray< String > itemTemplates;

		definitionManager.ReloadAll();
		definitionManager.GetTemplateFilesList( itemTemplates );

		for( Uint32 j = 0; j < itemTemplates.Size(); j++ )
		{
			filesWithDependencies.PushBack( itemTemplates[j] );
		}
	}

	// To pick up any runtime dependencies loaded directly from a SWF itself, and so undiscoverable by normal dependency analysis
	LOG_WCC( TXT("  SWF files") );
	{
		TDynArray < String > paths;
		GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT("gameplay\\gui_new\\"), TXT("*.redswf"), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

	// TODO : This is annoyingly slow! Maybe instead of scanning entire directory tree, the file types below
	// can be limited to specific folders?

	THashMap< String, TDynArray< String > > fileMap;
	LOG_WCC( TXT("  Scanning for all files") );
	{
		TDynArray < String > paths;
		GFileManager->FindFiles( GFileManager->GetDataDirectory(), TXT( "*.*" ), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String extension = CFilePath( paths[j] ).GetExtension();
			TDynArray< String >* files = fileMap.FindPtr( extension );
			if ( files == nullptr )
			{
				fileMap.Insert( extension, TDynArray< String >() );
				files = fileMap.FindPtr( extension );
			}

			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			files->PushBack( localPath );
		}
	}

	// Add all csv files
	LOG_WCC( TXT("  CSV files") );
	{
		TDynArray < String >* paths = fileMap.FindPtr( TXT("csv") );
		if ( paths != nullptr )
		{
			filesWithDependencies.PushBack( *paths );
		}
	}

#ifndef DO_RESAVE
	// Add all link files
	// Resave (and proper cooking) resolves these links, so we only need them when doing a straight copy.
	LOG_WCC( TXT("  Link files") );
	{
		TDynArray < String >* paths = fileMap.FindPtr( TXT("link") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}
	}
#endif

#if 0
	// TW> Don't add w2strings files!!!
	{
		TDynArray < String > paths;
		GFileManager->FindFiles( GFileManager->GetDataDirectory(), CLocalizationManager::STRINGS_FILE_PATH_POSTFIX.AsChar(), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}
#endif

	// Add all journal files, since a lot aren't picked up as dependencies
	LOG_WCC( TXT("   Journal files") );
	{
		TDynArray < String >* paths = fileMap.FindPtr( TXT("journal") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}
	}

	LOG_WCC( TXT("   Journal files") );
	{
		TDynArray < String >* paths = fileMap.FindPtr( TXT("w2je") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}
	}

	// Add all loose image files (*.dds, *.png -- can add others as needed, but looks like that's all we're using).
	// TODO : Only add DDS textures used by the SRTs/SWFs we gather? UI images loaded by strings in scripts, so probably can't do that...
	LOG_WCC( TXT("  Loose image files") );
	{
		TDynArray < String >* paths;

		paths = fileMap.FindPtr( TXT("dds") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}

		paths = fileMap.FindPtr( TXT("png") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}
	}

	// Add all usm movie files
	LOG_WCC( TXT("  USM files") );
	{
		TDynArray < String >* paths = fileMap.FindPtr( TXT("usm") );
		if ( paths != nullptr )
		{
			rawFiles.PushBack( *paths );
		}
	}

	LOG_WCC( TXT("  Soundbanks") );
	{
		String platformDir;
		switch( cookingPlatform )
		{
		case PLATFORM_PC:
			platformDir = TXT( "Pc\\" );
			break;
#ifndef WCC_LITE
		case PLATFORM_XboxOne:
			platformDir = TXT( "XboxOne\\" );
			break;
		case PLATFORM_PS4:
			platformDir = TXT( "PS4\\" );
			break;
#endif
		case PLATFORM_None:
			/* leave empty */
			break;
		default:
			break;
		}

		TDynArray < String > paths;
		if ( !platformDir.Empty() )
		{
			// Don't need the *.txt files except for editing
			GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT( "soundbanks\\" ) + platformDir, TXT("*.wem"), paths, true );
			GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT( "soundbanks\\" ) + platformDir, TXT("*.bnk"), paths, true );
		}
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}

#if 0
	// mcinek: We don't want to put streaming data into dzips
	// Add streaming sound banks
	{
		String streamingBanks = GFileManager->GetDataDirectory() + TXT("gameplay\\globals\\sounds_streaming.csv");
		C2dArray *arr = C2dArray::CreateFromString( streamingBanks );

		Uint32 colSize, rowSize;
		arr->GetSize( colSize, rowSize );
		for ( Uint32 rowNum = 0; rowNum < rowSize; ++rowNum )
		{
			rawFiles.PushBack( arr->GetValue( 0, rowNum ).ToLower() );
		}
	}
#endif

	// Collect gathered resources
	LOG_WCC( TXT("  Gathered resources") );
	{
		const TDynArray< CGatheredResource *>& gatheredResources = CGatheredResource::GetGatheredResources();	
		for( Uint32 i = 0; i < gatheredResources.Size(); i++ )
		{
			filesWithDependencies.PushBack( gatheredResources[i]->GetPath().ToString().ToLower() );
		}
	}
	
	if( cookingPlatform == PLATFORM_XboxOne )
	{
		LOG_WCC( TXT("  Kinect grammar") );
		TDynArray < String > paths;
		GFileManager->FindFiles( GFileManager->GetDataDirectory() + TXT( "kinect\\" ), TXT("*.cfg"), paths, true );
		for( Uint32 j = 0; j < paths.Size(); j++ )
		{
			String localPath;
			GDepot->ConvertToLocalPath( paths[j], localPath );
			rawFiles.PushBack( localPath.ToLower() );
		}
	}
}

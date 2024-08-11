/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/game/quest.h"
#include "../../common/game/questGraph.h"
#include "../../common/engine/graphBlock.h"
#include "../../common/game/questPhaseBlock.h"
#include "../../common/game/questSceneBlock.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "r4GameResource.h"
#include "questGraphWalker.h"
#include "r4BoidSpecies.h"

CGatheredResource resCustomQuestList( TXT("game\\custom_quests.csv"), RGF_NotCooked ); // it's added manually to the seed files

#ifndef NO_EDITOR

/// Analyzer for top level game structure
class CR4GameAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4GameAnalyzer)

public:
	CR4GameAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4game"); }
	virtual const Char* GetDescription() const { return TXT("Analyze game resource from the R4 game definition file (but not worlds)"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	static const AnsiChar* BUNDLE_NAME_GAME;
	static const AnsiChar* BUNDLE_NAME_MOVIES;

	void ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList );
};

BEGIN_CLASS_RTTI(CR4GameAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4GameAnalyzer);

const AnsiChar* CR4GameAnalyzer::BUNDLE_NAME_GAME = "blob";
const AnsiChar* CR4GameAnalyzer::BUNDLE_NAME_MOVIES = "movies";

CR4GameAnalyzer::CR4GameAnalyzer()
{
}

bool CR4GameAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// we need the game top level game definition file
	String gameDefinitionFile;
	if ( !options.GetSingleOptionValue( TXT("def"), gameDefinitionFile ) )
	{
		ERR_GAME( TXT("Expecting depoth path to game definition file") );
		return false;
	}

	// load game definition file
	THandle< CWitcherGameResource > gameRes = LoadResource<CWitcherGameResource>( gameDefinitionFile );
	if ( !gameRes )
	{
		ERR_GAME( TXT("Failed to load game resource '%ls'"), gameDefinitionFile.AsChar() );
		return false;
	}

	// add the game definition file to the game bundle, no content chunk ID
	outputList.SetBundleName( BUNDLE_NAME_GAME );
	outputList.AddFile( UNICODE_TO_ANSI( gameDefinitionFile.AsChar() ) );

	// get the root quest and analyze it
	{
		// load
		CTimeCounter timer;
		LOG_GAME( TXT("Loading main quest...") );
		CQuest* rootQuest = gameRes->GetMainQuest().Get();
		LOG_GAME( TXT("Quest loaded in %1.2fs"), timer.GetTimePeriod() );

		// analyze
		if ( rootQuest )
		{
			CQuestGraphWalker walker;
			CTimeCounter timer;

			LOG_GAME( TXT("Analyzing content distribution in quest '%ls'..."), rootQuest->GetDepotPath().AsChar() );
			walker.WalkQuest( rootQuest );

			// add additional files to the bundles
			walker.EmitToSeedFile( outputList );
			LOG_GAME( TXT("Quest content analyzed in %1.2fs"), timer.GetTimePeriod() );
		}
	}

	// extract chunk0 movies
	{
		outputList.SetBundleName( BUNDLE_NAME_MOVIES );
		outputList.SetContentChunk( CName( TXT("content0") ) );

		// intro movie + after nightmare
		outputList.AddFile( "movies\\cutscenes\\pre_rendered_cutscenes\\intro.usm" );
		outputList.AddFile( "movies\\cutscenes\\pre_rendered_cutscenes\\cs001_nightmare.usm" );
		
		outputList.AddFile( "movies\\cutscenes\\gamestart\\recap_wip.usm" );
		outputList.AddFile( "movies\\cutscenes\\gamestart\\credits_6000bitrate.usm" );
		outputList.AddFile( "movies\\cutscenes\\gamestart\\splash.usm" );
		//outputList.AddFile( "movies\\cutscenes\\gamestart\\loading.usm" );

		// storybook movies (loading screens)
		CDirectory* flashbackMovies = GDepot->FindPath( TXT("movies\\cutscenes\\storybook\\") );
		ExtractFiles( flashbackMovies, TXT("usm"), nullptr, true, outputList );

		// UI background movies
		outputList.AddFile( "movies\\gui\\menus\\endmovie.usm" );
		outputList.AddFile( "movies\\gui\\menus\\gui_background.usm" );
		outputList.AddFile( "movies\\gui\\menus\\mainmenu.usm" );

		CDirectory* videoSubtitles = GDepot->FindPath( TXT("movies\\") );
		ExtractFiles( videoSubtitles, TXT("subs"), nullptr, true, outputList );
	}

	// extract content2 movies
	{
		outputList.SetBundleName( BUNDLE_NAME_MOVIES );
		outputList.SetContentChunk( CName( TXT("content0") ) );

		outputList.AddFile( "movies\\cutscenes\\pre_rendered_cutscenes\\cs002_wild_hunt_chase_p2.usm" );
	}

	// extract all of the remaining USM movies
	{
		outputList.SetContentChunk( CName( TXT("content4") ) ); // HACK!
		outputList.SetBundleName( BUNDLE_NAME_MOVIES );

		ExtractFiles( GDepot->FindLocalDirectory( TXT("movies") ), TXT("usm"), nullptr, true, outputList );
	}

	// extract custom quests
	THandle< C2dArray > customQuests = resCustomQuestList.LoadAndGet< C2dArray >();
	if ( customQuests )
	{
		// add the quest definition list to the cook
		outputList.SetContentChunk( CName( TXT("content12") ) ); // HACK!
		outputList.SetBundleName( BUNDLE_NAME_GAME );
		outputList.AddFile( resCustomQuestList.GetPath().ToAnsiString() );

		// add custom quest definitions to the cook
		const Uint32 count = customQuests->GetNumberOfRows();
		for ( Uint32 i=0; i<count; ++i )
		{
			const String depotPath = customQuests->GetValue(0,i);
			const String name = customQuests->GetValue(1,i);

			// only show existing options
			if ( GDepot->FindFileUseLinks( depotPath, 0 ) != nullptr )
			{
				outputList.AddFile( UNICODE_TO_ANSI( depotPath.AsChar() ) );
			}
		}
	}

	// extract swarm boid templates
	{
		outputList.SetContentChunk( CName( TXT("content0") ) );
		CDefinitionsManager definitionsManager;
		CR4BoidSpecies boidSpecies;
		boidSpecies.InitParams(&definitionsManager);

		TDynArray<String> boidTemplatePaths;
		boidSpecies.GetBoidTemplatePaths( boidTemplatePaths );

		for( String& path : boidTemplatePaths )
		{
			outputList.AddFile( UNICODE_TO_ANSI( path.AsChar() ) );
		}
	}

	// done
	return true;
}

void CR4GameAnalyzer::ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
{
	if ( !mainDir )
		return;

	// filter files
	const TFiles& files = mainDir->GetFiles();
	for ( CDiskFile* file : files )
	{
		String temp;
		const String conformedPath = CFilePath::ConformPath( file->GetDepotPath(), temp );
		const String ext = StringHelpers::GetFileExtension( conformedPath );

		// filter out files with invalid extension
		if (fileExtensionFilter)
		{
			if ( ext != fileExtensionFilter )
				continue;
		}

		// excluded
		if (excludedFileExtensions)
		{
			if ( excludedFileExtensions->Exist(ext) )
				continue;
		}

		// add to list
		outputList.AddFile( UNICODE_TO_ANSI( conformedPath.AsChar() ) );
	}

	// recurse to sub directories
	if ( recursive )
	{		
		for ( CDirectory* subDir : mainDir->GetDirectories() )
		{
			ExtractFiles( subDir, fileExtensionFilter, excludedFileExtensions, recursive, outputList );
		}
	}
}


#endif

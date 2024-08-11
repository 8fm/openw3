/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/depot.h"

#ifndef NO_EDITOR

/// Analyzer for GUI related resources 
class CR4GuiAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4GuiAnalyzer)

public:
	CR4GuiAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4gui"); }
	virtual const Char* GetDescription() const { return TXT("Analyze related to R4 gui"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	static const AnsiChar* BUNLDE_NAME;

	// file extraction helper
	void ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList );
};

BEGIN_CLASS_RTTI(CR4GuiAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4GuiAnalyzer);

const AnsiChar* CR4GuiAnalyzer::BUNLDE_NAME = "r4gui";

CR4GuiAnalyzer::CR4GuiAnalyzer()
{
}

bool CR4GuiAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// All items are cooked into the "gui" bundle right now
	outputList.SetBundleName( BUNLDE_NAME );

	// locate the GUI directory
	CDirectory* guiDirectory = GDepot->FindPath( TXT("gameplay\\gui_new\\") );
	if ( !guiDirectory )
	{
		ERR_GAME( TXT("GUI directory not found") );
		return false;
	}

	// Extracted list includes: all SWFs, all minimap resources (regardless of format), all gui related dds textures
	ExtractFiles( guiDirectory, TXT("redswf"), NULL, true, outputList );

	// Extract all loose textures
	ExtractFiles( guiDirectory, TXT("dds"), NULL, true, outputList );
	ExtractFiles( guiDirectory, TXT("png"), NULL, true, outputList );

	// Extract all minimap resources
	ExtractFiles( guiDirectory->FindLocalDirectory( TXT("minimaps") ), NULL, NULL, true, outputList );
	ExtractFiles( guiDirectory->FindLocalDirectory( TXT("maps") ), NULL, NULL, true, outputList );

	// EP1 hack: we don't want to patch a whole movies bundle just because of this file 
	// (it will make the patch too big for the customers), so we're adding it here.
	outputList.AddFile( "movies\\gui\\menus\\main_menu_ep1.usm" );

	return true;
}

void CR4GuiAnalyzer::ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
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
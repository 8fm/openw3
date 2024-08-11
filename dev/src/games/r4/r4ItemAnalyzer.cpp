/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/depot.h"

#ifndef NO_EDITOR

/// Analyzer for item definitions
class CR4ItemAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4ItemAnalyzer)

public:
	CR4ItemAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4items"); }
	virtual const Char* GetDescription() const { return TXT("Analyze items from the R4 game"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	static const AnsiChar* BUNLDE_NAME;
};

BEGIN_CLASS_RTTI(CR4ItemAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4ItemAnalyzer);

const AnsiChar* CR4ItemAnalyzer::BUNLDE_NAME = "r4items";

CR4ItemAnalyzer::CR4ItemAnalyzer()
{
}

bool CR4ItemAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// All items are cooked into the "items" bundle right now
	outputList.SetBundleName( BUNLDE_NAME );

	// All items go to content0
	outputList.SetContentChunk( CName( TXT("content0") ) );

	// Reload items
	CDefinitionsManager definitionManager;
	definitionManager.ReloadAll();	

	// Get template files
	TDynArray< String > itemTemplates;
	definitionManager.GetTemplateFilesList( itemTemplates );

	// Add templates to the bundle list
	Uint32 numFiles = 0;
	for( Uint32 j = 0; j < itemTemplates.Size(); j++ )
	{
		const String& depotPath = itemTemplates[j];

		// find actual depot file
		CDiskFile* file = GDepot->FindFileUseLinks( depotPath, 0 );
		if ( file )
		{
			const StringAnsi actualDepotPath( UNICODE_TO_ANSI( file->GetDepotPath().AsChar() ) );
			if ( outputList.AddFile( actualDepotPath ) )
				++numFiles;
		}
	}

	// Done, display stats
	LOG_GAME( TXT("Found %i item templates in r4 game resources"), numFiles );
	return true;
}

#endif
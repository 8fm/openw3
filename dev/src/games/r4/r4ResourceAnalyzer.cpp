/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/resourceDefManager.h"

#ifndef NO_EDITOR

/// Analyzer for resource definitions
class CR4ResourceAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4ResourceAnalyzer)

public:
	CR4ResourceAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4res"); }
	virtual const Char* GetDescription() const { return TXT("Analyze resources from the R4 game"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	static const AnsiChar* BUNLDE_NAME;
};

BEGIN_CLASS_RTTI(CR4ResourceAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4ResourceAnalyzer);

const AnsiChar* CR4ResourceAnalyzer::BUNLDE_NAME = "blob";

CR4ResourceAnalyzer::CR4ResourceAnalyzer()
{
}

bool CR4ResourceAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// All items are cooked into the "items" bundle right now
	outputList.SetBundleName( BUNLDE_NAME );

	// Load all definitions
	SResourceDefManager::GetInstance().LoadAllDefinitions();

	// Iterate
	Bool status = true;
	Uint32 numFiles = 0;
	const CResourceDefManager::TResourceMap& resourceMap = SResourceDefManager::GetInstance().GetResourceMap();
	for( CResourceDefManager::TResourceMap::const_iterator iter = resourceMap.Begin(); iter != resourceMap.End(); ++iter )
	{
		const String& depotPath = iter->m_second.GetPath();

		// find actual depot file
		CDiskFile* file = GDepot->FindFileUseLinks( depotPath, 0 );
		if ( file )
		{
			const String actualDepotPath( file->GetDepotPath().AsChar() );
			if ( actualDepotPath != depotPath )
			{
				ERR_GAME( TXT("!!! LINK FILE IN XML !!! - XML references file '%ls' that was moved to '%ls'. Please update the XML (we don't have links on cook and this will not work)."),
					depotPath.AsChar(), actualDepotPath.AsChar() );
					status = false;

				continue;
			}

			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( actualDepotPath.AsChar() ) );
			if ( outputList.AddFile( actualDepotPathAnsi ) )
				++numFiles;
		}
	}

	// We failed - some link problems
	if ( !status )
	{
		ERR_GAME( TXT("Analyzing the game XML definitions reported errors. Cooking is not safe.") );
		return false;
	}

	// Done, display stats
	LOG_GAME( TXT("Found %d files in r4 game resources"), numFiles );
	return true;
}

#endif
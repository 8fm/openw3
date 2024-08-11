/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/xmlreader.h"

#ifndef NO_EDITOR

/// Analyzer for startup game resources
class CR4StartupAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4StartupAnalyzer)

public:
	CR4StartupAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4startup"); }
	virtual const Char* GetDescription() const { return TXT("Analyze game & engine startup resources"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	Bool ProcessFile( const String& filePath, CAnalyzerOutputList& outputList ) const;
	Bool LoadStartupExclusiveFilesList( const String& filePath, CAnalyzerOutputList& outputList ) const;

	static const AnsiChar* BUNDLE_NAME;
};

BEGIN_CLASS_RTTI(CR4StartupAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4StartupAnalyzer);

const AnsiChar* CR4StartupAnalyzer::BUNDLE_NAME = "startup";

CR4StartupAnalyzer::CR4StartupAnalyzer()
{
}

Bool CR4StartupAnalyzer::ProcessFile( const String& filePath, CAnalyzerOutputList& outputList ) const
{
	// find existing file in the depot
	CDiskFile* file = GDepot->FindFileUseLinks( filePath, 0 );
	if ( !file )
	{
		WARN_CORE( TXT("Ignoring file '%ls' from analyze step because it does not exist"), filePath.AsChar() );
		return false;
	}

	// add the file
	const String realFilePath = file->GetDepotPath();
	outputList.AddFile( UNICODE_TO_ANSI( realFilePath.AsChar() ) );
	return true;
}

bool CR4StartupAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	outputList.SetBundleName( BUNDLE_NAME );
	outputList.SetContentChunk( CName( TXT("content0") ) );

	LoadStartupExclusiveFilesList( GFileManager->GetDataDirectory() + TXT("startupexclusives.xml"), outputList );

	const TDynArray< CGatheredResource* > allGatheredResources = CGatheredResource::GetGatheredResources();
	for ( Uint32 i = 0; i < allGatheredResources.Size(); ++i )
	{
		const CGatheredResource* source = allGatheredResources[ i ];

		// gathered resources with the "no cook" flag are ignored
		if ( !source->IsCooked() )
			continue;

		// common files are processed in the common bundle
		if ( !source->IsStartup() )
			continue;;

		// get the gathered resource path
		const String resourcePath = source->GetPath().ToString();
		ProcessFile( resourcePath, outputList );
	}

	return true;
}

Bool CR4StartupAnalyzer::LoadStartupExclusiveFilesList( const String& filePath, CAnalyzerOutputList& outputList ) const
{
	// process the files, connect them in the hash set first to remove duplicates
	String xmlContent = TXT("");
	if ( !GFileManager->LoadFileToString( filePath, xmlContent, true ) )
	{
		ERR_GAME( TXT("The package descriptor in '%ls' is missing"), filePath.AsChar() );
		return false;
	}

	// get all depot files
	TDynArray< CDiskFile* > allFiles;
	{
		CTimeCounter timer;

		LOG_CORE( TXT("Enumerating depot...") );
		GDepot->CollectFiles( allFiles, String::EMPTY, true, false ); // get everything

		LOG_CORE( TXT("Depot enumerated in %1.2fs"), timer.GetTimePeriod() );
		LOG_CORE( TXT("Found %d files in depot"), allFiles.Size() );
	}

	THashSet< CDiskFile* > files;

	// scan
	CXMLReader reader( xmlContent );
	if( reader.BeginNode( TXT("StartupExclusiveFiles") ) == true )
	{
		while( reader.BeginNode( TXT("Entry") ) == true )
		{
			String entryValue;
			reader.Attribute( TXT("value"), entryValue );

			// we either have a direct path or a wildcard expression with *
			if ( !entryValue.ContainsSubstring( TXT("*") ) )
			{
				// direct path ?
				CDiskFile* file = GDepot->FindFileUseLinks( entryValue, 0 );
				if ( !file )
				{
					ERR_CORE( TXT("Ignoring file '%ls' from analyze step because it does not exist"), filePath.AsChar() );
				}
				else
				{
					files.Insert( file );
				}
			}
			else
			{
				// scan all files and select the matching ones
				Uint32 numFound = 0;
				for ( CDiskFile* file : allFiles )
				{
					if ( StringHelpers::WildcardMatch( file->GetDepotPath().AsChar(), entryValue.AsChar() ) )
					{
						files.Insert( file );
						numFound += 1;
					}
				}

				LOG_CORE( TXT("Found %d files using filter '%ls'"), numFound, entryValue.AsChar() );
			}

			reader.EndNode();
		}

		reader.EndNode( false );
	}

	// compose the final list
	LOG_CORE( TXT("Found %d startup files in total"), files.Size() );
	for ( CDiskFile* file : files )
	{
		outputList.AddFile( UNICODE_TO_ANSI( file->GetDepotPath().AsChar() ) );
	}

	return true;
}

#endif
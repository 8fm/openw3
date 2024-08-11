/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/gatheredResource.h"

#ifndef NO_EDITOR

/// Analyzer for common game resources
class CR4CommonAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4CommonAnalyzer)

public:
	CR4CommonAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4common"); }
	virtual const Char* GetDescription() const { return TXT("Analyze game common resources"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );

private:
	Bool ProcessFileRecursive( const String& filePath, CAnalyzerOutputList& outputList ) const;
	Bool ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList );

	static const AnsiChar* GENERIC_BUNDLE_NAME;
	static const AnsiChar* XML_BUNDLE_NAME;
	static const Char* CSV_EXT;
	static const Char* XML_EXT;
};

BEGIN_CLASS_RTTI(CR4CommonAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4CommonAnalyzer);

const AnsiChar* CR4CommonAnalyzer::GENERIC_BUNDLE_NAME = "blob";
const Char* CR4CommonAnalyzer::CSV_EXT = TXT("csv");
const AnsiChar* CR4CommonAnalyzer::XML_BUNDLE_NAME = "xml";
const Char* CR4CommonAnalyzer::XML_EXT = TXT("xml");

CR4CommonAnalyzer::CR4CommonAnalyzer()
{
}

Bool CR4CommonAnalyzer::ProcessFileRecursive( const String& filePath, CAnalyzerOutputList& outputList ) const
{
	// find existing file in the depot
	CDiskFile* file = GDepot->FindFileUseLinks( filePath, 0 );
	if ( !file )
	{
		WARN_CORE( TXT("Ignoring file '%ls' from analyze step because it does not exist"), filePath.AsChar() );
		return true;
	}

	// add the file
	const String realFilePath = file->GetDepotPath();
	if ( !outputList.AddFile( UNICODE_TO_ANSI( realFilePath.AsChar() ) ) )
		return true; // already there

	// CVS files that we are adding here may contain links to other resources (recursive)
	Bool status = true;
	if ( realFilePath.EndsWith( CSV_EXT ) )
	{
		// assume it's a file from depot
		String absoluteFilePath = GFileManager->GetDataDirectory();
		absoluteFilePath += realFilePath.AsChar();

		// some CSV files are ignored ;)
		if ( realFilePath == TXT("engine\\templates\\editor\\editor_templates.csv") )
			return true;
		if ( realFilePath == TXT("gameplay\\globals\\gameworlds.csv") )
			return true;
		if ( realFilePath.BeginsWith( TXT("test_cases\\") ) )
			return true;

		// load the CSV
		THandle<C2dArray> ar( C2dArray::CreateFromString(absoluteFilePath) );
		if (ar)
		{
			Uint32 numDeps = 0;

			// look for the resource paths (naive but works 100% of cases right now)
			const Uint32 numCols = ar->GetNumberOfColumns();
			const Uint32 numRows = ar->GetNumberOfRows();
			for (Uint32 i=0; i<numRows; ++i)
			{
				for (Uint32 j=0; j<numCols; ++j)
				{
					const String& cellText = ar->GetValueRef(j,i);
					if ( !cellText.Empty() && cellText.GetLength() < 1000 ) // TODO: magic number ?
					{
						// is a depot file path ?
						// we use the links here for safety
						CDiskFile* file = GDepot->FindFileUseLinks( cellText, 0 );
						if (file)
						{
							if ( file->GetDepotPath() != cellText )
							{
								ERR_GAME( TXT("!!! LINK FILE IN CSV '%ls' !!! - XML references file '%ls' that was moved to '%ls'. Please update the XML (we don't have links on cook and this will not work)."),
									filePath.AsChar(), cellText.AsChar(), file->GetDepotPath().AsChar() );
								status = false;
							}

							if ( ProcessFileRecursive( file->GetDepotPath(), outputList ) )
								numDeps += 1;
						}
					}
				}
			}

			// list dependencies of the CSV files
			if (numDeps)
			{
				LOG_GAME( TXT("Found %d dependencies in CSV '%ls'"), 
					 numDeps, realFilePath.AsChar() );
			}
		}
	}

	// added
	return status;
}

bool CR4CommonAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	// extract normal gathered resources
	outputList.SetBundleName( GENERIC_BUNDLE_NAME );

	const TDynArray< CGatheredResource* > allGatheredResources = CGatheredResource::GetGatheredResources();
	for ( Uint32 i = 0; i < allGatheredResources.Size(); ++i )
	{
		const CGatheredResource* source = allGatheredResources[ i ];

		// gathered resources with the "no cook" flag are ignored
		if ( !source->IsCooked() )
			continue;

		// startup files are processed in the startup bundle
		if ( source->IsStartup())
			continue;;

		// get the gathered resource path
		const String resourcePath = source->GetPath().ToString();
		ProcessFileRecursive( resourcePath, outputList );
	}

	// Extract all CSV files and the shit referenced by them into the common bundle
	// NOTE: some of the CSV files may be in the startup bundle but the startup filtering happend MUCH later
	Bool status = true;
	status &= ExtractFiles( GDepot, CSV_EXT, NULL, true, outputList );

	// Extract XML stuff into different bundle - they are NOT read as normal files
	outputList.SetBundleName( XML_BUNDLE_NAME );
	status &= ExtractFiles( GDepot, XML_EXT, NULL, true, outputList );

	// Report problems
	if ( !status )
	{
		ERR_GAME( TXT("Analyzing the game CSV definitions reported errors. Cooking is not safe.") );
		return false;
	}

	return true;
}

Bool CR4CommonAnalyzer::ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
{
	if ( !mainDir )
		return true;

	// filter files
	Bool status = true;
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
		status &= ProcessFileRecursive( conformedPath.AsChar(), outputList );
	}

	// recurse to sub directories
	if ( recursive )
	{		
		for ( CDirectory* subDir : mainDir->GetDirectories() )
		{
			status &= ExtractFiles( subDir, fileExtensionFilter, excludedFileExtensions, recursive, outputList );
		}
	}


	return status;
}

#endif
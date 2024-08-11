/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/depot.h"
#include "../../common/game/dlcDefinition.h"

#ifndef NO_EDITOR

/// Analyzer for DLC folders
class CR4DLCAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CR4DLCAnalyzer)

public:
	CR4DLCAnalyzer();

	// interface
	virtual const Char* GetName() const { return TXT("r4dlc"); }
	virtual const Char* GetDescription() const { return TXT("Analyze DLC directory from the R4 game"); }
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );
};

namespace
{
	const AnsiChar* GENERIC_BUNDLE_NAME = "blob";
	const Char* CSV_EXT = TXT("csv");
	const AnsiChar* XML_BUNDLE_NAME = "xml";
	const Char* XML_EXT = TXT("xml");

	Bool ProcessFileRecursive( const String& filePath, CAnalyzerOutputList& outputList )
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

	Bool ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
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
}
BEGIN_CLASS_RTTI(CR4DLCAnalyzer);
PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS(CR4DLCAnalyzer);

CR4DLCAnalyzer::CR4DLCAnalyzer()
{
}

bool CR4DLCAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	String dlcPath;
	if ( !options.GetSingleOptionValue( TXT("dlc"), dlcPath ) )
	{
		ERR_GAME( TXT("Expecting DLC definition path") );
		return false;
	}

	// load DCL definition
	THandle< CDLCDefinition > def = LoadResource< CDLCDefinition >( dlcPath );
	if ( !def )
	{
		ERR_GAME( TXT("Unable to load DLC definition from '%ls'"), dlcPath.AsChar() );
		return false;
	}

	// all DLCs are outputed directly
	outputList.SetBundleName( "blob" ); // default for all DCL
	outputList.SetContentChunk( CName( TXT("dlc") ) ); // default for all DCL

	// make sure that the DLC definition file is included
	outputList.AddFile( UNICODE_TO_ANSI( dlcPath.AsChar() ) );

	// analyze the DLC
	def->DoAnalyze( outputList );
	def->Discard();

	CDirectory* dlcDirectory = GDepot->FindPath(dlcPath);

	if( dlcDirectory )
	{
		TDynArray< String > resourcesPaths;
		dlcDirectory->FindResourcesByExtension( ResourceExtension< CLayer >(), resourcesPaths );
		for ( String filePath : resourcesPaths )
		{
			outputList.AddFile( UNICODE_TO_ANSI( filePath.AsChar() ) );
		}
	}

	// Extract all CSV files and the shit referenced by them into the common bundle
	// NOTE: some of the CSV files may be in the startup bundle but the startup filtering happend MUCH later
	Bool status = true;
	status &= ExtractFiles( dlcDirectory, CSV_EXT, NULL, true, outputList );

	// Extract XML stuff into different bundle - they are NOT read as normal files
	status &= ExtractFiles( dlcDirectory, XML_EXT, NULL, true, outputList );

	// done
	return status;
}

#endif
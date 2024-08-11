#include "build.h"

#include "wccVersionControl.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"

#include "../../common/engine/swfResource.h"

//////////////////////////////////////////////////////////////////////////

static IImporter* GetImporterForExt( const String& ext )
{
	if ( ext == TXT("swf") )
	{
		return IImporter::FindImporter( ClassID< CSwfResource >(), ext );
	}

	return nullptr;
}


class CSwfImportCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CSwfImportCommandlet, ICommandlet, 0 );

private:
	void ScanDirectoryForImports( const String& fromPath, const TDynArray< String >& exts, THashMap< String, TDynArray< String > >& outImportPaths );

public:
	CSwfImportCommandlet();

	// Executes commandlet command
	virtual bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const
	{
		return TXT("Bulk import resources preserving directory structure");
	}

	// Prints commandlet help
	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Imports resources. Use wcc import [option1 value1 [option2 value2 [...]]]") );
		LOG_WCC( TXT("Options:") );
		//LOG_WCC( TXT("   ext                       -- comma separateed list of extensions to scan for (if not specified it imports all extensions)") );
		LOG_WCC( TXT("   fromAbsPath			   -- absolute path to scan inside") );
		LOG_WCC( TXT("   toDepotPath			   -- depot path to import to") );
		//LOG_WCC( TXT("   vc disable                -- disable version control") );
		//LOG_WCC( TXT("Version control options (if not disabled):") );
		//LOG_WCC( TXT("   cl changelist             -- add the files in the given changelist number") );
		//LogWCCVersionControlOptions();
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CSwfImportCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CSwfImportCommandlet );

CSwfImportCommandlet::CSwfImportCommandlet()
{
	m_commandletName = CName( TXT("swfimport") );
}

void CSwfImportCommandlet::ScanDirectoryForImports( const String& fromPath, const TDynArray< String >& exts, THashMap< String, TDynArray< String > >& outImportPaths )
{
	for ( Uint32 i = 0; i < exts.Size(); ++i )
	{
		String pattern( TXT("*.") );
		pattern += exts[ i ];
		TDynArray< String >& ar = outImportPaths[ exts[i] ];
		GFileManager->FindFiles( fromPath, pattern, ar, true );
	}
}

bool CSwfImportCommandlet::Execute( const CommandletOptions& options )
{
	// Options
	String fromAbsPath;
	if ( options.HasOption( TXT( "f" ), TXT( "fromAbsPath" ) ) )
	{
		const auto& values = options.GetOptionValues( TXT( "f" ), TXT( "fromAbsPath" ) );
		if ( values.Size() != 1 )
		{
			PrintHelp();
			return false;
		}
		fromAbsPath = values.Front();
		fromAbsPath.ReplaceAll( TXT("/"), TXT("\\") );
		if ( !fromAbsPath.Empty() && !fromAbsPath.EndsWith( TXT("\\") ) )
		{
			fromAbsPath += TXT("\\");
		}
	}
	
	String toDepotPath;
	if ( options.HasOption( TXT( "t" ), TXT( "toDepotPath" ) ) )
	{
		const auto& values = options.GetOptionValues( TXT( "t" ), TXT( "toDepotPath" ) );
		if ( values.Size() != 1 )
		{
			PrintHelp();
			return false;
		}
		toDepotPath = values.Front();
		toDepotPath.ReplaceAll( TXT("/"), TXT("\\") );
		if ( !toDepotPath.Empty() && !toDepotPath.EndsWith( TXT("\\") ) )
		{
			toDepotPath += TXT("\\");
		}
	}

	THashSet< String > skipFileNames;
	skipFileNames.Insert( TXT("gfxfontlib.swf") );

	TDynArray< String> exts;
	exts.PushBack( TXT("swf") );
// 	if ( arguments.KeyExist( TXT("ext") ) )
// 	{
// 		String extensionsDef = arguments[TXT("ext")];
// 		extensionsDef.Slice( exts, TXT(",") );
// 
// 		TDynArray< String > exts;
// 		// Show warnings about specified unknown extensions
// 		for ( auto it=exts.Begin(); it != exts.End(); ++it )
// 		{
// 			String& ext = *it;
// 			if ( GetImporterForExt( ext ) )
// 			{
// 				exts.PushBack( ext );
// 			}
// 			else
// 			{
// 				WARN_WCC( TXT("Extension %s is not recognized"), ext.AsChar() );
// 			}
// 		}
// 	}
// 	else
// 	{
// 		ERR_WCC( TXT("No file extensions specified. Bailing out") );
// 		return false;
// 	}

	// Report what we'll do
	LOG_WCC( TXT("Import scanning base is:       %s"), fromAbsPath.AsChar() );
	
	THashMap< String, TDynArray< String > > filesToImport;
	// Scan directory contents
	LOG_WCC( TXT("Scanning for files...") );
	ScanDirectoryForImports( fromAbsPath, exts, filesToImport );

	// Import
	Uint32 succeed = 0, failed = 0;
//	LOG_WCC( TXT("Importing %d resource files:"), m_filesToResave.Size() );

	for ( Uint32 i = 0; i < exts.Size(); ++i )
	{
		const TDynArray< String >& absPaths= filesToImport[ exts[i] ];
		IImporter* importer = GetImporterForExt( exts[i] );
		if ( ! importer )
		{
			ERR_WCC(TXT("Should have had an importer for ext '%ls'"), exts[i].AsChar() );
			continue;
		}

		for ( Uint32 j = 0; j < absPaths.Size(); ++j )
		{
			const String& absPath = absPaths[ j ];
			const String depotPath = toDepotPath + absPath.StringAfter( fromAbsPath, true );
			CFilePath fp( depotPath );

			if ( skipFileNames.Exist( fp.GetFileNameWithExt() ) )
			{
				LOG_WCC( TXT("Skipping '%ls'"), absPath.AsChar() );
				continue;
			}

			IImporter::ImportOptions options;
			options.m_sourceFilePath = absPath;
						
			CResource* resource = importer->DoImport( options );
			if ( ! resource )
			{
				WARN_WCC( TXT("Failed to import '%ls'"), absPath.AsChar() );
				continue;
			}
			resource->SetImportFile( options.m_sourceFilePath );
			//TBD: update thumbnails etc as done in assetBrowser.cpp

			CDirectory* depotDir = nullptr;
			if ( ! depotPath.Empty() )
			{
				depotDir = GDepot->CreatePath( fp.GetPathString() + TXT("\\") );
			}
			else
			{
				depotDir = GDepot;
			}

			if ( ! depotDir )
			{
				WARN_WCC( TXT("Failed to create depot dir for '%ls'"), absPath.AsChar() );
				continue;
			}

			const String saveName( fp.GetFileName() + TXT(".redswf") );

			// Can I specifiy a CL or should I just manually create an empty file first..
			if ( ! resource->SaveAs( depotDir, saveName ) )
			{
				WARN_WCC( TXT("Failed to save '%ls\\%ls'"), depotDir->GetDepotPath().AsChar(), saveName.AsChar() );
			}
			resource->Discard();
			
			// Make sure to process discards, since easy to run out of texture memory!
			SGarbageCollector::GetInstance().CollectNow();
		}
	}

	return true;
}

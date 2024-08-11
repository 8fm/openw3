/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/engine/collisionCacheOptimizer.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/depot.h"

/// Helper commandlet that can be used to import asset into the engine
class CImportCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CImportCommandlet, ICommandlet, 0 );

public:
	CImportCommandlet();
	~CImportCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Import assets into the engine"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CImportCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CImportCommandlet );

CImportCommandlet::CImportCommandlet()
{
	m_commandletName = CName( TXT("import") );
}

CImportCommandlet::~CImportCommandlet()
{
}

bool CImportCommandlet::Execute( const CommandletOptions& options )
{
	// get the depot directory
	{
		String depotPath;
		if ( !options.GetSingleOptionValue( TXT("depot"), depotPath ) )
		{
			ERR_WCC( TXT("Missing depot file path") );
			return false;
		}

		// Rescan depot
		if ( depotPath != TXT("local") )
		{
			if( !depotPath.EndsWith(TXT("\\")) && !depotPath.EndsWith(TXT("/")) )
			{
				depotPath += TXT( "\\" );
			}

			LOG_WCC( TXT("Attaching depot at '%ls'"), depotPath.AsChar() );
			GDepot->Remap( depotPath, false );
		}
		else
		{
			LOG_WCC( TXT("Using local depot") );
		}
	}

	// get file path (absolute)
	String filePath;
	if ( !options.GetSingleOptionValue( TXT("file"), filePath ) )
	{
		ERR_WCC( TXT("Missing input file path") );
		return false;
	}

	// get input file path
	String outputPath;
	if ( !options.GetSingleOptionValue( TXT("out"), outputPath ) )
	{
		ERR_WCC( TXT("Missing output file path") );
		return false;
	}

	// decompose output path
	const CFilePath outputFilePath( outputPath );
	const String outputFileExt = outputFilePath.GetExtension();

	// get the output file name and determine the resource class by looking at the extension
	CClass* resourceClass = nullptr;
	{
		TDynArray< CClass* > resourceClasses;
		SRTTI::GetInstance().EnumClasses( ClassID<CResource>(), resourceClasses );

		for ( CClass* cur : resourceClasses )
		{
			CResource* defaultRes = cur->GetDefaultObject<CResource>();
			if ( defaultRes && outputFileExt == defaultRes->GetExtension() )
			{
				resourceClass = cur;
				break;
			}
		}

		// unknown class
		if ( !resourceClass )
		{
			ERR_WCC( TXT("Unable to determine resource class from extension '%ls'. Avaiable extensions:"), outputFileExt.AsChar() );

			THashMap< String, String > fileExtensions;
			for ( CClass* cur : resourceClasses )
			{
				CResource* defaultRes = cur->GetDefaultObject<CResource>();
				if ( defaultRes )
				{
					fileExtensions.Insert( defaultRes->GetExtension(), defaultRes->GetFriendlyDescription() );
				}
			}

			for ( const auto it : fileExtensions )
			{
				LOG_WCC( TXT("  %ls: %ls"), it.m_first.AsChar(), it.m_second.AsChar() );
			}

			return false;
		}
	}

	// find the right importer
	const CFilePath sourceFilePath( filePath );
	IImporter* importer = IImporter::FindImporter( resourceClass, sourceFilePath.GetExtension() );
	if ( !importer )
	{
		ERR_WCC( TXT("No importer found for source extension '%ls'. Use \"wcc help import\" to check list of supported extensions and formats."), 
			sourceFilePath.GetExtension().AsChar() );
		return false;
	}

	// Initialize GpuApi
	GpuApi::InitEnv();
	GpuApi::InitDevice( 100, 100, false, false );	

	// Enable resource fallback for the missing depot files
	GDepot->EnableFallbackResources( true );

	// execute import
	IImporter::ImportOptions importOptions;
	importOptions.m_sourceFilePath = filePath;

	importer->PrepareForImport( sourceFilePath.GetFileName(), importOptions );
	CResource* res = importer->DoImport( importOptions );
	if ( !res )
	{
		ERR_WCC( TXT("Failed to import data from '%ls'"), filePath.AsChar() );
		return false;
	}

	// Create writer.
	Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( outputPath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !writer.Get( ) )
	{
		ERR_WCC( TXT( "Failed to create writer for file '%ls'!" ), outputPath.AsChar( ) );
		return false;
	}

	// Save single object.
	CDependencySaver saver( *writer, NULL );
	DependencySavingContext context( res );
	if( !saver.SaveObjects( context ) )
	{
		WARN_WCC( TXT( "Failed to save imported resource to file '%ls'!" ), outputPath.AsChar( ) );
		return false;
	}

	// done
	LOG_WCC( TXT( "Imported file '%ls' from '%ls'!" ), outputPath.AsChar(), filePath.AsChar() );
	return true;
}

void CImportCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  import -depot=<local|absolutepath> -file=<inputfile> -out=<outputfile>" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Params:" ) );
	LOG_WCC( TXT( "  -depot=local          - Use local depot (r4data)" ) );
	LOG_WCC( TXT( "  -depot=absolutepath   - Use depot at given directory" ) );
	LOG_WCC( TXT( "  -file=inputfile       - Absolute path to file to import" ) );
	LOG_WCC( TXT( "  -out=outputfile       - Relative (depot) path for the output file" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Supported resource types and formats:" ) );

	// get all importers
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IImporter >(), importerClasses );

	TDynArray< CName > allowedImportClassesNames;
	allowedImportClassesNames.PushBack( CName( TXT("CBitmapTexture") ) );
	allowedImportClassesNames.PushBack( CName( TXT("CMesh") ) );

	// get all supported resource classes
	TDynArray< CClass* > resourceClasses;
	for ( CClass* importerClass : importerClasses )
	{
		IImporter* importer = importerClass->GetDefaultObject<IImporter>();
		if ( importer )
		{
			CClass* resourceClass = importer->GetSupportedResourceClass();
			if ( resourceClass && allowedImportClassesNames.Exist( resourceClass->GetName() ) )
			{
				resourceClasses.PushBackUnique( resourceClass );
			}
		}
	}

	// get all formats for each class
	for ( CClass* resourceClass : resourceClasses )
	{
		TDynArray< CFileFormat > fileFormats;
		IImporter::EnumImportFormats( resourceClass, fileFormats );

		if ( !fileFormats.Empty() )
		{
			const CResource* defaultResource = resourceClass->GetDefaultObject< CResource >();
			LOG_WCC( TXT("  %ls (%ls) importable from %d file format(s):"), 
				defaultResource->GetExtension(),
				defaultResource->GetFriendlyDescription(), fileFormats.Size() );

			for ( const auto& format : fileFormats )
			{
				LOG_WCC( TXT("    %ls: %ls"), 
					format.GetExtension().AsChar(),
					format.GetDescription().AsChar() );
			}
		}
	}	
}

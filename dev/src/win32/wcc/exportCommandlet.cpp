/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/engine/collisionCacheOptimizer.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/depot.h"
#include "../../common/core/exporter.h"

/// Helper commandlet that can be used to import asset into the engine
class CExportCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CExportCommandlet, ICommandlet, 0 );

public:
	CExportCommandlet();
	~CExportCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Export single assets from the engine"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CExportCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CExportCommandlet );

CExportCommandlet::CExportCommandlet()
{
	m_commandletName = CName( TXT("export") );
}

CExportCommandlet::~CExportCommandlet()
{
}

bool CExportCommandlet::Execute( const CommandletOptions& options )
{	
	// get the depot directory
	{
		String depotPath;
		if ( !options.GetSingleOptionValue( TXT("depot"), depotPath ) )
		{
			ERR_WCC( TXT("Missing depot file path") );
			return false;
		}

		if ( depotPath != TXT("local") )
		{
			if( !depotPath.EndsWith(TXT("\\")) && !depotPath.EndsWith(TXT("/")) )
			{
				depotPath += TXT( "\\" );
			}

			// Rescan depot
			LOG_WCC( TXT("Attaching depot at '%ls'"), depotPath.AsChar() );
			GDepot->Remap( depotPath, false );
		}
	}

	// get file path (absolute)
	String inputPath;
	if ( !options.GetSingleOptionValue( TXT("file"), inputPath ) )
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

	// Initialize GpuApi
	GpuApi::InitEnv();
	GpuApi::InitDevice( 100, 100, false, false );


	// Find the input file
	CDiskFile* inputFile = GDepot->FindFileUseLinks( inputPath, 0 );
	if ( !inputFile )
	{
		ERR_WCC( TXT("No file '%ls' existing under depot"), inputPath.AsChar() );
		return false;
	}

	// Enable resource fallback for the missing depot files
	GDepot->EnableFallbackResources( true );

	// Load file
	THandle< CResource > res = inputFile->Load();
	if ( !res.IsValid() )
	{
		ERR_WCC( TXT("Unable to load file '%ls' from depot"), inputPath.AsChar() );
		return false;
	}

	// decompose output path
	const CFilePath outputFilePath( outputPath );
	const String outputFileExt = outputFilePath.GetExtension();

	// find exporter
	IExporter* exporter = IExporter::FindExporter( res->GetClass(), outputFileExt );
	if ( !exporter )
	{
		ERR_WCC( TXT("No exporter found that exports '%ls' into format '%ls'"), res->GetClass()->GetName().AsChar(), outputFileExt.AsChar() );
		return false;
	}

	// setup export
	IExporter::ExportOptions exportOptions;

	// extra options
	{
		String lodTxt;
		if ( options.GetSingleOptionValue( TXT("lod"), lodTxt ) )
		{
			exportOptions.m_lodToUse = _wtoi( lodTxt.AsChar() );
		}
	}

	exportOptions.m_saveFileFormat = CFileFormat( outputFileExt, String::EMPTY );
	exportOptions.m_saveFilePath = outputPath;
	exportOptions.m_resource = res;

	// export the file
	if ( !exporter->DoExport( exportOptions ) )
	{
		ERR_WCC( TXT("Failed to export '%ls' into format '%ls'"), inputPath.AsChar(), outputFileExt.AsChar() );
		return false;
	}

	// exported
	LOG_WCC( TXT("Exported '%ls' into '%ls'"), inputPath.AsChar(), outputPath.AsChar() );
	return true;
}

void CExportCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  export -depot=<local|absolutepath> -file=<relativepath> -out=<absoluteoutputpath>" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Params:" ) );
	LOG_WCC( TXT( "  -depot=local          - Use local depot (r4data)" ) );
	LOG_WCC( TXT( "  -depot=absolutepath   - Use depot at given directory" ) );
	LOG_WCC( TXT( "  -file=relativepath    - Local (depot) path for the file to export" ) );
	LOG_WCC( TXT( "  -out=absolutepath     - Output absolute path for the exported file" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Supported resource types and formats:" ) );

	// get all importers
	TDynArray< CClass* > exporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IExporter >(), exporterClasses );

	TDynArray< CName > allowedImportClassesNames;
	allowedImportClassesNames.PushBack( CName( TXT("CBitmapTexture") ) );
	allowedImportClassesNames.PushBack( CName( TXT("CMesh") ) );

	// get all supported resource classes
	TDynArray< CClass* > resourceClasses;
	for ( CClass* exporterClass : exporterClasses )
	{
		IExporter* exporter = exporterClass->GetDefaultObject<IExporter>();
		if ( exporter )
		{
			CClass* resourceClass = exporter->GetSupportedResourceClass();
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
		IExporter::EnumExportFormats( resourceClass, fileFormats );

		if ( !fileFormats.Empty() )
		{
			const CResource* defaultResource = resourceClass->GetDefaultObject< CResource >();
			LOG_WCC( TXT("  %ls (%ls) exportable into %d file format(s):"), 
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

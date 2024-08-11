/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/class.h"
#include "../../common/core/rttiSystem.h"
#include "../../common/core/depot.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheBuilderCommandlet.h"
#include "baseCacheBuilder.h"

IMPLEMENT_ENGINE_CLASS( CCacheBuilderCommandlet );

//----

CCacheBuilderCommandlet::Settings::Settings()
	: m_platform( PLATFORM_None )
	, m_distributeModulo( 1 )
	, m_distributeOffset( 0 )
{
}

Bool CCacheBuilderCommandlet::Settings::Parse( const CommandletOptions& options )
{
	String platformName;
	if ( !options.GetSingleOptionValue( TXT("platform"), platformName ) )
	{
		ERR_WCC( TXT("Expecting platform name") );
		return false;
	}

	// match platform name
	if ( platformName == TXT("pc") )
	{
		m_platform = PLATFORM_PC;
	}
#ifndef WCC_LITE
	else if ( platformName == TXT("xboxone") || platformName == TXT("durango") )
	{
		m_platform = PLATFORM_XboxOne;
	}
	else if ( platformName == TXT("ps4") || platformName == TXT("orbis") )
	{
		m_platform = PLATFORM_PS4;
	}
#endif
	else
	{
		ERR_WCC( TXT("Invalid platform name: %ls"), platformName.AsChar() );
		return false;
	}

	// output path
	if ( !options.GetSingleOptionValue( TXT("out"), m_outputFilePath ) )
	{
		ERR_WCC( TXT("Expecting output file path") );
		return false;
	}

	// data base path
	if ( !options.GetSingleOptionValue( TXT("db"), m_dataBasePath ) )
	{
		ERR_WCC( TXT("Expecting data base input path") );
		return false;
	}

	// modulo
	String moduloTxt;
	if ( options.GetSingleOptionValue( TXT("modulo"), moduloTxt ) )
	{
		if ( !FromString< Uint32 >( moduloTxt, m_distributeModulo ) )
		{
			ERR_WCC( TXT("Unable to parse number from '%ls'"), moduloTxt.AsChar() );
			return false;
		}

		// offset must also be specified when we use the modulo setting
		String offsetTxt;
		if ( !options.GetSingleOptionValue( TXT("offset"), offsetTxt ) )
		{
			ERR_WCC( TXT("Modulo parameter was specified but offset parameter is missing") );
			return false;
		}

		if ( !FromString< Uint32 >( offsetTxt, m_distributeOffset ) )
		{
			ERR_WCC( TXT("Unable to parse number from '%ls'"), offsetTxt.AsChar() );
			return false;
		}

		// make sure the offset is smaller than the modulo
		if ( m_distributeOffset >= m_distributeModulo )
		{
			ERR_WCC( TXT("Offset parameter must be smaller than modulo parameter (%d<%d)"), m_distributeOffset, m_distributeModulo );
			return false;
		}
	}

	// custom base directory
	String baseDir;
	if ( options.GetSingleOptionValue( TXT("basedir"), baseDir ) )
	{
		if ( !baseDir.EndsWith( TXT("\\") ) && !baseDir.EndsWith( TXT("/") ) )
		{
			baseDir += TXT("\\");
		}

		LOG_WCC( TXT("Remapped depot directory to '%ls'"), baseDir.AsChar() );
		GDepot->Remap( baseDir );
	}

	// we need the tool name
	if ( options.GetFreeArguments().Empty() )
	{
		ERR_WCC( TXT("Expecting tool name") );
		return false;
	}

	// get the tool name (first free arg for now)
	m_builderName = options.GetFreeArguments()[0];



	// all settings parsed
	return true;
}

//----

CCacheBuilderCommandlet::CCacheBuilderCommandlet()
{
	m_commandletName = CName( TXT("buildcache") );
}

CCacheBuilderCommandlet::~CCacheBuilderCommandlet()
{
}

Bool CCacheBuilderCommandlet::Execute( const CommandletOptions& options )
{
	// parse settings
	if ( !m_settings.Parse(options))
		return false;

	// find tool class
	Red::TUniquePtr< ICacheBuilder > builder;
	TDynArray< CClass* > toolClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ICacheBuilder >(), toolClasses );
	for ( CClass* toolClass : toolClasses )
	{
		ICacheBuilder* cacheBuilder = toolClass->GetDefaultObject< ICacheBuilder >();
		if ( cacheBuilder && ( m_settings.m_builderName == cacheBuilder->GetName() ) )
		{
			builder.Reset( toolClass->CreateObject< ICacheBuilder >() );
			break;
		}
	}

	// no tool
	if ( !builder )
	{
		ERR_WCC( TXT("Cache builder for '%ls' not found"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// load the DB
	if ( !m_dataBase.LoadFromFile( m_settings.m_dataBasePath ) )
	{
		ERR_WCC( TXT("Failed to load data base from '%ls'"), m_settings.m_dataBasePath.AsChar() );
		return false;
	}

	// get the list of the classes required by the builder
	TDynArray< String > classesToProcess;
	builder->GetExtensions( classesToProcess, m_settings.m_platform );
	if ( classesToProcess.Empty() )
	{
		ERR_WCC( TXT("No resource classes specified for builder '%ls'"), m_settings.m_builderName.AsChar() );
		return false;
	}	

	// make sure output path exists
	if ( !GFileManager->CreatePath( m_settings.m_outputFilePath ) )
	{
		ERR_WCC( TXT("Failed to create output path '%ls'"), m_settings.m_outputFilePath.AsChar() );
		return false;
	}

	// initialize the cache builder
	if ( !builder->Initialize( m_settings.m_outputFilePath, m_settings.m_platform, options ) )
	{
		ERR_WCC( TXT("Failed to initialize cache builder '%ls'"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// get all entries from DB matching the classes we are interested in
	typedef TDynArray< CCookerDataBase::TCookerDataBaseID > TFileList;
	TFileList filesToProcess;
	CookDataBaseHelper::FileCollectorModulo<TFileList> fileCollector( filesToProcess, m_settings.m_distributeModulo, m_settings.m_distributeOffset );
	m_dataBase.QueryResources( fileCollector, CookDataBaseHelper::PerExtensionFilter( classesToProcess ) );

	LOG_WCC( TXT("Found %d files to process"), filesToProcess.Size() );

	// no files to process - that's NOT an error
	if ( filesToProcess.Empty() )
		return true;

	// process the files
	if ( !builder->Process( m_dataBase, filesToProcess ) )
	{
		ERR_WCC( TXT("Cache builder '%ls' failed"), m_settings.m_builderName.AsChar() );
		return false;
	}
	
	// ask the cache to save the results
	if ( !builder->Save() )
	{
		ERR_WCC( TXT("Saving result of cache builder '%ls' failed"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// done
	return true;
}

void CCacheBuilderCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage: " ) );
	LOG_WCC( TXT( "  buildcache <builder> -db <database> -out <absolutepath> [optional params]" ) );
	LOG_WCC( TXT( "" ) );

	TDynArray< CClass* > toolClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ICacheBuilder >(), toolClasses );
	if ( !toolClasses.Empty() )
	{
		LOG_WCC( TXT( "Avaiable cache builders:" ) );

		for ( Uint32 i=0; i<toolClasses.Size(); ++i )
		{
			CClass* toolClass = toolClasses[i];
			const ICacheBuilder* tool = toolClass->GetDefaultObject<ICacheBuilder>();

			LOG_WCC( TXT( "  %s - %s" ),
				tool->GetName(), tool->GetDescription() );
		}
	}
	else
	{
		LOG_WCC( TXT("No public analyzers avaiable") );
	}	

	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Additional options:" ) );
	LOG_WCC( TXT( "  -modulo=N      - Process every Nth file" ) );
	LOG_WCC( TXT( "  -offset=X      - Initial offset for processing (use only with -modulo)") );
	LOG_WCC( TXT( "  -db=<path>     - Path to cook.db" ) );
	LOG_WCC( TXT( "  -out=<path>    - Path to output cache file" ) );
}

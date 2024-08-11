/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"

#include "pathlibCooker.h"

class CPathLibCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CPathLibCommandlet, ICommandlet, 0 );

protected:
	Bool ComputePathLibWorld( const String& worldFilePath, Uint32 flags );
	Bool BundleUp();

public:
	CPathLibCommandlet();

	virtual const Char* GetOneLiner() const;

	virtual bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;


};

BEGIN_CLASS_RTTI( CPathLibCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CPathLibCommandlet );

RED_DEFINE_STATIC_NAME( pathlib )

CPathLibCommandlet::CPathLibCommandlet()
{
	m_commandletName = CNAME( pathlib );
}

Bool CPathLibCommandlet::ComputePathLibWorld( const String& worldFilePath, Uint32 flags )
{
	WorldLoadingContext loadingContext;

	CWorld* world = CWorld::LoadWorld( worldFilePath, loadingContext );
	if( !world )
	{
		ERR_WCC( TXT( "Can't load the '%s' world resource." ), worldFilePath.AsChar() );
		return false;
	}

	CPathLibCooker cooker( world, flags );
	
	Bool cookingIsSuccessfult = cooker.Cook();
	if ( !cookingIsSuccessfult )
	{
		ERR_WCC( TXT("PathLib cooking process has failed!") );
	}

	Bool bundlingIsSuccessful = cookingIsSuccessfult && BundleUp();
	if ( !bundlingIsSuccessful )
	{
		ERR_WCC( TXT("PathLib bundling process has failed!"));
	}



	CWorld::UnloadWorld( world );

	return cookingIsSuccessfult && bundlingIsSuccessful;
}

bool CPathLibCommandlet::BundleUp()
{
	return true;
	//Bool conversionOk = pathlib->ConvertLegacyNavdataFormat();
	//Bool bundlingSuccessful = false;
	//if ( conversionOk )
	//{

	//	LOG( TXT( "Successfully converted worlds %s legacy navdata!" ), worldFilePath.AsChar() );

	//	String depotPath;
	//	GDepot->GetAbsolutePath( depotPath );

	//	Red::Core::ResourceManagement::CResourcePaths& pathManager = GEngine->GetPathManager();

	//	CDiskFile* worldFile			= world->GetFile();
	//	CDirectory* worldDirectory		= worldFile->GetDirectory();
	//	const String& worldFilename		= worldFile->GetFileName();

	//	String jsonAbsolutePath;
	//	worldDirectory->GetAbsolutePath( jsonAbsolutePath );
	//	jsonAbsolutePath += TXT("/navi/bundleDef.json");

	//	String bundleDirectoryAbsolute = pathManager.GetPath( Red::Core::ResourceManagement::CResourcePaths::Path_BundlesDirectoryAbsolute );

	//	Char exePathStr[ MAX_PATH ];

	//	GetModuleFileName( NULL, exePathStr, MAX_PATH );
	//	Char* lastOccurence = Red::System::StringSearchLast( exePathStr, '\\' );
	//	if ( lastOccurence )
	//	{
	//		*lastOccurence = 0;
	//	}

	//	//-definition D:\tmp\skellige.json -depotpath E:\Perforce\Main\r4data\ -outputdir D:\tmp\skellige\ -verbose
	//	String commandLine = String::Printf
	//		(
	//		TXT( "%s\\bundlebuilder.exe -verbose -depotpath %" ) RED_PRIWs TXT( " -definition %" ) RED_PRIWs TXT( " -outputdir %" ) RED_PRIWs,
	//		exePathStr,
	//		depotPath.AsChar(),
	//		jsonAbsolutePath.AsChar(),
	//		bundleDirectoryAbsolute.AsChar()
	//		);
	//	RED_LOG( RED_LOG_CHANNEL( "Bundler" ), commandLine.AsChar() );

	//	// Run bundler and stop execution until it finishes
	//	if ( _wsystem( commandLine.AsChar() ) == 0 )
	//	{
	//		bundlingSuccessful = true;
	//	}

	//	if ( bundlingSuccessful )
	//	{
	//		LOG( TXT( "Navigation bundles created successfully!" ), worldFilePath.AsChar() );
	//	}
	//	else
	//	{
	//		LOG( TXT( "Couldn't run bundling proccess!" ), worldFilePath.AsChar() );
	//	}

	//}
	//else
	//{
	//	LOG( TXT( "Encountered problems during conversion of worlds %s legacy navdata!" ), worldFilePath.AsChar() );
	//}
}

bool CPathLibCommandlet::Execute( const CommandletOptions& options )
{
	auto arguments = options.GetFreeArguments();
	if( arguments.Size() < 1 )
	{
		ERR_WCC( TXT( "Invalid arguments for the 'pathlib' command" ) );
		LOG_WCC( TXT( "wcc pathlib rootSearchDir *args" ) );
		return false;
	}

	String rootDir;
	GDepot->GetAbsolutePath( rootDir );
	rootDir = rootDir + arguments[ 0 ].ToLower();
	String filePattern( TXT("*.w2w") );

	Uint32 flags = CPathLibCooker::FLAGS_DEFAULT;

	for ( Uint32 i = 1, n = arguments.Size(); i < n; ++i )
	{
		String option = arguments[ i ].ToLower();
		if ( option == TXT("ignoreobstacles") )
		{
			flags |= CPathLibCooker::FLAG_IGNORE_OBSTACLES;
		}
		else if ( option == TXT("nopathlib") )
		{
			flags |= CPathLibCooker::FLAG_NO_PATHLIB;
		}
	}

	TDynArray< String > worldPaths;
	GFileManager->FindFiles( rootDir, filePattern, worldPaths, true );
	if ( worldPaths.Empty() )
	{
		ERR_WCC( TXT("WCC command couldn't find and worlds to play with") );
		return false;
	}

	String localWorldPath;
	Bool ret = true;
	for ( Uint32 i = 0; i < worldPaths.Size(); i++ )
	{
		if( !GDepot->ConvertToLocalPath( worldPaths[i], localWorldPath ) )
		{
			ERR_WCC( TXT( "World path '%s' is not udner resource root: '%s'." ), worldPaths[i].AsChar(), GDepot->GetRootDataPath().AsChar() );
			continue;
		}
		ret &= ComputePathLibWorld( localWorldPath, flags );
	}

	if ( ret )
	{
		LOG_WCC( TXT( "WCC command completed with full success" ) );
		return true;
	}
	else
	{
		LOG_WCC( TXT( "WCC command completed with problems" ) );
		return false;
	}
}

const Char* CPathLibCommandlet::GetOneLiner() const
{
	return TXT( "Generate navigation data for given world" );
}

void CPathLibCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc pathlib rootSearchDir filePattern" ) );
}

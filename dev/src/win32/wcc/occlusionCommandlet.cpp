/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/engine/umbraJobs.h"

class COcclusionCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( COcclusionCommandlet, ICommandlet, 0 );

public:
	COcclusionCommandlet();

	Bool GenerateOcclusionData( const String& worldFilePath, const VectorI& bounds, STomeDataGenerationContext& umbraContext );

	virtual const Char* GetOneLiner() const;

	virtual Bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;

private:
	Bool	m_dumpScene;
	Bool	m_dumpRawTomeData;
	Bool	m_dumpExistingTomeData;
	Bool	m_forceRegenerate;
};

BEGIN_CLASS_RTTI( COcclusionCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( COcclusionCommandlet );

COcclusionCommandlet::COcclusionCommandlet()
	: m_dumpScene( false )
	, m_dumpRawTomeData( false )
	, m_dumpExistingTomeData( false )
	, m_forceRegenerate( false )
{
	m_commandletName = CNAME( cookocclusion );
}

template< class T >
RED_INLINE void ParseParameter( const ICommandlet::CommandletOptions& options, const String& paramName, T& paramValue )
{
	String stringValue;
	if ( options.GetSingleOptionValue( paramName, stringValue ) )
	{
		if ( !FromString< T >( stringValue, paramValue ) )
		{
			WARN_WCC( TXT( "Argument '%ls' has invalid value '%ls', default value '%ls' will be used." ), paramName.AsChar(), stringValue.AsChar(), ToString< T >( paramValue ).AsChar() );
		}
	}
	LOG_WCC( TXT( "Occlusion commandlet parameter '%ls'='%ls'" ), paramName.AsChar(), ToString< T >( paramValue ).AsChar() );
}

Bool COcclusionCommandlet::Execute( const CommandletOptions& options )
{
	if ( !options.HasOption( TXT( "world" ) ) )
	{
		ERR_WCC( TXT( "Missing argument 'world'. Please provide world path to calculate occlusion for." ) );
		return false;
	}

#ifdef USE_UMBRA
	auto arguments = options.GetFreeArguments();

	if ( options.HasOption( TXT( "dumpScene" ) ) )
	{
		m_dumpScene = true;
	}

	if ( options.HasOption( TXT( "dumpRawTomeData" ) ) )
	{
		m_dumpRawTomeData = true;
	}

	if ( options.HasOption( TXT( "dumpExistingTomeData" ) ) )
	{
		m_dumpExistingTomeData = true;
	}
	
	if ( options.HasOption( TXT( "forceRegenerate" ) ) )
	{
		m_forceRegenerate = true;
	}

	if ( options.HasOption( TXT("noassert" ) ) )
	{
		LOG_WCC( TXT("EditorEngine: Assertions has been DISABLED") );
		Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
	}
	
	Float smallestOccluder = UmbraSettings::SMALLEST_OCCLUDER;
	Float smallestHole = UmbraSettings::SMALLEST_HOLE;
	Float umbraTileSize = UmbraSettings::UMBRA_TILE_SIZE;
	Int32 boundsXMin = -1;
	Int32 boundsYMin = -1;
	Int32 boundsXMax = -1;
	Int32 boundsYMax = -1;

	// parse optional parameters
	ParseParameter< Float >( options, TXT( "smallestOccluder" ), smallestOccluder );
	ParseParameter< Float >( options, TXT( "smallestHole" ), smallestHole );
	ParseParameter< Float >( options, TXT( "tileSize" ), umbraTileSize );
	ParseParameter< Int32 >( options, TXT( "xMin" ), boundsXMin );
	ParseParameter< Int32 >( options, TXT( "xMax" ), boundsXMax );
	ParseParameter< Int32 >( options, TXT( "yMin" ), boundsYMin );
	ParseParameter< Int32 >( options, TXT( "yMax" ), boundsYMax );
	
	VectorI bounds( boundsXMin, boundsYMin, boundsXMax, boundsYMax );

	static const Float tileSize = 256.0f;
	SComputationParameters computationParams( smallestOccluder, smallestHole, tileSize, umbraTileSize );
	STomeDataGenerationContext umbraContext;
	umbraContext.computationParameters = computationParams;
	umbraContext.forceRegenerate = m_forceRegenerate;
	TList< String > worlds = options.GetOptionValues( TXT( "world" ) );

	Bool ret = true;
	for ( auto it = worlds.Begin(), itEnd = worlds.End(); it != itEnd; ++it )
	{
		const String& world = *it;
		ret &= GenerateOcclusionData( world, bounds, umbraContext );
	}
	return ret;
#else
	ERR_WCC( TXT( "Umbra is not available atm!" ) );
	return false;
#endif
}

bool COcclusionCommandlet::GenerateOcclusionData( const String& worldFilePath, const VectorI& bounds, STomeDataGenerationContext& umbraContext )
{
#ifdef USE_UMBRA
	if ( !GDepot->FindFile( worldFilePath ) )
	{
		ERR_WCC( TXT( "World path '%ls' not found." ), worldFilePath.AsChar() );
		return false;
	}

	WorldLoadingContext loadingContext;
	CWorld* world = CWorld::LoadWorld( worldFilePath, loadingContext );
	if( !world )
	{
		ERR_WCC( TXT( "Can't load world '%ls'." ), worldFilePath.AsChar() );
		return false;
	}

	Bool occlusionGenerationStatus = world->GenerateOcclusionForAllTilesSync( bounds, umbraContext, m_dumpScene, m_dumpRawTomeData, m_dumpExistingTomeData );
	
	CWorld::UnloadWorld( world );

	return occlusionGenerationStatus; 
#endif
	return false;
}

const Char* COcclusionCommandlet::GetOneLiner() const
{
	return TXT( "Generate occlusion for given worlds" );
}

void COcclusionCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc cookocclusion -world=pathToWorldFileN [-smallesOccluder=..] [-smallestHole=..] [-tileSize=..] [-xMin=..] [-xMax=..] [-yMin=..] [-yMax=..]" ) );
}

//////////////////////////////////////////////////////////////////////////

#define UMBRA_MEMORY_COMMANDLET_MIN_DENSITY (0.5f)
#define UMBRA_MEMORY_COMMANDLET_MAX_DENSITY (10000.0f)

class CUmbraMemoryCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CUmbraMemoryCommandlet, ICommandlet, 0 );

public:
	CUmbraMemoryCommandlet();

	Bool CalculateRuntimeMemory( const String& worldFilePath, const Float density );

	virtual const Char* GetOneLiner() const;

	virtual Bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CUmbraMemoryCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CUmbraMemoryCommandlet );

CUmbraMemoryCommandlet::CUmbraMemoryCommandlet()
{
	m_commandletName = CNAME( calculateRuntimeOcclusionMemory );
}

Bool CUmbraMemoryCommandlet::Execute( const CommandletOptions& options )
{
#ifdef USE_UMBRA
	auto arguments = options.GetFreeArguments();

	if ( options.HasOption( TXT("noassert") ) )
	{
		LOG_WCC( TXT("Assertions has been DISABLED") );
		Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
	}

	Float density = 100.0f;
	ParseParameter< Float >( options, TXT( "density" ), density );
	density = Clamp< Float >( density, UMBRA_MEMORY_COMMANDLET_MIN_DENSITY, UMBRA_MEMORY_COMMANDLET_MAX_DENSITY );

	String worldName = String::EMPTY;
	ParseParameter< String >( options, TXT( "world" ), worldName );
	if ( worldName.Empty() )
	{
		ERR_WCC( TXT( "No world path specified!" ) );
		return false;
	}

	return CalculateRuntimeMemory( worldName, density );
#else
	ERR_WCC( TXT( "Umbra is not available atm!" ) );
	return false;
#endif
}

Bool CUmbraMemoryCommandlet::CalculateRuntimeMemory( const String& worldFilePath, const Float density )
{
#ifdef USE_UMBRA
	ResourceLoadingContext resourceContext;
	if ( !GDepot->FindFile( worldFilePath ) )
	{
		ERR_WCC( TXT( "World path '%ls' not found." ), worldFilePath.AsChar() );
		return false;
	}

	WorldLoadingContext loadingContext;
	CWorld* world = CWorld::LoadWorld( worldFilePath, loadingContext );
	if( !world )
	{
		ERR_WCC( TXT( "Can't load world '%ls'." ), worldFilePath.AsChar() );
		return false;
	}

	Bool occlusionGenerationStatus = world->CalculateRuntimeOcclusionMemoryUsage( density );

	CWorld::UnloadWorld( world );

	return occlusionGenerationStatus;
#endif
	return false;
}

const Char* CUmbraMemoryCommandlet::GetOneLiner() const
{
	return TXT( "Calculate required memory for runtime occlusion data for the level" );
}

void CUmbraMemoryCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc calculateRuntimeOcclusionMemory [-density=..] -world=pathToWorldFile" ) );
}

//////////////////////////////////////////////////////////////////////////

class CDuplicatesCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CDuplicatesCommandlet, ICommandlet, 0 );

public:
	CDuplicatesCommandlet();

	virtual const Char* GetOneLiner() const;

	virtual Bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CDuplicatesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDuplicatesCommandlet );

CDuplicatesCommandlet::CDuplicatesCommandlet()
{
	m_commandletName = CNAME( findDuplicates );
}

Bool CDuplicatesCommandlet::Execute( const CommandletOptions& options )
{
#ifdef USE_UMBRA
	auto arguments = options.GetFreeArguments();

	if ( options.HasOption( TXT("noassert") ) )
	{
		LOG_WCC( TXT("Assertions has been DISABLED") );
		Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
	}

	SDuplicatesFinderContext context;
	context.useOutputFile = false;
	context.outputFilePath = String::EMPTY;

	String outputFile = String::EMPTY;
	ParseParameter< String >( options, TXT( "output" ), outputFile );
	if ( !outputFile.Empty() )
	{
		LOG_WCC( TXT( "Output will be written to: %s" ), outputFile.AsChar() );
		context.useOutputFile = true;
		context.outputFilePath = outputFile;
	}

	String worldName = String::EMPTY;
	ParseParameter< String >( options, TXT( "world" ), worldName );
	if ( worldName.Empty() )
	{
		ERR_WCC( TXT( "No world path specified!" ) );
		return false;
	}

	ResourceLoadingContext resourceContext;
	if ( !GDepot->FindFile( worldName ) )
	{
		ERR_WCC( TXT( "World path '%ls' not found." ), worldName.AsChar() );
		return false;
	}

	WorldLoadingContext loadingContext;
	CWorld* world = CWorld::LoadWorld( worldName, loadingContext );
	if( !world )
	{
		ERR_WCC( TXT( "Can't load world '%ls'." ), worldName.AsChar() );
		return false;
	}

	Bool result = world->GenerateDuplicatesList( context );

	CWorld::UnloadWorld( world );

	return result;
#else
	ERR_WCC( TXT( "Umbra is not available atm!" ) );
	return false;
#endif
}


const Char* CDuplicatesCommandlet::GetOneLiner() const
{
	return TXT( "Find duplicate geometry for the level" );
}

void CDuplicatesCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc findDuplicates -world=pathToWorldFile [-output=file]" ) );
}

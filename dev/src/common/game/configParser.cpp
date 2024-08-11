#include "build.h"
#include "configParser.h"
#include "../../common/core/codeParser.h"

bool GameConfig::LoadConfig( const String& game )
{
	String configPath;

#if defined( RED_PLATFORM_WINPC )
	Char basePath[ MAX_PATH ];
	configPath = TXT( "..\\gameconf.cfg" );
	GetFullPathName( configPath.AsChar(), MAX_PATH, basePath, NULL );
	configPath = basePath;
#elif defined( RED_PLATFORM_ORBIS )
	configPath = TXT("/app0/bin/gameconf.cfg");
#else
	RED_FATAL_ASSERT( false, "Config load is not implemented on this platform" );
#endif

	String cfgContents;	
	if ( !GFileManager->LoadFileToString( configPath, cfgContents, true ) )
	{
		RED_LOG_ERROR( GameConfig, TXT( "Can't load the game config file: '%ls'!" ), configPath.AsChar() );
		return false;
	}
	
	THashMap< String, THashMap< String, String > > paramsMap;
	if( !ParseConfig( cfgContents, paramsMap ) )
	{
		RED_LOG_ERROR( GameConfig, TXT( "Can't parse game config" ) );
		return false;
	}
	if( !paramsMap.KeyExist( game ) )
	{
		RED_LOG_ERROR( GameConfig, TXT( "Can't find game '%ls' in the config" ), game.AsChar() );
		return false;
	}

	THashMap< String, String > gameParamsMap = paramsMap[ game ];
	SGameConfigurationParameter cfgParams;
	cfgParams.name = game;
	cfgParams.userPathSuffix			= gameParamsMap[ TXT("title") ];
	cfgParams.dataPathSuffix			= gameParamsMap[ TXT("data") ];
	cfgParams.bundlePathSuffix			= gameParamsMap[ TXT("bundle") ];
	cfgParams.configDirectoryName		= gameParamsMap[ TXT("config") ];
	cfgParams.scriptsPathSuffix			= gameParamsMap[ TXT("scripts") ];
	cfgParams.gameClassName				= gameParamsMap[ TXT("gameClass") ];
	cfgParams.playerClassName			= gameParamsMap[ TXT("playerClass") ];
	cfgParams.telemetryClassName		= gameParamsMap[ TXT("telemetryClass") ];
	cfgParams.cameraDirectorClassName	= gameParamsMap[ TXT("cameraDirClassName") ];

#ifdef RED_PLATFORM_ORBIS
	cfgParams.userPathSuffix.ReplaceAll(TXT("\\"),TXT("/"));
	cfgParams.dataPathSuffix.ReplaceAll(TXT("\\"),TXT("/"));
	cfgParams.bundlePathSuffix.ReplaceAll(TXT("\\"),TXT("/"));
	cfgParams.scriptsPathSuffix.ReplaceAll(TXT("\\"),TXT("/"));
#endif

	GGameConfig::GetInstance().Initialize( cfgParams );

	return true;
}
bool GameConfig::ParseConfig( const String& configContents, THashMap< String, THashMap< String, String > >& outConfig )
{
	// Parse the configuration file
	CCodeParser parser( configContents );
	parser.AddDelimiters( TXT("{}") );
	parser.SetParseStrings();
	parser.SetIncludeStringQuotes( false );
	parser.SkipWhitespace();
	while ( parser.HasMore() )
	{
		// Scan the game's name 
		String gameName = parser.ScanToken();

		// Make sure the game isn't already defined
		if ( outConfig.KeyExist( gameName ) )
		{
			WARN_GAME( TXT("Game configuration parsing error: the game %ls is already defined"), gameName.AsChar() );
			break;
		}

		// Make sure an opening brace is following
		if ( parser.ScanToken() != TXT("{") )
		{
			WARN_GAME( TXT("Game configuration parsing error: missing opening brace after the game %ls's name"), gameName.AsChar() );
			break;
		}

		// Scan the game's configuration parameters
		while ( parser.HasMore() )
		{
			// Stop if we find a closing brace
			if ( parser.PeekToken() == TXT("}") )
			{
				break;
			}

			// Scan name and value 
			String paramName = parser.ScanToken();
			String paramValue = parser.ScanToken();

			// Make sure there is actually a pair there
			if ( paramValue == TXT("}") )
			{
				break;
			}

			outConfig[ gameName ][ paramName ] = paramValue;
		}

		// Make sure a closing brace is following
		if ( parser.ScanToken() != TXT("}") )
		{
			WARN_GAME( TXT("Game configuration parsing error: missing closing brace after the game %ls's params"), gameName.AsChar() );
		}

		parser.SkipWhitespace();
	}

	return true;
}

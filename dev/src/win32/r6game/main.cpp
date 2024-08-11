/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/gameConfiguration.h"
#include "gameEngine.h"
#include "r6MemoryWindows.h"
#include "../../common/core/tokenizer.h"

void SInitializeVersionControl()
{
}

void RegisterGameClasses()
{
	extern void RegisterR6GameClasses();
	RegisterR6GameClasses();
}

void RegisterGameNames()
{
	extern void RegisterR6Names();
	RegisterR6Names();
}

CGame* CreateGame()
{
	return CreateR6Game();
}

INT WINAPI wWinMain( HINSTANCE instance, HINSTANCE prevInstance, wchar_t* cmdLine, INT cmdShow )
{
#if defined(W2_PLATFORM_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// handle command line options that have to be handled asap
	CTokenizer tok( String( cmdLine ), TXT(" ") ); 
	for ( Uint32 i=0; i<tok.GetNumTokens(); ++i ) 
	{	
		String token = tok.GetToken( i );
		if ( token == TXT( "noassert" ) || token == TXT( "-noassert" ) ) // Disable assertions
		{
			RED_LOG( CommandLine, TXT("GameEngine: Assertions has been DISABLED") );

			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
		}else if ( token == TXT( "silentassert" ) || token == TXT( "-silentassert" ) ) // Enable silent assertions
		{
			RED_LOG( CommandLine, TXT("GameEngine: Silent assertions enabled") );

			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Continue, true );
		}			
		else if ( token == TXT( "silentcrash" ) || token == TXT( "-silentcrash" ) ) // Enable silent crashes
		{
			RED_LOG( CommandLine, TXT("GameEngine: Silent crash mode") );

			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_SilentCrashHook, true );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_Break, false );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_PopupHook, false );
		}
		else if ( token == TXT( "/revoke" ) ) 
		{
			return 0;
		}
	}

	// Set game configuration
	SGameConfigurationParameter param = 
	{
		TXT( "r6" ),
		TXT( "r6data" ),
		TXT( "Cyberpunk 2077" ),
		TXT( "bundles" ),
		TXT( "r6data\\scripts" ),
		TXT( "bin\\r6config" ),
		TXT( "CR6Game" ),
		TXT( "CR6Player" ),
		TXT( "CR6TelemetryScriptProxy" ),
		TXT( "CR6CameraDirector" )
	};

	GGameConfig::GetInstance().Initialize( param );

	// Initial stats
#ifndef RED_FINAL_BUILD
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "InitialStats" );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "InitialStats" );
#endif

	GIsGame = true;

	// Initialise the memory pools
	R6MemoryParameters memoryPoolParameters;
	CoreMemory::Initialise( &memoryPoolParameters );

	// Pass pool parameters to gpu memory system
	auto poolParameterFn = []( Red::MemoryFramework::PoolLabel l, const Red::MemoryFramework::IAllocatorCreationParameters* p ) 
	{ 
		GpuApi::SetPoolParameters( l, p ); 
	};
	R6GpuMemoryParameters gpuMemoryPoolParameters;
	gpuMemoryPoolParameters.ForEachPoolParameter( poolParameterFn );

	// Initialize platform
	SInitializePlatform( cmdLine );
#ifndef RED_FINAL_BUILD
	RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "PlatformInit" );
	RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "PlatformInit" );
#endif

	GEngine = new CGameEngine;

	if ( GEngine->Initialize() )
	{
		// Post engine initialization stats
#ifndef RED_FINAL_BUILD
		RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "PostEngineInit" );
		RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "PostEngineInit" );
#endif

		// Start main loop
		GEngine->MainLoop();
		GEngine->Shutdown();
	}

	// Shutdown engine platform
	SShutdownPlatform();

	// If GEngine is the GameEngine, the return value will always be 0
	Int32 returnValue = GEngine->GetReturnValue();

	// Delete engine
	delete GEngine;
	GEngine = NULL;

	return returnValue;
}

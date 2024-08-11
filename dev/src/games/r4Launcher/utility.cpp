/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "utility.h"

#include "../../common/core/stringLocale.h"
#include "../../common/core/tokenizer.h"
#include "../../common/redSystem/error.h"
#include "../../common/core/gameConfiguration.h"

CommandLineArguments::CommandLineArguments()
	: silentScripts( false )
{
	GetPlatformDefaultResolution( engineParameters.m_desktopWidth, engineParameters.m_desktopHeight );
}

void ExtractCommandLineArguments( const String & commandLine, CommandLineArguments & result )
{
	RED_LOG( RED_LOG_CHANNEL( Core ), TXT("Command Line: %") RED_PRIWs, commandLine.AsChar() );

	CTokenizer tokens( commandLine, TXT(" ") ); 
	for ( Uint32 index = 0, end = tokens.GetNumTokens(); index != end; ++index ) 
	{
		String token = tokens.GetToken( index );
		if ( token == TXT( "noassert" ) || token == TXT( "-noassert" ) ) // Disable assertions
		{
			RED_LOG( CommandLine, TXT("GameEngine: Assertions has been DISABLED") );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
		}
		else if ( token == TXT( "silentassert" ) || token == TXT( "-silentassert" ) ) // Enable silent assertions
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
		else if ( token == TXT( "silentscripts" ) || token == TXT( "-silentscripts" ) ) // Disable scripts compilation failure popup.
		{
			RED_LOG( CommandLine, TXT("GameEngine: Silent scripts mode") );
			result.silentScripts = true;
		}
		else if ( token == TXT("-texres") )
		{
			FromString< Int32 >( tokens.GetToken( index + 1 ), result.engineParameters.m_textureResolutionClamp );
			result.engineParameters.m_textureResolutionClamp = ::Clamp< Int32 >( result.engineParameters.m_textureResolutionClamp, 0, 4 );
		}
#ifndef RED_PLATFORM_CONSOLE
		else if ( token == TXT( "-width" ) )
		{
			FromString< Int32 >( tokens.GetToken( index + 1 ), result.engineParameters.m_desiredWidth );
		}
		else if ( token == TXT( "-height" ) )
		{
			FromString< Int32 >( tokens.GetToken( index + 1 ), result.engineParameters.m_desiredHeight );
		}
		else if ( token == TXT( "-window" ) || token == TXT( "-windowed" ) )
		{
			result.engineParameters.m_forceWindowed = true;
		}
		else if ( token == TXT( "-borderless" ) || token == TXT("-noborder") )
		{
			result.engineParameters.m_borderlessMode = true;
		}
#endif
		else if ( token == TXT( "nolog" ) || token == TXT( "-nolog" ) )
		{
			result.engineParameters.m_enableLogging = false;
		}
		else if( token == TXT( "logpriority" ) || token == TXT( "-logpriority" ) )
		{
			Int32 level = 0;
			FromString< Int32 >( tokens.GetToken( index + 1 ), level );
			Red::System::Log::EPriority priority = level < Red::System::Log::P_Count ? static_cast< Red::System::Log::EPriority >( level ) : Red::System::Log::P_Information;
			Red::System::Log::Manager::GetInstance().SetPriorityVisible( priority );
		}
		else if ( token == TXT( "-script" ) )
		{
			result.engineParameters.m_scriptToExecuteOnStartup = tokens.GetToken( index + 1 );
		}
		else if ( token == TXT( "-vdb" ) )
		{
			result.engineParameters.m_enablePhysicsDebugger = true;
		}
#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
		else if ( token.EndsWith( TXT(".w2w") ) || token.EndsWith( TXT(".redgame") ) )
		{
			result.engineParameters.m_worldToRun = token;
		}
#endif	
		else if ( token.EndsWith( TXT("-kinect") ) )
		{
			result.engineParameters.m_enableKinect = true;
		}
		else if ( token.EndsWith( TXT("-displayfps") ) )
		{
			result.engineParameters.m_enableFPSDisplay = true;
		}
	}

	result.engineParameters.m_resolutionOverride = ( result.engineParameters.m_desiredWidth != -1 && result.engineParameters.m_desiredHeight != -1 );
}

//**
//* Copyright © 2014 CD Projekt Red. All Rights Reserved.
//*/

#include "build.h"
#include "utility.h"

void ExtractCommandLineArguments( const Char* commandLine, CommandLineArguments & result )
{
	RED_LOG( RED_LOG_CHANNEL( Core ), TXT("Command Line: %") RED_PRIWs, commandLine );

	CTokenizer tokens( commandLine, TXT(" ") );
	for ( Uint32 index = 0, end = tokens.GetNumTokens(); index != end; ++index ) 
	{	
		String token = tokens.GetToken( index );
		if ( token == TXT( "noassert" ) || token == TXT( "-noassert" ) ) // Disable assertions
		{
			RED_LOG( CommandLine, TXT("Unit tests: Assertions has been DISABLED") );
			Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
		}
		else if ( token == TXT("-window") || token == TXT( "-windowed" ) )
		{
			result.m_params.m_windowed = true;
		}
		else if ( token == TXT( "width" ) )
		{
			FromString< Int32 >( tokens.GetToken( index + 1 ), result.m_params.m_width);
		}
		else if ( token == TXT( "height" ) )
		{
			FromString< Int32 >( tokens.GetToken( index + 1 ), result.m_params.m_height );
		}
		else if( token == TXT("saveref") )
		{
			result.m_params.m_saveReferences = true;
		}
		else if( token ==TXT( "error" ))
		{
			FromString< Float >( tokens.GetToken( index + 1 ), result.m_params.m_marginOfError);
		}
		else if( token == TXT( "test" ))
		{
			result.m_params.m_test = tokens.GetToken( index + 1 );
		}
	}
}
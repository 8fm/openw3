/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "commandLineParser.h"

CCommandLineParser::CCommandLineParser( Uint32 argc, const AnsiChar* argv[] )
{
	// Figure out how long the command line string is
	size_t totalLength = 0;
	for( Uint32 i = 0; i < argc; ++i )
	{
		// Space for the token
		totalLength += Red::System::StringLength( argv[ i ] );
		
		// Space for a dividing space
		++totalLength;
	}

	// Reserve enough memory so we only make one allocation
	m_commandLine.Reserve( totalLength + sizeof( Char ) );

	// Combine the strings
	for( Uint32 i = 0; i < argc; ++i )
	{
		size_t size = Red::System::StringLength( argv[ i ] ) + 1;

		Char* convertedArg = static_cast< Char* >( RED_ALLOCA( size * sizeof( Char ) ) );

		Red::System::StringConvert( convertedArg, argv[ i ], size );

		m_commandLine += convertedArg;
		m_commandLine += TXT( " " );
	}

	Deconstruct();
}

CCommandLineParser::CCommandLineParser( const Char* commandLine )
:	m_commandLine( commandLine )
{
	Deconstruct();
}

CCommandLineParser::CCommandLineParser( const String& commandLine )
:	m_commandLine( commandLine )
{
	Deconstruct();
}

CCommandLineParser::~CCommandLineParser()
{

}

void CCommandLineParser::Deconstruct()
{
	Uint32 index = 0;
	Uint32 commandLineLength = m_commandLine.GetLength();

	EState state = State_Analyze;

	Params* currentOptionParameters = nullptr;

	while( index < commandLineLength )
	{
		switch( state )
		{
		case State_Analyze:
			state = Analyze( index, currentOptionParameters );
			break;

		case State_SkipWhitespace:
			state = SkipWhitespace( index );
			break;

		case State_ReadOption:
			state = ReadOption( index, currentOptionParameters );
			break;

		case State_ReadQuotedParam:
			state = ReadQuotedParam( index, currentOptionParameters );
			break;

		case State_ReadParam:
			state = ReadParam( index, currentOptionParameters );
			break;

		case State_UnknownParam:
			state = SkipUnknownParam( index );
		}
	}
}

CCommandLineParser::EState CCommandLineParser::Analyze( Uint32& index, Params* params )
{
	const Char c = m_commandLine[ index ];

	if( IsWhitespace( c ) )
	{
		return State_SkipWhitespace;
	}
	else if( IsOptionStart( c ) )
	{
		// Skip past the '-'
		++index;

		return State_ReadOption;
	}
	else if( params && IsParamsStart( c ) )
	{
		// Skip past the '='
		++index;

		return State_ReadParam;
	}
	else if( params && IsQuotedParamStart( c ) )
	{
		return State_ReadQuotedParam;
	}
	else if( params )
	{
		return State_ReadParam;
	}
	else
	{
		return State_UnknownParam;
	}
}

CCommandLineParser::EState CCommandLineParser::SkipWhitespace( Uint32& index )
{
	while( IsWhitespace( m_commandLine[ index ] ) )
	{
		++index;
	}

	return State_Analyze;
}

CCommandLineParser::EState CCommandLineParser::ReadOption( Uint32& index, Params*& params )
{
	RED_FATAL_ASSERT( index > 0, "Invalid command line processing state" );
	RED_FATAL_ASSERT( IsOptionStart( m_commandLine[ index - 1 ] ), "Current character is not a valid option indicator" );

	String token = ReadToken( index, [ this ]( Char c ) { return !IsValidOptionChar( c ); } );

	if( token.GetLength() > 0 )
	{
		params = &( m_options[ token ] );
	}

	return State_Analyze;
}

CCommandLineParser::EState CCommandLineParser::ReadQuotedParam( Uint32& index, Params* params )
{
	RED_FATAL_ASSERT( index > 0, "Invalid command line processing state" );
	RED_FATAL_ASSERT( IsQuotedParamStart( m_commandLine[ index ] ), "Current character is not a valid quoted param indicator" );

	// Skip past the opening '"'
	++index;

	String token = ReadToken( index, [ this ]( Char c ) { return IsQuotedParamStart( c ); } );

	if( token.GetLength() > 0 )
	{
		params->PushBack( std::move( token ) );
	}

	// Skip past the closing '"'
	RED_FATAL_ASSERT( IsQuotedParamStart( m_commandLine[ index ] ), "Current character is not a valid quoted param indicator" );
	++index;

	return State_Analyze;
}

CCommandLineParser::EState CCommandLineParser::ReadParam( Uint32& index, Params* params )
{
	SkipWhitespace( index );

	if( IsQuotedParamStart( m_commandLine[ index ] ) )
	{
		return ReadQuotedParam( index, params );
	}

	String token = ReadToken( index, [ this ]( Char c ) { return IsWhitespace( c ); } );

	if( token.GetLength() > 0 )
	{
		params->PushBack( std::move( token ) );
	}

	return State_Analyze;
}

CCommandLineParser::EState CCommandLineParser::SkipUnknownParam( Uint32& index )
{
	Uint32 commandLineLength = m_commandLine.GetLength();
	while( index < commandLineLength && !IsWhitespace( m_commandLine[ index ] ) )
	{
		++index;
	}

	return State_Analyze;
}

String CCommandLineParser::ReadToken( Uint32& index, std::function< Bool ( Char ) > isTokenTerminator )
{
	String token;
	token.Reserve( 32 );

	Uint32 commandLineLength = m_commandLine.GetLength();
	while( index < commandLineLength && !isTokenTerminator( m_commandLine[ index ] ) )
	{
		token += m_commandLine[ index ];

		++index;
	}

	return token;
}

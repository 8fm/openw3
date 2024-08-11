/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __RED_COMMAND_LINE_PARSER_H__
#define __RED_COMMAND_LINE_PARSER_H__

#include <functional>
#include "../core/hashmap.h"
#include "../core/stringConversion.h"

class CCommandLineParser
{
public:
	typedef TDynArray< String > Params;

public:
	explicit CCommandLineParser( Uint32 argc, const AnsiChar* argv[] );
	explicit CCommandLineParser( const Char* commandLine );
	explicit CCommandLineParser( const String& commandLine );
	~CCommandLineParser();

	// Specify the option without the preceeding dash or slash
	RED_INLINE Bool HasOption( const String& option ) const;

	// Number of options specified on the command line
	RED_INLINE Uint32 GetNumberOfOptions() const;

	// Get all values for a particular option
	// Will assert if that option does not exist
	RED_INLINE const Params& GetValues( const String& options ) const;

	// Only for use in an emergency! Don't grab the raw command line without a good reason
	RED_INLINE const String& GetRawCommandLine() const;

	// Utility function, for ease of use
	template< typename T >
	RED_INLINE Bool GetFirstParam( const String& option, T& param ) const;

private:
	enum EState
	{
		State_Analyze = 0,
		State_SkipWhitespace,
		State_ReadOption,
		State_ReadQuotedParam,
		State_ReadParam,
		State_UnknownParam,
	};

	void Deconstruct();

	RED_INLINE Bool IsWhitespace( Char c ) const;
	RED_INLINE Bool IsOptionStart( Char c ) const;
	RED_INLINE Bool IsParamsStart( Char c ) const;
	RED_INLINE Bool IsQuotedParamStart( Char c ) const;
	RED_INLINE Bool IsValidOptionChar( Char c ) const;
	RED_INLINE Bool IsValidParamChar( Char c ) const;

	// State machine
	EState Analyze( Uint32& index, Params* params );
	EState SkipWhitespace( Uint32& index );
	EState ReadOption( Uint32& index, Params*& params );
	EState ReadQuotedParam( Uint32& index, Params* params );
	EState ReadParam( Uint32& index, Params* params );
	EState SkipUnknownParam( Uint32& index );

	String ReadToken( Uint32& index, std::function< Bool ( Char ) > isTokenTerminator );

private:
	String m_commandLine;
	THashMap< String, Params > m_options;
};

//////////////////////////////////////////////////////////////////////////

RED_INLINE Bool CCommandLineParser::HasOption( const String& option ) const
{
	return m_options.KeyExist( option );
}

RED_INLINE Uint32 CCommandLineParser::GetNumberOfOptions() const
{
	return m_options.Size();
}

RED_INLINE const CCommandLineParser::Params& CCommandLineParser::GetValues( const String& options ) const
{
	return m_options[ options ];
}

RED_INLINE const String& CCommandLineParser::GetRawCommandLine() const
{
	return m_commandLine;
}

//////////////////////////////////////////////////////////////////////////

template< typename T >
RED_INLINE Bool CCommandLineParser::GetFirstParam( const String& option, T& param ) const
{
	const Params* params = m_options.FindPtr( option );

	if( params && params->Size() > 0 )
	{
		if( FromString( ( *params )[ 0 ], param ) )
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

RED_INLINE Bool CCommandLineParser::IsWhitespace( Char c ) const
{
	switch( c )
	{
	case TXT( ' ' ):
	case TXT( '\t' ):
	case TXT( '\r' ):
	case TXT( '\n' ):
		return true;
	}

	return false;
}

RED_INLINE Bool CCommandLineParser::IsOptionStart( Char c ) const
{
	switch( c )
	{
	case TXT( '-' ):
	case TXT( '/' ):
		return true;
	}

	return false;
}

RED_INLINE Bool CCommandLineParser::IsParamsStart( Char c ) const
{
	switch( c )
	{
	case TXT( '=' ):
	case TXT( ':' ):
		return true;
	}

	return false;
}

RED_INLINE Bool CCommandLineParser::IsQuotedParamStart( Char c ) const
{
	return c == TXT( '\"' );
}

RED_INLINE Bool CCommandLineParser::IsValidOptionChar( Char c ) const
{
	return
	(
		( ( c >= TXT( 'A' ) ) && ( c <= TXT( 'Z' ) ) ) ||
		( ( c >= TXT( 'a' ) ) && ( c <= TXT( 'z' ) ) ) ||
		( ( c >= TXT( '0' ) ) && ( c <= TXT( '9' ) ) ) ||
		( ( c == TXT( '-' ) ) || ( c == TXT( '_' ) ) )
	);
}

RED_INLINE Bool CCommandLineParser::IsValidParamChar( Char c ) const
{
	return
	(
		IsValidOptionChar( c ) ||
		( ( c == TXT( '.' ) ) || ( c == TXT( '\\' ) ) ) ||
		( ( c == TXT( '/' ) ) || ( c == TXT( ':' ) ) )
	);
}

#endif // __RED_COMMAND_LINE_PARSER_H__

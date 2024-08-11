/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* DrUiD
*/

#pragma once


//////////////////////////////////////////////////////////////////////////
// headers
#include "../core/tokenizer.h"


//////////////////////////////////////////////////////////////////////////
// helpers
class CDebugServerHelpers
{
public:
	//////////////////////////////////////////////////////////////////////////
	//
	// get uint64 uid from string
	static RED_INLINE Uint64 GetUidFromString( const String& uidStr )
	{
		// get entity uid
		CTokenizer tokens( uidStr, TXT(":") );
		if ( tokens.GetNumTokens() != 2 )
			return 0;

		Uint32 uidA = 0, uidB = 0;
		FromString( tokens.GetToken(0), uidA );
		FromString( tokens.GetToken(1), uidB );
		return uidA|((Uint64)uidB<<32);
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// get uint64 uid as string
	static RED_INLINE String GetStringFromUid( const Uint64 uid )
	{
		return String::Printf( TXT( "%u:%u" ), (Uint32)(uid&0xFFFFFFFF), (Uint32)((uid>>32)&0xFFFFFFFF) );
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// convert polish unicode > ansi chars
	static RED_INLINE void ConvertPolishToAnsi( String& polishString )
	{
		static Char polishChars[] =
		{
			TXT( '\u0104' ), TXT( '\u0106' ), TXT( '\u0118' ), TXT( '\u0141' ), TXT( '\u0143' ), TXT( '\u00D3' ), TXT( '\u015A' ), TXT( '\u0179' ), TXT( '\u017B' ),
			TXT( '\u0105' ), TXT( '\u0107' ), TXT( '\u0119' ), TXT( '\u0142' ), TXT( '\u0144' ), TXT( '\u00F3' ), TXT( '\u015B' ), TXT( '\u017A' ), TXT( '\u017C' )
		};
		static Char ansiChars[] =
		{
			TXT( 'A' ), TXT( 'C' ), TXT( 'E' ), TXT( 'L' ), TXT( 'N' ), TXT( 'O' ), TXT( 'S' ), TXT( 'Z' ), TXT( 'Z' ),
			TXT( 'a' ), TXT( 'c' ), TXT( 'e' ), TXT( 'l' ), TXT( 'n' ), TXT( 'o' ), TXT( 's' ), TXT( 'z' ), TXT( 'z' )
		};
		for ( int no = 0; no < 18; ++no )
		{
			polishString.ReplaceAll( polishChars[no], ansiChars[no] );
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//
	// convert array of cnames into one string
	static String GetStringFromArrayOfCNames( const TDynArray<CName>& names )
	{
		String result;

		for ( TDynArray< CName >::const_iterator name = names.Begin(); name != names.End(); )
		{
			result += name->AsString();
			if ( ++name != names.End() )
			{
				result += TXT(", ");
			}
		}

		return result;
	}
};


//////////////////////////////////////////////////////////////////////////
// EOF

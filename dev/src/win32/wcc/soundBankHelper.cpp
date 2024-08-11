/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "soundBankHelper.h"

namespace
{
	const String TOKEN_STREAMED_AUDIO = TXT("Streamed Audio");
	const String TOKEN_END_OF_HEADERS = TXT("Notes");
	const Uint32 ENTRY_COLUMN_COUNT = 5;
}

Bool CWWiseDescriptionParser::GetStreamedAudioEntries(const String& content, TDynArray<CWWiseStreamedAudioEntry>& output)
{
	TDynArray<String> allTokens;
	TDynArray<String> newLineTokenList = content.Split( TXT("\n"), true );

	for( auto& newLineToken : newLineTokenList )
	{
		TDynArray<String> tokens = newLineToken.Split( TXT("\t"), true );
		allTokens.PushBack( tokens );
	}

	Uint32 startingToken = 0;

	// Search for "Streamed Audio" text
	while( startingToken < allTokens.Size() && allTokens[startingToken] != TOKEN_STREAMED_AUDIO )
		++startingToken;

	if( startingToken >= allTokens.Size() )
		return false;

	// Search for "Notes" text
	while( startingToken < allTokens.Size() && allTokens[startingToken] != TOKEN_END_OF_HEADERS )
		++startingToken;

	if( startingToken >= allTokens.Size() )
		return false;

	// Get all entries
	startingToken += 1;
	for( Uint32 i=startingToken; i+4<allTokens.Size(); i+=ENTRY_COLUMN_COUNT )
	{
		output.PushBack( CWWiseStreamedAudioEntry() );
		output[ output.Size()-1 ].id = allTokens[i];
		output[ output.Size()-1 ].name = allTokens[i+1];
		output[ output.Size()-1 ].audioSourceFile = allTokens[i+2];
		output[ output.Size()-1 ].generatedAudioFile = allTokens[i+3];
		output[ output.Size()-1 ].wWiseObjectPath = allTokens[i+4];
	}

	return true;
}

/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "tokenStream.h"

SSTokenStream::SSTokenStream( SSNewFileLexer& lexer )
:	m_lexer( lexer )
,	m_readPosition( 0 )
{

}

SSTokenStream::~SSTokenStream()
{

}

bool SSTokenStream::SkipFunctionBody()
{
	// Bison will already be one or more tokens ahead, which is what will have triggered the new state
	// So we need to reverse back to the beginning of the scope
	const vector< SSToken >& tokens = m_lexer.GetTokens();

	while( tokens[ m_readPosition ].m_token != TXT( '{' ) )
	{
		--m_readPosition;
	}

	int targetLevel = tokens[ m_readPosition ].m_level - 1;
	while( tokens[ m_readPosition ].m_level > targetLevel )
	{
		++m_readPosition;
	}

	RED_ASSERT( tokens[ m_readPosition ].m_text == TXT( "}" ) && tokens[ m_readPosition ].m_level == targetLevel, TXT( "We have not correctly skipped to the end of the function body" ) );
	
	return true;
}

bool SSTokenStream::SkipFunctionPropertyInitialization()
{
	while( m_lexer.GetTokens()[ m_readPosition ].m_token != TXT( ';' ) )
	{
		++m_readPosition;
	}

	return true;
}

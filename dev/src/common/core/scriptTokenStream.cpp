/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "scriptTokenStream.h"

CScriptTokenStream::CScriptTokenStream()
:	m_readPos( 0 )
{
}

Bool CScriptTokenStream::PopToken( CScriptToken& token )
{
	// Get token
	if ( m_readPos < m_tokens.Size() )
	{
		token = m_tokens[ m_readPos ];
		++m_readPos;
		return true;
	}

	// No more tokens
	return false;
}

Bool CScriptTokenStream::ExtractFunctionTokens( CScriptTokenStream& stream )
{
	// Make sure we are not on the top
	if ( m_readPos == 0 )
	{
		return false;
	}

	// Step back unless we are just after the bracket
	if ( m_tokens[ m_readPos-1 ].m_token != '{' )
	{
		--m_readPos;
	}

	// Level counter
	Int32 currentLevel = 1;

	// Extract tokens
	for (;;)
	{
		// Pop token
		CScriptToken token;
		if ( !PopToken( token ) )
		{
			break;
		}

		// Exiting bracket
		if ( token.m_token == '}' )
		{
			--currentLevel;
		}

		// Entering bracket
		if ( token.m_token == '{' )
		{
			++currentLevel;
		}

		// End of block ?
		if ( token.m_token == '}' && currentLevel == 0 )
		{
			// We need to unparse the bracket
			--m_readPos;
			return true;
		}

		// Push token to output stream
		stream.PushToken( token );
	}

	// Error, not able to extract code from current block
	return false; 
}

Bool CScriptTokenStream::ExtractInitCode( CScriptTokenStream& stream )
{
	// Make sure we are not on the top
	if ( m_readPos == 0 )
	{
		return false;
	}

	// Step back unless we are just before the assignment '=' sign
	if ( m_tokens[ m_readPos-1 ].m_token != '=' )
	{
		--m_readPos;
	}

	// include '=' sign in tokens
	ASSERT( m_tokens[ m_readPos - 1 ].m_token == '=' );
	--m_readPos;

	// Extract tokens
	CScriptToken token;
	do
	{
		// Pop token
		if ( !PopToken( token ) )
		{
			// Error, not able to extract code from current block
			return false;
		}

		// Push token to output stream
		stream.PushToken( token );

	} while( token.m_token != ';' );

	// We need to unparse the semicolon
	--m_readPos;
	return true;
}

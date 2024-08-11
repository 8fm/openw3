/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_TOKEN_STREAM_H__
#define __SCRIPT_STUDIO_TOKEN_STREAM_H__

#include "token.h"
#include "../fileLexer.h"

class SSTokenStream
{
public:
	SSTokenStream( SSNewFileLexer& lexer );
	~SSTokenStream();

	inline void Reset() { m_readPosition = 0; }

	inline bool PopToken( SSToken& token )
	{
		// Get token
		if ( m_readPosition < m_lexer.GetTokens().size() )
		{
			// Grab token
			token = m_lexer.GetTokens()[ m_readPosition ];

			// Move to text token in the array
			++m_readPosition;

			return true;
		}

		// No more tokens
		return false;
	}

	bool SkipFunctionBody();
	bool SkipFunctionPropertyInitialization();

private:

	SSNewFileLexer& m_lexer;
	unsigned int m_readPosition;
};

#endif // __SCRIPT_STUDIO_TOKEN_STREAM_H__

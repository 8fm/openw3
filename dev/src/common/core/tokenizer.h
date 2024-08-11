/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _TOKENIZE_H_
#define _TOKENIZE_H_

#include "string.h"
#include "pair.h"
#include "dynarray.h"

class CTokenizer
{
protected:
	String						m_toTokenize;
	String						m_delimiters;

	typedef String::const_iterator	tStringIter;
	typedef TPair<tStringIter, tStringIter> tTokenDelims;
	TDynArray< tTokenDelims >	m_tokenized;

public:
	CTokenizer( const String &toTokenize, const String &delimiters );

public:
	Uint32 GetNumTokens() const;

	String GetToken( Uint32 i ) const;

protected:
	void Tokenize();

	Bool IsDelimiter( Char c ) const;
};


// CEnumeratingTokenizer - should be used only locally, as delimiters are not stored in the tokenizer, and there's a limit on size of the tokenized string
template<int _SIZE = 256 >
class CStaticTokenizer
{
protected:
	Char		m_toTokenize[ _SIZE ];
	const Char*	m_delimiters;
	Char*		m_currentToken;

public:
	CStaticTokenizer( const Char* toTokenize, const Char* delimiters )
	{
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( toTokenize ) );
		ASSERT( (length+3) < _SIZE );

		m_delimiters = delimiters;
		m_currentToken = NULL;

		Uint32 readIndex = 0;
		Uint32 writeIndex = 0;
		bool outsideToken = true;
		for ( ;; )
		{
			if( readIndex == length )
				break;
			Char c = toTokenize[ readIndex++ ];
			if( IsDelimiter( c ) == true )
			{
				if( outsideToken == false )
				{
					// was parsing token
					m_toTokenize[ writeIndex++ ] = 0;
					outsideToken = true;
				}
				continue;
			}
			outsideToken = false;
			if( m_currentToken == NULL )
				m_currentToken = &m_toTokenize[ writeIndex ];
			m_toTokenize[ writeIndex++ ] = c;
		}
		m_toTokenize[ writeIndex++ ] = 0;
		m_toTokenize[ writeIndex++ ] = 0; // 2nd zero for EndTokenizeString
	}

	Char* GetNextToken()
	{
		Char* result = m_currentToken;
		Uint32 length = static_cast< Uint32 >( Red::System::StringLength( result ) );
		if( length == 0 )
			return NULL;
		m_currentToken = &m_currentToken[ length + 1 ];
		return result;
	}

protected:

	Bool IsDelimiter( Char c ) const
	{
		// lets assume that there are no more than 1-2 delimiters (so no fancy search is required)
		for( int i = 0; m_delimiters[i] != 0; i++ )
		{
			if( m_delimiters[i] == c )
				return true;
		}
		return false;
	}
};


#endif /* _TOKENIZE_H_ */
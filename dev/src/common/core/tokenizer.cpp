/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "tokenizer.h"

CTokenizer::CTokenizer( const String &toTokenize, const String &delimiters ) 
	: m_toTokenize( toTokenize )
	, m_delimiters( delimiters )
{
	// sort tokens for faster lookup
	Sort( m_delimiters.Begin(), m_delimiters.End() );	

	Tokenize();
}

Uint32 CTokenizer::GetNumTokens() const
{
	return m_tokenized.Size();
}

String CTokenizer::GetToken( Uint32 i ) const
{
	if ( i >= m_tokenized.Size() )
	{
		return String::EMPTY;
	}

	const tTokenDelims &currToken = m_tokenized[ i ];
	Uint32 tokenBegin = PtrDiffToUint32( ((void*)( currToken.m_first - m_toTokenize.Begin() ) ) );
	Uint32 tokenLength = PtrDiffToUint32( ((void*)( currToken.m_second - currToken.m_first ) ) );

	return String( m_toTokenize.TypedData() + tokenBegin, tokenLength );
}

Bool CTokenizer::IsDelimiter( Char c ) const
{
	return BinarySearch( m_delimiters.Begin(), m_delimiters.End(), c );
}

void CTokenizer::Tokenize()
{
	// Do not tokenize empty strings
	if ( m_toTokenize.Empty() )
	{
		return;
	}

	tStringIter currBegin = m_toTokenize.Begin();
	tStringIter currEnd = m_toTokenize.Begin();
	tStringIter *currIter = &currBegin;

	Bool outsideToken = true;
	while( *currIter != m_toTokenize.End() )
	{
		if ( IsDelimiter( *(*currIter) ) ==  outsideToken )
		{
			++(*currIter);
			continue;
		}
		else
		{
			// entering/leaving token
			if ( outsideToken )
			{
				// entering token, switch current iterator
				currEnd = currBegin;
				currIter = &currEnd;			
			}
			else
			{
				// leaving token, emitt
				m_tokenized.PushBack( tTokenDelims( currBegin, currEnd ) );

				currBegin = currEnd;
				currIter = &currBegin;
			}
			
			outsideToken = !outsideToken;
		}
	}

	if ( !outsideToken )
	{
		// emitt last token
		m_tokenized.PushBack( tTokenDelims( currBegin, currEnd ) );
	}
}



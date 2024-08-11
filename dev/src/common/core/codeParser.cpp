/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "codeParser.h"

CCodeParser::CCodeParser()
{
	Reset();
}

CCodeParser::CCodeParser( const String& code )
{
	Reset();
	SetCode( code );
}

CCodeParser::CCodeParser( const CCodeParser& parser )
	: m_code( parser.m_code )
	, m_head( parser.m_head )
	, m_lastToken( parser.m_lastToken )
	, m_headStack( parser.m_headStack )
	, m_delimiters( parser.m_delimiters )
	, m_symbols( parser.m_symbols )
	, m_handleCComments( parser.m_handleCComments )
	, m_parseStrings( parser.m_parseStrings )
	, m_includeStringQuotes( parser.m_includeStringQuotes )
{
}

void CCodeParser::UpdateSymbols()
{
	m_symbols.Clear();
	for ( Uint32 i=0; i<m_delimiters.Size(); ++i )
	{
		m_symbols.PushBackUnique( m_delimiters[i] );
	}
	m_symbols.Remove( 0 );
}

void CCodeParser::Reset()
{
	m_code.Clear();
	m_head = 0;
	m_lastToken.Clear();
	m_headStack.Clear();
	m_delimiters.Clear();
	m_handleCComments = true;
	m_symbols.Clear();
	m_handleCComments = true;
	m_includeStringQuotes = true;
}

void CCodeParser::SetHandleCComments( Bool enable /* = true */ )
{
	m_handleCComments = enable;
}

void CCodeParser::SetParseStrings( Bool enable /* = true */ )
{
	m_parseStrings = enable;
}

void CCodeParser::SetIncludeStringQuotes( Bool enable /* = true */ )
{
	m_includeStringQuotes = enable;
}

void CCodeParser::Rewind()
{
	m_head = 0;
}

void CCodeParser::SkipCComment()
{
	if ( m_head < m_code.GetLength() - 1 )
	{
		// Single line comment
		if ( m_code[m_head] == TXT('/') && m_code[m_head + 1] == TXT('/') )
		{
			SkipToEndOfLine();
		}
		// Multiline comment
		else if ( m_code[m_head] == TXT('/') && m_code[m_head + 1] == TXT('*') )
		{
			m_head += 2;
			while ( m_head < m_code.GetLength() - 1 )
			{
				if ( m_code[m_head] == TXT('*') && m_code[m_head + 1] == TXT('/') )
				{
					m_head += 2;
					break;
				}
				m_head++;
			}
		}
	}
}

void CCodeParser::SkipWhitespace()
{
	while ( m_head < m_code.GetLength() )
	{
		if ( m_handleCComments && m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
			( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
		{
			SkipCComment();
		}
		else if ( !IsWhiteSpace( m_code[m_head] ) )
		{
			break;
		}
		else
		{
			m_head++;
		}
	}
}

void CCodeParser::SkipToEndOfLine( Bool skipEndOfLine /* = true */ )
{
	while ( m_head < m_code.GetLength() && !( m_code[m_head] == TXT('\n') || m_code[m_head] == TXT('\r')) )
	{
		m_head++;
	}

	if ( skipEndOfLine )
	{
		while ( m_head < m_code.GetLength() && ( m_code[m_head] == TXT('\n') || m_code[m_head] == TXT('\r') ) )
		{
			m_head++;
		}
	}
}

void CCodeParser::SkipToWhitespace( Bool skipWhitespace /* = false */ )
{
	while ( m_head < m_code.GetLength() && !IsWhiteSpace( m_code[m_head] ) )
	{
		if ( m_handleCComments && m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
			( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
		{
			SkipCComment();
		}
		else
		{
			m_head++;
		}
	}

	if ( skipWhitespace )
	{
		SkipWhitespace();
	}
}

void CCodeParser::SkipToCharacter( Char c )
{
	while ( m_head < m_code.GetLength() && m_code[m_head] != c )
	{
		if ( m_handleCComments && m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
			( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
		{
			SkipCComment();
		}
		else
		{
			m_head++;
		}
	}
}

void CCodeParser::SkipToString( const String& s )
{
	while ( m_head < m_code.GetLength() && s != m_code.MidString( m_head, s.GetLength() ) )
	{
		if ( m_handleCComments && m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
			( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
		{
			SkipCComment();
		}
		else
		{
			m_head++;
		}
	}
}

void CCodeParser::Skip( Int32 chars /* = 1 */ )
{
	m_head += chars;
	if ( m_head > static_cast< Uint32 >( m_code.GetLength() ) )
	{
		m_head = static_cast< Uint32 >( m_code.GetLength() );
	}
}

const String& CCodeParser::ScanToken()
{
	SkipWhitespace();

	m_lastToken.Clear();

	if ( m_head >= m_code.GetLength() )
	{
		return m_lastToken;
	}

	if ( m_parseStrings && ( m_code[m_head] == TXT('\'') || m_code[m_head] == TXT('"') ) )
	{
		Char quote = m_code[m_head];
		m_head++;

		if ( m_includeStringQuotes )
		{
			m_lastToken += quote;
		}

		while ( m_head < m_code.GetLength() )
		{
			if ( m_code[m_head] == quote )
			{
				m_head++;
				break;
			}
			
			if ( m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('\\') )
			{
				m_head += 2;
				m_lastToken += TXT('\\');
				m_lastToken += m_code[m_head - 1];
			}
			else
			{
				m_lastToken += m_code[m_head];
				m_head++;
			}
		}

		if ( m_includeStringQuotes )
		{
			m_lastToken += quote;
		}

		return m_lastToken;
	}

	if ( !m_delimiters.Empty() && m_symbols.Exist( m_code[m_head] ) )
	{
		for ( Uint32 i=0; i<m_delimiters.Size(); ++i )
		{
			const String& delim = m_delimiters[i];
			bool found = false;
			if ( delim == m_code.MidString( m_head, delim.GetLength() ) )
			{
				if ( !found || delim.GetLength() > m_lastToken.GetLength() )
				{
					m_lastToken = delim;
					found = true;
				}
			}
			if ( found )
			{
				m_head += static_cast< Uint32 >( m_lastToken.GetLength() );
				return m_lastToken;
			}
		}
	}

	while ( m_head < m_code.GetLength() )
	{
		if ( m_handleCComments && m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
			( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
		{
			SkipCComment();
		}
		else
		{
			if ( IsWhiteSpace( m_code[m_head] ) || m_symbols.Exist( m_code[m_head] ) )
			{
				break;
			}

			m_lastToken += m_code[m_head];
			m_head++;
		}
	}

	return m_lastToken;
}

String CCodeParser::PeekToken()
{
	String prevLastToken = m_lastToken, result;
	PushHead();
	result = ScanToken();
	PopHead();
	m_lastToken = prevLastToken;
	return result;
}

const String& CCodeParser::ScanCBlock()
{
	String result;
	SkipWhitespace();
	if ( m_head < m_code.GetLength() && m_code[m_head] == TXT('{') )
	{
		Uint32 curlyCounter = 0;
		Bool updateCounter = true;
		while ( m_head < m_code.GetLength() )
		{
			if ( m_code[m_head] == TXT('{') )
			{
				if ( updateCounter )
				{
					curlyCounter++;
				}
				result += TXT('{');
				m_head++;
			}
			else if ( m_code[m_head] == TXT('}') )
			{
				if ( updateCounter )
				{
					curlyCounter--;
				}
				result += TXT('}');
				m_head++;
				if ( curlyCounter == 0 )
				{
					break;
				}
			}
			else if ( m_code[m_head] == TXT('\'') || m_code[m_head] == TXT('"') )
			{
				Bool prevParseStrings = m_parseStrings;
				Bool prevIncludeQuotes = m_includeStringQuotes;
				m_parseStrings = m_includeStringQuotes = true;
				result += ScanToken();
				m_parseStrings = prevParseStrings;
				m_includeStringQuotes = prevIncludeQuotes;
			}
			else if ( m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('/') &&
				( m_code[m_head + 1] == TXT('/') || m_code[m_head + 1] == TXT('*') ) )
			{
				if ( m_code[m_head + 1] == TXT('/') )
				{
					while ( m_head < m_code.GetLength() && m_code[m_head] != TXT('\r') && m_code[m_head] != TXT('\n') )
					{
						result += m_code[m_head];
						m_head++;
					}
				}
				else
				{
					updateCounter = false;
					result += TXT('/');
					m_head++;
				}
			}
			else if ( m_head < m_code.GetLength() - 1 && m_code[m_head] == TXT('*') && m_code[m_head + 1] == TXT('/') )
			{
				updateCounter = true;
				result += TXT('*');
				m_head++;
			}
			else
			{
				result += m_code[m_head];
				m_head++;
			}
		}
	}
	m_lastToken = result;
	return m_lastToken;
}

String CCodeParser::PeekCBlock()
{
	String prevLastToken = m_lastToken, result;
	PushHead();
	result = ScanCBlock();
	PopHead();
	m_lastToken = prevLastToken;
	return result;
}

void CCodeParser::SetCode(const String& code )
{
	m_code = code;
	m_headStack.Clear();
	Rewind();
}

void CCodeParser::ClearDelimiters()
{
	m_delimiters.Clear();
}

void CCodeParser::AddDelimiters( const TDynArray<String>& delimiters )
{
	m_delimiters.PushBackUnique( delimiters );
	UpdateSymbols();
}

void CCodeParser::AddDelimiters( const String& delimitersString )
{
	Char tmp[2];
	tmp[1] = 0;
	for ( Uint32 i=0; i<delimitersString.Size(); ++i )
	{
		tmp[0] = delimitersString[i];
		m_delimiters.PushBackUnique( tmp );
	}
	UpdateSymbols();
}

void CCodeParser::AddDelimiter( const String& delimiter )
{
	m_delimiters.PushBackUnique( delimiter );
	UpdateSymbols();
}

void CCodeParser::AddStandardDelimiters()
{
	AddDelimiter( TXT("++") );
	AddDelimiter( TXT("--") );
	AddDelimiter( TXT("||") );
	AddDelimiter( TXT("&&") );
	AddDelimiter( TXT("==") );
	AddDelimiter( TXT("!=") );
	AddDelimiter( TXT("<=") );
	AddDelimiter( TXT(">=") );
	AddDelimiter( TXT("<<") );
	AddDelimiter( TXT(">>") );
	AddDelimiter( TXT("+=") );
	AddDelimiter( TXT("-=") );
	AddDelimiter( TXT("*=") );
	AddDelimiter( TXT("/=") );
	AddDelimiter( TXT("%=") );
	AddDelimiter( TXT("&=") );
	AddDelimiter( TXT("|=") );
	AddDelimiter( TXT("^=") );
	AddDelimiter( TXT("<<=") );
	AddDelimiter( TXT(">>=") );
	AddDelimiter( TXT("->") );
	AddDelimiter( TXT("->*") );
	AddDelimiter( TXT(".*") );
	AddDelimiter( TXT("::") );
	AddDelimiters( TXT(";{}!~%^&*()-=+[]:\\|'\",<.>/?@`") );
	UpdateSymbols();
}

void CCodeParser::PushHead()
{
	m_headStack.PushBack( m_head );
}

void CCodeParser::PopHead()
{
	m_head = m_headStack.PopBack();
}

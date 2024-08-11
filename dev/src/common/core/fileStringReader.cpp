/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "string.h"
#include "fileStringReader.h"

namespace Red
{

CAnsiStringFileReader::CAnsiStringFileReader()
	: m_buffer( nullptr )
	, m_end( nullptr )
	, m_pos( nullptr )
	, m_line( 1 )
{}

CAnsiStringFileReader::CAnsiStringFileReader( class IFile* file )
	: m_buffer( nullptr )
	, m_end( nullptr )
	, m_pos( nullptr )
	, m_line( 1 )
{
	if ( file )
	{
		static_assert( sizeof(AnsiChar) == 1, "AnsiChar size is not 1 byte ?!" );

		const Uint32 length = (const Uint32) file->GetSize();
		m_buffer = (AnsiChar*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, (length+1) );
		file->Seek( 0 );
		file->Serialize( m_buffer, length );
		m_buffer[ length ] = 0;

		m_end = m_buffer + length;
		m_pos = m_buffer;
	}
}

CAnsiStringFileReader::CAnsiStringFileReader( const StringAnsi& string )
	: m_buffer( nullptr )
	, m_end( nullptr )
	, m_pos( nullptr )
	, m_line( 1 )
{
	if ( !string.Empty() )
	{
		static_assert( sizeof(AnsiChar) == 1, "AnsiChar size is not 1 byte ?!" );

		const Uint32 length = string.GetLength();
		m_buffer = (AnsiChar*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, (length+1));
		Red::MemoryCopy( m_buffer, string.Data(), length );
		m_buffer[ length ] = 0;

		m_end = m_buffer + length;
		m_pos = m_buffer;
	}
}

CAnsiStringFileReader::~CAnsiStringFileReader()
{
	if ( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, m_buffer );
		m_buffer = nullptr;
	}

	m_end = nullptr;
	m_pos = nullptr;
}

Bool CAnsiStringFileReader::ParseKeyword( const AnsiChar* keyword, const Uint32 length /*= (Uint32)-1*/, const Bool allowLineBreak /*= true*/ )
{
	if ( !SkipWhitespaces( allowLineBreak ) )
		return false;

	const Uint32 compareLength = Min< Uint32 >( length, (const Uint32) Red::StringLength(keyword) );
	if ( 0 == Red::StringCompare( m_pos, keyword, compareLength ) )
	{
		m_pos += compareLength;
		return true;
	}

	return false;
}

Bool CAnsiStringFileReader::ParseNumber( StringAnsi& outNumber, const Bool allowLineBreak /*= true*/ )
{
	if ( !SkipWhitespaces( allowLineBreak ) )
		return false;

	// extract numerical value
	const AnsiChar* read = m_pos;
	if ( isdigit(*read) || *read == '-' )
	{
		// skip the sign
		if (*read == '-' )
			read+=1;

		// parse the numerical part
		Bool hasDot = false;
		while ( read < m_end )
		{
			// decimal point
			if ( *read == '.' )
			{
				if ( hasDot )
					break;
				hasDot = true;
				++read;
				continue;
			}

			// we only support digits
			if ( !isdigit( *read ) )
				break;

			// continue
			++read;
		}

		// optional 'f' ending
		if ( *read == 'f' )
			++read;

		// assemble final number
		RED_ASSERT( read > m_pos, TXT("Unexpected null token") );
		outNumber = StringAnsi( m_pos, read - m_pos );
		m_pos = read;

		return true;
	}

	// no number parsed
	return false;
}

Bool CAnsiStringFileReader::ParseIdent( StringAnsi& outIdent, const Bool allowLineBreak /*= true*/ )
{
	if ( !SkipWhitespaces( allowLineBreak ) )
		return false;

	// extract ident characters
	const AnsiChar* read = m_pos;
	if ( *read == '_' || isalnum(*read) )
	{
		// Grab normal text
		while ( (read < m_end) && ( *read == '_' || *read == '/' || isalnum(*read) ) )
		{
			read++;
		}

		// create output string
		RED_ASSERT( read > m_pos, TXT("Unexpected null token") );
		outIdent = StringAnsi( m_pos, read - m_pos );
		m_pos = read;

		return true;
	}

	// no valid identifier
	return false;
}

Bool CAnsiStringFileReader::ParseString( StringAnsi& outString, const Bool allowLineBreak /*= true*/ )
{
	if ( !SkipWhitespaces( allowLineBreak ) )
		return false;

	// extract from quotes
	const AnsiChar* read = m_pos;
	if ( *m_pos == '\'' || *m_pos == '\"' )
	{
		const AnsiChar delim = *read++;

		// scan until we find matching quote
		while ( read < m_end )
		{
			if ( *read == delim )
				break;

			++read;
		}

		// output only valid strings
		if ( *read == delim )
		{
			outString = StringAnsi( m_pos+1, (read-m_pos)-1 );
			m_pos = read+1; // move past the quote
			return true;
		}
	}
	else
	{
		// normal string - parse till we find a white space
		while ( read < m_end )
		{
			if ( *read <= ' ' )
				break;

			++read;
		}

		// output string
		RED_ASSERT( read > m_pos, TXT("Unexpected null token") );
		outString = StringAnsi( m_pos, (read-m_pos) );
		m_pos = read;
		return true;
	}

	// no valid string found
	return false;
}

Bool CAnsiStringFileReader::ParseToken( StringAnsi& outToken, const Bool allowLineBreak /*= true*/ )
{
	if ( !SkipWhitespaces( allowLineBreak ) )
		return false;

	// stuff in quotes is always parsed as string
	if ( *m_pos == '\'' || *m_pos == '\"' )
		return ParseString( outToken, allowLineBreak );

	// number ?
	if ( ParseNumber( outToken, allowLineBreak ) )
		return true;

	// try to parse as an ident
	if ( ParseIdent( outToken, allowLineBreak ) )
		return true;

	// parse as a single character token
	outToken = StringAnsi( m_pos, 1 );
	m_pos += 1;
	return true;	
}

void CAnsiStringFileReader::SkipCurrentLine()
{
	while ( m_pos < m_end )
	{
		if ( *m_pos++ == '\n' )
		{
			m_line += 1;
			break;
		}
	}
}

Uint32 CAnsiStringFileReader::GetLine() const
{
	return m_line;
}

Bool CAnsiStringFileReader::EndOfFile() const
{
	return (m_pos >= m_end);
}

Bool CAnsiStringFileReader::SkipWhitespaces( const Bool allowLineBreak )
{
	while ( (m_pos < m_end) && (*m_pos <= ' ') )
	{
		// count lines
		if ( *m_pos == '\n' )
		{
			if ( !allowLineBreak )
				return false;

			m_line += 1;
		}

		++m_pos;
	}

	return (m_pos != m_end);
}

} // Red
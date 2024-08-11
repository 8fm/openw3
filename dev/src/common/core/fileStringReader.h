/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace Red
{
	/// Helper class that allows you to easily parse the content of a ANSI string file
	class CAnsiStringFileReader
	{
	public:
		CAnsiStringFileReader();
		CAnsiStringFileReader( class IFile* file ); // read the file content
		CAnsiStringFileReader( const StringAnsi& string ); // copy from string
		~CAnsiStringFileReader();

		// parse matching keyword, optionally part of the keyword can be matched
		Bool ParseKeyword( const AnsiChar* keyword, const Uint32 length = (Uint32)-1, const Bool allowLineBreak = true );

		// parse a valid identifier (alphanumeric string, not in quotes)
		Bool ParseIdent( StringAnsi& outIdent, const Bool allowLineBreak = true );

		// parse a generic number string (supports +-, integer, float, decimal point and optional 'f')
		Bool ParseNumber( StringAnsi& outNumber, const Bool allowLineBreak = true );

		// parse string - will respect quotes
		Bool ParseString( StringAnsi& outString, const Bool allowLineBreak = true );

		// parse a token - token is a single character, identifier or a string
		Bool ParseToken( StringAnsi& outToken, const Bool allowLineBreak = true );

		// skip to the end of the current line
		void SkipCurrentLine();

		// get current line (for error reporting)
		Uint32 GetLine() const;

		// have we reached end of file ?
		Bool EndOfFile() const;

	private:
		AnsiChar*		m_buffer;
		
		const AnsiChar*	m_end;
		const AnsiChar*	m_pos;
		Uint32			m_line;		// current line

		Bool SkipWhitespaces( const Bool allowLineBreak );
	};

} // Red
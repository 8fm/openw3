/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "string.h"

class CCodeParser
{
protected:
	String m_code;
	Uint32 m_head;
	String m_lastToken;
	TDynArray<Int32> m_headStack;
	TDynArray<String> m_delimiters;
	String m_symbols;
	Bool m_handleCComments;
	Bool m_parseStrings;
	Bool m_includeStringQuotes;

	void UpdateSymbols();

public:
	CCodeParser();
	CCodeParser( const String& code );
	CCodeParser( const CCodeParser& parser );

	void Reset();

	void SetHandleCComments( Bool enable = true );
	RED_INLINE Bool GetHandleCComments() const { return m_handleCComments; };

	void SetParseStrings( Bool enable = true );
	RED_INLINE Bool GetParseStrings() const { return m_parseStrings; }
	void SetIncludeStringQuotes( Bool enable = true );
	RED_INLINE Bool GetIncludeStringQuotes() const { return m_includeStringQuotes; }

	RED_INLINE Bool HasMore() const { return m_head < m_code.GetLength(); }

	void Rewind();
	void SkipCComment();
	void SkipWhitespace();
	void SkipToEndOfLine( Bool skipEndOfLine = true );
	void SkipToWhitespace( Bool skipWhitespace = false );
	void SkipToCharacter( Char c );
	void SkipToString( const String& s );
	void Skip( Int32 chars = 1 );

	const String& ScanToken();
	String PeekToken();
	RED_INLINE const String& GetLastToken() const { return m_lastToken; }

	const String& ScanCBlock();
	String PeekCBlock();

	RED_INLINE Char GetNextCharacter() const { return m_head < m_code.GetLength() ? m_code[m_head] : 0; }
	RED_INLINE Int32 GetHead() const { return m_head; }
	void SetCode(const String& code );
	RED_INLINE const String& GetCode() const { return m_code; }

	void ClearDelimiters();
	void AddDelimiters( const TDynArray<String>& delimiters );
	void AddDelimiters( const String& delimitersString );
	void AddDelimiter( const String& delimiter );
	void AddStandardDelimiters();
	RED_INLINE const TDynArray<String>& GetDelimiters() const { return m_delimiters; }

	void PushHead();
	void PopHead();
};

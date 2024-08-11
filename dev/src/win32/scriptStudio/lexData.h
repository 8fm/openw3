#pragma once

struct SSBracket
{
	int m_position;
	int m_line;
	wchar_t m_type;
};

struct SSBracketPair
{
	SSBracket m_open;
	SSBracket m_close;
};

struct SSComment
{
	int m_startLine;
	int m_endLine;
	int m_firstSucceedingCodeLine;
	int m_startPosition;
	wstring m_text;
};

struct SSLexicalData
{
	vector< SSBracketPair > m_brackets; //!< All Brackets
	map< int, int > m_bracketPositionToIndex; //!< Mapped by read positions

	vector< SSComment > m_comments; //!< All Comments
	map< int, int > m_commentStartLineToIndex;
	map< int, int > m_commentSucceedingCodeLinetoIndex;
};

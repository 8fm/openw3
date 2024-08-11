/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fileParserType.h"
#include "fileLexer.h"
#include "lexData.h"
#include "../../common/redSystem/log.h"

SSNewFileLexer::SSNewFileLexer( const Red::Scripts::LexerDefinition& definition, SSLexicalData* lexData, const wstring& file )
:	Red::Scripts::Lexer( definition )
,	m_lexData( lexData )
,	m_file( file )
{

}

SSNewFileLexer::~SSNewFileLexer()
{

}

void SSNewFileLexer::OnEmitToken( Red::Scripts::BisonId type, const Red::System::Char* token )
{
	SSToken newToken;
	newToken.m_text = token;
	newToken.m_level = m_level;
	newToken.m_context.m_file = m_file;
	newToken.m_context.m_line = m_line;
	newToken.m_token = type;
	m_tokens.push_back( newToken );
}

void SSNewFileLexer::OnEmitError( const Red::System::Char* error )
{
	// Show error
	RED_LOG_ERROR( RED_LOG_CHANNEL( FileLexer ), wxT( "Error: %" ) RED_PRIWs, error );
}

void SSNewFileLexer::OnEmitBracket( const Red::Scripts::Lexer::Bracket& open, const Red::Scripts::Lexer::Bracket& close )
{
	if( m_lexData )
	{
		SSBracketPair bracketPair;

		bracketPair.m_open.m_position	= open.m_position;
		bracketPair.m_open.m_line		= open.m_line;
		bracketPair.m_open.m_type		= BRACKETS[ open.m_dir ][ open.m_type ];

		bracketPair.m_close.m_position	= close.m_position;
		bracketPair.m_close.m_line		= close.m_line;
		bracketPair.m_close.m_type		= BRACKETS[ close.m_dir ][ close.m_type ];

		// Store Bracket Pair
		int index = m_lexData->m_brackets.size();
		m_lexData->m_brackets.push_back( bracketPair );

		// Insert opening bracket
		m_lexData->m_bracketPositionToIndex[ bracketPair.m_open.m_position ] = index;

		// Insert closing bracket
		m_lexData->m_bracketPositionToIndex[ bracketPair.m_close.m_position ] = index;
	}
}

void SSNewFileLexer::OnEmitComment( const Comment& comment )
{
	if( m_lexData )
	{
		SSComment newComment;

		newComment.m_startLine					= comment.m_startLine;
		newComment.m_endLine					= comment.m_endLine;
		newComment.m_firstSucceedingCodeLine	= comment.m_firstSucceedingNonCommentLine;
		newComment.m_startPosition				= comment.m_startPosition;
		newComment.m_text						= comment.m_text;

		int index = m_lexData->m_comments.size();
		m_lexData->m_comments.push_back( newComment );

		m_lexData->m_commentStartLineToIndex[ newComment.m_startLine ]							= index;
		m_lexData->m_commentSucceedingCodeLinetoIndex[ newComment.m_firstSucceedingCodeLine ]	= index;
	}
}

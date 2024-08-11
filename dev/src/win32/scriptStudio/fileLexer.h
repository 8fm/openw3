/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_LEXER_H__
#define __SCRIPT_STUDIO_LEXER_H__

#include "../../common/scripts/Lexer/lexer.h"

#include "lexer/token.h"
struct SSLexicalData;

class SSNewFileLexer : public Red::Scripts::Lexer
{
public:
	SSNewFileLexer( const Red::Scripts::LexerDefinition& definition, SSLexicalData* lexData, const wstring& file );
	virtual ~SSNewFileLexer();

	inline const vector< SSToken >& GetTokens() const { return m_tokens; }

private:
	virtual void OnEmitToken( Red::Scripts::BisonId type, const Red::System::Char* token ) override final;
	virtual void OnEmitError( const Red::System::Char* error ) override final;
	virtual void OnEmitBracket( const Red::Scripts::Lexer::Bracket& open, const Red::Scripts::Lexer::Bracket& close ) override final;
	virtual void OnEmitComment( const Comment& comment ) override final;

private:
	vector< SSToken >	m_tokens;
	SSLexicalData*		m_lexData;
	wstring				m_file;
};

#endif // __SCRIPT_STUDIO_LEXER_H__

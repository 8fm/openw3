/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "scriptLexer.h"
#include "scriptFileContext.h"
#include "scriptTokenStream.h"
#include "scriptCompiler.h"

CScriptLexer::CScriptLexer( const Red::Scripts::LexerDefinition& definition, CScriptTokenStream& tokenStream, const String& file, CScriptCompiler* compiler )
:	Red::Scripts::Lexer( definition )
,	m_tokenStream( tokenStream )
,	m_file( file )
,	m_compiler( compiler )
{

}

CScriptLexer::~CScriptLexer()
{

}

void CScriptLexer::OnEmitToken( Red::Scripts::BisonId type, const Char* token )
{
	CScriptFileContext context( m_file, m_line );
	m_tokenStream.PushToken( token, type, m_level, context );
}

void CScriptLexer::OnEmitError( const Char* error )
{
	CScriptFileContext context( m_file, m_line );

	// Emit error
	if ( m_compiler )
	{
		m_compiler->ScriptError( context, error );
	}
	else
	{
		LOG_CORE( TXT( "[Parse]: %" ) RED_PRIWs TXT( ": %" ) RED_PRIWs, error );
	}
}

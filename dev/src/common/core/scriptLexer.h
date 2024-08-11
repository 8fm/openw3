/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __CORE_SCRIPT_LEXER_H__
#define __CORE_SCRIPT_LEXER_H__

#include "../scripts/Lexer/lexer.h"
#include "../scripts/Memory/allocatorProxy.h"
#include "string.h"

class CScriptTokenStream;
class CScriptCompiler;

class CScriptLexer : public Red::Scripts::Lexer
{
public:
	CScriptLexer( const Red::Scripts::LexerDefinition& definition, CScriptTokenStream& tokenStream, const String& file, CScriptCompiler* compiler );
	virtual ~CScriptLexer();

private:
	virtual void OnEmitToken( Red::Scripts::BisonId type, const Char* token ) final;
	virtual void OnEmitError( const Char* error ) final;

private:
	CScriptTokenStream& m_tokenStream;
	String m_file;
	CScriptCompiler* m_compiler;
};

#endif // __CORE_SCRIPT_LEXER_H__

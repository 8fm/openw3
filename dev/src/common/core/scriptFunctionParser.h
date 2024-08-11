/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CScriptSyntaxNode;
class CScriptCompiler;
struct CScriptFunctionStub;
class CScriptFileContext;

/// Script function parser
class CScriptFunctionParser
{
protected:
	CScriptCompiler*		m_compiler;			//!< Master compiler
	CScriptSyntaxNode*		m_rootSyntaxNode;	//!< Compiled root syntax node
	Bool					m_hasError;			//!< There were a parsing errors

public:
	CScriptFunctionParser( CScriptCompiler* compiler );
	~CScriptFunctionParser();

	//! Compile function
	CScriptSyntaxNode* ParseFunctionCode( CScriptFunctionStub* stub );

	//! Emit parsing error
	void EmitError( const CScriptFileContext& context, const String& text );

	//! Set root syntax node for compiled function
	void SetRootSyntaxNode( CScriptSyntaxNode* node );
};
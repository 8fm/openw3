/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CScriptSyntaxNode;
struct ScriptSyntaxNodeType;
class CClass;
class CScriptCompiler;
class CFunction;
struct CScriptSystemStub;
struct CScriptClassStub;
struct CScriptFunctionStub;
class CScriptFileContext;
class CScriptCodeNodeCompilationPool;

/// Function compiler
class CScriptFunctionCompiler
{
protected:
	CScriptCompiler*	m_compiler;			//!< Base compiler
	CClass*				m_class;			//!< Class being compiled
	CFunction*			m_function;			//!< Function being compiler
	CScriptSystemStub&	m_stubs;
	Bool				m_strictMode;		//!< Strict mode compilation

public:
	//! Get the class being compiled
	RED_INLINE CClass* GetCompiledClass() const { return m_class; }

	//! Get the function being compiled
	RED_INLINE CFunction* GetCompiledFunction() const { return m_function; }

	//! Is strict mode
	RED_INLINE Bool IsStrictMode() { return m_strictMode; }

public:
	CScriptFunctionCompiler( CScriptCompiler* compiler, CScriptSystemStub& stubs, Bool strictMode );
	~CScriptFunctionCompiler();

	//! Compile function
	Bool Compile( CScriptClassStub* classStub, CScriptFunctionStub* functionStub, CScriptCodeNodeCompilationPool& pool );

	//! Emit fatal parsing error message
	void EmitError( const CScriptFileContext& context, const String& text );

	//! Emit non-fatal error message
	void EmitWarning( const CScriptFileContext& context, const String& text );

	CScriptCompiler* GetScriptCompiler() { return m_compiler; }

	CScriptSystemStub& GetStubs() { return m_stubs; }
};

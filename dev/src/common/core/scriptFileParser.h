/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../scripts/Lexer/definition.h"
#include "../scripts/Memory/allocatorProxy.h"

class CScriptCompiler;
struct CScriptSystemStub;
struct CScriptClassStub;
struct CScriptStructStub;
struct CScriptEnumStub;
struct CScriptFunctionStub;
struct CScriptPropertyStub;
class CScriptFileContext;
class CScriptTokenStream;
class CScriptDefaultValue;

/// Script file parser
class CScriptFileParser
{
protected:
	CScriptCompiler*		m_compiler;								//!< Master compiler
	CScriptClassStub*		m_currentClass;							//!< Opened class
	CScriptStructStub*		m_currentStruct;						//!< Opened structure
	CScriptEnumStub*		m_currentEnum;							//!< Opened enum
	CScriptFunctionStub*	m_currentFunction;						//!< Opened function
	CScriptPropertyStub*	m_lastAddedFunctionLocalPropertyStub;	//!< Last added function local property stub
	CScriptSystemStub&		m_stubs;

	Red::Scripts::LexerDefinition m_definition;
	Red::AllocatorProxy m_allocProxy;

public:
	CScriptFileParser( CScriptSystemStub& outputStubs, CScriptCompiler* compiler );
	~CScriptFileParser();

	//! Parse the script file definitions
	Bool ParseFile( const String& scriptFilePath, const String& scriptShortFilePath );

	//! Emit parsing error
	void EmitError( const CScriptFileContext* context, const String& text );

public:
	//! Start class definition
	void StartClass( const CScriptFileContext* context, const String& name, const String& extents, Uint32 flags );

	//! Start state definition
	void StartState( const CScriptFileContext* context, const String& name, const String& parentClass, Uint32 flags, const String& extends );

	//! Start structure definition
	void StartStruct( const CScriptFileContext* context, const String& name, Uint32 flags );

	//! Start an enum definition
	void StartEnum( const CScriptFileContext* context, const String& name );

	//! Start function definition
	void StartFunction( const CScriptFileContext* context, const String& name, Uint32 flags );

	//! Function has no body
	void SetFunctionUndefined();

	//! Add class/struct/local function property
	void AddProperty( const CScriptFileContext* context, const String& name, Uint32 flags, const String& typeName, Bool isFunctionParameter );

	//! Add class auto bindable property declaration
	void AddBindableProperty( const CScriptFileContext* context, const String& name, Uint32 flags, const String& typeName, const String& binding );

	//! Sets function local property initialization code
	void SetLastFunctionPropertyInitCode( CScriptTokenStream& initCode );

	//! Resets function local property initialization code
	void ResetFunctionPropertyList();

	//! Add enum option
	void AddEmumOption( const CScriptFileContext* context, const String& name, Uint32 value );

	//! Add default value
	void AddDefaultValue( const CScriptFileContext* context, const String& name, CScriptDefaultValue* value );

	//! Add a hint
	void AddHint( const CScriptFileContext* context, const String& name, const String& hint );

	//! Add function code
	void AddCode( const CScriptTokenStream& tokens );

	//! Set function return value
	void SetReturnValueType( const String& retTypeName );

	//! Pop context
	void PopContext();

private:
	static void* Realloc( void* ptr, size_t size );
	static void Free( void* ptr );
};

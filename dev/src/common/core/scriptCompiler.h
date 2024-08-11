/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptFieldStubs.h"

class CScriptFileContext;

/// Script log/error interface
class IScriptLogInterface
{
public:
	virtual ~IScriptLogInterface() {};

	//! Just info
	virtual void Log( const String& text ) = 0;

	//! Compilation warning
	virtual void Warn( const CScriptFileContext& context, const String& text ) = 0;

	//! Compilation error
	virtual void Error( const CScriptFileContext& context, const String& text ) = 0;
};

/// Script compiler
class CScriptCompiler
{
protected:
	IScriptLogInterface*	m_output;			//!< Output interface
	Bool					m_hasError;			//!< Compilation error was emitted
	Bool					m_strictMode;		//!< Strict mode compilation

public:
	//! Get output interface
	RED_INLINE IScriptLogInterface* GetOutput() const { return m_output; }

	//! Is strict mode
	RED_INLINE Bool IsStrictMode() { return m_strictMode; }

public:
	CScriptCompiler( IScriptLogInterface* output );
	~CScriptCompiler();

public:
	//! Output log
	void ScriptLog( const Char* text, ... );

	//! Compilation warning
	void ScriptWarn( const CScriptFileContext& context, const Char* text, ... );

	//! Compilation error
	void ScriptError( const CScriptFileContext& context, const Char* text, ... );

public:
	//! Compile files from location
	Bool CompileFiles( const TDynArray< String >& scriptFiles, Bool fullContext );

	CScriptFunctionStub* FindFunctionStub( CScriptClassStub* parent, const String& name );

	CScriptFunctionStub* FindImplementedFunctionInHierarchy( CScriptSystemStub& definitions, CScriptClassStub* classStub, const String& function, CScriptClassStub* stopAtClassStub );

protected:
	//! Parse script file definition
	Bool ParseFileDefinition( const String& scriptFilePath );

	//! Create type definitions
	Bool CreateTypes( CScriptSystemStub& definitions );

	//! Create structure from stub
	Bool CreateStruct( CScriptStructStub* stub );

	//! Create enum from stub
	Bool CreateEnum( CScriptEnumStub* stub );

	//! Create classes from stub
	Bool CreateClass( CScriptClassStub* stub );

	//! Bind parent class to script classes
	Bool BindParentClass( CScriptSystemStub& definitions, CScriptClassStub* stub );

	//! Bind state classes to state machines
	Bool BindStateClass( CScriptClassStub* stub );

	//! Bind state classes to state machines
	Bool BindStateParentClass( CScriptClassStub* stub );

	//! Create functions in classes
	Bool CreateFunctions( CScriptSystemStub& definitions );

	//! Create global function
	Bool CreateGlobalFunction( CScriptFunctionStub* stub );

	//! Create class function
	Bool CreateClassFunction( CScriptClassStub* classStub, CScriptFunctionStub* stub );

	//! Create properties
	Bool CreateProperties( CScriptSystemStub& definitions );

	//! Create struct properties
	Bool CreateStructProperties( CScriptStructStub* stub );

	//! Create class properties
	Bool CreateClassProperties( CScriptClassStub* stub );

	//! Create function properties
	Bool CreateFunctionProperties( CScriptFunctionStub* stub );

	//! Bind super function
	Bool BindSuperFunctions( CScriptFunctionStub* stub );

	//! Bind functions
	Bool BindFunctions( CScriptSystemStub& definitions );

	//! Calculate data layout
	Bool BuildDataLayout( CScriptSystemStub& definitions );

	//! Create default values for classes and structures
	Bool CreateDefaultValues( CScriptSystemStub& definitions );
	
	//! Compile functions code
	Bool CompileFunctions( CScriptSystemStub& definitions );

protected:
	//! Create default value from script stub
	CDefaultValue* CreateDefaultValue( CProperty* prop, IRTTIType* rawType, const CScriptDefaultValue* valueStub );

	//! Create default value definition in given class
	Bool CreateDefaultValue( CClass* createdClass, CScriptDefaultValueStub* stub );

	//! Convert raw type to script type
	Bool ConvertRawType( IRTTIType* rawType, String& scriptTypeName );

	//! Check if function headers matches
	Bool MatchFunctionHeader( CScriptFunctionStub* baseContextStub, const CFunction* base, const CFunction* super );

	//! Check if native property type (from C++ code) is compatible with definition of the imported type in scripts
	Bool CheckTypeCompatibility( const IRTTIType* nativeType, const IRTTIType* importType ) const;

	//! Create property
	CProperty* CreateProperty( CScriptPropertyStub* stub, CClass* parentClass, Uint32 extraFlags );

	//! Signal the start of compilation
	void CompilationStarted();

	//! Signal the end of compilation
	void CompilationFinished();

	void CheckClassesForCompleteness( CScriptSystemStub& definitions );
};

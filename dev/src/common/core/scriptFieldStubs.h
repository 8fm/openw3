/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptFileContext.h"
#include "scriptToken.h"
#include "scriptTypeInfo.h"
#include "scriptTokenStream.h"

class CClass;
class CFunction;
class CProperty;
class CEnum;
class CScriptDefaultValue;
class CDefaultValue;

/// Generic flags
enum EScriptStubFlags
{
	SSF_Import			= FLAG( 0 ),
	SSF_Editable		= FLAG( 1 ),
	SSF_Const			= FLAG( 2 ),
	SSF_Timer			= FLAG( 3 ),
	SSF_Abstract		= FLAG( 4 ),
	SSF_Entry			= FLAG( 5 ),
	SSF_Auto			= FLAG( 6 ),
	SSF_Inlined			= FLAG( 7 ),
	SSF_Out				= FLAG( 8 ),
	SSF_Optional		= FLAG( 9 ),
	SSF_Final			= FLAG( 10 ),
	SSF_Private			= FLAG( 11 ),
	SSF_Protected		= FLAG( 12 ),
	SSF_Public			= FLAG( 13 ),
	SSF_Event			= FLAG( 14 ),
	SSF_Latent			= FLAG( 15 ),
	SSF_Exec			= FLAG( 16 ),
	SSF_Unused			= FLAG( 17 ),
	SSF_Scene			= FLAG( 18 ),
	SSF_Saved			= FLAG( 19 ),
	SSF_Quest			= FLAG( 20 ),
	SSF_Cleanup			= FLAG( 21 ),
	SSF_Reward			= FLAG( 22 ),
	SSF_StateMachine	= FLAG( 23 ),
};

/// Property stub
struct CScriptPropertyStub
{
	typedef TDynArray< CScriptToken, MC_ScriptCompilation, MemoryPool_ScriptCompilation > Tokens;

	CScriptFileContext	m_context;
	String				m_name;
	String				m_hint;
	Uint32				m_flags;
	String				m_typeName;
	CScriptTypeInfo		m_type;
	CProperty*			m_createdProperty;
	Tokens				m_initCodeTokens;
	String				m_binding;		// only bindable properties

	CScriptPropertyStub( const CScriptFileContext& context, const String& name, Uint32 flags, const String& typeName );
	~CScriptPropertyStub();

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Function stub
struct CScriptFunctionStub
{
	typedef TDynArray< CScriptPropertyStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > PropertyStubs;

	CScriptFileContext		m_context;
	String					m_name;
	Uint32					m_flags;
	CScriptPropertyStub*	m_retValue;
	PropertyStubs			m_params;
	PropertyStubs			m_locals;
	CScriptTokenStream		m_code;
	CFunction*				m_createdFunction;

	CScriptFunctionStub( const CScriptFileContext& context, const String& name, Uint32 flags );
	~CScriptFunctionStub();

	void AddParam( CScriptPropertyStub* param );
	void AddLocal( CScriptPropertyStub* local );
	void SetReturnType( const String& type );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Default value stub
struct CScriptDefaultValueStub
{
	CScriptFileContext		m_context;
	String					m_name;
	CScriptDefaultValue*	m_value;
	CDefaultValue*			m_createdValue;

	CScriptDefaultValueStub( const CScriptFileContext& context, const String& name, CScriptDefaultValue* value );
	~CScriptDefaultValueStub();

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Struct stub
struct CScriptStructStub
{
	typedef TDynArray< CScriptPropertyStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > PropertyStubs;
	typedef TDynArray< CScriptDefaultValueStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > DefaultValueStubs;

	CScriptFileContext	m_context;
	String				m_name;
	Uint32				m_flags;
	PropertyStubs		m_fields;
	DefaultValueStubs	m_defaultValues;
	CClass*				m_createdStruct;

	CScriptStructStub( const CScriptFileContext& context, const String& name, Uint32 flags );
	~CScriptStructStub();

	void AddField( CScriptPropertyStub* field );
	void AddDefaultValue( CScriptDefaultValueStub* value );
	void AddHint( const String& name, const String& hint );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Class stub
struct CScriptClassStub
{
	typedef TDynArray< CScriptPropertyStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > PropertyStubs;
	typedef TDynArray< CScriptFunctionStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > FunctionStubs;
	typedef TDynArray< CScriptDefaultValueStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > DefaultValueStubs;

	CScriptFileContext	m_context;
	String				m_name;
	String				m_extends;
	String				m_stateParentClass;
	Bool				m_isState;
	Uint32				m_flags;
	PropertyStubs		m_properties;
	FunctionStubs		m_functions;
	DefaultValueStubs	m_defaultValues;
	CClass*				m_createdClass;

	CScriptClassStub( const CScriptFileContext& context, const String& name, const String& extends, Uint32 flags );
	~CScriptClassStub();

	void AddProperty( CScriptPropertyStub* prop );
	void AddFunction( CScriptFunctionStub* func );
	void AddDefaultValue( CScriptDefaultValueStub* value );
	void AddHint( const String& name, const String& hint );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Enum option stub
struct CScriptEnumOptionStub
{
	CScriptFileContext		m_context;
	String					m_name;
	Int32					m_value;

	CScriptEnumOptionStub( const CScriptFileContext& context, const String& name, Int32 value );
	~CScriptEnumOptionStub();

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Enum stub
struct CScriptEnumStub
{
	typedef TDynArray< CScriptEnumOptionStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > EnumOptionStubs;

	CScriptFileContext	m_context;
	String				m_name;
	EnumOptionStubs		m_options;
	CEnum*				m_createdEnum;

	CScriptEnumStub( const CScriptFileContext& context, const String& name );
	~CScriptEnumStub();

	void AddOption( CScriptEnumOptionStub* option );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

/// Script system stub
struct CScriptSystemStub
{
	typedef TDynArray< CScriptClassStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > ClassStubs;
	typedef TDynArray< CScriptStructStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > StructStubs;
	typedef TDynArray< CScriptEnumStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > EnumStubs;
	typedef TDynArray< CScriptFunctionStub*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > FunctionStubs;

	ClassStubs		m_classes;		//!< Top level classes
	StructStubs		m_structs;		//!< Top level structures
	EnumStubs		m_enums;		//!< Top level enums
	FunctionStubs	m_functions;	//!< Top level functions

	CScriptSystemStub();
	~CScriptSystemStub();

	void AddClass( CScriptClassStub* classStub );
	void AddStruct( CScriptStructStub* structStub );
	void AddEnum( CScriptEnumStub* enumStub );
	void AddFunction( CScriptFunctionStub* functionStub );

	Uint32 CountTypes() const;
	Uint32 CountFunctions() const;

	CScriptClassStub* FindClassStub( const String& name );
	CClass* FindClass( const String& name );

	CScriptFunctionStub* FindFunctionStub( CScriptClassStub* parent, const String& name );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};

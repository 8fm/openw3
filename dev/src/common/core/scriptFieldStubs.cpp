/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptFieldStubs.h"
#include "scriptCompiler.h"
#include "property.h"
#include "scriptDefaultValue.h"

CScriptPropertyStub::CScriptPropertyStub( const CScriptFileContext& context, const String& name, Uint32 flags, const String& typeName )
	: m_context( context )
	, m_name( name )
	, m_flags( flags )
	, m_typeName( typeName )
	, m_createdProperty( nullptr )
{
}

CScriptPropertyStub::~CScriptPropertyStub()
{
}

CScriptFunctionStub::CScriptFunctionStub( const CScriptFileContext& context, const String& name, Uint32 flags )
	: m_context( context )
	, m_name( name )
	, m_flags( flags )
	, m_retValue( nullptr )
	, m_createdFunction( nullptr )
{
}

CScriptFunctionStub::~CScriptFunctionStub()
{
	if( m_retValue )
	{
		delete m_retValue;
	}

	m_params.ClearPtr();
	m_locals.ClearPtr();
}

void CScriptFunctionStub::SetReturnType( const String& retTypeName )
{
	if ( !retTypeName.Empty() )
	{
		m_retValue = new CScriptPropertyStub( m_context, TXT("__ret"), PF_FuncRetValue, retTypeName );
	}
	else
	{
		m_retValue = nullptr;
	}
}

void CScriptFunctionStub::AddParam( CScriptPropertyStub* param )
{
	ASSERT( param );
	m_params.PushBack( param );
}

void CScriptFunctionStub::AddLocal( CScriptPropertyStub* local )
{
	ASSERT( local );
	m_locals.PushBack( local );

}

CScriptDefaultValueStub::CScriptDefaultValueStub( const CScriptFileContext& context, const String& name, CScriptDefaultValue* value )
	: m_context( context )
	, m_name( name )
	, m_value( value )
	, m_createdValue( nullptr )
{};

CScriptDefaultValueStub::~CScriptDefaultValueStub()
{
	if ( m_value )
	{
		delete m_value;
		m_value = nullptr;
	}
}

CScriptStructStub::CScriptStructStub( const CScriptFileContext& context, const String& name, Uint32 flags )
	: m_context( context )
	, m_name( name )
	, m_createdStruct( nullptr )
	, m_flags( flags )
{
}

CScriptStructStub::~CScriptStructStub()
{
	m_fields.ClearPtr();
	m_defaultValues.ClearPtr();
}

void CScriptStructStub::AddField( CScriptPropertyStub* field )
{
	ASSERT( field );
	m_fields.PushBack( field );
}

void CScriptStructStub::AddDefaultValue( CScriptDefaultValueStub* value )
{
	ASSERT( value );
	m_defaultValues.PushBack( value );
}

void CScriptStructStub::AddHint( const String& name, const String& hint )
{
	for ( Uint32 i = 0; i < m_fields.Size(); ++i )
	{
		if ( m_fields[ i ]->m_name == name )
		{
			m_fields[ i ]->m_hint = hint;
			return;
		}
	}
}

CScriptClassStub::CScriptClassStub( const CScriptFileContext& context, const String& name, const String& extends, Uint32 flags )
	: m_context( context )
	, m_name( name )
	, m_extends( extends )
	, m_flags( flags )
	, m_createdClass( nullptr )
	, m_isState( nullptr )
{
}

CScriptClassStub::~CScriptClassStub()
{
	m_properties.ClearPtr();
	m_functions.ClearPtr();
	m_defaultValues.ClearPtr();
}

void CScriptClassStub::AddProperty( CScriptPropertyStub* prop )
{
	ASSERT( prop );
	m_properties.PushBack( prop );
}

void CScriptClassStub::AddFunction( CScriptFunctionStub* func )
{
	ASSERT( func );
	m_functions.PushBack( func );
}

void CScriptClassStub::AddDefaultValue( CScriptDefaultValueStub* value )
{
	ASSERT( value );
	m_defaultValues.PushBack( value );
}

void CScriptClassStub::AddHint( const String& name, const String& hint )
{
	for ( Uint32 i = 0; i < m_properties.Size(); ++i )
	{
		if ( m_properties[ i ]->m_name.EqualsNC( name ) )
		{
			m_properties[ i ]->m_hint = hint;
			return;
		}
	}
}

CScriptEnumOptionStub::CScriptEnumOptionStub( const CScriptFileContext& context, const String& name, Int32 value )
	: m_context( context )
	, m_name( name )
	, m_value( value )
{
}

CScriptEnumOptionStub::~CScriptEnumOptionStub()
{
}

CScriptEnumStub::CScriptEnumStub( const CScriptFileContext& context, const String& name )
	: m_context( context )
	, m_name( name )
	, m_createdEnum( nullptr )
{
}

CScriptEnumStub::~CScriptEnumStub()
{
	m_options.ClearPtr();
}

void CScriptEnumStub::AddOption( CScriptEnumOptionStub* option )
{
	ASSERT( option );
	m_options.PushBack( option );
}

CScriptSystemStub::CScriptSystemStub()
{
}

CScriptSystemStub::~CScriptSystemStub()
{
	m_classes.ClearPtr();
	m_structs.ClearPtr();
	m_enums.ClearPtr();
	m_functions.ClearPtr();
}

void CScriptSystemStub::AddClass( CScriptClassStub* classStub )
{
	ASSERT( classStub );
	m_classes.PushBack( classStub );
}

void CScriptSystemStub::AddStruct( CScriptStructStub* structStub )
{
	ASSERT( structStub );
	m_structs.PushBack( structStub );
}

void CScriptSystemStub::AddEnum( CScriptEnumStub* enumStub )
{
	ASSERT( enumStub );
	m_enums.PushBack( enumStub );
}

void CScriptSystemStub::AddFunction( CScriptFunctionStub* functionStub )
{
	ASSERT( functionStub );
	m_functions.PushBack( functionStub );
}

Uint32 CScriptSystemStub::CountTypes() const
{
	Uint32 count = 0;
	count += m_classes.Size();
	count += m_structs.Size();
	count += m_enums.Size();
	return count;
}

Uint32 CScriptSystemStub::CountFunctions() const
{
	Uint32 count = 0;

	// Count global functions
	count += m_functions.Size();

	// Count function from classes
	for ( Uint32 i=0; i<m_classes.Size(); i++ )
	{
		CScriptClassStub* stub = m_classes[i];
		count += stub->m_functions.Size();
	}
	return count;
}

CScriptClassStub* CScriptSystemStub::FindClassStub( const String& name )
{
	for( auto stubIter : m_classes )
	{
		CScriptClassStub* stub = stubIter;

		if ( stub->m_name == name.AsChar() )
		{
			return stub;
		}
	}

	return nullptr;
}

CClass* CScriptSystemStub::FindClass( const String& name )
{
	if( CScriptClassStub* stub = FindClassStub( name ) )
	{
		return stub->m_createdClass;
	}

	return nullptr;
}

CScriptFunctionStub* CScriptSystemStub::FindFunctionStub( CScriptClassStub* parent, const String& name )
{
	RED_FATAL_ASSERT( parent, "Must have a class specified to search for a member function" );
	
	for( auto stubIter : parent->m_functions )
	{
		CScriptFunctionStub* stub = stubIter;

		if( stub->m_name == name )
		{
			return stub;
		}
	}

	return nullptr;
}

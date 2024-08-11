/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "itemUniqueId.h"
#include "../core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( SItemUniqueId );

const SItemUniqueId SItemUniqueId::INVALID = SItemUniqueId( 0 );

void SItemUniqueId::Load( IGameLoader* loader )
{
	loader->ReadValue( CNAME( id ), m_value );
}

void SItemUniqueId::Save( IGameSaver* saver )
{
	saver->WriteValue( CNAME( id ), m_value );
}

void SItemUniqueId::StreamLoad( ISaveFile* loader, Uint32 version )
{
	Uint16 value = 0;
	*loader << value;
	m_value = value;
}

void SItemUniqueId::StreamSave( ISaveFile* saver )
{
	ASSERT( m_value < 0x10000 );
	Uint16 value = static_cast< Uint16 > ( m_value );
	*saver << value;
}

void funcEqualItemUniqueId( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, a, SItemUniqueId::INVALID );
	GET_PARAMETER( SItemUniqueId, b, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;
	RETURN_BOOL( a == b );
}

void funcNotEqualItemUniqueId( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, a, SItemUniqueId::INVALID );
	GET_PARAMETER( SItemUniqueId, b, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;
	RETURN_BOOL( a != b );
}

static void funcUniqueIdToString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_STRING( ToString( itemId.GetValue() ) );
}

static void funcNameToUniqueId( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemId, CName::NONE );
	FINISH_PARAMETERS;

	Uint32 value;
	if ( !FromString( itemId.AsString(), value ) )
	{
		RETURN_STRUCT( SItemUniqueId, SItemUniqueId::INVALID );
	}
	RETURN_STRUCT( SItemUniqueId, SItemUniqueId( value ) );
}

void RegisterItemUniqueIdFunctions()
{
	NATIVE_BINARY_OPERATOR( Equal, funcEqualItemUniqueId, Bool, SItemUniqueId, SItemUniqueId );
	NATIVE_BINARY_OPERATOR( NotEqual, funcNotEqualItemUniqueId, Bool, SItemUniqueId, SItemUniqueId );
	NATIVE_GLOBAL_FUNCTION( "UniqueIdToString", funcUniqueIdToString );
	NATIVE_GLOBAL_FUNCTION( "NameToUniqueId", funcNameToUniqueId );
}

IMPLEMENT_ENGINE_CLASS( SItemUniqueIdGenerator );

void SItemUniqueIdGenerator::Load( IGameLoader* loader )
{
	loader->ReadValue( CNAME( id ), m_counter );
}

void SItemUniqueIdGenerator::Save( IGameSaver* saver )
{
	saver->WriteValue( CNAME( id ), m_counter );
}

void SItemUniqueIdGenerator::StreamLoad( ISaveFile* loader, Uint32 version )
{
	Uint16 counter;
	*loader << counter;
	m_counter = counter;
}

void SItemUniqueIdGenerator::StreamSave( ISaveFile* saver )
{
	ASSERT( m_counter < 0x10000 );

	Uint16 counter = static_cast< Uint16 > ( m_counter );
	*saver << counter;
}
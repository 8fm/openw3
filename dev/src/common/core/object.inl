#pragma once

#include "functionCalling.h"


/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

RED_INLINE Bool CObject::IsContained( CObject* parentObject ) const
{
	const CObject* testObject = this;
	while ( testObject )
	{	
		// Scope matches
		if ( testObject == parentObject )
		{
			return true;			
		}

		// Go up
		testObject = testObject->GetParent();
	}

	// Scope doest not matches
	return false;
}

RED_INLINE CObject* CObject::GetRoot() const
{
	const CObject* obj = this;
	while ( obj->GetParent() )
	{
		obj = obj->GetParent();
	}
	return (CObject* )obj;
}

// Find parent of specified class moving up in object hierarchy
template< class T > 
RED_INLINE T* CObject::FindParent() const
{
	const CObject* obj = this;
	while ( obj )
	{
		if ( obj->IsA<T>() )
		{
			return (T*)( obj );
		}
		obj = obj->GetParent();
	}
	return NULL;
}

// Helper for creating object
template <class T>
T* CreateObject( CObject* parent = NULL, Uint16 flags=0 )
{
	return static_cast< T* >( CObject::CreateObjectStatic( ClassID<T> (), parent, flags, true ) );
}

// Helper for creating object
template <class T>
T* CreateObject( CClass* objectClass, CObject* parent = NULL, Uint16 flags=0 )
{
	return static_cast< T* >( CObject::CreateObjectStatic( objectClass, parent, flags, true ) );
}

//! Get offset to data
RED_INLINE void* CProperty::GetOffsetPtr( void* base ) const 
{
	if ( IsScripted() )
	{
		ASSERT( m_parent->IsBasedOn( ClassID< IScriptable >() ) );
		IScriptable* realObject = reinterpret_cast< IScriptable* >( base );
		ASSERT( realObject->GetScriptPropertyData() );		
		return OffsetPtr( realObject->GetScriptPropertyData(), m_offset );
	}
	else
	{
		return OffsetPtr( base, m_offset );
	}
}

//! Get offset to data
RED_INLINE const void* CProperty::GetOffsetPtr( const void* base ) const 
{
	if ( IsScripted() )
	{
		const CObject* realObject = reinterpret_cast< const CObject* >( base );
		ASSERT( realObject->GetScriptPropertyData() );
		return OffsetPtr( realObject->GetScriptPropertyData(), m_offset );
	}
	else
	{
		return OffsetPtr( base, m_offset );
	}
}


// Object flags
enum EObjectFlagsOld
{
	Old_OF_Inlined			= FLAG( 4 ),		
	Old_OF_Scripted			= FLAG( 7 ),		
	Old_OF_Transient		= FLAG( 9 ),		
	Old_OF_Referenced		= FLAG( 10 ),		
	Old_OF_DefaultObject	= FLAG( 12 ),		
	Old_OF_ScriptCreated	= FLAG( 13 ),		
	Old_OF_HasHandle		= FLAG( 14 ),		
	Old_OF_WasCooked		= FLAG( 16 ),		
};

RED_INLINE Uint16 CObject::Remap32BitFlagsTo16Bit( Uint32 flags )
{
	Uint16 result = 0;
	if ( flags & Old_OF_Inlined )
	{
		result |= OF_Inlined;
	}
	if ( flags & Old_OF_Scripted )
	{
		result |= OF_Scripted;
	}
	if ( flags & Old_OF_Transient )
	{
		result |= OF_Transient;
	}
	if ( flags & Old_OF_Referenced )
	{
		result |= OF_Referenced;
	}
	if ( flags & Old_OF_DefaultObject )
	{
		result |= OF_DefaultObject;
	}
	if ( flags & Old_OF_ScriptCreated )
	{
		result |= OF_ScriptCreated;
	}
	if ( flags & Old_OF_HasHandle )
	{
		result |= OF_HasHandle;
	}
	if ( flags & Old_OF_WasCooked )
	{
		result |= OF_WasCooked;
	}

	return result;
}
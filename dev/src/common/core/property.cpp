/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "property.h"
#include "class.h"
#include "resource.h"

static Bool IsResourcePointerType( IRTTIType* type )
{
	if ( type )
	{
		if ( type->GetType() == RT_Pointer )
		{
			CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( type );
			CClass* pointedClass = static_cast< CClass* >( pointerType->GetPointedType() );
			return pointedClass && pointedClass->IsBasedOn( ClassID< CResource >() );
		}

		if ( type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( type );
			return IsResourcePointerType( arrayType->GetInnerType() );
		}

		if ( type->GetType() == RT_NativeArray )
		{
			CRTTINativeArrayType* arrayType = static_cast< CRTTINativeArrayType* >( type );
			return IsResourcePointerType( arrayType->GetInnerType() );
		}

	}

	return false;
}

CProperty::CProperty( IRTTIType* type,
					  CClass* owner, 					  
					  Uint32 offset, 
					  const CName& name, 
					  const String& hint, 
					  Uint32 flags, 
					  const String &editorType,
					  Bool arrayCustomEditor )
	: m_parent( owner )
	, m_type( type )
	, m_name( name )
	, m_offset( offset )
	, m_flags( flags )
	, m_hash( 0 )
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	, m_hint( hint )
	, m_customEditor( editorType )
	, m_arrayCustomEditor( arrayCustomEditor )
	, m_setter( NULL )
#endif
{
	RED_UNUSED( hint );
	RED_UNUSED( editorType );
	RED_UNUSED( arrayCustomEditor );
	RED_UNUSED( editorType );
}

Uint32 CProperty::CalcDataLayout( const TDynArray< CProperty*, MC_RTTI >& properties, Uint32 initialOffset, const Uint32 initialAlignment /*= 4*/ )
{
	// First we align the initial offset with the class offset
	Uint32 offset = static_cast< Uint32 >( AlignOffset( initialOffset, initialAlignment ) );

	// Pack the properties using the initial alignment
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* prop = properties[i];

		const Uint32 propSize = prop->GetType()->GetSize();
		const Uint32 propAlignment = prop->GetType()->GetAlignment();

		// Align property placement to the requested alignment
		// Note that the offset is ABSOLUTE (so base class is accounted for)
		offset = static_cast< Uint32 >( AlignOffset( offset, propAlignment ) );

		// Place property
		prop->m_offset = offset;
		offset += propSize;
	}

	// Return offset at the end of the last property in the structure, don't align it (it will be aligned by the next structure if there is one)
	return offset;
}

void CProperty::InstallPropertySetter( CClass* objectClass, const Char* propertyName, IPropertySetter* setter )
{
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	ASSERT( objectClass );
	ASSERT( setter );

	// Find property
	CProperty* prop = objectClass->FindProperty( CName( propertyName ) );
	ASSERT( prop );

	// Change setter
	if ( prop )
	{
		ASSERT( !prop->m_setter );
		prop->m_setter = setter;
	}
#else
	RED_UNUSED( objectClass );
	RED_UNUSED( propertyName );
	RED_UNUSED( setter );
#endif
}

void CProperty::ChangePropertyFlag( CClass* objectClass, const CName& propertyName, Uint32 clearFlags, Uint32 setFlags )
{
	ASSERT( objectClass );

	// Find property
	CProperty* prop = objectClass->FindProperty( propertyName );
	ASSERT( prop );

	// Change properties
	if ( prop )
	{
		prop->m_flags &= ~clearFlags;
		prop->m_flags |= setFlags;
	}
}

Uint64 CProperty::ComputePropertyHash( const CName className, const CName propertyName )
{
	Uint64 hash = Red::CalculateHash64( className.AsAnsiChar() );
	hash = Red::CalculateHash64( propertyName.AsAnsiChar(), hash );
	return hash;
}
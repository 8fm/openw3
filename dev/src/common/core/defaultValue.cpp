/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "defaultValue.h"
#include "object.h"
#include "handleMap.h"

CDefaultValue::CDefaultValue()
	: m_property( nullptr )
	, m_inlineClass( nullptr )
{
}

CDefaultValue::CDefaultValue( CProperty* prop )
	: m_property( prop )
	, m_inlineClass( nullptr )
{
	ASSERT( prop );
}

CDefaultValue::~CDefaultValue()
{
	m_subValues.ClearPtr();
}

void CDefaultValue::ToString() const
{

}

Bool CDefaultValue::Apply( IScriptable* owner, void* object, Bool createInlinedObjects ) const
{
	ASSERT( m_property );
	ASSERT( object != NULL );

	// Apply to property in object
	void* data = m_property->GetOffsetPtr( object );
	return Apply( owner, m_property->GetType(), data, createInlinedObjects );
}

Bool CDefaultValue::Apply( IScriptable* owner, const IRTTIType* type, void* data, Bool createInlinedObjects ) const
{
	// Simple type
	ERTTITypeType typeType = type->GetType();
	if ( typeType == RT_Enum || typeType == RT_Simple || typeType == RT_Fundamental )
	{
		ASSERT( type->GetName() == m_data.GetType() );
		type->Copy( data, m_data.GetData() );
		return true;
	}

	// Array type
	if ( typeType == RT_Array )
	{
		// Get the inner type of array
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( type );
		const IRTTIType* innerType = arrayType->GetInnerType();

		// Clear the array we are setting
		arrayType->Clean( data );

		// Create elements ( unnamed ! )
		const Uint32 numElements = m_subValues.Size();
		if ( numElements )
		{
			// Allocate array space
			Uint32 baseIndex = arrayType->AddArrayElement( data, numElements );
			for ( Uint32 i=0; i<numElements; i++ )
			{
				// Get element memory
				void* elementData = arrayType->GetArrayElement( data, baseIndex + i );
				m_subValues[i]->Apply( owner, innerType, elementData, createInlinedObjects );
			}
		}

		return true;
	}

	// Static array type
	if ( typeType == RT_NativeArray )
	{
		// Get the inner type of array
		const CRTTINativeArrayType* arrayType = static_cast< const CRTTINativeArrayType* >( type );
		IRTTIType* innerType = arrayType->GetInnerType();

		// Create elements ( unnamed ! )
		const Uint32 numElements = m_subValues.Size();
		if ( numElements )
		{
			// Allocate array space
			const Uint32 arraySize = arrayType->GetArraySize( data );
			for ( Uint32 i=0; i<numElements; i++ )
			{
				if ( i < arraySize )
				{
					void* elementData = arrayType->GetArrayElement( data, i );
					m_subValues[i]->Apply( owner, innerType, elementData, createInlinedObjects );
				}
			}
		}

		return true;
	}

	// Structure
	if ( typeType == RT_Class )
	{
		// Apply sub values
		for ( Uint32 i=0; i<m_subValues.Size(); i++ )
		{
			CDefaultValue* subValue = m_subValues[i];
			subValue->Apply( owner, data, createInlinedObjects );
		}

		return true;
	}

	// Handle
	if ( typeType == RT_Handle )
	{
		// Create object
		ASSERT( m_inlineClass );
		IScriptable* subScriptableObject = m_inlineClass->CreateObject< IScriptable >();

		// Set property value
		THandle< IScriptable > handle( subScriptableObject );
		type->Copy( data, &handle );

		// Set sub values
		if ( nullptr != subScriptableObject )
		{
			// Apply sub values
			for ( Uint32 i=0; i<m_subValues.Size(); i++ )
			{
				CDefaultValue* subValue = m_subValues[i];
				subValue->Apply( subScriptableObject, subScriptableObject, createInlinedObjects );
			}
		}

		return true;
	}

	HALT(  "Invalid default value" );
	return false;
}

void CDefaultValue::AddSubValue( CDefaultValue* value )
{
	ASSERT( value );
	m_subValues.PushBack( value );
}

void CDefaultValue::SetInlineClass( CClass* inlineClass )
{
	ASSERT( inlineClass );
	ASSERT( !m_inlineClass );
	m_inlineClass = inlineClass;
}

void CDefaultValue::SetValue( const CVariant& value )
{
	m_data = value;
}
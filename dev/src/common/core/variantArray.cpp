/**
* Copyright ?2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "variantArray.h"
#include "variant.h"

CVariantArray::CVariantArray( CVariant& variant )
	: m_variant( variant )
{
}

CVariantArray::CVariantArray( const CVariantArray& other )
	: m_variant( other.m_variant )
{
}

CVariantArray& CVariantArray::operator=( const CVariantArray& other )
{
	m_variant = other.m_variant;
	return *this;
}

bool CVariantArray::IsArray() const
{
	// only works for dynamic arrays
	return m_variant.IsValid() && m_variant.GetRTTIType()->GetType() == RT_Array;
}

Uint32 CVariantArray::Size() const
{
	// Get the size of the array using the array property
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type&& type->GetType() == RT_Array )
	{
		return (( CRTTIArrayType* )type)->GetArraySize( m_variant.GetData() );
	}

	// No array
	return 0;
}

Bool CVariantArray::Clear()
{
	// Get the element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		arrayType->Clean( m_variant.GetData() );
		return true;
	}

	// Array not cleared
	return false;
}

Bool CVariantArray::Get( Uint32 index, CVariant& result ) const
{
	// Get the element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;

		const Uint32 size = arrayType->GetArraySize( m_variant.GetData() );
		if ( index < size )
		{
			const void* arrayElement = arrayType->GetArrayElement( m_variant.GetData(), index );
			result.Init( CName( arrayType->GetInnerType()->GetName() ), arrayElement );
			return true;
		}
	}

	// Element not extracted
	return false;
}

Bool CVariantArray::Set( Uint32 index, const CVariant& val )
{
	// Set the element in the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;

		const Uint32 size = arrayType->GetArraySize( m_variant.GetData() );
		if ( index < size )
		{
			// Check type
			if ( val.IsValid() && val.GetType() == arrayType->GetInnerType()->GetName() )
			{
				void* arrayElement = arrayType->GetArrayElement( m_variant.GetData(), index );
				arrayType->GetInnerType()->Copy( arrayElement, val.GetData() );
				return true;
			}
		}
	}

	// Element not set
	return false;
}

Bool CVariantArray::Delete( Uint32 index )
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		const Uint32 size = arrayType->GetArraySize( m_variant.GetData() );
		if ( index < size )
		{
			arrayType->DeleteArrayElement( m_variant.GetData(), index );
			return true;
		}
	}

	// Element not deleted
	return false;
}

Bool CVariantArray::Delete( const CVariant& val, Bool all )
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;

		// Check type
		if ( val.IsValid() && val.GetType() == arrayType->GetInnerType()->GetName() )
		{
			if ( all )
			{
				const Uint32 arraySize = arrayType->GetArraySize( m_variant.GetData() );
				for ( Uint32 i=0; i<arraySize; i++ )
				{
					const void* arrayElement = arrayType->GetArrayElement( m_variant.GetData(), i );
					if ( arrayType->GetInnerType()->Compare( arrayElement, val.GetData(), 0 ) )
					{
						if ( arrayType->DeleteArrayElement( m_variant.GetData(), i ) )
						{
							return true;
						}
					}
				}
			}
			else
			{
				// Remove all elements
				Bool elementDeleted = false;
				const Int32 arraySize = arrayType->GetArraySize( m_variant.GetData() );
				for ( Int32 i=arraySize-1; i>=0; --i )
				{
					const void* arrayElement = arrayType->GetArrayElement( m_variant.GetData(), i );
					if ( arrayType->GetInnerType()->Compare( arrayElement, val.GetData(), 0 ) )
					{
						if ( arrayType->DeleteArrayElement( m_variant.GetData(), i ) )
						{
							elementDeleted = true;
						}
					}
				}

				// Anything deleted ?
				return elementDeleted;
			}
		}
	}

	// Element not deleted
	return false;
}

Bool CVariantArray::PushBack( const CVariant& val )
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;

		// Check type
		if ( val.IsValid() && val.GetType() == arrayType->GetInnerType()->GetName() )
		{
			Int32 newIndex = arrayType->AddArrayElement( m_variant.GetData(), 1 );
			if ( newIndex != -1 )
			{
				return Set( newIndex, val );
			}
		}
	}

	// Element not deleted
	return false;

}

Bool CVariantArray::Find( const CVariant& val, Int32& index ) const
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;

		// Check type
		if ( val.IsValid() && val.GetType() == arrayType->GetInnerType()->GetName() )
		{
			const Uint32 arraySize = arrayType->GetArraySize( m_variant.GetData() );
			for ( Uint32 i=0; i<arraySize; i++ )
			{
				const void* arrayElement = arrayType->GetArrayElement( m_variant.GetData(), i );
				if ( arrayType->GetInnerType()->Compare( arrayElement, val.GetData(), 0 ) )
				{
					// Element found
					index = i;
					return true;
				}
			}
		}
	}

	// Element not found
	return false;
}

Bool CVariantArray::Contains( const CVariant& val ) const
{
	Int32 dummyIndex = -1;
	return Find( val, dummyIndex );
}

void* CVariantArray::GetData()
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		return arrayType->GetArrayElement( m_variant.GetData(), 0 );
	}

	// Not an array
	return NULL;
}

const void* CVariantArray::GetData() const
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		return arrayType->GetArrayElement( m_variant.GetData(), 0 );
	}

	// Not an array
	return NULL;
}

void* CVariantArray::GetItemData( Uint32 index )
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		return arrayType->GetArrayElement( m_variant.GetData(), index );
	}

	// Not an array
	return NULL;
}

const void* CVariantArray::GetItemData( Uint32 index ) const
{
	// Remove element from the variant array
	IRTTIType* type = m_variant.GetRTTIType();
	if ( type && type->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = ( CRTTIArrayType* )type;
		return arrayType->GetArrayElement( m_variant.GetData(), index );
	}

	// Not an array
	return NULL;
}


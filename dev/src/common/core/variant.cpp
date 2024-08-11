/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "variant.h"
#include "fileSkipableBlock.h"

CVariant::~CVariant()
{	
	Clear();
}

Uint32 CVariant::GetDataSize() const 
{ 
	return m_rttiType ? m_rttiType->GetSize() : 0; 
}

void CVariant::Clear()
{
	// Cleanup data
	if ( m_rttiType )
	{
		// Destroy property value
		m_rttiType->Destruct( m_data );

		if( m_holdStaticPointer == false )
		{
			RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_Variant, m_data );	
		}
		m_data = NULL;

		// Release RTTI type
		m_rttiType = NULL;
		m_type = CName::NONE;
	}
	else
	{
		ASSERT( !m_type );
		ASSERT( !m_data );
	}
}

Bool gAssertOnTagListVariant = true;

void CVariant::Init( CName typeName, const void* initialData, bool holdStaticPointer )
{
	m_holdStaticPointer = holdStaticPointer;
	
	// Cleanup current data
	if ( typeName && ( m_type != typeName ) )
	{
		// Destroy current value
		Clear();

		// Find RTTI type
		IRTTIType* rttiType = SRTTI::GetInstance().FindType( typeName );
		if ( rttiType )
		{
			ASSERT( m_data == NULL );
			ASSERT( rttiType->GetSize() );

			if( m_holdStaticPointer == true )
			{
				m_data = (void*)initialData;
				m_type = typeName;
				m_rttiType = rttiType;
			}
			else
			{
				// Initialize data
				const Uint32 size = rttiType->GetSize();
				
				
				m_data = RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_Variant , size );
				

				if ( m_data )
				{
					// Initialize variant type
					m_type = typeName;
					m_rttiType = rttiType;

					// Allocate memory
					Red::System::MemorySet( m_data, 0, rttiType->GetSize() );
					m_rttiType->Construct( m_data );
				}
				else
				{
					WARN_CORE( TXT("Unable to allocate %i bytes for variant '%ls'"), rttiType->GetSize(), typeName.AsString().AsChar() );
				}
			}
		}
	}

	// Set value
	if ( m_rttiType && initialData && (holdStaticPointer == false) )
	{
		ASSERT( m_data );
		ASSERT( m_rttiType );
		m_rttiType->Copy( m_data, initialData );
	}
}

CVariant& CVariant::operator=( const CVariant& other )
{
	if ( other.GetType() )
	{
		// Initialize to other data
		Init( other.GetType(), other.GetData() );
	}
	else
	{
		// Destroy current value
		Clear();
	}

	return *this;
}

Bool CVariant::operator==( const CVariant& other ) const
{
	if ( m_rttiType && other.m_rttiType )
	{
		ASSERT( m_data );
		ASSERT( other.m_data );

		if ( m_type == other.m_type )
		{			
			return m_rttiType->Compare( m_data, other.m_data, 0 );
		}
	}
	
	return false;
}

Bool CVariant::operator!=( const CVariant& other ) const
{
	return !(operator==(other));
}

Bool CVariant::IsArray() const
{
	// this only returns true for dynamic arrays!
	return m_rttiType && m_rttiType->GetType() == RT_Array;
}

String CVariant::ToString() const
{
	// Empty
	if ( !m_rttiType )
	{
		return TXT("None");
	}
	else
	{
		ASSERT( m_data );
		String value;
		m_rttiType->ToString( m_data, value );
		return value;
	}
}

Bool CVariant::ToString( String& value ) const
{
	if ( m_rttiType )
	{
		ASSERT( m_data );
		return m_rttiType->ToString( m_data, value );
	}

	// Invalid type
	return false;
}

Bool CVariant::FromString( const Char* value )
{
	if ( m_rttiType )
	{
		ASSERT( m_data );
		return m_rttiType->FromString( m_data, value );
	}

	// Invalid type
	return false;
}

Bool CVariant::FromString( const String& value )
{
	if ( m_rttiType )
	{
		ASSERT( m_data );
		return m_rttiType->FromString( m_data, value );
	}

	// Invalid type
	return false;
}

void CVariant::Serialize( IFile& file )
{
	// Save value
	if( file.IsGarbageCollector() )
	{
		if ( m_rttiType && m_rttiType->NeedsGC() )
		{			
			ASSERT( m_data );
			ASSERT( m_rttiType );
			m_rttiType->Serialize( file, m_data );
		}
	}
	else if ( file.IsWriter() )
	{
		// Start with type name
		file << m_type;

		CFileSkipableBlock block( file );

		// Protected value block
		if ( m_rttiType )
		{			
			ASSERT( m_data );
			ASSERT( m_rttiType );
			m_rttiType->Serialize( file, m_data );
		}
	}
	else if ( file.IsReader() )
	{
		// Load type name
		CName typeName;
		file << typeName;

		// Restore property value
		{
			CFileSkipableBlock block( file );

			// Create property
			Init( typeName, NULL );

			// Property initialized, load data
			if ( m_rttiType&& typeName )
			{
				ASSERT( m_data );
				m_rttiType->Serialize( file, m_data );
			}
			else
			{
				// Skip data
				block.Skip();
			}
		}
	}
}

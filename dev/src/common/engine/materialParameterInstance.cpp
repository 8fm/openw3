/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialParameterInstance.h"
#include "materialParameter.h"

MaterialParameterInstance::MaterialParameterInstance()
	: m_type( NULL )
	, m_data( NULL )
{
}

MaterialParameterInstance::MaterialParameterInstance( const CName& paramName, CClass* paramClass, const void* data /*= NULL*/ )
	: m_name( paramName )
{
	// Get type from parameter class
	m_type = paramClass->GetDefaultObject<CMaterialParameter>()->GetParameterProperty()->GetType();
	ASSERT( m_type );

	// Prepare data buffer
	const Uint32 dataSize = m_type->GetSize();
	m_data = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_MaterialParameters, dataSize, DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT );
	Red::System::MemorySet( m_data, 0, dataSize );
	m_type->Construct( m_data );

	// Copy initial value
	if ( data )
	{
		m_type->Copy( m_data, data );
	}
}

MaterialParameterInstance::MaterialParameterInstance( const CName& paramName, const CName& typeName, const void* data /*= NULL*/ )
	: m_name( paramName )
{
	// Get type from parameter class
	m_type = SRTTI::GetInstance().FindType( typeName );
	RED_FATAL_ASSERT( m_type, "Unknown rtti type. I'm going to crash now." );

	// Prepare data buffer
	const Uint32 dataSize = m_type->GetSize();
	m_data = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_MaterialParameters, dataSize, DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT );
	Red::System::MemorySet( m_data, 0, dataSize );
	m_type->Construct( m_data );

	// Copy initial value
	if ( data )
	{
		m_type->Copy( m_data, data );
	}
}

MaterialParameterInstance::MaterialParameterInstance( const MaterialParameterInstance& other )
	: m_name( other.m_name )
	, m_type( other.m_type )
	, m_data( nullptr )
{
	if ( m_type != nullptr )
	{
		// Prepare data buffer
		const Uint32 dataSize = m_type->GetSize();
		m_data = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_MaterialParameters, dataSize, DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT );
		Red::System::MemorySet( m_data, 0, dataSize );
		m_type->Construct( m_data );
		m_type->Copy( m_data, other.m_data );
	}
}

MaterialParameterInstance& MaterialParameterInstance::operator=( const MaterialParameterInstance& other )
{
	if ( this != &other )
	{
		// Destroy data
		if ( m_data )
		{
			// Destroy parameter data (for handles - releases the reference)
			if ( m_type )
			{
				m_type->Destruct( m_data );
			}

			// Free memory allocated for parameter	
			RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_MaterialParameters, m_data );
		}

		// Reset
		m_name = other.m_name;
		m_type = nullptr;
		m_data = nullptr;

		// Create new one
		if ( other.m_type != nullptr )
		{
			// Get type from parameter class
			m_type = other.m_type;

			// Prepare data buffer
			const Uint32 dataSize = m_type->GetSize();
			m_data = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_MaterialParameters, dataSize, DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT );
			Red::System::MemorySet( m_data, 0, dataSize );
			m_type->Construct( m_data );
			m_type->Copy( m_data, other.m_data );
		}
	}

	return *this;
}

MaterialParameterInstance::~MaterialParameterInstance()
{
	// Destroy data
	if ( m_data )
	{
		// Destroy parameter data (for handles - releases the reference)
		if ( m_type )
		{
			m_type->Destruct( m_data );
		}

		// Free memory allocated for parameter	
		RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_MaterialParameters, m_data );
	}

	// Reset
	m_type = nullptr;
	m_data = nullptr;
}

Bool MaterialParameterInstance::Serialize( IFile& file )
{
	// Just GC
	if ( file.IsGarbageCollector() )
	{
		RED_FATAL_ASSERT( m_type, "No RTTI type bound to this MaterialParameterInstance. This should not happen; DEBUG me! In the meantime, I'm going to crash." );

		if( m_type ) // defensive programming. There seem to be an edge case where m_type can be null during GC.
		{
			m_type->Serialize( file, GetData() );
		}

		return true;
	}

	// Save value
	if ( file.IsWriter() )
	{
		// Save property type
		m_type->Serialize( file, GetData() );
		return true;
	}

	// Load value
	if ( file.IsReader() )
	{
		// Load value
		m_type->Serialize( file, GetData() );
		return true;
	}

	// Not serialized
	return false;
}

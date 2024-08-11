/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "instanceDataLayout.h"
#include "instanceBuffer.h"
#include "rttiType.h"
#include "instanceDataLayoutCompiler.h"
#include "set.h"

InstanceDataLayout::InstanceDataLayout()
	: m_size( 0 )
{
}

InstanceDataLayout::~InstanceDataLayout()
{
	// There are still some buffers using this layout, WTF ?
#if !defined(NO_EDITOR)
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_bufferMutex );
	if ( m_buffers.Size() > 0 )
	{
		RED_HALT( "Relasing instance data layout where there are still %i buffer(s) using it. Please DEBUG.", m_buffers.Size() );
		for ( Uint32 i=0; i<m_buffers.Size(); i++ )
		{
			String bufferInfo = m_buffers[i]->m_description;
			WARN_CORE( TXT("[%i]: %s"), i, bufferInfo.AsChar() );
		}
	}
#endif
}

InstanceBuffer* InstanceDataLayout::CreateBuffer( CObject* owner, const String& info ) const
{
	// Allocate buffer
	InstanceBuffer* buffer = new InstanceBuffer( this, owner, info );

	// Initialize data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( buffer->GetData(), var->GetOffset() );
		type->Construct( varData );
	}

#if !defined(NO_EDITOR)
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_bufferMutex );
		m_buffers.PushBack( buffer );
	}
#endif

	// Return created buffer
	return buffer;
}

void InstanceDataLayout::ClearBuffer( InstanceBuffer& buffer ) const
{
	// Destroy data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( buffer.GetData(), var->GetOffset() );
		type->Destruct( varData );
	}

	// Clear memory
	Red::System::MemorySet( buffer.GetData(), 0, buffer.GetSize() );

	// Initialize data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( buffer.GetData(), var->GetOffset() );
		type->Construct( varData );
	}
}

void InstanceDataLayout::ReleaseBuffer( InstanceBuffer& buffer ) const
{
	// Remove the buffer from the list
#if !defined(NO_EDITOR)
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_bufferMutex );
		ASSERT( m_buffers.Exist( &buffer ) );
		m_buffers.Remove( &buffer );
	}

	// Destroy data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		if ( var && type && buffer.GetData() )
		{
			// Initialize field
			void* varData = OffsetPtr( buffer.GetData(), var->GetOffset() );
			type->Destruct( varData );
		}
	}
#else

	// Destroy data
	for ( Uint32 i=0; i<m_destructVars.Size(); i++ )
	{
		const InstanceVar* var = m_destructVars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		if ( var && type && buffer.GetData() )
		{
			// Initialize field
			void* varData = OffsetPtr( buffer.GetData(), var->GetOffset() );
			type->Destruct( varData );
		}
	}

#endif

	// Mark memory
#ifdef _DEBUG
	Red::System::MemorySet( buffer.GetData(), 0xCC, buffer.GetSize() );
#endif
}

void InstanceDataLayout::CopyBuffer( InstanceBuffer& dest, const InstanceBuffer& source ) const
{
	ASSERT( dest.m_layout == this, TXT("Buffers should be copied with their own layout") );
	ASSERT( dest.m_layout == source.m_layout, TXT("Buffers have different layout") );

	// Destroy existing data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( dest.GetData(), var->GetOffset() );
		type->Destruct( varData );
	}

	// Clear memory
	Red::System::MemorySet( dest.GetData(), 0, dest.GetSize() );

	// Initialize data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( dest.GetData(), var->GetOffset() );
		type->Construct( varData );

		// Copy
		const void* varSrcData = OffsetPtr( source.GetData(), var->GetOffset() );
		type->Copy( varData, varSrcData );
	}
}

void InstanceDataLayout::SerializeBuffer( IFile& file, InstanceBuffer& buffer ) const
{
	// Destroy data
	for ( Uint32 i=0; i<m_vars.Size(); i++ )
	{
		const InstanceVar* var = m_vars[i];
		ASSERT( var );

		IRTTIType* type = var->GetType();
		ASSERT( type );

		// Initialize field
		void* varData = OffsetPtr( buffer.GetData(), var->GetOffset() );
		type->Serialize( file, varData );
	}
}

void InstanceDataLayout::ChangeLayout( const InstanceDataLayoutCompiler& newLayout )
{
#if !defined(NO_EDITOR)

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_bufferMutex );

	// We have some buffer that uses current layout, we need to reformat them
	if ( m_buffers.Size() )
	{
		// Get variables
		TSet< const InstanceVar* > newVarsSet;
		const TInstanceVarList& newVars = newLayout.GetVariables();
		for ( Uint32 i=0; i<newVars.Size(); i++ )
		{
			newVarsSet.Insert( newVars[i].m_first );
		}

		// Get the list of variables that were removed from layout
		TDynArray< InstanceVar* > removedVariables;
		for ( Uint32 i=0; i<m_vars.Size(); i++ )
		{
			const InstanceVar* var = m_vars[i];
			if ( newVarsSet.Find( var ) == newVarsSet.End() )
			{
				// Instance variable was removed from the layout
				removedVariables.PushBack( const_cast< InstanceVar* >( var ) );
			}
		}

		// Convert buffers
		for ( Uint32 i=0; i<m_buffers.Size(); i++ )
		{
			// Get the buffer data
			void* oldBufferData = m_buffers[i]->GetData();

			// Destroy variables
			for ( Uint32 j=0; j<removedVariables.Size(); j++ )
			{
				const InstanceVar* removedVar = removedVariables[j];
				removedVar->GetType()->Destruct( OffsetPtr( oldBufferData, removedVar->GetOffset() ) );
			}

			// Allocate new buffer data
			const Uint32 newDataSize = newLayout.GetCurrentSize();
			if ( newDataSize > 0 )
			{
				// Create new buffer data
				void *newBufferData = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_InstanceBuffer, newDataSize );
				Red::System::MemorySet( newBufferData, 0, newDataSize );

				// Copy existing variables from old locations in old buffer and initialize new values
				for ( Uint32 j=0; j<newVars.Size(); j++ )
				{
					const InstanceVar* newVar = newVars[j].m_first;
					const Uint32 newVarOffset = newVars[j].m_second;

					// If var was in the old layout than copy the data from the old place
					if ( m_vars.Exist( newVar ) )
					{
						// Copy value ( fast, no copy constructor called )
						const Uint32 varDataSize = newVar->GetType()->GetSize();
						const void* oldVarData = OffsetPtr( oldBufferData, newVar->GetOffset() );
						void* newVarData = OffsetPtr( newBufferData, newVarOffset );
						Red::System::MemoryCopy( newVarData, oldVarData, varDataSize );
					}
					else
					{
						// Initialize new value
						void* newVarData = OffsetPtr( newBufferData, newVarOffset );
						newVar->GetType()->Construct( newVarData );
					}
				}

				// Bind new data buffer
				m_buffers[i]->m_data = newBufferData;
				m_buffers[i]->m_size = newDataSize;
			}
			else
			{
				// Bind empty data buffer ( can happen )
				m_buffers[i]->m_data = NULL;
				m_buffers[i]->m_size = 0;
			}

			// Destroy old buffer data
			RED_MEMORY_FREE( MemoryPool_Default, MC_InstanceBuffer, oldBufferData );
		}

		// Mark removed variables as not addressed
		for ( Uint32 j=0; j<removedVariables.Size(); j++ )
		{
			InstanceVar* removedVar = removedVariables[j];
			RED_ASSERT( removedVar );
			removedVar->SetOffset( 0xFFFFFFFF );
#ifdef RED_ASSERTS_ENABLED
			removedVar->SetIndex( 0xFFFFFFFF );
#endif
		}
	}

	// Set layout size
	m_size = newLayout.GetCurrentSize();

	// Set new list of variables
	m_vars.Clear();
	const TInstanceVarList& newVars = newLayout.GetVariables();
	for ( Uint32 i=0; i<newVars.Size(); i++ )
	{
		// Set variable offset
		InstanceVar* var = newVars[i].m_first;
		RED_ASSERT( var );
		var->SetOffset( newVars[i].m_second );
#ifdef RED_ASSERTS_ENABLED
		var->SetIndex( i );
#endif
		// Add to list of variables
		m_vars.PushBack( var );

		// Add to list of variables that require destruction
		if ( var->GetType()->NeedsCleaning() || var->GetType()->GetType() == RT_Class )
		{
			m_destructVars.PushBack( var );
		}
	}

	#else

	// Set layout size
	m_size = newLayout.GetCurrentSize();

	#endif
}

void InstanceDataLayout::ValidateVariable( const InstanceVar* var ) const
{
	ASSERT( var->m_index < m_vars.Size() && var->m_offset == m_vars[ var->m_index ]->m_offset );
}

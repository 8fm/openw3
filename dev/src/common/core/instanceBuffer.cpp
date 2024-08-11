/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "instanceBuffer.h"
#include "instanceDataLayout.h"

InstanceBuffer::InstanceBuffer( const InstanceDataLayout* layout, CObject* owner, const String& info )
	: m_layout( layout )
	, m_data( NULL )
	, m_owner( owner )
	, m_description( info )
#if defined(_DEBUG)
	, m_isRawBuffer( false )
#endif
{
	ASSERT( layout );

	// Allocate data
	m_size = layout->GetSize();
	if ( m_size )
	{
		// Memory is allocated from specialized pool
		m_data = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_InstanceBuffer, m_size, 16 );
		ASSERT( m_data );

		// TODO: get rid of this if possible
		// Initialize memory
		Red::System::MemorySet( m_data, 0, m_size );
	}
}

InstanceBuffer::~InstanceBuffer()
{
	// Internal data should be freed by now
	ASSERT( !m_layout );

	// Free allocated memory
	if ( m_data )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_InstanceBuffer, m_data );
		m_data = NULL;
	}
}

void InstanceBuffer::Clear()
{
	ASSERT( m_layout );
	m_layout->ClearBuffer( *this );
}

void InstanceBuffer::Release()
{
#if defined(_DEBUG)
	ASSERT( ! m_isRawBuffer, TXT("This is raw buffer. Release it with ReleaseRaw()") );
#endif
	ASSERT( m_layout );

	// Release the internal layout data
	if ( m_layout )
	{
		m_layout->ReleaseBuffer( *this );
		m_layout = NULL;
	}

	// Delete data buffer
	delete this;
}

void InstanceBuffer::ReleaseRaw()
{
#if defined(_DEBUG)
	ASSERT( m_isRawBuffer, TXT("This isn't raw buffer. Release it with Release()") );
#endif
	// clear layout, we're raw buffer, so we don't need to release it via layout and destructor expects to have layout cleared
	m_layout = NULL;
	// Delete data buffer
	delete this;
}

void InstanceBuffer::Serialize( IFile& file )
{
	ASSERT( m_layout );
	m_layout->SerializeBuffer( file, *this );
}

InstanceBuffer* InstanceBuffer::CreateCopy( CObject* owner, const String& info ) const
{
	InstanceBuffer* copy = m_layout->CreateBuffer( owner, info );
	copy->operator =( *this );
	return copy;
}

InstanceBuffer* InstanceBuffer::CreateRawCopy( CObject* owner, const String& info ) const
{
	InstanceBuffer* copy = new InstanceBuffer( m_layout, owner, info );
	Red::System::MemoryCopy( copy->m_data, m_data, m_size );
#if defined(_DEBUG)
	copy->m_isRawBuffer = true;
#endif
	return copy;
}

void InstanceBuffer::operator =( const InstanceBuffer& obj )
{
	ASSERT( m_layout );
	m_layout->CopyBuffer( *this, obj );
}

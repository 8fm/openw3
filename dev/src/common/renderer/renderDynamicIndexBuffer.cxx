/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0

CRenderDynamicIndexBuffer::CRenderDynamicIndexBuffer( Uint32 size, IDirect3DIndexBuffer9* buffer )
	: m_buffer( buffer )
	, m_size( size )
	, m_offset( 0 )
	, m_isLocked( false )
	, m_reset( false )
{
}

CRenderDynamicIndexBuffer::~CRenderDynamicIndexBuffer()
{
	ASSERT( !m_isLocked );
	
	// buffer could be lost
	if( m_buffer != NULL )
	{
		m_buffer->Release();
	}
}

void CRenderDynamicIndexBuffer::Bind()
{
	GetDevice()->SetIndices( m_buffer );
}

void CRenderDynamicIndexBuffer::Reset()
{
	ASSERT( !m_isLocked );

	// Reset buffer
	m_reset = true;
	m_offset = 0;
}

Bool CRenderDynamicIndexBuffer::WillFit( Uint32 count ) const
{
	// Check size
	const Uint32 size = count * sizeof( Uint16 );
	return m_offset + size <= m_size;
}

Uint16* CRenderDynamicIndexBuffer::LockSpace( Uint32 count, Uint32& firstIndex )
{
	ASSERT( !m_isLocked );

	// Lock the buffer
	void* bufferMem = NULL;
	const Uint32 size = count * sizeof( Uint16 );
	if ( size + m_offset > m_size )
	{
		HALT( "Out of index buffer memory. BufferReset was not called soon enough." );
		return NULL;
	}

	// Lock the space
	const Uint32 lockFlags = D3DLOCK_NOSYSLOCK | ( m_reset ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE );
	m_buffer->Lock( m_offset, size, &bufferMem, lockFlags );

	// Lock failed
	if ( !bufferMem )
	{
		HALT( "Dynamic index  buffer lock failed: %i bytes @ offset %i, buffer size %i", size, m_offset, m_size );
		return NULL;
	}

	// Remember
	m_reset = false;
	m_isLocked = true;
	firstIndex = m_offset / 2;
	m_offset += size;
	return (Uint16*)bufferMem;
}

void CRenderDynamicIndexBuffer::UnlockSpace()
{
	ASSERT( m_isLocked );
	m_buffer->Unlock();
	m_isLocked = false;
}

CName CRenderDynamicIndexBuffer::GetCategory() const
{
	return CNAME( RenderDynamicIndexBuffer );
}

Uint32 CRenderDynamicIndexBuffer::GetUsedVideoMemory() const
{
	return m_size;
}

CRenderDynamicIndexBuffer* CRenderDynamicIndexBuffer::Create( Uint32 size )
{
	// Create vertex buffer
	IDirect3DIndexBuffer9* buffer = NULL;
	HRESULT hRet = GetDevice()->CreateIndexBuffer( size, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &buffer, NULL );
	if ( !buffer || FAILED(hRet) )
	{
		WARN_RENDERER( TXT("Unable to create dynamic index buffer of size %i: 0x%X"), size, hRet );
		return NULL;
	}

	// Create the wrapper
	return new CRenderDynamicIndexBuffer( size, buffer );
}

void CRenderDynamicIndexBuffer::OnDeviceLost()
{
	ASSERT( !m_isLocked );

	// Release the dynamic resource
	if ( m_buffer )
	{
		m_buffer->Release();
		m_buffer = NULL;
	}
}

void CRenderDynamicIndexBuffer::OnDeviceReset()
{
	ASSERT( !m_isLocked );

	// Allocate vertex buffer
	HRESULT hRet = GetDevice()->CreateIndexBuffer( m_size, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_buffer, NULL );
	if ( !m_buffer || FAILED(hRet) )
	{
		HALT( TXT("Unable to restore dynamic vertex buffer of size %i: 0x%X"), m_size, hRet );
	}
}

#endif
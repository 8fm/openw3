/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0

CRenderDynamicVertexBuffer::CRenderDynamicVertexBuffer( Uint32 size, IDirect3DVertexBuffer9* buffer )
	: m_buffer( buffer )
	, m_size( size )
	, m_offset( 0 )
	, m_isLocked( false )
{
}

CRenderDynamicVertexBuffer::~CRenderDynamicVertexBuffer()
{
	ASSERT( !m_isLocked );

	// mcinek-fix: Device could be lost for some reasons
	if( m_buffer != NULL )
	{
		m_buffer->Release();
	}
}

void CRenderDynamicVertexBuffer::Bind( Uint32 streamIndex, Uint32 offset, Uint32 stride )
{
	GetDevice()->SetStreamSource( streamIndex, m_buffer, offset, stride );
}

void* CRenderDynamicVertexBuffer::LockSpace( Uint32 size, Uint32& offset )
{
	ASSERT( !m_isLocked );

	// Lock the buffer
	void* bufferMem = NULL;
	if ( size + m_offset > m_size )
	{
		// Restart
		m_offset = 0;
		m_buffer->Lock( m_offset, size, &bufferMem, D3DLOCK_DISCARD );
	}
	else
	{
		// Continue
		m_buffer->Lock( m_offset, size, &bufferMem, D3DLOCK_NOOVERWRITE );
	}

	// Lock failed
	if ( !bufferMem )
	{
		HALT( TXT("Dynamic vertex buffer lock failed: %i bytes @ offset %i, buffer size %i"), size, m_offset, m_size );
		return NULL;
	}

	// Remember
	m_isLocked = true;
	offset = m_offset;
	m_offset += size;
	return bufferMem;
}

void CRenderDynamicVertexBuffer::UnlockSpace()
{
	ASSERT( m_isLocked );
	m_buffer->Unlock();
	m_isLocked = false;
}

CName CRenderDynamicVertexBuffer::GetCategory() const
{
	return CNAME( RenderDynamicVertexBuffer );
}

Uint32 CRenderDynamicVertexBuffer::GetUsedVideoMemory() const
{
	return m_size;
}

CRenderDynamicVertexBuffer* CRenderDynamicVertexBuffer::Create( Uint32 size )
{
	// Create vertex buffer
	IDirect3DVertexBuffer9* buffer = NULL;
	HRESULT hRet = GetDevice()->CreateVertexBuffer( size, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &buffer, NULL );
	if ( !buffer || FAILED(hRet) )
	{
		WARN_RENDERER( TXT("Unable to create dynamic vertex buffer of size %i: 0x%X"), size, hRet );
		return NULL;
	}

	// Create the wrapper
	return new CRenderDynamicVertexBuffer( size, buffer );
}

void CRenderDynamicVertexBuffer::OnDeviceLost()
{
	ASSERT( !m_isLocked );

	// Release the dynamic resource
	if ( m_buffer )
	{
		m_buffer->Release();
		m_buffer = NULL;
	}
}

void CRenderDynamicVertexBuffer::OnDeviceReset()
{
	ASSERT( !m_isLocked );

	// Allocate vertex buffer
	HRESULT hRet = GetDevice()->CreateVertexBuffer( m_size, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &m_buffer, NULL );
	if ( !m_buffer || FAILED(hRet) )
	{
		HALT( TXT("Unable to restore dynamic vertex buffer of size %i: 0x%X"), m_size, hRet );
	}
}

#endif
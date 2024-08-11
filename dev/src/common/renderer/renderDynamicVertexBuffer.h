#pragma once

#if 0

/// Dynamic vertex buffer
class CRenderDynamicVertexBuffer : public IDynamicRenderResource
{
protected:
	IDirect3DVertexBuffer9*		m_buffer;
	Uint32						m_size;
	Uint32						m_offset;
	Bool						m_isLocked;

public:
	//! Get buffer size
	RED_INLINE Uint32 GetSize() const { return m_size; }

public:
	CRenderDynamicVertexBuffer( Uint32 size, IDirect3DVertexBuffer9* buffer );
	~CRenderDynamicVertexBuffer();

	// Bind
	void Bind( Uint32 streamIndex, Uint32 offset, Uint32 stride );

	// Lock space in buffer
	void* LockSpace( Uint32 size, Uint32& offset );

	// Unlock space
	void UnlockSpace();

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

public:
	// Lock space in buffer
	template< class T >
	RED_INLINE T* LockSpace( Uint32 count, Uint32& offset )
	{
		return static_cast< T* >( LockSpace( sizeof(T) * count, offset ) );
	}

public:
	//! Create vertex buffer
	static CRenderDynamicVertexBuffer* Create( Uint32 size );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
};

#endif
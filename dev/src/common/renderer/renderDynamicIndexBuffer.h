#pragma once

#if 0

/// Dynamic index buffer
class CRenderDynamicIndexBuffer : public IDynamicRenderResource
{
protected:
	IDirect3DIndexBuffer9*		m_buffer;
	Uint32						m_size;
	Uint32						m_offset;
	Bool						m_isLocked;
	Bool						m_reset;

public:
	//! Get buffer size
	RED_INLINE Uint32 GetSize() const { return m_size; }

public:
	CRenderDynamicIndexBuffer( Uint32 size, IDirect3DIndexBuffer9* buffer );
	~CRenderDynamicIndexBuffer();

	// Bind
	void Bind();

	// Reset buffer
	void Reset();

	// Check if allocation will fit in the buffer
	Bool WillFit( Uint32 count ) const;

	// Lock space in buffer
	Uint16* LockSpace( Uint32 count, Uint32& firstIndex );

	// Unlock space
	void UnlockSpace();

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

public:
	//! Create vertex buffer
	static CRenderDynamicIndexBuffer* Create( Uint32 size );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
};

#endif
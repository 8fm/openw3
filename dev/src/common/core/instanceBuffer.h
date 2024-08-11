/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "instanceVar.h"
#include "instanceDataLayout.h"

/// Buffer for instanced data
class InstanceBuffer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_InstanceBuffer );

	friend class InstanceDataLayout;

protected:
	String							m_description;	//!< Debug description
	CObject*						m_owner;		//!< Owner of this instance buffer
	const InstanceDataLayout*		m_layout;		//!< Buffer layout
	Uint32							m_size;			//!< Size of the buffer
	void*							m_data;			//!< Data
#if defined(_DEBUG)
	Bool							m_isRawBuffer;	//!< Is it raw buffer
#endif

public:
	//! Get buffer size
	RED_INLINE Uint32 GetSize() const { return m_size; }

	//! Get data buffer 
	RED_INLINE void* GetData() { return m_data; }

	//! Get data buffer 
	RED_INLINE const void* GetData() const { return m_data; }

	//! Get buffer layout
	RED_INLINE const InstanceDataLayout* GetLayout() const { return m_layout; }

	//! Get owner of this instance buffer
	RED_INLINE CObject* GetOwner() const { return m_owner; }

public:
	//! Clear buffer to default values
	void Clear();

	//! Release buffer ( frees all the memory )
	void Release();

	//! Release raw buffer ( frees all the memory, but doesn't call destructors )
	void ReleaseRaw();

	//! Serialize data
	void Serialize( IFile& file );

	//! Create copy of buffer
	InstanceBuffer* CreateCopy( CObject* owner, const String& info ) const;

	//! Create copy of raw buffer (without calling constructors)
	InstanceBuffer* CreateRawCopy( CObject* owner, const String& info ) const;

public:
	//! Get data for instance variable ( slow, debug )
	template< class T >
	T& operator[]( const TInstanceVar<T>& var )
	{
#ifdef RED_ASSERTS_ENABLED
		m_layout->ValidateVariable( &var );
#endif

		RED_FATAL_ASSERT( ( var.GetOffset() + sizeof( T ) ) <= m_size, "Offset is out of bound!" );
		return *(T*) OffsetPtr( m_data, var.GetOffset() );
	}

	//! Get data for instance variable ( slow, debug )
	template< class T >
	const T& operator[]( const TInstanceVar<T>& var ) const
	{
#ifdef RED_ASSERTS_ENABLED
		m_layout->ValidateVariable( &var );
#endif
		RED_FATAL_ASSERT( ( var.GetOffset() + sizeof( T ) ) <= m_size, "Offset is out of bound!" );
		return *( const T*) OffsetPtr( m_data, var.GetOffset() );
	}

public:
	//! Copy data from other instance buffer (using layout)
	void operator =( const InstanceBuffer& obj );

private:
	// Allocation and deallocation of instance buffer is done by third-party
	InstanceBuffer( const InstanceDataLayout* layout, CObject* owner, const String& info );
	~InstanceBuffer();

private:
	// Copying of instance data buffer is not allowed ( we do not know what kind of data is there )
	RED_INLINE InstanceBuffer( const InstanceBuffer& ) {}
};

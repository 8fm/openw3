/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "file.h"
#include "../redSystem/utility.h"

/// Memory based file writer
class CMemoryFileWriter : public IFileEx, public IFileDirectMemoryAccess, Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

private:
	CBaseArray & m_data;			// By using CBaseArray, we can write to any dynamic array of Uint8, regardless of pool/memclass
	EMemoryClass m_memoryClass;		// Store mem class + pool for CBaseArray functionality
	Uint32 m_offset;	

public:
	template< EMemoryClass memClass, RED_CONTAINER_POOL_TYPE memPool >
	CMemoryFileWriter( TDynArray< Uint8, memClass, memPool >& data );

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual void Close() override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileEx interface
	virtual const void* GetBuffer() const override;
	virtual size_t GetBufferCapacity() const override;

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;
};

template< EMemoryClass memClass, RED_CONTAINER_POOL_TYPE memPool >
CMemoryFileWriter::CMemoryFileWriter( TDynArray< Uint8, memClass, memPool >& data )
	: IFileEx( FF_Buffered | FF_MemoryBased | FF_Writer | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
	, m_data( data )
	, m_memoryClass( memClass )
	, m_offset( data.Size() )
{
}

/// Memory based file writer
class CMemoryFileWriterExternalBuffer : public IFileEx, public IFileDirectMemoryAccess, Red::System::NonCopyable
{
private:
	void*				m_buffer;		//!< Data buffer
	Uint32				m_offset;		//!< Current offset
	Uint32				m_realSize;		//!< Maximum offset
	Uint32				m_size;			//!< Data size

public:
	CMemoryFileWriterExternalBuffer( void* buffer, Uint32 size );

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual void Close() override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileEx interface
	virtual const void* GetBuffer() const override;
	virtual size_t GetBufferCapacity() const override;

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;

	RED_INLINE Uint32 GetRealSize() const { return m_realSize; }
};

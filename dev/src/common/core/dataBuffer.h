/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "rttiType.h"

//------

/// Abstract data buffer memory allocation strategy
/// This is usually used as singleton
class IDataBufferAllocator
{
public:
	virtual ~IDataBufferAllocator() {};
	virtual void* Allocate( const Uint32 size ) const = 0;
	virtual void Free( void* ptr, const Uint32 size ) const = 0;
};

//------

/// Abstract data buffer memory allocation strategy
/// This is usually used as singleton
template< EMemoryClass memoryClass = MC_DataBuffer, RED_CONTAINER_POOL_TYPE memoryPool = MemoryPool_Default >
class TDataBufferAllocator : public IDataBufferAllocator
{
public:
	virtual void* Allocate( const Uint32 size ) const override
	{
		return RED_MEMORY_ALLOCATE( memoryPool, memoryClass, size );
	}

	virtual void Free( void* ptr, const Uint32 size ) const override
	{
		RED_MEMORY_FREE( memoryPool, memoryClass, ptr );
	}

	static const IDataBufferAllocator& GetInstance()
	{
		static TDataBufferAllocator< memoryClass, memoryPool > theInstance;
		return theInstance;
	}
};

//------

static RED_FORCE_INLINE const IDataBufferAllocator& DefaultDataBufferAllocator()
{
	return TDataBufferAllocator< MC_DataBuffer, MemoryPool_Default >::GetInstance();
}

//------

/// Generic buffer for resource data
class DataBuffer
{
public:
	//! 4GB maximum
	typedef Uint32 TSize;

	//! Get size of data in the buffer ( always valid )
	RED_INLINE const TSize GetSize() const { return m_dataSize; }

	//! Get pointer to data ( valid only if loaded )
	RED_INLINE void* GetData() const { return m_dataHandle; }

	//! Get the internal memory size
	RED_INLINE const TSize GetInternalMemSize() const { return m_dataSize; }

	//! Get RTTI type name
	RED_INLINE static const CName& GetTypeName() { return st_typeName; }

	//! Allocate buffer of given size, optionally: copy the initialization data
	DataBuffer( const IDataBufferAllocator& allocator = DefaultDataBufferAllocator(), const TSize size = 0, const void* data = nullptr );

	//! Copy constructor
	DataBuffer( const DataBuffer& other );

	//! Move constructor
	DataBuffer( DataBuffer&& other );

	//! Cleanup
	~DataBuffer();

	//! Clear data
	void Clear();

	//! Resize the buffer, uses existing allocation strategy
	void Allocate( const TSize newSize );

	//! Serialization
	void Serialize( class IFile& file );

	//! Assignment operator - will COPY the data
	DataBuffer& operator=( const DataBuffer& other );

	//! Move operator - will COPY the data
	DataBuffer& operator=( DataBuffer&& other );

	//! Moves data to another buffer (not a memory operation), handle is no longer accessible from this object
	void MoveHandle( DataBuffer& other );

	//! Copy data from another buffer
	void CopyHandle( const DataBuffer& other );

	//! Compare content - slow
	bool CompareContent( const DataBuffer& other ) const;

	// This is a hack to allow a BufferProxy (or BufferHandle) to take ownership of the contents of a DataBuffer
	// It exists because it's too late to perform a resave of the data we're switching over
	// Dangerous!
	void ClearWithoutFree();

private:
	//! Reallocate memory for the data buffer
	static void* ReallocateMemory( const IDataBufferAllocator& allocator, void* dataPtr, const TSize newSize, const TSize oldSize );

	const IDataBufferAllocator*		m_allocator;		//!< Allocate to use when allocating/freeing stuff from this data buffer
	void*							m_dataHandle;		//!< Handle to data
	TSize							m_dataSize;			//!< Size of the data to load.

	static CName					st_typeName;
};

//------

// We should not allow this data structure to grow big - should be as compact as possible
static_assert( sizeof(DataBuffer) <= 24, "DataBuffer size can't be bigger than 24" );

//------

/// RTTI type description
class CRTTIDataBufferType : public IRTTIType
{
public:
	virtual const CName& GetName() const override { return DataBuffer::GetTypeName(); }
	virtual ERTTITypeType GetType() const override { return RT_Simple; }
	virtual Uint32 GetSize() const override { return sizeof(DataBuffer); }
	virtual Uint32 GetAlignment() const override { return 4; }
	virtual void Construct( void* object ) const override;
	virtual void Destruct( void* /*object*/ ) const override;
	virtual Bool Compare( const void* data1, const void* data2, Uint32 /*flags*/ ) const override;
	virtual void Copy( void* dest, const void* src ) const override;
	virtual void Clean( void* data ) const override;
	virtual Bool Serialize( IFile& file, void* data ) const override;
	virtual Bool ToString( const void* data, String& valueString ) const override;
	virtual Bool FromString( void* data, const String& valueString ) const override;
	virtual Bool DebugValidate( const void* /*data*/ ) const override;
	virtual Bool NeedsCleaning() override { return true; }
	virtual Bool NeedsGC()  override { return false; }
};

//------

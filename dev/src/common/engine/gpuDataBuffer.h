/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once


#include "../../common/gpuApiUtils/gpuApiMemory.h"


class CGpuDataBufferObject;
class IFileLatentLoadingToken;


// Data buffer used for placement creation of GpuApi resources. It provides possibility of transferring ownership of the data buffer,
// either to another GpuDataBuffer, or completely unlinking the void*. Once ownership has been transferred, it's still possible to
// reload the original data.

class GpuDataBuffer
{
public:
	typedef Uint32 TSize;

private:
	IFileLatentLoadingToken*		m_loadingToken;		//!< Pointer to data in a source file.
	CGpuDataBufferObject*			m_data;				//!< Loaded data.
	TSize							m_dataSize;
	Uint32							m_dataAlignment;	//!< Required alignment of GPU data

	GpuApi::EInPlaceResourceType	m_type;

public:
	//! Get size of data in the buffer ( always valid )
	RED_INLINE TSize GetSize() const { return m_dataSize; }

	//! Get pointer to data handle ( valid only if loaded )
	Red::MemoryFramework::MemoryRegionHandle GetDataHandle();
	Red::MemoryFramework::MemoryRegionHandle GetDataHandle() const;

	RED_INLINE GpuApi::EInPlaceResourceType GetResourceType() const { return m_type; }

public:
	//! Constructs empty data buffer, in invalid state. Use assignment, CopyHandle, or MoveHandle to make it usable.
	GpuDataBuffer();

	//! Constructs empty data buffer.
	GpuDataBuffer( GpuApi::EInPlaceResourceType type );

	//! Allocate buffer of given size, optionally from existing data.
	GpuDataBuffer( GpuApi::EInPlaceResourceType type, TSize size, const void* data, Uint32 alignment=(Uint32)-1 );

	//! Copy existing buffer. This will create a copy of any loaded data.
	GpuDataBuffer( const GpuDataBuffer& other );

	~GpuDataBuffer();

	//! Assignment will cause the memory to be copied.
	GpuDataBuffer& operator =( const GpuDataBuffer& other );

	//! Clear data
	void Clear();

	//! Allocate new buffer.
	void Allocate( TSize size );

	//! Serialize from file.
	void Serialize( IFile& file );

	//! Copies handle from another buffer (not a memory operation), handle is still accessible from the other object
	void CopyHandle( const GpuDataBuffer& other );

	//! Move handle from another GpuApiDataBuffer to this one. Other will no longer have access to the data.
	void MoveHandle( GpuDataBuffer& other );

	//! Remove reference to data buffer. Caller becomes responsible for freeing the memory. Can call Load() to reload data from file.
	Red::MemoryFramework::MemoryRegionHandle UnlinkDataHandle();

	//! Load data 
	Bool Load();

	//! Unload data
	Bool Unload();

private:
	//! Unlink from data file. Load() will no longer be available.
	void UnlinkFromFile();

	Bool ReallocateMemory( TSize size );
};

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/latentDataBuffer.h"


/// Data buffer for bitmap mipmap, can be loaded asynchronously
class BitmapMipLatentDataBuffer : public LatentDataBuffer
{
public:
	//! Constructs empty data buffer
	BitmapMipLatentDataBuffer();

	//! Constructs data buffer from existing data
	BitmapMipLatentDataBuffer( const void* data, TSize size );

	//! Allocate buffer of given size ( empty )
	BitmapMipLatentDataBuffer( TSize size );

	//! Copy constructor
	BitmapMipLatentDataBuffer( const BitmapMipLatentDataBuffer& other );

	//! Cleanup
	virtual ~BitmapMipLatentDataBuffer();

	//! BitmapDataBuffer objects should always allocate their memory from the GpuApi texture pool
	virtual Bool ReallocateMemory( TSize size, Red::MemoryFramework::MemoryClass newMemoryClass, TSize alignment = 16 ) override;

	//! Assignment operator
	BitmapMipLatentDataBuffer& operator=( const BitmapMipLatentDataBuffer& other );
};

/// Data buffer for bitmap data, loaded synchronously and allocated from the Gpu Api
class BitmapDataBuffer : public DataBuffer
{
public:
	//! Constructs data buffer from existing data
	BitmapDataBuffer( const Uint32 size = 0, const void* data = nullptr );

	//! Copy constructor
	BitmapDataBuffer( const BitmapDataBuffer& other );

	//! Assignment operator
	BitmapDataBuffer& operator=( const BitmapDataBuffer& other );

	//! Assignment operator
	BitmapDataBuffer& operator=( const DataBuffer& other );
};
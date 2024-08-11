/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Preloaded data block
class CCollisionCachePreloadedData
{
public:
	CCollisionCachePreloadedData();
	~CCollisionCachePreloadedData();

	// Initialize preloading buffer, returns data to the allocated memory
	void* Initialize( const Uint32 dataOffset, const Uint32 dataSize );

	// Do we have preloaded data for given data range ? returns the pointer to the data if we have it or NULL if we don't
	// NOTE: the offset is the absolute offset in the file, not local offset in the preload buffer
	const void* GetPreloadedData( const Uint32 fileOffset, const Uint32 size ) const;

private:
	typedef TDynArray< Uint8, MC_CompiledCollision, MemoryPool_Physics > TCollisionData;

	TCollisionData					m_data;				//!< Preloaded data size
	Uint32							m_dataOffset;		//!< Starting offset for preloaded data
};
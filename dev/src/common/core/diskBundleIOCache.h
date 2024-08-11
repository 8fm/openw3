/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "ringAllocator.h"

// TODO: get rid of this crappy cache
// This is only required because current bundles are not supporting any block based compression
// They require the whole files to be read and decompressed AT ONCE - what a bull...
class CDiskBundleIOCache
{
public:
	CDiskBundleIOCache();
	~CDiskBundleIOCache();

	void Initialize( Uint32 bufferSize );

	CRingBufferBlock* AllocateBlock( const Uint32 size, const Uint32 alignment );

private:
	const static Uint32		MAX_BLOCKS = 32;

	void*					m_memory;
	CRingBufferAllocator	m_allocator;
	Uint32 m_bufferSize;
};

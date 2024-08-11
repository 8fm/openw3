/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef BUNDLE_DATA_BUFFER_H_
#define BUNDLE_DATA_BUFFER_H_

#include "../redThreads/redThreadsAtomic.h"

enum EBundleBufferType
{
	BBT_NONE,
	BBT_IO,
	BBT_DATA,
	BBT_MAX
};
//////////////////////////////////////////////////////////////////////////
//
// NOTES:
// This should be done instead of deriving from NonCopyable but VS2012 
// still doesn't support it. Don't try to copy these buffers!
// CBundleDataBuffer() = delete;
// CBundleDataBuffer& operator=( const CBundleDataBuffer& ) = delete;
// CBundleDataBuffer( const CBundleDataBuffer& ) = delete;
//////////////////////////////////////////////////////////////////////////
class CBundleDataBuffer : public Red::System::NonCopyable
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Constructor
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE CBundleDataBuffer( void* dataBuffer, Uint32 size, Uint32 flags, EBundleBufferType bufferType = BBT_NONE, Uint32 initialRefCount = 0 );
	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	virtual ~CBundleDataBuffer()
	{}

	//////////////////////////////////////////////////////////////////////////
	// Public Methods
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE Uint32 AddRef();
	RED_INLINE Uint32 Release();
	RED_INLINE void* GetDataBuffer() const;
	RED_INLINE const Uint32 GetSize() const;
	RED_INLINE const Uint32 GetFlags() const;
	RED_INLINE const EBundleBufferType GetBufferType() const;
	RED_INLINE void InvalidateDataBuffer();
private:
	Red::Threads::CAtomic< void* >		m_dataBuffer;		// raw data
	Red::Threads::CAtomic< Uint32 >		m_referenceCount;	// atomic refcount
	Uint32								m_size;				// total buffer size, we do not support larger buffers
	Uint32								m_flags;			// memory type, etc, etc
	EBundleBufferType					m_bufferType;		// Type of buffer assigned.
};

#include "bundleDataBuffer.inl"

#endif //BUNDLE_DATA_BUFFER_H_
/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef BUNDLE_DATA_HANDLE_H_
#define BUNDLE_DATA_HANDLE_H_
#include "bundleDataBuffer.h"

class CBundleDataHandle
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Constructors
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE CBundleDataHandle();
	RED_INLINE CBundleDataHandle( CBundleDataBuffer* dataBuffer, Uint32 localOffset = 0 );
	RED_INLINE CBundleDataHandle( const CBundleDataHandle& other );
	RED_INLINE CBundleDataHandle( CBundleDataHandle&& other );

	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE ~CBundleDataHandle();	
	//////////////////////////////////////////////////////////////////////////
	// Operators
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE CBundleDataHandle& operator=( const CBundleDataHandle& other );
	RED_INLINE CBundleDataHandle& operator=( CBundleDataHandle&& other );
	
	//////////////////////////////////////////////////////////////////////////
	// Public Methods
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE void* GetRaw() const;
	RED_INLINE CBundleDataBuffer* GetBufferInternal() const;

private:
	CBundleDataBuffer*		m_buffer;
	Uint32					m_localOffset;
};

#include "bundleDataHandle.inl"

#endif // BUNDLE_DATA_HANDLE_H_
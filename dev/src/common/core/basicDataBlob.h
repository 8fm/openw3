/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "atomicSharedPtr.h"

/// Data blob
class CDataBlob
{
public:
	typedef std::function< void( void* ptr ) > DeallocateFunction;

	CDataBlob();
	CDataBlob( const Uint32 dataSize ); // create an empty data container
	CDataBlob( const void* data, const Uint32 dataSize ); // create a copy of data
	CDataBlob( void* data, const Uint32 dataSize, DeallocateFunction dealloc ); // transfer ownership of data
	~CDataBlob();

	// Get data
	RED_INLINE void* GetData() { return m_data; }

	// Get data (read only)
	RED_INLINE const void* GetData() const { return m_data; }

	// Get size of data
	RED_INLINE const Uint32 GetDataSize() const { return m_dataSize; }

private:
	void*						m_data;
	Uint32						m_dataSize;
	DeallocateFunction			m_dealloc;
};

/// Handle to the response data (reference counted, is sent from thread hence the atomic shared ptr)
typedef Red::TAtomicSharedPtr< CDataBlob >	DataBlobPtr;

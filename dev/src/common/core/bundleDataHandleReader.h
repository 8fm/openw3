/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef BUNDLE_DATA_HANDLE_REDAER__H_
#define BUNDLE_DATA_HANDLE_REDAER__H_

#include "file.h"
#include "bundleDataHandle.h"

/// IFile wrapper for reading from CBundleDataHandle
class CBundleDataHandleReader : public IFile, public IFileDirectMemoryAccess
{
public:
	CBundleDataHandleReader( const BundleDataHandle& dataHandle, const Uint32 size );
	virtual ~CBundleDataHandleReader();

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() { return static_cast< IFileDirectMemoryAccess* >( this ); }

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const { return (Uint8*) m_bundleDataBuffer->GetBuffer(); }
	virtual Uint32 GetBufferSize() const  { return m_bundleDataBuffer->GetSize(); }

private:
	BundleDataHandle m_bundleDataBuffer;		// Cached buffer used to read into
	Uint32 m_localReadOffset;					// Local offset into the buffer
	Uint32 m_localFileSize;
};

#endif //


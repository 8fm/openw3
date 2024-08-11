/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "fileProxy.h"

/// Shadow writer - creates a bit perfect copy of the data saved to the file, can be used for transparent CRC calculation of the saved data
class CShadowFileWriter : public CFileProxy, public IFileDirectMemoryAccess
{
public:
	CShadowFileWriter( IFile* file, const Uint64 baseOffset );
	virtual ~CShadowFileWriter();

private:
	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileDirectMemoryAccess
	virtual Uint8* GetBufferBase() const;
	virtual Uint32 GetBufferSize() const;

	TDynArray< Uint8 >			m_data;
	Uint32						m_size;
	Uint64						m_baseOffset;
};
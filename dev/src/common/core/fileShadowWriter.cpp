/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fileShadowWriter.h"

CShadowFileWriter::CShadowFileWriter( IFile* file, const Uint64 baseOffset )
	: CFileProxy( file )
	, m_baseOffset( baseOffset )
	, m_size( 0 )
{
	m_data.Resize( 128 * 1024 );
}

CShadowFileWriter::~CShadowFileWriter()
{
}

void CShadowFileWriter::Serialize( void* buffer, size_t size )
{
	const Uint64 currentOffset = m_file->GetOffset();

	// save data to actual file
	CFileProxy::Serialize( buffer, size );

	// make sure we are not trying to write something before our allowed offset
	if ( currentOffset < m_baseOffset )
	{
		ERR_CORE( TXT("ShadowWriter: file pointer moved before the base offset (%d<%d)"), currentOffset, m_baseOffset );
		return;
	}

	// compute writing region
	const Uint32 writeStart = (Uint32) (currentOffset - m_baseOffset);
	const Uint32 writeEnd = (Uint32) (writeStart + size);
	while ( writeEnd > m_data.Size() )
	{
		m_data.Resize( m_data.Size() * 2 );
	}

	// write data
	Red::MemoryCopy( &m_data[ writeStart ], buffer, size );
	m_size = Max( m_size, writeEnd );
}

Uint8* CShadowFileWriter::GetBufferBase() const
{
	return (Uint8*) m_data.Data();
}

Uint32 CShadowFileWriter::GetBufferSize() const
{
	return m_size;
}

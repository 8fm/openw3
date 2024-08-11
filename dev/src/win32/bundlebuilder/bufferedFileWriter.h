/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __BUFFERED_FILE_WRITER_H__
#define __BUFFERED_FILE_WRITER_H__

#include "../../common/core/core.h"

namespace Bundler
{
	class CBufferedFileWriter
	{
	public:
		CBufferedFileWriter();
		~CBufferedFileWriter();

		Bool Initialise( Uint32 size );

		Bool Open( const AnsiChar* path );
		void Close();

		Bool Write( const void* data, Uint32 size );

		Bool Seek( Uint32 position );

	private:
		Bool Flush();

		FILE* m_fileHandle;
		Uint8* m_buffer;

		Uint32 m_size;
		Uint32 m_writePosition;
	};
}

#endif // __BUFFERED_FILE_WRITER_H__

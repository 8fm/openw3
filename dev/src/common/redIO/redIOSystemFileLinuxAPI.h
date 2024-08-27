/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#if defined( RED_PLATFORM_LINUX )

namespace io
{
	Uint32 LastError();

	namespace linux
	{
		class SystemFile
		{
			RED_USE_MEMORY_POOL( red::PoolEngine );

		public:
			SystemFile();
			SystemFile( SystemFile&& other );
			SystemFile& operator=( SystemFile&& rhs );
			~SystemFile();
			Bool Open( const char* path, Uint32 openFlags );
			Bool Close();

			Bool Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead );
			Bool Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten );
			Bool Seek( Int64 offset, ESeekOrigin seekOrigin );
			Int64 Tell() const;
			Bool Truncate( Uint64 totalFileSize );
			Bool Flush();

			Bool IsValid() const;

			Uint64 GetFileSize() const;
			Uint64 GetFileTimestamp() const;
			Uint32 GetFileHandle() const;
			Uint32 GetFileID() const;

		private:
			int	m_fileDescriptor;

		};
	}

	namespace prv
	{
		using SystemFile = ::io::linux::SystemFile;
	}
}

#endif // #if defined( RED_PLATFORM_LINUX )

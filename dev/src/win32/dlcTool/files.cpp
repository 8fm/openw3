#include "build.h"
#include "files.h"

namespace DLCTool
{

//////////////////////////////////////////////////////////////////////////////////////

	DLCResourceReader::DLCResourceReader( HMODULE module, HRSRC resource, HGLOBAL resPtr )
		: m_module( module )
		, m_resPtr( resPtr )
		, m_size( 0 )
		, m_offset( 0 )
		, m_resBeginPtr( NULL )
	{
		// Extract base file offset
		m_size = SizeofResource( m_module, resource );
		m_resBeginPtr = LockResource( m_resPtr );
	}

	DLCResourceReader::~DLCResourceReader()
	{
		FreeResource( m_resPtr );
	}

	void DLCResourceReader::Serialize( void* data, UINT size )
	{
		if ( m_resBeginPtr )
		{
			UINT sizeLeft = m_size - m_offset;
			UINT bytesToRead = size;
			if ( sizeLeft < size )
			{
				ERR( String_Error_IOReadError, size, sizeLeft );
				bytesToRead = sizeLeft;
			}
			memcpy_s( data, size, (char*)m_resBeginPtr + m_offset, bytesToRead );
			m_offset += bytesToRead;
		}
	}

	DWORD DLCResourceReader::GetSize() const
	{
		return m_size;
	}

	UINT DLCResourceReader::GetOffset() const
	{
		return m_offset;
	}

	void DLCResourceReader::Seek( UINT offset )
	{
		m_offset = offset;
	}

	DLCResourceReader* DLCResourceReader::CreateReader( int resourceName )
	{
		HMODULE module = GetModuleHandle(NULL);
		HRSRC resource = FindResource( module, MAKEINTRESOURCE( resourceName ), TEXT( "BINARY" ) );
		HGLOBAL resPtr = LoadResource( module, resource );
		if ( resPtr == NULL )
		{
			return NULL;
		}

		// Create read stream
		return new DLCResourceReader( module, resource, resPtr );
	}

//////////////////////////////////////////////////////////////////////////////////////

	DLCFileReader::DLCFileReader( HANDLE hFileHandle, UINT baseOffset )
		: m_fileHandle( hFileHandle )
		, m_baseOffset( baseOffset )
		, m_size( 0 )
	{
		// Extract base file offset
		m_size = GetFileSize( hFileHandle, NULL ) - baseOffset;
	}

	DLCFileReader::~DLCFileReader()
	{
		// Close file handle
		if ( m_fileHandle )
		{
			CloseHandle( m_fileHandle );
			m_fileHandle = NULL;
		}
	}

	void DLCFileReader::Serialize( void* data, UINT size )
	{
		if ( m_fileHandle )
		{
			DWORD bytesRead = 0;
			ReadFile( m_fileHandle, data, size, &bytesRead, NULL );
			if ( bytesRead != size )
			{
				ERR( String_Error_IOReadError, size, bytesRead );
			}
		}
	}

	DWORD DLCFileReader::GetSize() const
	{
		return m_size;
	}

	UINT DLCFileReader::GetOffset() const
	{
		return SetFilePointer( m_fileHandle, 0, 0, FILE_CURRENT ) - m_baseOffset;
	}

	void DLCFileReader::Seek( UINT offset )
	{
		SetFilePointer( m_fileHandle, m_baseOffset + offset, 0, FILE_BEGIN );
	}

	DLCFileReader* DLCFileReader::CreateReader( const WCHAR* filePath )
	{
		HANDLE hFile = ::CreateFileW( filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			return new DLCFileReader( hFile, 0 );
		}
		else
		{
			return NULL;
		}
	}


//////////////////////////////////////////////////////////////////////////////////////

	DLCFileWriter::DLCFileWriter( HANDLE hFileHandle )
		: m_fileHandle( hFileHandle )
	{
	}

	DLCFileWriter::~DLCFileWriter()
	{
		// Close file handle
		if ( m_fileHandle )
		{
			CloseHandle( m_fileHandle );
			m_fileHandle = NULL;
		}
	}

	void DLCFileWriter::Serialize( void* data, UINT size )
	{
		if ( m_fileHandle )
		{
			DWORD writtenBytes = 0;
			WriteFile( m_fileHandle, data, size, &writtenBytes, NULL );
			if ( size != writtenBytes )
			{
				ERR( String_Error_IOWriteError, size, writtenBytes );
			}
		}
	}

	DWORD DLCFileWriter::GetSize() const
	{
		return GetFileSize( m_fileHandle, NULL );
	}

	UINT DLCFileWriter::GetOffset() const
	{
		return SetFilePointer( m_fileHandle, 0, 0, FILE_CURRENT );
	}

	void DLCFileWriter::Seek( UINT offset )
	{
		SetFilePointer( m_fileHandle, offset, 0, FILE_BEGIN );
	}

	DLCFileWriter* DLCFileWriter::CreateWriter( const WCHAR* filePath )
	{
		// Make sure path exists
		CreatePath( filePath );

		// Remove file write protection
		DWORD fileAttr = GetFileAttributesW( filePath );
		if ( fileAttr != INVALID_FILE_ATTRIBUTES )
		{
			// Remove read-only flag only if truly read-only
			if ( (fileAttr & FILE_ATTRIBUTE_READONLY) != 0 )
			{
				fileAttr &= ~FILE_ATTRIBUTE_READONLY;
				if ( !SetFileAttributesW( filePath, fileAttr ) )
				{
					ERR( String_Error_IOReadOnly, filePath);
					return NULL;
				}
			}
		}

		// Create file
		HANDLE hFile = ::CreateFileW( filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			return new DLCFileWriter( hFile );
		}
		else
		{
			return NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////

	RawFileReader::RawFileReader( const WCHAR* fileName )
		: m_fileHandle( NULL )
		, m_baseOffset( 0 )
		, m_size( 0 )
	{
		m_fileHandle = ::CreateFileW( fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
		if ( m_fileHandle != INVALID_HANDLE_VALUE )
		{
			// Extract base file offset
			m_size = GetFileSize( m_fileHandle, NULL ) ;
		}
	}

	RawFileReader::RawFileReader( HMODULE hModuleHandle, UINT baseOffset )
		: m_fileHandle( NULL )
		, m_baseOffset( baseOffset )
		, m_size( 0 )
	{
		WCHAR moduleName[ 1024 ];
		GetModuleFileNameW( hModuleHandle, moduleName, ARRAYSIZE(moduleName) );
		m_fileHandle = ::CreateFile( moduleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );

		m_size = GetFileSize( m_fileHandle, NULL );
		m_size -= baseOffset;
		Seek( 0 );
	}

	RawFileReader::~RawFileReader()
	{
		// Close file handle
		if ( m_fileHandle )
		{
			CloseHandle( m_fileHandle );
			m_fileHandle = NULL;
		}
	}

	void RawFileReader::Serialize( void* data, UINT size )
	{
		if ( m_fileHandle )
		{
			DWORD bytesRead = 0;
			ReadFile( m_fileHandle, data, size, &bytesRead, NULL );
			if ( bytesRead != size )
			{
				ERR( String_Error_IOReadError, size, bytesRead );
			}
		}
	}

	DWORD RawFileReader::GetSize() const
	{
		return m_size;
	}

	UINT RawFileReader::GetOffset() const
	{
		return SetFilePointer( m_fileHandle, 0, 0, FILE_CURRENT ) - m_baseOffset;
	}

	void RawFileReader::Seek( UINT offset )
	{
		SetFilePointer( m_fileHandle, m_baseOffset + offset, 0, FILE_BEGIN );
	}

//////////////////////////////////////////////////////////////////////////////////////

	RawFileWriter::RawFileWriter( const WCHAR* fileName )
		: m_fileHandle( NULL )
	{
		// Make sure path exists
		CreatePath( fileName );

		// Remove file write protection
		DWORD fileAttr = GetFileAttributesW( fileName );
		if ( fileAttr != INVALID_FILE_ATTRIBUTES )
		{
			// Remove read-only flag only if truly read-only
			if ( (fileAttr & FILE_ATTRIBUTE_READONLY) != 0 )
			{
				fileAttr &= ~FILE_ATTRIBUTE_READONLY;
				if ( !SetFileAttributesW( fileName, fileAttr ) )
				{
					ERR( String_Error_IOReadOnly, fileName);
					return;
				}
			}
		}

		// Create file
		m_fileHandle = ::CreateFileW( fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	}

	RawFileWriter::~RawFileWriter()
	{
		// Close file handle
		if ( m_fileHandle )
		{
			FlushFileBuffers( m_fileHandle );
			CloseHandle( m_fileHandle );
			m_fileHandle = NULL;
		}
	}

	void RawFileWriter::Serialize( void* data, UINT size )
	{
		if ( m_fileHandle )
		{
			DWORD writtenBytes = 0;
			WriteFile( m_fileHandle, data, size, &writtenBytes, NULL );
			if ( size != writtenBytes )
			{
				ERR( String_Error_IOWriteError, size, writtenBytes );
			}
		}
	}

	DWORD RawFileWriter::GetSize() const
	{
		return GetFileSize( m_fileHandle, NULL );
	}

	UINT RawFileWriter::GetOffset() const
	{
		return SetFilePointer( m_fileHandle, 0, 0, FILE_CURRENT );
	}

	void RawFileWriter::Seek( UINT offset )
	{
		SetFilePointer( m_fileHandle, offset, 0, FILE_BEGIN );
	}

//////////////////////////////////////////////////////////////////////////////////////

	/// Read compressed DWORD
	void SerializeCompressedNumber( DLCFileStream* stream, INT& val )
	{
		INT oldVal = val;

		DWORD noSign = ( val > 0 ) ? 1 : -1;

		BYTE byt = (BYTE)((( val > 0 )?(0):(0x80) ) | ( (noSign > 0x3F) ? (0x40 | (noSign & 0x3F) ) : (noSign & 0x3F) ));
		stream->Serialize( &byt, 1 );
		BYTE firstByte = byt;

		val = 0;
		(DWORD&)val = byt & 0x3F;

		// larger or equal than 2 ^ 6
		if ( byt & 0x40 )
		{
			// more bytes than 1
			noSign >>= 6;

			byt = (BYTE)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
			stream->Serialize( &byt, 1 );
			(DWORD&)val |= ( byt & 0x7F )<< 6;

			// larger than 2 ^ 13
			if ( byt & 0x80 )
			{
				// more bytes than 2
				noSign >>= 7;

				byt = (BYTE)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
				stream->Serialize( &byt, 1 );
				(DWORD&)val |= ( byt & 0x7F )<< 13;

				// larger than 2 ^ 20
				if ( byt & 0x80 )
				{
					// more bytes than 3
					noSign >>= 7;

					byt = (BYTE)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
					stream->Serialize( &byt, 1 );
					(DWORD&)val |= ( byt & 0x7F )<< 20;

					// larger than 2 ^ 27
					if ( byt & 0x80 )
					{
						// more bytes than 4
						noSign >>= 7;

						byt = (BYTE)( (( noSign > 0x7f ) ? (0x80):(0)) | (noSign & 0x7F) );
						stream->Serialize( &byt, 1 );
						(DWORD&)val |= ( byt & 0x7F )<< 27;
					}
				}
			}
		}
		val = firstByte & 0x80 ? -val : val;
	}

	INT ReadCompressedNumber( DLCFileStream* stream )
	{
		INT value = 0;
		SerializeCompressedNumber( stream, value );
		return value;
	}

//////////////////////////////////////////////////////////////////////////////////////

}
#include "build.h"
#include "compressedFile.h"
#include "../core/memoryFileReader.h"
#include "../core/memoryFileWriter.h"
#include "../core/gameSave.h"


#ifdef RED_PLATFORM_WIN32
#	include "../../../external/dexzip/dzip.h"
#	pragma comment( lib, "../../../external/dexzip/dzip.lib" )
#	define USE_DZIP 
#endif

#ifdef RED_PLATFORM_WIN64
#	include "../../../external/dexzip/dzip.h"
#	pragma comment( lib, "../../../external/dexzip/dzip64.lib" )
#	define USE_DZIP 
#endif

#define COMPRESSION_MAGIC_LZ4 'CLZ4'
#define COMPRESSION_MAGIC_CHUNKED_LZ4 'LZ4C'

#ifdef USE_DZIP
	namespace DzipHelper
	{
		static void* __stdcall _my_dzip_allocate( size_t size )
		{
			return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Dzip, size, CalculateDefaultAlignment( size ) );
		}

		static void __stdcall _my_dzip_free( void* ptr )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_Dzip, ptr );
		}

		static void* __stdcall _my_dzip_realloc( void* ptr, size_t size )
		{
			return RED_MEMORY_REALLOCATE( MemoryPool_Default, ptr, MC_Dzip, size );
		}

		void Init()
		{
			struct Initializer
			{
				Initializer() { dzip_setallocator( _my_dzip_allocate, _my_dzip_free, _my_dzip_realloc ); }
			};

			static Initializer init;
		}
	};
#endif

///////////////////////////////////////////////////////////////////////////////

CCompressedFileWriter::CCompressedFileWriter( const String& fileName )
	: IFileEx( FF_Buffered | FF_MemoryBased | FF_Writer | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
	, m_fileName( fileName )
	, m_writer( new CMemoryFileWriter( m_data ) )
{
	
}

CCompressedFileWriter::~CCompressedFileWriter()
{
	delete m_writer;
	m_writer = NULL;

	LOG_ENGINE( TXT("CDZipFileWriter deleted, m_data size: %ld"), m_data.DataSize() );
}

void CCompressedFileWriter::Serialize( void* buffer, size_t size )
{
	m_writer->Serialize( buffer, size );
}

Uint64 CCompressedFileWriter::GetOffset() const
{
	return m_writer->GetOffset();
}

Uint64 CCompressedFileWriter::GetSize() const
{
	return m_writer->GetSize();
}

void CCompressedFileWriter::Seek( Int64 offset )
{
	m_writer->Seek( offset );
}

void CCompressedFileWriter::Close()
{
	// ! compress !
	Uint32 decompressedSize = ( Uint32 ) m_writer->GetSize();
	Red::Core::Compressor::CLZ4 compressor;
	TIMER_BLOCK( compress )
		compressor.Compress( m_data.Data(), decompressedSize );
	END_TIMER_BLOCK( compress )

	GFileManager->DeleteFile( m_fileName );
	IFile *writer = GFileManager->CreateFileWriter( m_fileName, FOF_AbsolutePath );
	ASSERT( writer );

	if ( writer )
	{
		Uint32 magic = COMPRESSION_MAGIC_LZ4;
		*writer << magic; 		
		*writer << decompressedSize; 

		writer->Serialize( ( void* ) compressor.GetResult(), compressor.GetResultSize() );
		delete writer;
	}
}

const void* CCompressedFileWriter::GetBuffer() const
{
	return m_data.Data();
}

size_t CCompressedFileWriter::GetBufferCapacity() const
{
	return m_data.Capacity();
}

///////////////////////////////////////////////////////////////////////////////

CCompressedFileReader::CCompressedFileReader( const String& fileName )
	: IFile( FF_Reader | FF_FileBased )
	, m_fileName( fileName )
	, m_reader( NULL )
{
	// FIXME: temporary implementation!
	IFile *reader = GFileManager->CreateFileReader( fileName, FOF_AbsolutePath );
	ASSERT( reader );

	// read four CC
	Uint32 fourCC = 0;
	if ( reader )
	{
		*reader << fourCC;
	}

#ifdef USE_DZIP	
	if ( fourCC == 'PIZD' )	// 'DZIP'
	{
		// yay! good old-fashioned dzip file! decompress!
		delete m_reader;
		m_data.ClearFast();

		DzipHelper::Init();
		dzip* arch = dzip_open_w( (wchar_t*)m_fileName.AsChar(), DZIP_READ );
		if ( arch )
		{
			dzip_auto_header( arch, 0 );
			Uint32 filesCount = dzip_get_num_files( arch );

			dzip_file* file = dzip_fopen( arch, 0 );

			if ( file )
			{
				size_t size = dzip_fsize( file );
				m_data.Grow( size );

				dzip_fread( m_data.Data(), size, file );
				dzip_fclose( file );
			}

			dzip_close( arch );
		}
	}
	else
#endif

	if ( fourCC == COMPRESSION_MAGIC_LZ4 )
	{
		Uint32 decompressedSize = 0;
		*reader << decompressedSize;

		const Uint32 compressedSize = ( Uint32 ) reader->GetSize() - 8;
		void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_L4Z, compressedSize, CalculateDefaultAlignment( compressedSize ) );
		if ( mem )
		{
			reader->Serialize( mem, compressedSize );			
			m_data.ResizeFast( decompressedSize );

			Red::Core::Decompressor::CLZ4 decompressor;
			decompressor.Initialize( mem, m_data.Data(), compressedSize, decompressedSize );
			decompressor.Decompress();

			RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, mem );
		}
		delete reader;
	}
	else if ( fourCC == COMPRESSION_MAGIC_CHUNKED_LZ4 )
	{
		Uint32 decompressedSize = 0;
		*reader << decompressedSize;

		const Uint32 compressedSize = ( Uint32 ) reader->GetSize() - 8;
		void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_L4Z, compressedSize, CalculateDefaultAlignment( compressedSize ) );
		if ( mem )
		{
			reader->Serialize( mem, compressedSize );			
			m_data.ResizeFast( decompressedSize );

			SChunkedLZ4Compressor< DEFAULT_LZ4_CHUNK_SZIE > decompressor( mem, ( void* ) m_data.Data(), compressedSize, decompressedSize );
			decompressor.Decompress();

			RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, mem );
		}
	}
	else
	{
		if ( reader )
		{
			reader->Seek( 0 );
			size_t fileSize = static_cast< size_t >( reader->GetSize() );
			RED_ASSERT( (Uint64)fileSize == reader->GetSize(), TXT("Unexpectedly large file '%ls'"), reader->GetFileNameForDebug() );
			m_data.ResizeFast( fileSize );
			reader->Serialize( m_data.TypedData(), fileSize );
			delete reader;
		}
	}

	m_reader = new CMemoryFileReader( m_data, 0 );
}

CCompressedFileReader::~CCompressedFileReader()
{
	delete m_reader;
	m_reader = NULL;
}

void CCompressedFileReader::Serialize( void* buffer, size_t size )
{
	m_reader->Serialize( buffer, size );
}

Uint64 CCompressedFileReader::GetOffset() const
{
	return m_reader->GetOffset();
}

Uint64 CCompressedFileReader::GetSize() const
{
	return m_reader->GetSize();
}

void CCompressedFileReader::Seek( Int64 offset )
{
	m_reader->Seek( offset );
}

///////////////////////////////////////////////////////////////////////////////

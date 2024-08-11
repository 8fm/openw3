/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "filesys.h"
#include "filePath.h"
#include "rawFileReader.h"
#include "rawFileSearcher.h"
#include "bufferedReader.h"
#include "bufferedWriter.h"
#include "memoryFileReader.h"
#include "rawFileReader.h"
#include "rawFileWriter.h"
#include "fileSystemProfiler.h"
#include "fileDecompression.h"

#include <locale.h>

CFileManager* GFileManager;
const Uint32 CFileManager::DEFAULT_BUFFER_SIZE( 4096 );

const Char CFileManager::DIRECTORY_SEPARATOR( MACRO_TXT( DIRECTORY_SEPARATOR_LITERAL ) );
const Char CFileManager::ALTERNATIVE_DIRECTORY_SEPARATOR( MACRO_TXT( ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL ) );

const Char CFileManager::DIRECTORY_SEPARATOR_STRING[] = { CFileManager::DIRECTORY_SEPARATOR, TXT( '\0' ) };

CFileManager::CFileManager( const Char* rootDirectory, const Char* baseDirectory, const Char* dataDirectory, const Char* bundleDirectory, const Bool isReadOnly )
	: m_rootDirectory( rootDirectory )
	, m_baseDirectory( baseDirectory )
	, m_dataDirectory( dataDirectory )
	, m_bundleDirectory( bundleDirectory )
	, m_userDirectory( dataDirectory )
	, m_applicationFilename( NULL )
	, m_readOnly( isReadOnly )
	, m_decompression( nullptr )
{
	// temp directory should exist only on PC platform
	m_tempDirectory = m_baseDirectory;
	m_tempDirectory += TXT("..\\temp\\");

	// log the paths
	LOG_CORE( TXT("Read only mode: %ls"), m_readOnly ? TXT("yes") : TXT("no") );
	LOG_CORE( TXT("Root path: '%ls'"), m_rootDirectory.AsChar() );
	LOG_CORE( TXT("Base path: '%ls'"), m_baseDirectory.AsChar() );
	LOG_CORE( TXT("Data path: '%ls'"), m_dataDirectory.AsChar() );
	LOG_CORE( TXT("Bundle path: '%ls'"), m_bundleDirectory.AsChar() );
	LOG_CORE( TXT("Temp path: '%ls'"), m_tempDirectory.AsChar() );

	// initialize profiling
	GFileManagerProfiler.Initilize( m_baseDirectory );

	// create decompression engine
	m_decompression = new CFileDecompression();
}

CFileManager::~CFileManager()
{
	// close decompression engine
	if ( m_decompression )
	{
		delete m_decompression;
		m_decompression = nullptr;
	}

	// close profiler (if not closed already)
	GFileManagerProfiler.Shutdown();

	// delete temporary data with application filename
	if ( m_applicationFilename )
	{
		delete m_applicationFilename;
		m_applicationFilename = NULL;
	}
}

// Set process name
void CFileManager::SetApplicationFilename( const Char* filename )
{
	if ( !filename )
	{
		return;
	}
	m_applicationFilename = new CFilePath( String( filename ) );
}

void CFileManager::HandleIOError( IFile& file, const String& message )
{
#if !defined(RED_LOGGING_ENABLED)
	RED_UNUSED( file );
	RED_UNUSED( message );
#endif
	ERR_CORE( TXT("%ls in file '%ls'"), message.AsChar(), file.GetFileNameForDebug() );
}

IFile* CFileManager::CreateFileReader( const String& path, Uint32 openFlags, Uint32 bufferSize ) const
{
	IFile* readerInterface = NULL;

	String tmp;
	if( !( openFlags & FOF_AbsolutePath ) )
	{
		tmp = m_dataDirectory + path;
	}

	const String& resolvedPath = ( openFlags & FOF_AbsolutePath ) ? path : tmp;

	// Create read interface, IO Managed or raw
	readerInterface = CRawFileReader::Create( resolvedPath.AsChar() );
	if ( readerInterface ) 
	{
		// Add buffering	
		if ( openFlags & FOF_Buffered )
		{
			// Do not create a buffering wrapper on files with direct memory access, that's pointless
			if ( readerInterface->QueryDirectMemoryAccess() == nullptr )
			{
				readerInterface = new CBufferedReader( readerInterface, bufferSize );
			}
		}

		// Add decompression
		if ( openFlags & FOF_Compressed )
		{

		}

		// Add decryption
		if ( openFlags & FOF_Encrypted )
		{

		}

		// Pre-read
		if ( openFlags & FOF_MapToMemory )
		{
			Uint32 size = (Uint32)readerInterface->GetSize();
			RED_ASSERT( (Uint64)size == readerInterface->GetSize(), TXT("Unexpectedly large file '%ls'"), readerInterface->GetFileNameForDebug() );
			CMemoryFileReaderWithBuffer* newReaderInterface = new CMemoryFileReaderWithBuffer( size );
			readerInterface->Serialize( newReaderInterface->GetData(), size );
			delete readerInterface;
			readerInterface = newReaderInterface;
		}
	}

	return readerInterface;
}

IFile* CFileManager::CreateFileWriter( const String& path, Uint32 openFlags, Uint32 bufferSize ) const
{
	String tmp;
	if( !( openFlags & FOF_AbsolutePath ) )
	{
		tmp = m_dataDirectory + path;
	}

	const String& resolvedPath = ( openFlags & FOF_AbsolutePath ) ? path : tmp;

	// Create raw writer
	Bool append = ( openFlags & FOF_Append ) ? true : false;
	IFile* writeInterface = CRawFileWriter::Create( resolvedPath.AsChar(), append );
	if ( writeInterface )
	{
		// Appending, move to the file end
		if ( openFlags & FOF_Append )
		{
			writeInterface->Seek( writeInterface->GetSize() );
		}

		// Add buffering
		if ( openFlags & FOF_Buffered )
		{
			// Do not create a buffering wrapper on files with direct memory access, that's pointless
			if ( writeInterface->QueryDirectMemoryAccess() == nullptr )
			{
				writeInterface = new CBufferedWriter( writeInterface, bufferSize );
			}
		}
	}

	// TODO: Allow wrapping it in a temporary interface (to ensure atomic file writes)
	
	return writeInterface;
}

Uint64 CFileManager::GetFileSize( const String& absoluteFilePath ) const
{
	return GSystemIO.GetFileSize( absoluteFilePath.AsChar() );
}

Red::System::DateTime CFileManager::GetFileTime( const Char* absoluteFilePath ) const
{
	return CSystemIO::GetFileTime( absoluteFilePath );
}

Red::System::DateTime CFileManager::GetFileTime( const String& absoluteFilePath ) const
{
	return CSystemIO::GetFileTime( absoluteFilePath.AsChar() );
}

void CFileManager::FindFiles( const String& baseDirectory, const String& pattern, TDynArray< String >& absoluteFilePaths, Bool recursive )
{
	// Search for files and directories under current search path
	CRawFileSearcher searcher( baseDirectory + pattern );

	// Grab file names
	for ( Uint32 i = 0; i < searcher.m_files.Size(); ++i )
	{
		const String& fileName = searcher.m_files[i];
		absoluteFilePaths.PushBack( baseDirectory + fileName );
	}

	// Recurse to subdirectories directories
	if ( recursive )
	{
		// Grab list of subdirectories
		CRawFileSearcher dirSearcher( baseDirectory + TXT("*.") );

		// Recurse down the tree
		for ( Uint32 i = 0; i < dirSearcher.m_directories.Size(); ++i )
		{
			const String& dirName = dirSearcher.m_directories[i];
			String subdir = String::Printf( TXT( "%ls%ls%c" ), baseDirectory.AsChar(), dirName.AsChar(), DIRECTORY_SEPARATOR );

			FindFiles( subdir, pattern, absoluteFilePaths, recursive );
		}
	}
}

void CFileManager::FindFilesRelative( const String& rootDirectory, const String& subDirectory, const String& pattern, TDynArray< String >& filePaths, Bool recursive )
{
	// Search for files and directories under current search path
	CRawFileSearcher searcher( rootDirectory + subDirectory + pattern );

	// Grab file names
	for ( Uint32 i = 0; i < searcher.m_files.Size(); ++i )
	{
		const String& fileName = searcher.m_files[i];
		filePaths.PushBack( subDirectory + fileName );
	}

	// Recurse to subdirectories directories
	if ( recursive )
	{
		// Grab list of subdirectories
		CRawFileSearcher dirSearcher( rootDirectory + subDirectory + TXT("*.") );

		// Recurse down the tree
		for ( Uint32 i = 0; i < dirSearcher.m_directories.Size(); ++i )
		{
			const String& dirName = dirSearcher.m_directories[i];
			String subdir = String::Printf( TXT( "%ls%ls%c" ), subDirectory.AsChar(), dirName.AsChar(), DIRECTORY_SEPARATOR );

			FindFilesRelative( rootDirectory, subdir, pattern, filePaths, recursive );
		}
	}
}

void CFileManager::FindDirectories( const String& baseDirectory, TDynArray< String >& directoryNames ) const
{
	// Grab list of subdirectories
	CRawFileSearcher dirSearcher( baseDirectory + TXT("*.") );
	directoryNames = dirSearcher.m_directories;
}

Bool CFileManager::CopyFile( const String& sourceAbsoluteFilePath, const String& destAbsoluteFilePath, Bool forceOverride )
{
	return GSystemIO.CopyFile( sourceAbsoluteFilePath.AsChar(), destAbsoluteFilePath.AsChar(), !forceOverride );
}

Bool CFileManager::MoveFile( const String& sourceAbsoluteFilePath, const String& destAbsoluteFilePath )
{
	GSystemIO.DeleteFile(destAbsoluteFilePath.AsChar());
	return GSystemIO.MoveFile( sourceAbsoluteFilePath.AsChar(), destAbsoluteFilePath.AsChar() );
}

Bool CFileManager::DeleteFile( const String& absoluteFilePath )
{
	return GSystemIO.DeleteFile( absoluteFilePath.AsChar() );
}

Bool CFileManager::IsFileReadOnly( const String& absoluteFilePath )
{
	return GSystemIO.IsFileReadOnly( absoluteFilePath.AsChar() );
}

Bool CFileManager::FileExist( const String& absoluteFilePath )
{
	return GSystemIO.FileExist( absoluteFilePath.AsChar() );
}

Bool CFileManager::SetFileReadOnly( const String& absoluteFilePath, Bool readOnlyFlag )
{
	return GSystemIO.SetFileReadOnly( absoluteFilePath.AsChar(), readOnlyFlag );
}

Bool CFileManager::CreatePath( const String& absoluteFilePath )
{
	return GSystemIO.CreatePath( absoluteFilePath.AsChar() );
}

Bool CFileManager::LoadFileToString( const String& absoluteFilePath, String& outString, Bool isAbsolutePath/*=false*/ )
{
	IFile* reader = CreateFileReader( absoluteFilePath, isAbsolutePath ? FOF_AbsolutePath : 0 );
	if( reader )
	{
		bool loadResult = LoadFileToString( reader, outString );
		
		// Close file with a success
		delete reader;
		
		return loadResult;
	}
	return false;
}

Bool CFileManager::LoadFileToString( IFile* reader, String& outString )
{
	if ( reader )
	{
		// Load file to string
		Uint32 size = static_cast< Uint32 >( reader->GetSize() );
		Uint32 allocationSize = size + 2; // + 2 magic number is to make sure last character will be \0 in unicode.
		AnsiChar* chars = reinterpret_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, sizeof( AnsiChar ) * allocationSize) ); 
		
		// sanity check
		if ( chars )
		{
			reader->Serialize( chars, size );
			chars[ size ] = 0;
			chars[ size + 1 ] = 0;

			// Detect unicode/ansi file
			if ( size >= 2 && (Uint8)(chars[0]) == 0xFF && (Uint8)(chars[1]) == 0xFE )
			{
				// Unicode file
				outString = ( Char* ) ( &chars[2] );
			}
			else
			{
				// Convert to normal string
				// Omit magic number for UTF-8 file (this is a problem for Chinese locale)
				int utfMagicNumberShift = 0;
				if ( size > 3 && (Uint8)(chars[0]) == 0xEF && (Uint8)(chars[1]) == 0xBB && (Uint8)(chars[2]) == 0xBF )
				{
					utfMagicNumberShift = 3;
				}

				outString = ANSI_TO_UNICODE( chars + utfMagicNumberShift );
			}

			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, chars );
		}
		else
		{
			return false;
		}

		return true;
	}

	// No file loaded
	return false;

}

Bool CFileManager::LoadFileToBuffer( const String& absoluteFilePath, TDynArray< Uint8 >& outBuffer, const bool appendZero /*= false*/ )
{
	IFile* file = CreateFileReader( absoluteFilePath, FOF_AbsolutePath );
	if ( !file )
		return false;

	const Bool ret = LoadFileToBuffer( file, outBuffer, appendZero );
	delete file;

	return ret;
}

Bool CFileManager::LoadFileToBuffer( IFile* file, TDynArray< Uint8 >& outBuffer, const bool appendZero /*= false*/ )
{
	// no file
	if ( !file )
		return false;

	// measure file
	const size_t fileSize = static_cast< size_t >( file->GetSize() );
	RED_ASSERT( (Uint64)fileSize == file->GetSize(), TXT("Unexpectedly large file '%ls'"), file->GetFileNameForDebug() );

	// preallocate file buffer
	const size_t memorySize = appendZero ? (fileSize+2) : fileSize; // UTF16-hack

	outBuffer.Resize( memorySize );

	// zero out the end
	if ( appendZero )
	{
		// TODO: array operator accepts only In32 :P
		outBuffer[ (Int32)(fileSize) ] = 0;
		outBuffer[ (Int32)(fileSize+1) ] = 0;
	}

	// load the data
	file->Seek(0);
	file->Serialize(outBuffer.Data(), fileSize);

	return true;
}

Bool CFileManager::SaveAnsiStringToFile( const String& absoluteFilePath, const StringAnsi& toSaveString, Bool append ) const
{
	Uint32 flags = FOF_Buffered|FOF_AbsolutePath;
	if ( append )
	{
		flags |= FOF_Append;
	}
	
	IFile* saver = CreateFileWriter( absoluteFilePath, flags );
	if ( saver )
	{
		// Save data to file
		saver->Serialize( ( void* )toSaveString.Data(), toSaveString.GetLength() );
		delete saver;
		return true;
	}

	return false;
}

Bool CFileManager::SaveStringToFile( const String& absoluteFilePath, const String& toSaveString, Bool append ) const
{
	Uint32 flags = FOF_Buffered|FOF_AbsolutePath;
	if ( append )
	{
		flags |= FOF_Append;
	}
	
	IFile* saver = CreateFileWriter( absoluteFilePath, flags );
	if ( saver )
	{
		// Save data to file
		saver->Serialize( UNICODE_TO_ANSI( toSaveString.AsChar() ), toSaveString.GetLength() );
		delete saver;
		return true;
	}

	return false;
}

Bool CFileManager::SaveStringToFileWithUTF8( const String& absoluteFilePath, const String& toSaveString ) const
{
	RED_UNUSED( absoluteFilePath );
	RED_UNUSED( toSaveString );
#ifndef RED_PLATFORM_CONSOLE
	IFile* saver = CreateFileWriter( absoluteFilePath, FOF_Buffered|FOF_AbsolutePath );
	if ( saver )
	{
		// allocate buffer of appropriate size
		Int32 bufferSize = WideCharToMultiByte( CP_UTF8, 0, toSaveString.TypedData(), static_cast< Int32 >( toSaveString.GetLength() ), nullptr, 0, 0, 0 );
		char* utf8Export = new char[ bufferSize ];

		WideCharToMultiByte( CP_UTF8, 0, toSaveString.TypedData(), static_cast< Int32 >( toSaveString.GetLength() ), utf8Export, bufferSize, 0, 0 );
		saver->Serialize( utf8Export, bufferSize );

		delete[] utf8Export;

		delete saver;
		return true;
	}
#endif
	return false;
}

Bool CFileManager::SaveStringToFileWithUTF16( IFile& file, const String& toSaveString ) const
{
	// do a copy in order not to screw the original string
	String clone( toSaveString );

	// This converts it to big endian on PC, which our engine doesn't properly read anyway
#if 0
	Char* ptr = clone.TypedData();
	for ( Uint32 i = 0; i < clone.Size(); ++i, ++ptr )
	{
		*ptr = ( (*ptr) >> 8 ) | ( (*ptr) << 8 );
	}
#endif

	Uint8 unicode16MagicNumber[2];
	unicode16MagicNumber[0] = 0xFF;
	unicode16MagicNumber[1] = 0xFE;
	file.Serialize( unicode16MagicNumber, 2 );
	file.Serialize( clone.TypedData(), clone.GetLength() * 2 );

	return true;
}

Red::System::DateTime CFileManager::GetFolderTimestamp( const String& absoluteFilePath, Bool recursive ) const
{
	Red::System::DateTime latestTimestamp;

	// Grab list of subdirectories
	CRawFileSearcher dirSearcher( absoluteFilePath + TXT("*") );

	for ( Uint32 i = 0; i < dirSearcher.m_files.Size(); ++i )
	{
		Red::System::DateTime fileTime = GetFileTime( absoluteFilePath + dirSearcher.m_files[ i ] );
		
		if ( fileTime > latestTimestamp )
		{
			latestTimestamp = fileTime;
		}
	}

	if( recursive )
	{
		for ( Uint32 i = 0; i < dirSearcher.m_directories.Size(); ++i )
		{
			String folder = String::Printf( TXT( "%ls%ls%c" ), absoluteFilePath.AsChar(), dirSearcher.m_directories[ i ].AsChar(), DIRECTORY_SEPARATOR );
			Red::System::DateTime folderTime( GetFolderTimestamp( folder, recursive ) );

			if ( folderTime > latestTimestamp )
			{
				latestTimestamp = folderTime;
			}
		}
	}

	return latestTimestamp;
}

#ifndef RED_PLATFORM_CONSOLE
String CFileManager::GenerateTemporaryFilePath() const
{
	Char path[256];
	GetTempPath(256, path);
	Red::System::GUID guid = Red::System::GUID::Create();
	Char filename[256];
	guid.ToString( filename, 256 );
	String fullPath( path );
	fullPath += TXT( "lava\\" );
	GSystemIO.CreateDirectory( fullPath.AsChar() );
	fullPath += filename;
	fullPath += TXT( ".tmp" );
	return fullPath;
}
#endif

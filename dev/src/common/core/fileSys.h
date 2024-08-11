/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _FILESYS_H
#define _FILESYS_H

#include "string.h"

// File open flags
enum EFileOpenFlags
{
	FOF_Buffered		= FLAG( 0 ),
	FOF_Compressed		= FLAG( 1 ),
	FOF_Encrypted		= FLAG( 2 ),
	FOF_Append			= FLAG( 3 ),
	FOF_DoNotIOManage	= FLAG( 4 ),
	FOF_AbsolutePath	= FLAG( 5 ),
	FOF_MapToMemory		= FLAG( 6 ),
};

class CIOManager;
class CAsyncFileReader;
class CAsyncReadToken;
class CFileDecompression;
class CFilePath;

/************************************************************************/
/* File manager															*/
/************************************************************************/
class CFileManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static const Uint32 DEFAULT_BUFFER_SIZE;
	static const Char DIRECTORY_SEPARATOR;
	static const Char DIRECTORY_SEPARATOR_STRING[];

	static const Char ALTERNATIVE_DIRECTORY_SEPARATOR;

#if defined( RED_PLATFORM_ORBIS )
#	define DIRECTORY_SEPARATOR_LITERAL						'/'
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL			'\\'

#	define DIRECTORY_SEPARATOR_LITERAL_STRING				"/"
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL_STRING	"\\"
#else
#	define DIRECTORY_SEPARATOR_LITERAL						'\\'
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL			'/'

#	define DIRECTORY_SEPARATOR_LITERAL_STRING				"\\"
#	define ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL_STRING	"/"
#endif

public:
	// Get the root directory (actual base)
	RED_INLINE const String& GetRootDirectory() const { return m_rootDirectory; }

	// TBD: Should rename "GetBinDirectory()"
	// Get game base directory ( binary file directory )
	RED_INLINE const String& GetBaseDirectory() const { return m_baseDirectory; }

	// Get game data directory
	RED_INLINE const String& GetDataDirectory() const { return m_dataDirectory; }

	// Get bundles directory
	RED_INLINE const String& GetBundleDirectory() const { return m_bundleDirectory; }

	// Get user data directory
	RED_INLINE const String& GetUserDirectory() const { return m_userDirectory; }

	// Get temporary data directory
	RED_INLINE const String& GetTempDirectory() const { return m_tempDirectory; }

	// Sets specific user data directory;
	RED_INLINE void SetUserDirectory( const Char* userDirectory ) { m_userDirectory = userDirectory; }

	// Gets the application filename, valid only on Windows
	RED_INLINE CFilePath* GetApplicationFilename() const { return m_applicationFilename; }

	// Is file system in read only mode (retail data) ?
	RED_INLINE const Bool IsReadOnly() const { return m_readOnly; }

	// Set application filename
	void SetApplicationFilename( const Char* filename );

	// Is given character a directory separator
	RED_FORCE_INLINE static Bool IsDirectorySeparator( Char c ) { return c == DIRECTORY_SEPARATOR || c == ALTERNATIVE_DIRECTORY_SEPARATOR; }

	// Get file system decompression engine
	RED_FORCE_INLINE CFileDecompression* GetDecompressionEngine() const { return m_decompression; }

public:
	CFileManager( const Char* rootDirectory, const Char* baseDirectory, const Char* dataDirectory, const Char* bundleDirectory, const Bool isReadOnly );
	RED_MOCKABLE ~CFileManager();

	// Handle IO error on file access
	void HandleIOError( IFile& file, const String& message );

	// Open a file and create file reader, does not open empty files
	IFile* CreateFileReader( const String& path, Uint32 openFlags = 0, Uint32 bufferSize = DEFAULT_BUFFER_SIZE ) const;

	// Open a file and create file writer
	IFile* CreateFileWriter( const String& path, Uint32 openFlags = 0, Uint32 bufferSize = DEFAULT_BUFFER_SIZE ) const;

	// Get size of the file
	RED_MOCKABLE Uint64 GetFileSize( const String& absoluteFilePath ) const;

	// Get file timestamp
	Red::System::DateTime GetFileTime( const Char* absoluteFilePath ) const;
	Red::System::DateTime GetFileTime( const String& absoluteFilePath ) const;

	// Get latest timestamp in folder
	Red::System::DateTime GetFolderTimestamp( const String& absoluteFilePath, Bool recursive ) const;

	// Find files at given directory using given pattern
	void FindFiles( const String& baseDirectory, const String& pattern, TDynArray< String >& absoluteFilePaths, Bool recursive );

	// Find files relative to rootDirectory at given directory (root + sub) using given pattern
	void FindFilesRelative( const String& rootDirectory, const String& subDirectory, const String& pattern, TDynArray< String >& filePaths, Bool recursive );

	// Find directories at given directory
	void FindDirectories( const String& baseDirectory, TDynArray< String >& directoryNames ) const;

	// Copy file from source location to destination location
	Bool CopyFile( const String& sourceAbsoluteFilePath, const String& destAbsoluteFilePath, Bool forceOverride );

	// Move file from source location to destination location
	Bool MoveFile( const String& sourceAbsoluteFilePath, const String& destAbsoluteFilePath );

	//! Delete file
	Bool DeleteFile( const String& absoluteFilePath );

	// Check if the file is read only
	Bool IsFileReadOnly( const String& absoluteFilePath );

	// Check if the file exist
	Bool FileExist( const String& absoluteFilePath );

	// Set or remove read only flag from file
	Bool SetFileReadOnly( const String& absoluteFilePath, Bool readOnlyFlag );

	// Create path
	Bool CreatePath( const String& absoluteFilePath );

	//! Load file to string, handles Unicode
	Bool LoadFileToString( const String& absoluteFilePath, String& outString, Bool isAbsolutePath = false );

	//! Load file to string, handles Unicode
	Bool LoadFileToString( IFile* file, String& outString );

	//! Load file to memory buffer (with optional null termination)
	Bool LoadFileToBuffer( const String& absoluteFilePath, TDynArray< Uint8 >& outBuffer, const bool appendZero = false );

	//! Load file to memory buffer (with optional null termination)
	Bool LoadFileToBuffer( IFile* file, TDynArray< Uint8 >& outBuffer, const bool appendZero = false );

	//! Save string to file
	Bool SaveAnsiStringToFile( const String& absoluteFilePath, const StringAnsi& toSaveString, Bool append = false ) const;

	//! Save string to file
	Bool SaveStringToFile( const String& absoluteFilePath, const String& toSaveString, Bool append = false ) const;

	//! Save string to file in UTF8 format
	Bool SaveStringToFileWithUTF8( const String& absoluteFilePath, const String& toSaveString ) const;

	//! Save string to file in UTF16 format
	Bool SaveStringToFileWithUTF16( IFile& file, const String& toSaveString ) const;

#ifndef RED_PLATFORM_CONSOLE
	String GenerateTemporaryFilePath() const;
#endif

protected:
	CFileManager() {} // UNIT TEST ONLY

private:
	String			m_rootDirectory;		//!< Root of entire game, one above bin
	String			m_baseDirectory;		//!< Base executable directory
	String			m_dataDirectory;		//!< Game data directory
	String			m_bundleDirectory;		//!< Bundle data directory
	String			m_userDirectory;		//!< User data directory - by default same as data directory, unless set explicitly
	String			m_tempDirectory;		//!< Temporary (local) directory
	CFilePath*		m_applicationFilename;	//!< Valid only on Win32 releases, has the application file path (eg. 'C:\Witcher2\editor.Release.exe' or 'C:\Witcher2\Witcher2.exe')
	Bool			m_readOnly;				//!< Are we in read only mode ?

	// file system decompression engine
	CFileDecompression*	m_decompression;

};

// File manager instance
extern CFileManager* GFileManager;

#endif // _FILESYS_H

#include "build.h"
#include "utils.h"

namespace DLCTool
{
	// Are we running with administrative privledges
	bool HasAdminPrivledges()
	{
		SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
		PSID AdministratorsGroup;

		// Initialize SID.
		if ( !AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup ) )
		{
			// Initializing SID Failed.
			return false;
		}

		// Check whether the token is present in admin group.
		BOOL IsInAdminGroup = FALSE;
		if ( !CheckTokenMembership( NULL, AdministratorsGroup, &IsInAdminGroup ) )
		{
			// Error occurred.
			IsInAdminGroup = FALSE;
		}

		// Free SID and return.
		FreeSid( AdministratorsGroup );
		return (IsInAdminGroup == TRUE);
	}
	
	// Check if file exists
	bool FileExists( const WCHAR* absolutePath )
	{
		HANDLE hFile = ::CreateFileW( absolutePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			CloseHandle( hFile );
			return true;
		}

		return false;
	}

	// Make sure path ends with "\"
	void FixPath( WCHAR* path, UINT pathMax )
	{
		UINT len = static_cast< UINT >( wcslen( path ) );
		if ( path[len] != '\\' || path[len] != '/' )
		{
			if ( len < pathMax )
			{
				path[ len ] = '\\';
				path[ len+1 ] = 0;
			}
		}
	}

	// Get game installation directory
	bool GetGameDirectory( WCHAR* gameDir, UINT gameDirMax )
	{
		const WCHAR* keyPath64 = TEXT("SOFTWARE\\Wow6432Node\\CD Projekt Red\\The Witcher 3");
		const WCHAR* keyPath32 = TEXT("SOFTWARE\\CD Projekt Red\\The Witcher 3");
		const WCHAR* keyName = TEXT("InstallFolder");

		// Get game registry key, new path
		HKEY gameRegistryKey;
		if(	RegOpenKeyExW( HKEY_LOCAL_MACHINE, keyPath64, 0, KEY_READ, &gameRegistryKey ) ==  ERROR_SUCCESS )
		{
			WORD registryValueType = REG_SZ;
			WORD registryValueSize = gameDirMax;
			long registryQueryResult = RegQueryValueExW( gameRegistryKey, keyName, NULL, (LPDWORD) &registryValueType, (LPBYTE) gameDir, (LPDWORD) &registryValueSize );
			if ( registryQueryResult == ERROR_SUCCESS && registryValueType == REG_SZ )
			{
				FixPath( gameDir, gameDirMax );
				RegCloseKey( gameRegistryKey );
				return true;
			}

			// Close key, no value in here
			RegCloseKey( gameRegistryKey );
		}

		// Get game registry key, old path
		if(	RegOpenKeyExW( HKEY_LOCAL_MACHINE, keyPath32, 0, KEY_READ, &gameRegistryKey ) ==  ERROR_SUCCESS )
		{
			WORD registryValueType = REG_SZ;
			WORD registryValueSize = gameDirMax;
			long registryQueryResult = RegQueryValueExW( gameRegistryKey, keyName, NULL, (LPDWORD) &registryValueType, (LPBYTE) gameDir, (LPDWORD) &registryValueSize );
			if ( registryQueryResult == ERROR_SUCCESS && registryValueType == REG_SZ )
			{
				FixPath( gameDir, gameDirMax );
				RegCloseKey( gameRegistryKey );
				return true;
			}

			// Close key, no value in here
			RegCloseKey( gameRegistryKey );
		}

		// As a last resort - if current directory contains witcher3.exe than it may be a valid directory
		WCHAR currentDirectory[ 1024 ];
		GetCurrentDirectoryW( ARRAYSIZE(currentDirectory), currentDirectory );
		FixPath( currentDirectory, ARRAYSIZE(currentDirectory) );

		// Test cast when we are in the main directory
		{
			WCHAR testFile[ 1024 ];
			wcscpy_s( testFile, ARRAYSIZE(testFile), currentDirectory );
			wcscat_s( testFile, ARRAYSIZE(testFile), TEXT("witcher3.exe") );
			if ( FileExists( testFile ) )
			{
				wcscpy_s( gameDir, gameDirMax, currentDirectory );
				wcscat_s( gameDir, gameDirMax, TEXT("..\\") );
				return true;
			}
		}

		// Test the case when we are one level up the main directory
		{
			// Test cast when we are in the main directory
			WCHAR testFile[ 1024 ];
			wcscpy_s( testFile, ARRAYSIZE(testFile), currentDirectory );
			wcscat_s( testFile, ARRAYSIZE(testFile), TEXT("bin\\witcher3.exe") );
			if ( FileExists( testFile ) )
			{
				wcscpy_s( gameDir, gameDirMax, currentDirectory );
				return true;
			}
		}

		// No game path was extracted
		return false;
	}

	// Calculate 32bit hash
	DWORD CalcBufferHash32Merge( const void* buffer, DWORD sizeInBytes, DWORD previousHash )
	{
		const BYTE* data = (const BYTE*) buffer;
		DWORD hash = previousHash;
		while( sizeInBytes-- )
		{
			hash ^= *data++;
			hash *= 0x01000193;
		}
		return hash;
	}

	// Calculate 64bit hash
	DWORD64 CalcBufferHash64Merge( const void* buffer, DWORD sizeInBytes, DWORD64 previousHash )
	{
		const BYTE* data = (const BYTE*) buffer;
		DWORD64 hash = previousHash;
		while( sizeInBytes-- )
		{
			hash ^= *data++;
			hash *= 0x100000001B3;
		}
		return hash;
	}

	// Calculate file hash
	DWORD64 CalcFileHash( DLCFileStream* stream )
	{
		DWORD64 hash = 0;

		// Get data from file
		if ( stream )
		{
			// Get file size
			stream->Seek( 0 );
			DWORD size = stream->GetSize();

			// Process in blocks
			while ( size > 0 )
			{
				BYTE buffer[ 4096 ];

				// Read data
				DWORD maxRead = min( ARRAYSIZE(buffer), size );
				stream->Serialize( buffer, maxRead );

				// Update hash
				hash = CalcBufferHash64Merge( buffer, maxRead, hash );

				// Advance file
				size -= maxRead;
			}
		}

		// Return calculated hash
		return hash;
	}

	// Get temp file path
	bool GetTempFilePath( WCHAR* path, DWORD pathMax )
	{
		// Format temporary file name
		WCHAR dirPath[ 512 ];
		if ( 0 != GetTempPath( ARRAYSIZE(dirPath), dirPath ) )
		{
			if ( 0 != GetTempFileName( dirPath, TEXT("patch"), 0, path ) )
			{
				return true;
			}
		}

		// No file path generated
		return false;
	}

	// Get temp file path
	bool GetTempFilePathEx( WCHAR* path, DWORD pathMax, const WCHAR* fileName )
	{
		// Format temporary file name
		WCHAR dirPath[ 512 ];
		if ( 0 != GetTempPath( ARRAYSIZE(dirPath), dirPath ) )
		{
			wcscpy_s( path, pathMax, dirPath );
			wcscat_s( path, pathMax, fileName );
			return true;
		}

		// No file path generated
		return false;
	}

	// Copy content from one file to the other
	void PasteFile( DLCFileStream* source, DLCFileStream* dest, DWORD sizeToCopy )
	{
		// Get size from the source file if not specified
		if ( !sizeToCopy )
		{
			sizeToCopy = source->GetSize();
		}

		// Copy data
		while ( sizeToCopy > 0 )
		{
			// Calculate how much to copy
			BYTE buffer[ 4096 ];
			DWORD maxRead = min( ARRAYSIZE(buffer), sizeToCopy );

			// Copy data
			source->Serialize( buffer, maxRead );
			dest->Serialize( buffer, maxRead );

			// Move pointer
			sizeToCopy -= maxRead;
		}
	}

	// Create directories
	void CreatePath( const WCHAR* path )
	{
		// Copy to local storage
		WCHAR localPath[ 1024 ];
		wcscpy_s( localPath, ARRAYSIZE(localPath), path );

		// Create directories
		WCHAR* pos = localPath;
		while ( *pos )
		{
			// Create path
			if ( *pos == '\\' || *pos == '/' )
			{
				WCHAR org = *pos;
				*pos = 0;

				// Handle error
				if ( ERROR_PATH_NOT_FOUND == CreateDirectory( localPath, NULL ) )
				{
					// Unable to create final path
					ERR( String_Error_IODirError, localPath );
					return;
				}

				*pos = org;
			}

			// Next char
			pos++;
		}
	}

	// Extract resource
	bool ExtractResourceFile( HINSTANCE hInstance, WORD resourceID, const WCHAR* outputFileName )
	{
		bool success = false;

		// First find and load the required resource           
		HRSRC hResource = FindResourceW( hInstance, MAKEINTRESOURCE(resourceID), TEXT("BINARY") );
		if ( hResource != NULL )
		{
			HGLOBAL hFileResource = LoadResource( hInstance, hResource );
			if ( hFileResource != NULL )
			{
				// Now open and map this to a disk file           
				LPVOID lpFile = LockResource( hFileResource );
				if ( lpFile != NULL )
				{
					DWORD dwSize = SizeofResource( hInstance, hResource );

					// Open the file and filemap           
					HANDLE hFile = CreateFile( outputFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
					if ( hFile != INVALID_HANDLE_VALUE )
					{
						HANDLE hFilemap = CreateFileMapping( hFile, NULL, PAGE_READWRITE, 0, dwSize, NULL );
						if ( hFilemap != NULL )
						{
							LPVOID lpBaseAddress = MapViewOfFile( hFilemap, FILE_MAP_WRITE, 0, 0, 0 );            
							if ( lpBaseAddress != NULL )
							{
								// Write the file
								CopyMemory( lpBaseAddress, lpFile, dwSize );

								// Done
								success = true;

								// Unmap the file and close the handles
								UnmapViewOfFile( lpBaseAddress );
							}

							// Close mapping
							CloseHandle( hFilemap ); 
						}

						// Close file handle
						CloseHandle( hFile );
					}
				}
			}
		}

		// Return state
		return success;
	} 

	bool DetectLanguage( Char* language )
	{
		const Char* REGISTRY_KEY = L"Software\\CD Projekt RED\\The Witcher 3";
		const Char* REGISTRY_KEY2 = L"Software\\Wow6432Node\\CD Projekt RED\\The Witcher 3";

		// Read language settings
		HKEY localMacine = NULL;
		DWORD ignore = 10 * sizeof( Char ), res, type = REG_SZ;
		if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGISTRY_KEY, 0, KEY_READ, &localMacine ) == ERROR_SUCCESS )
		{
			ignore = 10 * sizeof( Char );
			res = RegQueryValueEx( localMacine, TEXT("Language"), 0, &type, (BYTE*)language, &ignore );
			if ( res == ERROR_SUCCESS ) 
			{
				RegCloseKey( localMacine );
				SetLanguage( language );
				return true;
			}

			RegCloseKey( localMacine );
		}
		else if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGISTRY_KEY2, 0, KEY_READ, &localMacine ) == ERROR_SUCCESS )
		{
			ignore = 10 * sizeof( Char );
			res = RegQueryValueEx( localMacine, TEXT("Language"), 0, &type, (BYTE*)language, &ignore );
			if ( res == ERROR_SUCCESS ) 
			{
				RegCloseKey( localMacine );
				SetLanguage( language );
				return true;
			}

			RegCloseKey( localMacine );
		}

		return false;
	}

	void ToUnicode( const CHAR* src, WCHAR* dst, DWORD bufferSize )
	{
		if ( src == NULL )
		{
			return;
		}

		DWORD len = static_cast< DWORD >( ::strlen( src ) ) + 1;
		if ( len > bufferSize )
		{
			len = bufferSize;
		}

		mbsrtowcs( dst, &src, len, NULL );
	}
};
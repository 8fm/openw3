/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace DLCTool
{
	// Are we running with administrative privledges
	bool HasAdminPrivledges();

	// Get game installation directory
	bool GetGameDirectory( WCHAR* gameDir, UINT gameDirMax );

	// Check if given file exists
	bool FileExists( const WCHAR* absolutePath );

	// Make sure path ends with "\"
	void FixPath( WCHAR* path, UINT pathMax );

	// Calculate 32 bit hash
	DWORD CalcBufferHash32Merge( const void* buffer, DWORD sizeInBytes, DWORD previousHash );

	// Calculate 64 bit hash
	DWORD64 CalcBufferHash64Merge( const void* buffer, DWORD sizeInBytes, DWORD64 previousHash );

	// Calculate file hash
	DWORD64 CalcFileHash( DLCFileStream* stream );

	// Get temp file path
	bool GetTempFilePath( WCHAR* path, DWORD pathMax );

	// Get temp file path
	bool GetTempFilePathEx( WCHAR* path, DWORD pathMax, const WCHAR* fileName );

	// Paste file
	void PasteFile( DLCFileStream* source, DLCFileStream* dest, DWORD sizeToCopy );

	// Create directories
	void CreatePath( const WCHAR* path );

	// Extract file from resource
	bool ExtractResourceFile( HINSTANCE hInstance, WORD resourceID, const WCHAR* outputFileName );

	// Detect language settings
	bool DetectLanguage( Char* language );

	// To unicode
	void ToUnicode( const CHAR* src, WCHAR* dst, DWORD bufferSize );

	class RegScopedKey
	{
	public:

		HKEY m_hKey;

		RegScopedKey ( HKEY key, TCHAR* subKey )
			: m_hKey(NULL)
		{
			if (ERROR_SUCCESS != RegOpenKey(key, subKey, &m_hKey) )
				RegCreateKey( key, subKey, &m_hKey );
		}

		~RegScopedKey()
		{
			if (m_hKey != NULL)
				RegCloseKey(m_hKey);
		}
	};
}
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/crt.h"

/************************************************************************/
/* File searcher for Windows											*/
/************************************************************************/

class CRawFileSearcher
{
public:
	TDynArray< String >		m_directories;
	TDynArray< String >		m_files;

public:
	CRawFileSearcher( const String& searchPath )
	{
		for (CSystemFindFile findFile = searchPath.AsChar(); findFile; ++findFile)
		{
			// Important on the PS4 when running from local disk!
			// Otherwise infinite recursion on the current directory.
			if ( (Red::System::StringCompare( findFile.GetFileName(), TXT(".") ) == 0) ||
				 (Red::System::StringCompare( findFile.GetFileName(), TXT("..") ) == 0) )
			{
				continue;
			}

			if (findFile.IsDirectory())
			{
				// Unique cause on PS4 sceKernelGetdents returns duplicated directories when patch (like overlay APP_HOME) is applied
				m_directories.PushBackUnique(findFile.GetFileName());
			}
			else
			{
				m_files.PushBack(findFile.GetFileName());
			}
		}
#if 0
		// Find first matching file
		WIN32_FIND_DATA data;  
		HANDLE handle = FindFirstFile( searchPath.AsChar(), &data );

		// Grab matching files
		if ( handle != INVALID_HANDLE_VALUE )
		{
			do
			{
				const String fileName = data.cFileName;

				// Skip current and upper directories
				if ( fileName == TXT(".") || fileName == TXT("..") )
				{
					continue;
				}

				// Recurse to directories if requested
				if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					m_directories.PushBack( fileName );
				}
				else
				{
					m_files.PushBack( fileName );
				}

			} while ( FindNextFile( handle, &data ));

			// Close search handle
			FindClose( handle );
		}
#endif
	}
};

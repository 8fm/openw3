/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "cookedLocaleKeys.h"

class CCookedSpeechesAdapter_Legacy
{
public:

	Bool Load( IFile& file, CCookedSpeeches& cookedSpeeches )
	{
		// We can't verify version integrity, so it's ignored.
		// The number comes from the old cooker cmd line.
		Uint32 version;
		file << version;

		Uint16 fileKeyPart0;
		Uint16 fileKeyPart1;

		// First part of file key.
		file << fileKeyPart0;

		// Go to the end of file to gather speech map offset.
		Uint64 mapOffset;
		file.Seek( file.GetSize( ) - sizeof( mapOffset ) );
		file << mapOffset;

		// Second part of file key.
		file.Seek( mapOffset - sizeof( fileKeyPart1 ) );
		file << fileKeyPart1;

		cookedSpeeches.m_fileKey = fileKeyPart1 | ( fileKeyPart0 << 16 );
		cookedSpeeches.m_langKey = CookedLocaleKeys::GetLanguageKey( cookedSpeeches.m_fileKey );

		// File key is corrupted!
		if( cookedSpeeches.m_fileKey != 0 && cookedSpeeches.m_langKey == 0 )
		{
			cookedSpeeches.m_fileKey = 0;
			return false;
		}

		// Read strings keys map
		file << cookedSpeeches.m_offsetMap;

		// Go figure..
		if( cookedSpeeches.m_offsetMap.Empty( ) )
		{
			cookedSpeeches.m_fileKey = 0;
			cookedSpeeches.m_langKey = 0;
			return false;
		}

		return true;
	}
};
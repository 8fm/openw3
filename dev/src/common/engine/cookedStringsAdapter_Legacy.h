/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "cookedLocaleKeys.h"

class CCookedStringsAdapter_Legacy
{
public:

	Bool Load( IFile& file, CCookedStrings& cookedStrings )
	{
		// We can't verify version integrity, so it's ignored.
		// The number comes from the old cooker cmd line.
		Uint32 version;
		file << version;

		Uint16 fileKeyPart0;
		Uint16 fileKeyPart1;

		// First part of file key.
		file << fileKeyPart0;

		// Manually serialize keys.
		{
			Uint32 sortedMapSize = 0;
			file << CCompressedNumSerializer( sortedMapSize );

			// Probably corrupted, or invalid.
			if( sortedMapSize == 0 )
				return false;

			String key;
			Uint32 stringId;

			cookedStrings.m_keysMap.Reserve( sortedMapSize );
			for( Uint32 i = 0; i < sortedMapSize; ++i )
			{
				file << key;
				file << stringId;

				// Ignore invalid entries.
				if( static_cast< Int32 >( stringId ) <= 0 )
					continue;

				cookedStrings.AddKey( key, stringId );
			}
			cookedStrings.m_keysMap.Resort( );
		}

		// Second part of file key.
		file << fileKeyPart1;

		cookedStrings.m_fileKey = fileKeyPart1 | ( fileKeyPart0 << 16 );
		cookedStrings.m_langKey = CookedLocaleKeys::GetLanguageKey( cookedStrings.m_fileKey );

		// File key is corrupted!
		if( cookedStrings.m_fileKey != 0 && cookedStrings.m_langKey == 0 )
		{
			cookedStrings.m_fileKey = 0;
			cookedStrings.m_keysMap.Clear( );
			return false;
		}

		// Checksum (not used).
		Uint32 checksum;
		file << checksum;
		checksum ^= cookedStrings.m_langKey;
		RED_UNUSED( checksum );

		// Manually serialize strings.
		{
			Uint32 sortedMapSize = 0;
			file << CCompressedNumSerializer( sortedMapSize );

			Uint32 stringId;
			String text;

			cookedStrings.m_offsetMap.Reserve( sortedMapSize );
			for( Uint32 i = 0; i < sortedMapSize; ++i )
			{
				file << stringId;
				file << text;

				// Ignore invalid entries.
				if( static_cast< Int32 >( stringId ) <= 0 )
					continue;

				// Turn them into a plain (text,id) pair and it to the map.
				text.Validate( ( Uint16 )( cookedStrings.m_langKey >> 8 ) & 0x0000FFFF );
				cookedStrings.AddString( text, stringId ^ cookedStrings.m_langKey );
			}
			cookedStrings.m_offsetMap.Resort( );
		}

		return true;
	}
};
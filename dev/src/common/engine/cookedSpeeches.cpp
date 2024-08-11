/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cookedSpeeches.h"
#include "cookedSpeechesAdapter_Legacy.h"
#include "cookedLocaleKeys.h"

const Uint32 CCookedSpeeches::FILE_MAGIC = 'WSPC';
const Uint32 CCookedSpeeches::FILE_VERSION = VER_CURRENT;

CCookedSpeeches::CCookedSpeeches( )
: m_fileKey( 0 )
{
#ifdef RED_FINAL_BUILD
	m_fileKey = 0x54384368;
#endif
}

Bool CCookedSpeeches::Load( IFile& file )
{
	const Uint64 baseOffset = file.GetOffset( );

	SHeader header;
	file.Serialize( &header, sizeof( header ) );

	// Maybe it's in the old format.
	if( header.m_magic != FILE_MAGIC )
	{
		WARN_ENGINE( TXT( "Cooked speeches file magic is invalid (%08X != %08X), trying old format..." ), header.m_magic, FILE_MAGIC );
		file.Seek( baseOffset );
		CCookedSpeechesAdapter_Legacy adapter;
		if( !adapter.Load( file, *this ) )
		{
			WARN_ENGINE( TXT( "Failed to read cooked speeches file!" ) );
			return false;
		}
		return true;
	}

	if( header.m_version > VER_CURRENT )
	{
		WARN_ENGINE( TXT( "Cooked speeches file version is from a newer engine build (%d > %d)!" ), header.m_version, VER_CURRENT );
		return false;
	}

	Uint16 fileKeyPart0;
	Uint16 fileKeyPart1;

	// First part of file key.
	file << fileKeyPart0;

	m_offsetMap.SerializeBulk( file );

	// Second part of file key.
	file << fileKeyPart1;

	m_fileKey = fileKeyPart1 | ( fileKeyPart0 << 16 );
	m_langKey = CookedLocaleKeys::GetLanguageKey( m_fileKey );

	return true;
}

Bool CCookedSpeeches::Save( IFile& file, IFile& srcFile )
{
	SHeader header;
	header.m_magic = FILE_MAGIC;
	header.m_version = VER_CURRENT;

	file.Serialize( &header, sizeof( header ) );

	Uint16 fileKeyPart = ( m_fileKey >> 16 ) & 0x0000FFFF;
	file << fileKeyPart;

	m_offsetMap.Resort( );

	Uint64 fileOffset = file.GetOffset( );

	m_offsetMap.SerializeBulk( file );

	fileKeyPart = m_fileKey & 0x0000FFFF;
	file << fileKeyPart;

	for( TOffsetMap::iterator it = m_offsetMap.Begin( ); it != m_offsetMap.End( ); ++it )
	{
		SOffset& data = it->m_second;
		if( data.m_voSize != 0 )
		{
			Uint8* buffer = new Uint8 [ data.m_voSize ];
			srcFile.Seek( data.m_voOffset );
			srcFile.Serialize( buffer, data.m_voSize );
			data.m_voOffset = file.GetOffset( );
			file.Serialize( buffer, data.m_voSize );
			delete [ ] buffer;
		}
		if( data.m_lipsyncSize != 0 )
		{
			Uint8* buffer = new Uint8 [ data.m_lipsyncSize ];
			srcFile.Seek( data.m_lipsyncOffset );
			srcFile.Serialize( buffer, data.m_lipsyncSize );
			data.m_lipsyncOffset = file.GetOffset( );
			file.Serialize( buffer, data.m_lipsyncSize );
			delete [ ] buffer;
		}
	}

	file.Seek( fileOffset );
	m_offsetMap.SerializeBulk( file );

	return true;
}

void CCookedSpeeches::SetLanguage( const String& lang )
{
	CookedLocaleKeys::GetSecureKeys( lang, m_fileKey, m_langKey );
}

void CCookedSpeeches::AddOffset( const SOffset& speechOffset, TSpeechId speechId )
{
	m_offsetMap.BulkInsert( speechId ^ m_langKey, speechOffset );
}

Bool CCookedSpeeches::GetOffset( TSpeechId speechId, SOffset& speechOffset ) const
{
	speechId ^= m_langKey;
	return m_offsetMap.Find( speechId, speechOffset );
}
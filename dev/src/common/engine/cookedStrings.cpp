/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cookedStrings.h"
#include "cookedStringsAdapter_Legacy.h"
#include "cookedLocaleKeys.h"

const Uint32 CCookedStrings::FILE_MAGIC = 'WSTR';
const Uint32 CCookedStrings::FILE_VERSION = VER_CURRENT;

CCookedStrings::CCookedStrings( )
: m_fileKey( 0 )
{
#ifdef RED_FINAL_BUILD
	m_fileKey = 0x58387139;
#endif
}

Bool CCookedStrings::Load( IFile& file )
{
	const Uint64 baseOffset = file.GetOffset( );

	SHeader header;
	file.Serialize( &header, sizeof( header ) );

	// Maybe it's in the old format.
	if( header.magic != FILE_MAGIC )
	{
		WARN_ENGINE( TXT( "Cooked strings file magic is invalid (%08X != %08X), trying old format..." ), header.magic, FILE_MAGIC );
		file.Seek( baseOffset );
		CCookedStringsAdapter_Legacy adapter;
		if( !adapter.Load( file, *this ) )
		{
			WARN_ENGINE( TXT( "Failed to read cooked strings file!" ) );
			return false;
		}
		return true;
	}

	if( header.version > VER_CURRENT )
	{
		WARN_ENGINE( TXT( "Cooked strings file version is from a newer engine build (%d > %d)!" ), header.version, VER_CURRENT );
		return false;
	}

	Uint16 fileKeyPart0;
	Uint16 fileKeyPart1;

	// First part of file key.
	file << fileKeyPart0;

	m_offsetMap.SerializeBulk( file );
	m_keysMap.SerializeBulk( file );
	m_strings.SerializeBulk( file );

	// Second part of file key.
	file << fileKeyPart1;

	m_fileKey = fileKeyPart1 | ( fileKeyPart0 << 16 );
	m_langKey = CookedLocaleKeys::GetLanguageKey( m_fileKey );

	return true;
}

Bool CCookedStrings::Save( IFile& file )
{
	const Uint64 baseOffset = file.GetOffset( );

	SHeader header;
	header.magic = FILE_MAGIC;
	header.version = VER_CURRENT;

	file.Serialize( &header, sizeof( header ) );

	Uint16 fileKeyPart = ( m_fileKey >> 16 ) & 0x0000FFFF;
	file << fileKeyPart;

	m_offsetMap.Resort( );
	m_keysMap.Resort( );

	m_offsetMap.SerializeBulk( file );
	m_keysMap.SerializeBulk( file );
	m_strings.SerializeBulk( file );

	fileKeyPart = m_fileKey & 0x0000FFFF;
	file << fileKeyPart;

	return true;
}

void CCookedStrings::SetLanguage( const String& lang )
{
	CookedLocaleKeys::GetSecureKeys( lang, m_fileKey, m_langKey );
}

void CCookedStrings::AddString( const String& string, TStringId stringId )
{
	if ( string.Empty( ) )
		return;

	String tempString = string;
	tempString.Validate( ( Uint16 )( m_langKey >> 8 ) & 0x0000FFFF );

	const Uint32 length = tempString.Size( );
	Uint32 offset = m_strings.Size( );
	m_strings.Grow( length );
	Red::MemoryCopy( &m_strings[ offset ], tempString.AsChar( ), sizeof( Char ) * length );

	m_offsetMap.BulkInsert( stringId ^ m_langKey, SStringDescriptor( offset, length - 1 ) );
}

void CCookedStrings::AddKey( const String& key, TStringId stringId )
{
	if( key.Empty( ) )
		return;

	TKeyId hash = key.CalcHash( );
	AddKey( hash, stringId );
}

void CCookedStrings::AddKey( TKeyId keyId, TStringId stringId )
{
	m_keysMap.BulkInsert( keyId, stringId ^ m_langKey );
}

Bool CCookedStrings::GetString( TStringId stringId, String& string ) const
{
	stringId ^= m_langKey;

	SStringDescriptor descriptor;
	if( !m_offsetMap.Find( stringId, descriptor ) )
		return false;

	string = String( &m_strings[ descriptor.offset ], descriptor.length );
	string.Validate( ( Uint16 )( m_langKey >> 8 ) & 0x0000FFFF );

	return true;
}

Bool CCookedStrings::GetStringIdByKey( const String& key, TStringId& stringId ) const
{
	TKeyId keyId = key.CalcHash( );
	return GetStringIdByKey( keyId, stringId );
}

Bool CCookedStrings::GetStringIdByKey( TKeyId keyId, TStringId& stringId ) const
{
	if( m_keysMap.Find( keyId, stringId ) )
	{
		stringId ^= m_langKey;
		return true;
	}
	return false;
}

Bool CCookedStrings::GetKeyByStringId( TStringId stringId, TKeyId& keyId )
{
	CCookedStrings::TKeysMap::const_iterator keyIt = m_keysMap.FindByValue( stringId ^ m_langKey );
	if( keyIt != m_keysMap.End( ) )
	{
		keyId = keyIt->m_first;
		return true;
	}
	return false;
}
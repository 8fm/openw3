/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CCookedStrings
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	static const Uint32 FILE_MAGIC;
	static const Uint32 FILE_VERSION;

	struct SHeader
	{
		Uint32	magic;
		Uint32	version;
	};

	typedef Uint32 TStringId;
	typedef Uint32 TStringOffset;
	typedef Uint32 TKeyId;

	// We need to store the length of the strings, cause after
	// encryption some chars might get turned into 0x00 0x00.

	struct SStringDescriptor
	{
		TStringOffset	offset;
		Uint32			length;

		SStringDescriptor( ) : offset( 0 ), length( 0 ) { }
		SStringDescriptor( TStringOffset o, Uint32 l )
			: offset( o ), length( l ) { }
	};

	// Strings and string id's are stored encrypted.
	// For keys, their plain hashed value is stored.

	typedef TSortedMap< TStringId, SStringDescriptor > TOffsetMap;
	typedef TSortedMap< TKeyId, TStringId > TKeysMap;
	typedef TDynArray< Char > TStringTable;

public:

	TOffsetMap		m_offsetMap;
	TKeysMap		m_keysMap;
	TStringTable	m_strings;

	Uint32			m_fileKey;
	Uint32			m_langKey;

public:

	CCookedStrings( );

	Bool Load( IFile& file );
	Bool Save( IFile& file );

	void SetLanguage( const String& lang );

	// Input & output for these functions is not encrypted.

	void AddString( const String& string, TStringId stringId );
	void AddKey( const String& key, TStringId stringId );
	void AddKey( TKeyId keyId, TStringId stringId );

	Bool GetString( TStringId stringId, String& string ) const;
	Bool GetStringIdByKey( const String& key, TStringId& stringId ) const;
	Bool GetStringIdByKey( TKeyId keyId, TStringId& stringId ) const;
	Bool GetKeyByStringId( TStringId stringId, TKeyId& keyId );
};
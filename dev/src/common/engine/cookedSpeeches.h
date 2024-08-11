/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CCookedSpeeches
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	static const Uint32 FILE_MAGIC;
	static const Uint32 FILE_VERSION;

	struct SHeader
	{
		Uint32	m_magic;
		Uint32	m_version;
	};

	struct SOffset
	{
		Uint64 m_voOffset;
		Uint64 m_voSize;
		Uint64 m_lipsyncOffset;
		Uint64 m_lipsyncSize;

		SOffset( )
			: m_voOffset( 0 )
			, m_voSize( 0 )
			, m_lipsyncOffset( 0 )
			, m_lipsyncSize( 0 )
		{ }

		friend IFile& operator << ( IFile& file, SOffset& offset )
		{
			file << offset.m_voOffset;
			file << offset.m_voSize;
			file << offset.m_lipsyncOffset;
			file << offset.m_lipsyncSize;
			return file;
		}
	};

	typedef Uint32 TSpeechId;
	typedef TSortedMap< TSpeechId, SOffset > TOffsetMap;

public:

	TOffsetMap	m_offsetMap;
	Uint32		m_fileKey;
	Uint32		m_langKey;

public:

	CCookedSpeeches( );

	Bool Load( IFile& file );
	Bool Save( IFile& file, IFile& srcFile );

	void SetLanguage( const String& lang );

	void AddOffset( const SOffset& speechOffset, TSpeechId speechId );
	Bool GetOffset( TSpeechId speechId, SOffset& speechOffset ) const;

	Bool IsEmpty() const { return m_offsetMap.Empty(); }
};
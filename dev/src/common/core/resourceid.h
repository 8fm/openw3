/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef _RED_RESOURCE_ID_H_
#define _RED_RESOURCE_ID_H_

#include "../redSystem/hash.h"
#include "stringConversion.h"

namespace Red
{
	namespace Core
	{
		namespace ResourceManagement
		{
			class CResourceId
			{
			public:
				static const AnsiChar SEPERATOR = ':';
				static const Uint32 NUM_PARTS_64 = CHash128::NUM_PARTS_64;
				static const Uint32 ID_STRING_BUFFER_SIZE = 38;
			public:
				RED_INLINE CResourceId() {}
				RED_INLINE explicit CResourceId( const String& path ) { Generate( path ); }
				RED_INLINE explicit CResourceId( const StringAnsi& path ) { Generate( path ); }

				RED_INLINE ~CResourceId() {}

				RED_INLINE void Generate( const String& path )
				{
					Generate( UNICODE_TO_ANSI( path.AsChar() ) );
				}

				RED_INLINE void Generate( const StringAnsi& path )
				{
					Generate( path.AsChar() );
				}

				RED_INLINE void Generate( const AnsiChar* path )
				{
					size_t pathLength = System::StringLength( path );
					size_t pathSize = pathLength + 1;

					AnsiChar* normalizedPath = static_cast< AnsiChar* >( RED_ALLOCA( pathSize ) );
					System::StringCopy( normalizedPath, path, pathSize );

					for( Uint32 i = 0; i < pathLength; ++i )
					{
						switch( normalizedPath[ i ] )
						{
						case '\\':
						case '/':
							normalizedPath[ i ] = SEPERATOR;
						}
					}
					// Since we already know the size, we cast to void* to explicitly state the version of the function we want to call
					m_hash = System::CalculateHash128( static_cast< const void* >( normalizedPath ), pathLength );
				}

				RED_INLINE CResourceId& operator=( const CResourceId& other )
				{
					m_hash = other.m_hash;
					return *this;
				}

				RED_INLINE Bool operator==( const CResourceId& other ) const
				{
					return m_hash == other.m_hash;
				}

				RED_INLINE Bool operator==( const CResourceId& other )
				{
					return m_hash == other.m_hash;
				}

				RED_INLINE Bool operator<( const CResourceId& other ) const
				{
					return m_hash < other.m_hash;
				}

				RED_INLINE Uint64 operator[]( Uint32 index ) const
				{
					return m_hash[ index ];
				}

				RED_INLINE Uint64& operator[]( Uint32 index )
				{
					return m_hash[ index ];
				}

				RED_INLINE Bool IsZero() const
				{
					return m_hash.IsZero();
				}

				RED_INLINE Bool ToString( AnsiChar* buf, const Uint32 bufSize = ID_STRING_BUFFER_SIZE ) const
				{
					return m_hash.ToString( buf, bufSize );
				}

				RED_INLINE Bool FromString( const AnsiChar*& buf )
				{
					const AnsiChar* ptr = buf;

					Uint64 words[ 4 ];

					for (int i=0; i<4; ++i)
					{
						if ( !GParseHex( ptr, words[i] ) ) return false;

						if ( i >= 0 && i <= 2 )
							if ( !GParseKeyword( ptr, "-" ) ) return  false;
					}

					m_hash[0] = (words[0] << 32) | words[1];
					m_hash[1] = (words[2] << 32) | words[3];
					return true;
				}

				RED_FORCE_INLINE Uint32 CalcHash() const
				{
					ASSERT( !( sizeof( System::CHash128 ) & 3 ) );
					return Red::System::CalculateHash32FromUint32Array( (const Uint32*) &m_hash, sizeof( System::CHash128 ) >> 2 );;
				}

			private:
				System::CHash128	m_hash;
			};
		};
	}
}

#endif // _RED_RESOURCE_ID_H_

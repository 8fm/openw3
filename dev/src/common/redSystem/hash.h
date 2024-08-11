/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_HASH_H_
#define _RED_HASH_H_

#include "types.h"

// For StringLength
#include "crt.h"

// For RotateLeft
#include "bitUtils.h"

// For __m128i
#if defined( RED_PLATFORM_WINPC )
#include <emmintrin.h>
#endif

#define RED_FNV_OFFSET_BASIS32	( 2166136261u )
#define RED_FNV_PRIME32			( 16777619u )

#define RED_FNV_OFFSET_BASIS64	( 14695981039346656037u )
#define RED_FNV_PRIME64			( 1099511628211u )

#define RED_MURMURHASH_SEED		( 0x5eedba5e )

namespace Red
{
	namespace System
	{
		typedef Uint32 THash32;
		typedef Uint64 THash64;

		// Algorithm: Murmur32
		RED_FORCE_INLINE THash32 CalculatePathHash32( const AnsiChar* path )
		{
			const Uint32 m = 0x5bd1e995;
			const Int32 r = 24;

			// Initialize the hash to a 'random' value
			Uint32 len = (Uint32) Red::StringLength( path );
			Uint32 h = RED_MURMURHASH_SEED ^ len;

			// Mix 4 bytes at a time into the hash
			const Uint8* data = (const Uint8*) path;
			while (len >= 4)
			{
				Uint32 k = *(const Uint32*)data;

				k *= m; 
				k ^= k >> r; 
				k *= m; 

				h *= m; 
				h ^= k;

				data += 4;
				len -= 4;
			}

			// Handle the last few bytes of the input array
			switch(len)
			{
				case 3: h ^= data[2] << 16;
				case 2: h ^= data[1] << 8;
				case 1: h ^= data[0];
					h *= m;
			};

			// Do a few final mixes of the hash to ensure the last few
			// bytes are well-incorporated.

			h ^= h >> 13;
			h *= m;
			h ^= h >> 15;

			return h;
		}

		// Algorithm: Murmur32
		RED_FORCE_INLINE THash32 CalculatePathHash32( const Char* path )
		{
			AnsiChar buf[ 512 ];

			// ultra lame unicode->Ansi conversion
			const Char* read = path;
			AnsiChar* write = buf;
			while ( *read && write < &buf[ ARRAY_COUNT_U32(buf)-1 ] )
			{
				*write++ = (AnsiChar) *read++;
			}
			*write = 0;

			// calculate path from ANSI string
			return CalculatePathHash32( buf );
		}

		// Algorithm: FNV-1a
		// Calculate hash for a path string (it will normalize it)
		RED_FORCE_INLINE THash64 CalculatePathHash64( const AnsiChar* buffer )
		{
			const Uint8* data = (const Uint8*)( buffer );

			THash64 hash = RED_FNV_OFFSET_BASIS64;
			while ( *data )
			{
				Uint8 ch = *data++;

				// normalize the path
				if ( ch >= 'A' && ch <= 'Z' )
					ch = (ch - 'A') + 'a';
				else if ( ch == '/' )
					ch = '\\';

				hash ^= ch;
				hash *= RED_FNV_PRIME64;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		RED_FORCE_INLINE THash64 CalculatePathHash64( const Char* path )
		{
			AnsiChar buf[ 512 ];

			// ultra lame unicode->Ansi conversion
			const Char* read = path;
			AnsiChar* write = buf;
			while ( *read && write < &buf[ ARRAY_COUNT_U32(buf)-1 ] )
			{
				*write++ = (AnsiChar) *read++;
			}
			*write = 0;

			// calculate path from ANSI string
			return CalculatePathHash64( buf );
		}

		// Algorithm: FNV-1a
		// Calculated at run time
		RED_FORCE_INLINE THash32 CalculateHash32( const void* buffer, size_t bufferSize, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			const Uint8* data = static_cast< const Uint8* >( buffer );
			THash32 hash = baseHash;
			while( bufferSize-- )
			{
				hash ^= *data++;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculated at run time; bufferLength indicates number of Uint32 elements in buffer
		RED_FORCE_INLINE THash32 CalculateHash32FromUint32Array( const Uint32* buffer, size_t bufferLength, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			THash32 hash = baseHash;
			while( bufferLength-- )
			{
				hash ^= *buffer++;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculated at run time
		RED_FORCE_INLINE THash32 CalculateHash32( const AnsiChar* str, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			return CalculateHash32( str, StringLength( str ) + 1, baseHash );
		}

		// Algorithm: FNV-1a
		// Calculate ANSI string hash
		RED_FORCE_INLINE THash32 CalculateAnsiHash32( const AnsiChar* buffer, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			THash32 hash = baseHash;
			while ( *buffer )
			{
				hash ^= *buffer++;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculate lower case ANSI string hash
		RED_FORCE_INLINE THash32 CalculateAnsiHash32LowerCase( const AnsiChar* buffer, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			THash32 hash = baseHash;
			while ( *buffer )
			{
				THash32 val = *buffer++;
				if ( val >= 'A' && val <= 'Z') 
					val = (val - 'A') + 'a';

				hash ^= val;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculate ANSI string hash (will break if using unicode)
		RED_FORCE_INLINE THash32 CalculateAnsiHash32( const Char* buffer, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			THash32 hash = baseHash;
			while ( *buffer )
			{
				hash ^= *buffer++;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculate lower case ANSI string hash (will break if using unicode)
		RED_FORCE_INLINE THash32 CalculateAnsiHash32LowerCase( const Char* buffer, THash32 baseHash = RED_FNV_OFFSET_BASIS32 )
		{
			THash32 hash = baseHash;
			while ( *buffer )
			{
				THash32 val = *buffer++;
				if ( val >= 'A' && val <= 'Z') 
					val = (val - 'A') + 'a';

				hash ^= val;
				hash *= RED_FNV_PRIME32;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculated at run time
		RED_FORCE_INLINE THash64 CalculateHash64( const void* buffer, size_t bufferSize, THash64 baseHash = RED_FNV_OFFSET_BASIS64 )
		{
			const Uint8* data = static_cast< const Uint8* >( buffer );
			THash64 hash = baseHash;
			while( bufferSize-- )
			{
				hash ^= *data++;
				hash *= RED_FNV_PRIME64;
			}

			return hash;
		}

		// Algorithm: FNV-1a
		// Calculated at run time
		RED_FORCE_INLINE THash64 CalculateHash64( const AnsiChar* str, THash64 baseHash = RED_FNV_OFFSET_BASIS64 )
		{
			return CalculateHash64( str, StringLength( str ) + 1, baseHash );
		}

		// Algorithm: FNV-1a
		// Calculated at run time
		RED_FORCE_INLINE THash64 CalculateHash64SkipWhitespaces( const AnsiChar* str, size_t bufferSize, THash64 baseHash = RED_FNV_OFFSET_BASIS64 )
		{
			const AnsiChar* data = str;
			THash64 hash = baseHash;
			while( bufferSize-- )
			{
				AnsiChar c = *data++;
				if ( c != ' ' && c != '\t' && c != '\r' && c != '\n' )
				{
					hash ^= c;
					hash *= RED_FNV_PRIME64;
				}
			}
			return hash;
		}

		// Algorithm: FNV-1a
		// Calculated at compile time
		class CompileTimeHash32
		{
		private:
			template < unsigned int N, unsigned int I >
			struct Fnv
			{
				RED_FORCE_INLINE static unsigned int Calc( const AnsiChar(& str)[ N ] )
				{
					return ( Fnv< N, I - 1 >::Calc( str ) ^ ( str[ I - 1 ] ) ) * RED_FNV_PRIME32;
				}
			};

			template < unsigned int N >
			struct Fnv< N, 1 >
			{
				RED_FORCE_INLINE static unsigned int Calc( const AnsiChar(& str)[ N ] )
				{
					return ( RED_FNV_OFFSET_BASIS32 ^ str[ 0 ] ) * RED_FNV_PRIME32;
				}
			};

		public:

			// Algorithm: FNV-1a
			// Calculated at compile time
			template < unsigned int N >
			RED_FORCE_INLINE CompileTimeHash32( const AnsiChar(& str)[ N ] )
				: m_hash( Fnv< N, N >::Calc( str ) )
			{
			}

		public:
			THash32 m_hash;
		};

// This is disabled as we don't have support for console platforms yet - so we use the basic non-smid implementation.
//#define RED_HASH_128_SIMD

		// Algorithm: Murmurhash3
		// Calculated at run time
		// Adapted from the source code obtained at https://code.google.com/p/smhasher/
		class CHash128
		{
		protected:
#ifdef RED_HASH_128_SIMD
#	if defined( RED_COMPILER_MSC )

			struct SHash128Simd
			{
				__m128i m_value;

				RED_INLINE SHash128Simd()
				:	m_value( _mm_setzero_si128() )
				{}

				RED_INLINE SHash128Simd( const SHash128Simd& other )
				:	m_value( other.m_value )
				{
				}

				// Comparisons
				RED_INLINE Bool operator==( const SHash128Simd& other ) const
				{
					// SIMD4.1
					// return _mm_testc_si128( m_value, other.m_value ) == 1;
					
					// SIMD2
					__m128i result = _mm_cmpeq_epi32( m_value, other.m_value );
					return _mm_movemask_epi8( result ) == 0xffff;
				}

				RED_INLINE Bool operator<( const SHash128Simd& other ) const
				{
					// SIMD2
					// Compare each as a set of 4 int32s
					__m128i lt	= _mm_cmplt_epi32( m_value, other.m_value );
					__m128i gt	= _mm_cmpgt_epi32( m_value, other.m_value );

					// _mm_movemask_epi8 seems to put the most significant part into the least significant bits of the mask...
					int ltmask	= _mm_movemask_epi8( lt );
					int gtmask	= _mm_movemask_epi8( gt );
					
					for( Uint32 i = 0; i < ( sizeof( __m128i ) / sizeof( int ) ); ++i )
					{
						// So we test the 8421 bits
						if( ltmask & 0x0000000f )
						{
							return true;
						}
						else if( gtmask & 0x0000000f )
						{
							return false;
						}

						// And shift right upon equality for the next lesser significant bits
						ltmask >>= 4;
						gtmask >>= 4;
					}

					return false;
				}

				RED_INLINE Uint64 operator[]( Uint32 index ) const
				{
					return reinterpret_cast< const Uint64* >( &m_value )[ index ];
				}

				RED_INLINE Uint64& operator[]( Uint32 index )
				{
					return reinterpret_cast< Uint64* >( &m_value )[ index ];
				}
			};

			typedef SHash128Simd SHash128;
#	else
#	error CHash128 internal type not implemented for this compiler
#	endif
#else
			struct SHash128Std
			{
				static const Uint32 NUM_PARTS = 2u;
				static const Uint32 SIZE = NUM_PARTS * sizeof( Uint64 );

				Uint64 m_parts[ NUM_PARTS ];

				RED_INLINE SHash128Std()
				{
					MemoryZero( m_parts, SIZE );
				}

				RED_INLINE SHash128Std( const SHash128Std& other )
				{
					MemoryCopy( m_parts, other.m_parts, SIZE );
				}

				RED_INLINE Bool operator==( const SHash128Std& other ) const
				{
					return MemoryCompare( &m_parts, &other.m_parts, SIZE ) == 0;
				}

				RED_INLINE Bool operator<( const SHash128Std& other ) const
				{
					for( Uint32 i = 0; i < NUM_PARTS; ++i )
					{
						if( m_parts[ i ] < other.m_parts[ i ] )
						{
							// Less than
							return true;
						}
						else if( m_parts[ i ] > other.m_parts[ i ] )
						{
							// Greater than
							return false;
						}
					}

					// All parts are equal
					return false;
				}

				RED_INLINE Uint64 operator[]( Uint32 index ) const
				{
					return m_parts[ index ];
				}

				RED_INLINE Uint64& operator[]( Uint32 index )
				{
					return m_parts[ index ];
				}
			};

			typedef SHash128Std SHash128;

#endif // RED_HASH_128_SIMD

		public:
			static const Uint32 NUM_PARTS_64 = 2u;

		protected:
			static const Uint64 BIG_CONSTANT_ONE	= 0xff51afd7ed558ccd;
			static const Uint64 BIG_CONSTANT_TWO	= 0xc4ceb9fe1a85ec53;
			static const Uint64 BIG_CONSTANT_THREE	= 0x87c37b91114253d5;
			static const Uint64 BIG_CONSTANT_FOUR	= 0x4cf5ad432745937f;
			static const size_t BLOCK_SIZE			= sizeof( SHash128 );

		protected:
			SHash128 m_hash;

		public:
			RED_INLINE CHash128() {}
			RED_INLINE CHash128( const void* buffer, size_t len, Uint32 seed = RED_MURMURHASH_SEED ) { Calculate( buffer, len, seed ); }
			RED_INLINE CHash128( const AnsiChar* buffer, Uint32 seed = RED_MURMURHASH_SEED ) { Calculate( buffer, StringLength( buffer ) * sizeof( AnsiChar ), seed ); }
			RED_INLINE CHash128( const UniChar* buffer, Uint32 seed = RED_MURMURHASH_SEED ) { Calculate( buffer, StringLength( buffer ) * sizeof( UniChar ), seed ); }
			RED_INLINE CHash128( const CHash128& other ): m_hash( other.m_hash ) {}

			RED_INLINE Bool operator==( const CHash128& other ) const
			{
				return m_hash == other.m_hash;
			}

			RED_INLINE Bool operator!=( const CHash128& other ) const
			{
				return !( *this == other );
			}

			RED_INLINE Bool operator<( const CHash128& other ) const
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

			RED_INLINE Bool IsZero() const { return *this == CHash128(); }

			RED_INLINE Bool ToString( AnsiChar* buf, const Uint32 bufSize ) const
			{
				if ( bufSize < STRING_BUF_SIZE+1 ) // count the null termination
					return false;

				Red::SNPrintF( buf, bufSize, "%08X-%08X-%08X-%08X", 
					(m_hash[0] >> 32) & 0xFFFFFFFF, 
					(m_hash[0] >> 0) & 0xFFFFFFFF, 
					(m_hash[1] >> 32) & 0xFFFFFFFF, 
					(m_hash[1] >> 0) & 0xFFFFFFFF );

				return true;
			}

		protected:
			static const Uint32 STRING_BUF_SIZE = 4*9+1; // Hash128 is saved as: FFFFFFFF-FFFFFFFF-FFFFFFFF-FFFFFFFF

			RED_INLINE Uint64 fmix64 ( Uint64 k )
			{
				k ^= k >> 33;
				k *= BIG_CONSTANT_ONE;
				k ^= k >> 33;
				k *= BIG_CONSTANT_TWO;
				k ^= k >> 33;

				return k;
			}

			RED_INLINE void Calculate( const void* buffer, size_t len, Uint32 seed )
			{
				using Red::System::BitUtils::RotateLeft;

				const Uint8* data = static_cast< const Uint8* >( buffer );
				const size_t nblocks = len / BLOCK_SIZE;

				Uint64 h1 = seed;
				Uint64 h2 = seed;

				//----------
				// body
				const Uint64* blocks = static_cast< const Uint64* >( buffer );

				for( size_t i = 0; i < nblocks; ++i )
				{
					Uint64 k1 = blocks[ i * 2 + 0 ];
					Uint64 k2 = blocks[ i * 2 + 1 ];

					k1 *= BIG_CONSTANT_THREE;
					k1  = RotateLeft( k1, 31 );
					k1 *= BIG_CONSTANT_FOUR;
					h1 ^= k1;

					h1  = RotateLeft( h1, 27 );
					h1 += h2;
					h1  = h1 * 5 + 0x52dce729;

					k2 *= BIG_CONSTANT_FOUR;
					k2  = RotateLeft( k2, 33 );
					k2 *= BIG_CONSTANT_THREE;
					h2 ^= k2;

					h2  = RotateLeft( h2, 31 );
					h2 += h1;
					h2  = h2 * 5+0x38495ab5;
				}

				//----------
				// tail

				const Uint8* tail = data + nblocks * BLOCK_SIZE;

				Uint64 k1 = 0;
				Uint64 k2 = 0;

				switch( len & ( BLOCK_SIZE - 1 ) )
				{
				case 15: k2 ^= static_cast< Uint64 >( tail[14] ) << 48;
				case 14: k2 ^= static_cast< Uint64 >( tail[13] ) << 40;
				case 13: k2 ^= static_cast< Uint64 >( tail[12] ) << 32;
				case 12: k2 ^= static_cast< Uint64 >( tail[11] ) << 24;
				case 11: k2 ^= static_cast< Uint64 >( tail[10] ) << 16;
				case 10: k2 ^= static_cast< Uint64 >( tail[ 9] ) << 8;
				case  9: k2 ^= static_cast< Uint64 >( tail[ 8] ) << 0;
					k2 *= BIG_CONSTANT_FOUR;
					k2  = RotateLeft( k2, 33 );
					k2 *= BIG_CONSTANT_THREE;
					h2 ^= k2;

				case  8: k1 ^= static_cast< Uint64 >( tail[ 7] ) << 56;
				case  7: k1 ^= static_cast< Uint64 >( tail[ 6] ) << 48;
				case  6: k1 ^= static_cast< Uint64 >( tail[ 5] ) << 40;
				case  5: k1 ^= static_cast< Uint64 >( tail[ 4] ) << 32;
				case  4: k1 ^= static_cast< Uint64 >( tail[ 3] ) << 24;
				case  3: k1 ^= static_cast< Uint64 >( tail[ 2] ) << 16;
				case  2: k1 ^= static_cast< Uint64 >( tail[ 1] ) << 8;
				case  1: k1 ^= static_cast< Uint64 >( tail[ 0] ) << 0;
					k1 *= BIG_CONSTANT_THREE;
					k1  = RotateLeft( k1, 31 );
					k1 *= BIG_CONSTANT_FOUR;
					h1 ^= k1;
				};

				//----------
				// finalization

				h1 ^= len;
				h2 ^= len;

				h1 += h2;
				h2 += h1;

				h1 = fmix64( h1 );
				h2 = fmix64( h2 );

				h1 += h2;
				h2 += h1;

				m_hash[ 0 ] = h1;
				m_hash[ 1 ] = h2;
			}
		};

		// Algorithm: Murmurhash3
		// Calculated at run time
		// Adapted from the source code obtained at https://code.google.com/p/smhasher/
		RED_FORCE_INLINE CHash128 CalculateHash128( const void* buffer, size_t len, Uint32 seed = RED_MURMURHASH_SEED )
		{
			return CHash128( buffer, len, seed );
		}

		RED_FORCE_INLINE CHash128 CalculateHash128( const AnsiChar* buffer, Uint32 seed = RED_MURMURHASH_SEED )
		{
			return CHash128( buffer, seed );
		}
	}
}

#endif // _RED_HASH_H_

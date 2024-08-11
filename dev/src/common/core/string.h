/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "memory.h"
#include "dynarray.h"
#include "set.h"
#include <wctype.h> // For towlower

/// Unicode string

template < class T >
class TString : public TDynArray< T, MC_String, MemoryPool_Strings >
{
private:
	typedef TDynArray< T, MC_String, MemoryPool_Strings > tSuper;

private:
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Strings, MC_String );
public:
	typedef TDynArray< T, MC_String, MemoryPool_Strings > BaseClass;
	
public:
	typedef typename BaseClass::iterator iterator;
	typedef typename BaseClass::const_iterator const_iterator;

public:
	using BaseClass::TypedData;
	using BaseClass::Size;
	using BaseClass::Resize;
	using BaseClass::Grow;
	using BaseClass::Erase;

protected:
	using BaseClass::m_buf;
	using BaseClass::m_size;

public:
	static TString< T > EMPTY;
	// Constructors
	TString();
	TString( const T* str );
	TString( const T* str, size_t length );
	TString( const TString< T >& str );
	TString( TString< T >&& str );
	// Destructor
	~TString();

	// Operator Overloads
	const TString< T >& operator=( const T* str );
	const TString< T >& operator=(TString< T >&& str);
	const TString< T >& operator=( const TString< T >& str ) { tSuper::operator=(str); return *this; }
	const TString< T >& operator+=( T c );
	const TString< T >& operator+=( const T* str );
	const TString< T >& operator+=( const TString< T >& str );
	const TString< T > operator+( const T* str ) const;
	const TString< T > operator+( const TString< T >& str ) const;
	friend const TString< Char > operator+( const Char* str1, const TString< Char >& str2 );
	friend const TString< char > operator+( const char* str1, const TString< char >& str2 );
	Bool operator==( const TString< T >& str ) const;
	Bool operator!=( const TString< T >& str ) const;
	Bool operator<( const TString< T >& str ) const;
	friend Bool operator<( const Char* lhs, const TString< Char > &rhs );
	friend Bool operator<( const char* lhs, const TString< char > &rhs );
	Bool operator==( const T* str ) const;
	Bool operator!=( const T* str ) const;
	Bool operator<( const T* str ) const;

	Bool Empty() const;
	void Clear();

	Bool EqualsNC( const TString< T >& str ) const;
	Bool EqualsNC( const T* str ) const;
	
	// Iterators
	RED_FORCE_INLINE const_iterator Begin() const
	{
		return reinterpret_cast< T* >( m_buf );
	}

	RED_FORCE_INLINE const_iterator End() const
	{
		return reinterpret_cast< T* >( m_buf ) + GetLength(); 
	}

	RED_FORCE_INLINE iterator Begin()
	{
		return reinterpret_cast< T* >( m_buf );
	}

	RED_FORCE_INLINE iterator End()
	{
		return reinterpret_cast< T* >( m_buf ) + GetLength();
	}

		//! Get string length
	RED_FORCE_INLINE Uint32 GetLength() const
	{
		// Array of length 0 is an empty string
		// Array of length 1 is still an empty string
		// Array of length N is a N-1 long string
		return m_size ? static_cast< Uint32 >( m_size - 1 ) : 0;
	}

	//! Get constant buffer
	RED_FORCE_INLINE const T* AsChar() const
	{
		// Empty string are represented as String::EMPTY
		return m_size > 1 ? reinterpret_cast< const T* >(m_buf) : reinterpret_cast< const T* >(TXT(""));
	}

	// Get left part of the string
	TString< T > LeftString( size_t count ) const;
	// Get right part of string
	TString< T > RightString( size_t count ) const;
	// Get middle part of the string
	TString< T > MidString( size_t start, size_t count = 100000 ) const;

	// Find methods.
	Bool FindSubstring( const T* subString, size_t& subStringIndex, Bool rightSide = false, size_t startOffset = 0 ) const;
	Bool FindSubstring( const TString< T >& subString, size_t& subStringIndex, Bool rightSide = false, size_t startOffset = 0 ) const;
	Bool FindCharacter( const T character, size_t& characterIndex, Bool rightSide = false ) const;
	Bool FindCharacter( const T character, size_t& characterIndex, size_t startPos, Bool rightSide = false ) const;
	Bool FindCharacter( const T character, size_t& characterIndex, size_t startPos, size_t endPos, Bool rightSide = false ) const;
	
	Bool ContainsSubstring( const T* subString, Bool rightSide = false, size_t startOffset = 0, Bool noCase = true ) const;
	Bool ContainsSubstring( const TString< T >& subString, Bool rightSide = false, size_t startOffset = 0, Bool noCase = true  ) const;
	Bool ContainsCharacter( const T character, Bool rightSide = false ) const;

	// Returns the number of tokens found
	size_t GetTokens( const T delim, Bool unique, TDynArray< TString< T > > &tokens ) const; 

	//! Returns true if any of the given filter strings is a part of this string
	Bool MatchAny( const TDynArray< TString< T > > &filters ) const;
	//! Returns true if every of the given filter strings is a part of this string
	Bool MatchAll( const TDynArray< TString< T > > &filters ) const;
	
	// Replace substring with another
	Bool Replace( const TString< T >& src, const TString< T >& target, Bool rightSide=false );

	// Replace substring with another
	Bool ReplaceAll( const TString< T >& src, const TString< T >& target, Bool trim = true );

	void ReplaceAll( T src, T target );

	// Split string into two parts
	Bool Split( const TString< T >& separator, TString< T >* leftPart, TString< T >* rightPart, Bool rightSide=false ) const;
	// Split string into two parts
	Bool Split( const TDynArray< TString< T > >& separators, TString< T > *leftPart, TString< T > *rightPart, Bool rightSide=false ) const;
	//! Split into a list of tokens
	TDynArray< TString< T > > Split( const TString< T > &separator, Bool trim = true ) const;
	//! Split into a list of tokens
	TDynArray< TString< T > > Split( const TDynArray< TString< T > >& separators ) const;

	// Join parts using given glue
	static TString< T > Join( const TDynArray< TString< T > > &parts, const TString< T > &glue );

	// Slice into parts using given separator
	void Slice( TDynArray< TString< T > >& parts, const TString< T >& separator ) const;
	// Slice into parts using given separator
	void Slice( TSet< TString< T > >& parts, const TString< T >& separator ) const;
	
	// Check if ending matches pattern
	Bool EndsWith( const TString< T > &str ) const;
	//! Check if start matches pattern
	Bool BeginsWith( const TString< T > &str ) const;

	// Returns part of the string before given separator, returns empty string if separator is not found
	TString< T > StringBefore( const TString< T > &separator, Bool rightSide=false ) const;
	// Returns part of the string after given separator, returns empty string if separator is not found
	TString< T > StringAfter( const TString< T > &separator, Bool rightSide=false ) const;

	// Return lower case string
	TString< T > ToLower() const;

	// Return upper case string
	TString< T > ToUpper() const;
	
	// Make string lowercase ( in place )
	RED_FORCE_INLINE void MakeLower()
	{
		CUpperToLower( TypedData(), Size() );
	}

	RED_FORCE_INLINE void MakeLower( Uint32 start, Uint32 end )
	{
		RED_ASSERT( start < GetLength() );
		RED_ASSERT( end <= GetLength() );
		RED_ASSERT( start < end );
		CUpperToLower( TypedData() + start, end - start );
	}

	// Make string uppercase ( in place )
	RED_FORCE_INLINE void MakeUpper()
	{
		CLowerToUpper( TypedData(), Size() );
	}

	RED_FORCE_INLINE void MakeUpper( Uint32 start, Uint32 end )
	{
		RED_ASSERT( start < GetLength() );
		RED_ASSERT( end <= GetLength() );
		RED_ASSERT( start < end );
		CLowerToUpper( TypedData() + start, end - start );
	}

	// Return uppercase string including unicode diacritic characters - much more expensive
	// Supports following character sets: Basic Latin, Latin-1 Supplement, Latin Extended-A, Cyrylic
	// Based on Unicode case mappings: http://unicode.org/reports/tr21/tr21-5.html#References
	TString< T > ToUpperUnicode() const;

	// Return lowercase string including unicode diacritic characters - much more expensive
	// Supports following character sets: Basic Latin, Latin-1 Supplement, Latin Extended-A, Cyrylic
	// Based on Unicode case mappings: http://unicode.org/reports/tr21/tr21-5.html#References
	TString< T > ToLowerUnicode() const;

	// Removes all white space from the string beginning
	void TrimLeft();
	// Removes all occurrences of 'c' from the string beginning
	void TrimLeft( T c );
	// Removes all white space from the string ending
	void TrimRight();
	// Removes all occurrences of 'c' from the string ending
	void TrimRight( T c );

	// Removes all white space from the both string sides
	void Trim();

	// Removes all white space from the string beginning
	TString< T > TrimLeftCopy() const;
	
	// Removes all white space from the string ending
	TString< T > TrimRightCopy() const;

	// Removes all white space from the both string sides
	TString< T > TrimCopy() const;

	// Change string content
	TString< T >& Set( const T* str );
	TString< T >& Set( const T* str, size_t length );

	// Append data to string
	TString< T >& Append( const T* buf, size_t size );

	// Append data to string
	TString< T >& Append( T c );

	void RemoveWhiteSpaces();
	void RemoveWhiteSpacesAndQuotes();
	size_t CountChars( T c ) const;
	
	// String encoding method - name changed to mislead crackers
	Bool Validate( Uint16 crc );

	// String encoding method - obsolete method to support old cooked strings
	Bool Encode( Uint16 crc );

	void Checksum( Uint32& crc ) const;

	Uint32 CalcHash() const { return SimpleHash( AsChar(), GetLength() ); }
	/**
	 *	Alternative hash function implementation
	 *	@see http://www.partow.net/programming/hashfunctions/#ELFHashFunction
	 *	@see http://www.eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
	 *	@see implementation of qhash( const QString & ) - Qt 4.6
	 */
	Uint32 CalcQHash() const;
	void SimpleHash( Uint32& hash ) const;
	static Uint32 SimpleHash( const T* text, Uint32 length );

	static TString< T > PrintfV( const T* format, va_list arglist );

	// Create string from formated expression
	static TString< T > Printf( const T* format, ... );

	// ANSI formating
	static AnsiChar* ASprintf( AnsiChar* dst, const AnsiChar* fmt, ... );

	// Create a string from a single char
	static TString< T > Chr( T c );

	enum EByteNumberFormat
	{
		BNF_Spaces,			// like 10 056 654
		BNF_Number,			// like 10056654
		BNF_Short,			// like 9.6 MB
		BNF_Precise			// like	9 MB 604 kB 974 B
	};

	static TString< T > FormatByteNumber( Uint64 numBytes, EByteNumberFormat format = BNF_Spaces );

	// Serialize string data
	void Serialize( IFile& file );

	// Serialization
	friend IFile& operator<<( IFile& file, TString< Char > &ar );
	friend IFile& operator<<( IFile& file, TString< char > &ar );
};

// Implicitly instantiated in "template<> RED_INLINE String ToString( const Float& value )"
// So the specialization needs to be in the header here.
template <>
RED_INLINE TString< Char > TString< Char >::Printf( const Char* format, ... )
{
	va_list arglist;
	va_start(arglist, format);
	Char formattedBuf[ 4096 ];
	Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist ); 
	va_end(arglist);
	return formattedBuf;
}

typedef TString< Char > String;
typedef TString< AnsiChar > StringAnsi;

template < size_t N >
class StringBuffer
{
private:
	Char m_buf[N];

public:
	RED_INLINE StringBuffer()
	{
		m_buf[0] = 0;
	}

	RED_INLINE StringBuffer( const Char* chr )
	{
		InitFromChar( chr );
	}

	RED_INLINE StringBuffer( const String &str )
	{
		InitFromChar( str.AsChar() );
	}

	RED_INLINE const AnsiChar* AsAnsiChar() const
	{
		return UNICODE_TO_ANSI( m_buf );
	}

	RED_INLINE AnsiChar* AsAnsiChar()
	{
		return UNICODE_TO_ANSI( m_buf );
	}

	RED_INLINE Char* AsChar()
	{
		return m_buf;
	}

	RED_INLINE const Char* AsChar() const
	{
		return m_buf;
	}

	RED_INLINE void InitFromChar( const Char* chr )
	{
		Red::System::StringCopy( m_buf, chr, N );
	}

	RED_INLINE void ToLower()
	{
		size_t len = Red::System::StringLength( m_buf );

		for ( size_t i = 0; i < len; ++i )
		{
			m_buf[i] = towlower( m_buf[i] );
		}
	}

	RED_INLINE size_t Size() const
	{
		return Red::System::StringLength( m_buf );
	}

	RED_INLINE void Replace( Char from, Char to )
	{
		size_t len = Red::System::StringLength( m_buf );

		for ( size_t i = 0; i < len; ++i )
		{
			if ( m_buf[i] == from )
			{
				m_buf[i] = to;
			}
		}
	}

	RED_INLINE void CopyToS( Char* dst, Uint32 max ) const
	{
		Red::System::StringCopy( dst, m_buf, max );
	}

	RED_INLINE void SubBufCopyToS( Char* dst, size_t max, size_t start, size_t count ) const
	{
		ASSERT( max > count );
		ASSERT( start + count <= Red::System::StringLength( m_buf ) );
		Red::System::MemoryCopy( dst, &m_buf[start], sizeof(Char) * count );
		dst[ count ] = 0;
	}

	RED_INLINE Char& operator[]( size_t n )
	{
		return m_buf[n];
	}

	RED_INLINE const Char& operator[]( size_t n ) const
	{
		return m_buf[n];
	}

	bool operator==( const String &s )
	{
		return Red::System::StringCompare( m_buf, s.AsChar() ) == 0;
	}

	bool operator==( const Char *chr )
	{
		return Red::System::StringCompare( m_buf, chr ) == 0;
	}

	bool operator==( const StringBuffer& s )
	{
		return Red::System::StringCompare( m_buf, s.m_buf ) == 0;
	}

	size_t FindFirstOf( const Char c )
	{
		size_t counter = 0;
		size_t len = Red::System::StringLength( m_buf );

		while( counter < len )
		{
			if( m_buf[counter] == c )
			{
				return counter;
			}
			++counter;
		}
		return len;
	}

	size_t FindLastOf( const Char c )
	{
		size_t len = Red::System::StringLength( m_buf );
		size_t counter = len - 1;

		while( counter > 0 )
		{
			if( m_buf[counter] == c )
			{
				return counter;
			}
			--counter;
		}
		return len;
	}

public:
	friend IFile &operator<<( IFile &file, StringBuffer &name )
	{
		ASSERT( !file.IsWriter() );

		if ( file.IsGarbageCollector() )
		{
			return file;
		}

		Bool useOldSerialization = false;
		size_t sizeToRead = 0;

		{
			Int32 size = 0;

			file << CCompressedNumSerializer( size );

			if ( size > 0 )
			{
				useOldSerialization = true;
			}

			ASSERT( (Uint32)Abs( size ) < N );
			if( (Uint32)Abs( size ) >= N )
			{
				size = N;
			}

			sizeToRead = Abs( size );

			name.m_buf[ N-1 ] = 0;
			if ( sizeToRead < N )
			{
				name.m_buf[ sizeToRead ] = 0;
			}

		}

		if ( sizeToRead )
		{
			if ( useOldSerialization )
			{
				// Serialize whole buffer
				ASSERT( sizeToRead < N );

				size_t toRead = Red::Math::NumericalUtils::Min( sizeToRead, N-1 );
				file.Serialize( name.m_buf, toRead * sizeof( Char ) );
				if( toRead < sizeToRead )
				{
					file.Seek( file.GetOffset() + ( sizeToRead - toRead ) * sizeof( Char ) );
				}
			}
			else
			{
				AnsiChar* buf = reinterpret_cast<AnsiChar*>( RED_ALLOCA( sizeToRead ) );

				file.Serialize( buf, sizeToRead * sizeof( AnsiChar ) );

				for ( size_t i = 0; i < sizeToRead; ++i )
				{
					name.m_buf[i] = buf[i];
				}
			}
		}

		return file;
	}

};

namespace StringHelpers
{
	// extract the extension from the file name
	template< typename CharType >
	static TString<CharType> GetFileExtension( const TString<CharType>& str )
	{
		const CharType dot[2] = { '.', 0 };
		TString<CharType> fileExtension;
		str.Split( dot, NULL, &fileExtension, true );
		return fileExtension;
	}

	template< typename CharType >
	static Int32 StrchrR( const TString<CharType>& str, CharType ch )
	{
		size_t index=0;
		if ( !str.FindCharacter( ch, index, true ) )
			return -1;

		return (Int32)index;
	}

	// extract the base path from the file path
	template< typename CharType >
	static TString<CharType> GetBaseFilePath( const TString<CharType>& str )
	{
		const Int32 pos = Max< Int32 >( StrchrR(str, (CharType)'\\'), StrchrR(str, (CharType)'/') );
		if ( pos == -1 )
			return str;

		return str.LeftString( pos );
	}

	// Use this to strip serialised strings from runtime without loading them
	// Parameter is just used to determine how it was serialised
	template< typename CharType >
	RED_INLINE void SkipSerialisation( IFile& file, const TString<CharType>& data )
	{
		RED_FATAL_ASSERT( false, "Not implemented for this string type!" );
	}

	// Keep this up to date with TString<Char>::Serialize!
	template<> RED_INLINE void SkipSerialisation( IFile& file, const TString<Char>& data )
	{
		Int32 size = 0;
		file << CCompressedNumSerializer( size );
		if ( size < 0 )
		{
			file.Seek( file.GetOffset() + ( Abs( size ) * sizeof( Uint8 ) ) );
		}
		else
		{
			file.Seek( file.GetOffset() + ( size * sizeof( Char ) ) );
		}
	}

	// Keep this up to date with TString<char>::Serialize!
	template<> RED_INLINE void SkipSerialisation( IFile& file, const TString<char>& data )
	{
		Int32 size = 0;
		file << CCompressedNumSerializer( size );
		if ( size != 0 )
		{
			file.Seek( file.GetOffset() + ( size * sizeof( char ) ) );
		}
	}

	template< typename CharType >
	RED_INLINE Bool WildcardMatch(const CharType* str, const CharType* match)
	{
		while (*match)
		{
			if (!*str)
				return *match == '*' && !*(match + 1);

			if (*match == '*')
				return WildcardMatch(str, match + 1) || WildcardMatch(str + 1, match);

			if ( !( *match == '?' || toupper(*str) == toupper(*match) ) )
				return false;

			++str;
			++match;
		}
		return !*str;
	}
}
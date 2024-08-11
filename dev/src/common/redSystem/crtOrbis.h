/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CRT_ORBIS_H_
#define _RED_CRT_ORBIS_H_

#include "types.h"

#include "numericalLimits.h"

// For String and Memory operations
#include <cstring>
#include <wchar.h>

// For printf functionality
#include <cstdio>
#include <cstdarg>

// For Unicode/Ansi conversion
#include <cstdlib>

// For String to Int conversion checking
#include <errno.h>
#include <limits.h>

#define RED_ALLOCA(size) ::alloca((size))

// Since the c standard does not have a native TRUNCATE parameter for strncpy_s, etc, we roll our own
// Incidentally, this means you can't have a string of length MAX_SIZE_T-1
#define RED_TRUNCATE Red::System::MemSize(-1)

namespace Red
{
	namespace System
	{
		//////////////////////////////////////////////////////////////////////////
		// Memory functions

		RED_FORCE_INLINE int MemoryCompare( const void* a, const void* b, size_t size)							{ return std::memcmp( a, b, size ); }
		
		//FIXME: redSystem restrict macros
		RED_FORCE_INLINE void MemoryCopy( void* __restrict dest, const void* __restrict source, size_t size )	{ std::memcpy( dest, source, size ); }
		RED_FORCE_INLINE void MemoryMove( void* dest, const void* source, size_t size )							{ std::memmove( dest, source, size ); }
		RED_FORCE_INLINE void MemorySet( void* buffer, Int32 value, size_t size )								{ std::memset( buffer, value, size ); }
		RED_FORCE_INLINE void MemoryZero( void* buffer, size_t size )											{ std::memset( buffer, 0, size ); }

		//////////////////////////////////////////////////////////////////////////
		// String functions

		// Compile time string length calculation
		template< size_t N >
		RED_FORCE_INLINE size_t StringLengthCompileTime( const AnsiChar (&)[ N ] )							{ return N - 1; }

		template< size_t N >
		RED_FORCE_INLINE size_t StringLengthCompileTime( const UniChar (&)[ N ] )							{ return N - 1; }

		// Run time string length calculation
		RED_FORCE_INLINE size_t StringLength( const AnsiChar* str )											{ return std::strlen( str ); }
		RED_FORCE_INLINE size_t StringLength( const UniChar* str )											{ return ::wcslen( str ); }
		RED_FORCE_INLINE size_t StringLength( const AnsiChar* str, size_t maxBufferSize )					{ return ::strnlen_s( str, maxBufferSize ); }
		RED_FORCE_INLINE size_t StringLength( const UniChar* str, size_t maxBufferSize )					{ return ::wcsnlen_s( str, maxBufferSize ); }

		// Search for first occurrence of a character
		RED_FORCE_INLINE AnsiChar*			StringSearch( AnsiChar* str, AnsiChar searchTerm )				{ return std::strchr( str, searchTerm ); }
		RED_FORCE_INLINE const AnsiChar*	StringSearch( const AnsiChar* str, AnsiChar searchTerm )		{ return std::strchr( str, searchTerm ); }
		RED_FORCE_INLINE UniChar*			StringSearch( UniChar* str, UniChar searchTerm )				{ return ::wcschr( str, searchTerm ); }
		RED_FORCE_INLINE const UniChar*		StringSearch( const UniChar* str, UniChar searchTerm )			{ return ::wcschr( str, searchTerm ); }

		// Search for last occurrence of a character
		RED_FORCE_INLINE AnsiChar*			StringSearchLast( AnsiChar* str, AnsiChar searchTerm )			{ return std::strrchr( str, searchTerm ); }
		RED_FORCE_INLINE const AnsiChar*	StringSearchLast( const AnsiChar* str, AnsiChar searchTerm )	{ return std::strrchr( str, searchTerm ); }
		RED_FORCE_INLINE UniChar*			StringSearchLast( UniChar* str, UniChar searchTerm )			{ return ::wcsrchr( str, searchTerm ); }
		RED_FORCE_INLINE const UniChar*		StringSearchLast( const UniChar* str, UniChar searchTerm )		{ return ::wcsrchr( str, searchTerm ); }

		// Search for the first instance of a string
		RED_FORCE_INLINE AnsiChar*			StringSearch( AnsiChar* str, const AnsiChar* searchTerm )		{ return std::strstr( str, searchTerm ); }
		RED_FORCE_INLINE const AnsiChar*	StringSearch( const AnsiChar* str, const AnsiChar* searchTerm )	{ return std::strstr( str, searchTerm ); }
		RED_FORCE_INLINE UniChar*			StringSearch( UniChar* str, const UniChar* searchTerm )			{ return ::wcsstr( str, searchTerm ); }
		RED_FORCE_INLINE const UniChar*		StringSearch( const UniChar* str, const UniChar* searchTerm )	{ return ::wcsstr( str, searchTerm ); }

		// Search for the first instance of any character specified in searchTerm 
		RED_FORCE_INLINE AnsiChar*			StringSearchSet( AnsiChar* str, const AnsiChar* searchTerm )		{ return std::strpbrk( str, searchTerm ); }
		RED_FORCE_INLINE const AnsiChar*	StringSearchSet( const AnsiChar* str, const AnsiChar* searchTerm )	{ return std::strpbrk( str, searchTerm ); }
		RED_FORCE_INLINE UniChar*			StringSearchSet( UniChar* str, const UniChar* searchTerm )			{ return ::wcspbrk( str, searchTerm ); }
		RED_FORCE_INLINE const UniChar*		StringSearchSet( const UniChar* str, const UniChar* searchTerm )	{ return ::wcspbrk( str, searchTerm ); }

		RED_FORCE_INLINE Bool StringCopy( AnsiChar* dest, const AnsiChar* source, size_t destSize, size_t sourceToCopy = RED_TRUNCATE )
		{
			MemSize sourceLength = StringLength( source, sourceToCopy );
			return ::strncpy_s( dest, destSize, source, sourceLength );
		}

		RED_FORCE_INLINE Bool StringCopy( UniChar* dest, const UniChar* source, size_t destSize, size_t sourceToCopy = RED_TRUNCATE )			
		{ 
			MemSize sourceLength = StringLength( source, sourceToCopy );
			return ::wcsncpy_s( dest, destSize, source, sourceLength ) == 0; 
		}

		RED_FORCE_INLINE Bool StringConcatenate( AnsiChar* dest, const AnsiChar* source, size_t destSize, size_t sourceToCopy = RED_TRUNCATE )	
		{ 
			MemSize sourceLength = StringLength( source, sourceToCopy );
			return ::strncat_s( dest, destSize, source, sourceLength ) == 0; 
		}
		
		RED_FORCE_INLINE Bool StringConcatenate( UniChar* dest, const UniChar* source, size_t destSize, size_t sourceToCopy = RED_TRUNCATE )	
		{ 
			MemSize sourceLength = StringLength( source, sourceToCopy );
			return ::wcsncat_s( dest, destSize, source, sourceLength ) == 0; 
		}

		RED_FORCE_INLINE size_t WideCharToStdChar( AnsiChar* dest, const UniChar* source, size_t destSize )
		{
			size_t charsWritten = 0;
			MemSize sourceLength = StringLength( source );
			errno_t errval = wcstombs_s( &charsWritten, dest, destSize, source, sourceLength );
			RED_UNUSED( errval );

			// The null terminator is not included in the length
			return charsWritten;
		}

		RED_FORCE_INLINE size_t StdCharToWideChar( UniChar* dest, const AnsiChar* source, size_t destSize )
		{
			size_t charsWritten = 0;
			MemSize sourceLength = StringLength( source );
			errno_t errval = mbstowcs_s( &charsWritten, dest, destSize, source, sourceLength );
			RED_UNUSED( errval );

			// The null terminator is not included in the length
			return charsWritten;
		}

		RED_FORCE_INLINE Int32 StringCompare( const AnsiChar* a, const AnsiChar* b )						{ return std::strcmp( a, b ); }
		RED_FORCE_INLINE Int32 StringCompare( const UniChar* a, const UniChar* b )							{ return ::wcscmp( a, b ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const AnsiChar* a, const AnsiChar* b )					{ return ::strcasecmp( a, b ); }

		////////////////////////////////////////////////////////////////////////////////////////
		// !!!! NOTE !!!!
		// This function will only work as intended for strings containing characters compatible with the default locale
		Int32 StringCompareNoCase( const UniChar* a, const UniChar* b );

		// Compare at most 'max' characters
		RED_FORCE_INLINE Int32 StringCompare( const AnsiChar* a, const AnsiChar* b, size_t max )			{ return std::strncmp( a, b, max ); }
		RED_FORCE_INLINE Int32 StringCompare( const UniChar* a, const UniChar* b, size_t max )				{ return ::wcsncmp( a, b, max ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const AnsiChar* a, const AnsiChar* b, size_t max )		{ return ::strncasecmp( a, b, max ); }

		////////////////////////////////////////////////////////////////////////////////////////
		// !!!! NOTE !!!!
		// This function will only work as intended for strings containing characters compatible with the default locale
		Int32 StringCompareNoCase( const UniChar* a, const UniChar* b, size_t max );

		enum Base
		{
			BaseAuto = 0,		// Detect based on format of number
			BaseEight = 8,		// Octal
			BaseTen = 10,		// Decimal
			BaseSixteen = 16,	// Hex
		};

		namespace Internal
		{
			template< typename IntType, typename InputType >
			RED_FORCE_INLINE IntType ApplyNumericLimit( InputType value )
			{
				IntType result = 0;

				if( value > static_cast< InputType >( NumericLimits< IntType >::Max() ) )
				{
					errno = ERANGE;
					result = NumericLimits< IntType >::Max();
				}
				else if( value < static_cast< InputType >( NumericLimits< IntType >::Min() ) )
				{
					errno = ERANGE;
					result = NumericLimits< IntType >::Min();
				}
				else
				{
					result = static_cast< IntType >( value );
				}
				
				return result;
			}
		}

		RED_FORCE_INLINE Bool StringToInt( Int32& out, const AnsiChar* in, AnsiChar** end, Base base )
		{
			errno = 0;
			long value = std::strtol( in, end, base );
			out = Internal::ApplyNumericLimit< Int32 >( value );
			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Int32& out, const UniChar* in, UniChar** end, Base base )
		{
			errno = 0;
			long value = ::wcstol( in, end, base );
			out = Internal::ApplyNumericLimit< Int32 >( value );
			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Uint32& out, const AnsiChar* in, AnsiChar** end, Base base )
		{
			errno = 0;
			unsigned long value = std::strtoul( in, end, base );
			out = Internal::ApplyNumericLimit< Uint32 >( value );
			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Uint32& out, const UniChar* in, UniChar** end, Base base )
		{
			errno = 0;
			unsigned long value = ::wcstoul( in, end, base );
			out = Internal::ApplyNumericLimit< Uint32 >( value );
			return errno == 0;
		}

		RED_FORCE_INLINE Double	StringToDouble( const AnsiChar* str )										{ return ::atof( str ); }
		Double	StringToDouble( const Char* str );

		//////////////////////////////////////////////////////////////////////////
		// Print Functions
		RED_FORCE_INLINE Int32 VSNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, va_list arg )	{ return ::vsnprintf_s( buffer, count, format, arg ); }
		RED_FORCE_INLINE Int32 VSNPrintF( UniChar* buffer, size_t count, const UniChar* format, va_list arg )	{ return ::vsnwprintf_s( buffer, count, format, arg ); }

		Int32 SNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, ... );
		Int32 SNPrintF( UniChar* buffer, size_t count, const UniChar* format, ... );

		//////////////////////////////////////////////////////////////////////////
		// For use only by Red System
		namespace Internal
		{
			RED_FORCE_INLINE Bool FileOpen( FILE** handle, const AnsiChar* filename, const AnsiChar* mode )	{ return ::fopen_s( handle, filename, mode ) == 0; }
			                 Bool FileOpen( FILE** handle, const UniChar* filename, const UniChar* mode );
			RED_FORCE_INLINE Bool FileClose( FILE* handle )													{ return ::fclose( handle ) == 0; }
			RED_FORCE_INLINE Bool FileFlush( FILE* handle )													{ return ::fflush( handle ) == 0; }
			                Int32 FilePrintF( FILE* handle, const AnsiChar* format, ... );
			                Int32 FilePrintF( FILE* handle, const UniChar* format, ... );
			RED_FORCE_INLINE void FilePrint( FILE* handle, const AnsiChar* buffer )							{ ::fputs( buffer, handle ); }
			RED_FORCE_INLINE void FilePrint( FILE* handle, const UniChar* buffer )							{ ::fputws( buffer, handle ); }

			RED_FORCE_INLINE void EmergencyExit() { _Exit( EXIT_SUCCESS ); }
		}
	}
}

#endif
/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CRT_WINDOWS_H_
#define _RED_CRT_WINDOWS_H_

#include "types.h"

// For String and Memory operations
#include <cstring>

// For printf functionality
#include <cstdio>
#include <cstdarg>

// For Unicode/Ansi conversion
#include <cstdlib>

// For file access
#include <share.h>

#define RED_ALLOCA(size) ::alloca((size))

namespace Red
{
	namespace System
	{
		//////////////////////////////////////////////////////////////////////////
		// Memory functions

		RED_FORCE_INLINE int MemoryCompare( const void* a, const void* b, size_t size)						{ return std::memcmp( a, b, size ); }

		//FIXME: redSystem restrict macros
		RED_FORCE_INLINE void MemoryCopy( void* __restrict dest, const void* __restrict source, size_t size ) { std::memcpy( dest, source, size ); }
		RED_FORCE_INLINE void MemoryMove( void* dest, const void* source, size_t size )						{ std::memmove( dest, source, size ); }
		RED_FORCE_INLINE void MemorySet( void* buffer, Int32 value, size_t size )							{ std::memset( buffer, value, size ); }
		RED_FORCE_INLINE void MemoryZero( void* buffer, size_t size )										{ std::memset( buffer, 0, size ); }

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

		RED_FORCE_INLINE Bool StringCopy( AnsiChar* dest, const AnsiChar* source, size_t destSize, size_t sourceToCopy = _TRUNCATE )		{ return ::strncpy_s( dest, destSize, source, sourceToCopy ) == 0; }
		RED_FORCE_INLINE Bool StringCopy( UniChar* dest, const UniChar* source, size_t destSize, size_t sourceToCopy = _TRUNCATE )			{ return ::wcsncpy_s( dest, destSize, source, sourceToCopy ) == 0; }

		RED_FORCE_INLINE Bool StringConcatenate( AnsiChar* dest, const AnsiChar* source, size_t destSize, size_t sourceToCopy = _TRUNCATE )	{ return ::strncat_s( dest, destSize, source, sourceToCopy ) == 0; }
		RED_FORCE_INLINE Bool StringConcatenate( UniChar* dest, const UniChar* source, size_t destSize, size_t sourceToCopy = _TRUNCATE )	{ return ::wcsncat_s( dest, destSize, source, sourceToCopy ) == 0; }

		RED_FORCE_INLINE size_t WideCharToStdChar( AnsiChar* dest, const UniChar* source, size_t destSize )
		{
			size_t charsWritten = 0;
			wcstombs_s( &charsWritten, dest, destSize, source, _TRUNCATE );

			// Do not include the null terminator in the length
			return charsWritten - 1;
		}

		RED_FORCE_INLINE size_t StdCharToWideChar( UniChar* dest, const AnsiChar* source, size_t destSize )
		{
			size_t charsWritten = 0;
			mbstowcs_s( &charsWritten, dest, destSize, source, _TRUNCATE );

			// Do not include the null terminator in the length
			return charsWritten - 1;
		}

		RED_FORCE_INLINE Int32 StringCompare( const AnsiChar* a, const AnsiChar* b )						{ return std::strcmp( a, b ); }
		RED_FORCE_INLINE Int32 StringCompare( const UniChar* a, const UniChar* b )							{ return ::wcscmp( a, b ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const AnsiChar* a, const AnsiChar* b )					{ return ::_stricmp( a, b ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const UniChar* a, const UniChar* b )					{ return ::_wcsicmp( a, b ); }

		// Compare at most 'max' characters
		RED_FORCE_INLINE Int32 StringCompare( const AnsiChar* a, const AnsiChar* b, size_t max )			{ return std::strncmp( a, b, max ); }
		RED_FORCE_INLINE Int32 StringCompare( const UniChar* a, const UniChar* b, size_t max )				{ return ::wcsncmp( a, b, max ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const AnsiChar* a, const AnsiChar* b, size_t max )		{ return ::_strnicmp( a, b, max ); }
		RED_FORCE_INLINE Int32 StringCompareNoCase( const UniChar* a, const UniChar* b, size_t max )		{ return ::_wcsnicmp( a, b, max ); }

		enum Base
		{
			BaseAuto = 0,		// Detect based on format of number
			BaseEight = 8,		// Octal
			BaseTen = 10,		// Decimal
			BaseSixteen = 16,	// Hex
		};

		RED_FORCE_INLINE Bool StringToInt( Int32& out, const AnsiChar* in, AnsiChar** end, Base base )
		{
			errno = 0;
			out = static_cast< Int32 >( std::strtol( in, end, base ) );

			// If errno == ERANGE, "out" has been set to INT_MIN or INT_MAX

			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Int32& out, const UniChar* in, UniChar** end, Base base )
		{
			errno = 0;
			out = static_cast< Int32 >( ::wcstol( in, end, base ) );

			// If errno == ERANGE, "out" has been set to INT_MIN or INT_MAX

			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Uint32& out, const AnsiChar* in, AnsiChar** end, Base base )
		{
			errno = 0;
			out = static_cast< Uint32 >( std::strtoul( in, end, base ) );

			// If errno == ERANGE, "out" has been set to INT_MIN or INT_MAX

			return errno == 0;
		}

		RED_FORCE_INLINE Bool StringToInt( Uint32& out, const UniChar* in, UniChar** end, Base base )
		{
			errno = 0;
			out = static_cast< Uint32 >( ::wcstoul( in, end, base ) );

			// If errno == ERANGE, "out" has been set to INT_MIN or INT_MAX

			return errno == 0;
		}

		RED_FORCE_INLINE Double	StringToDouble( const AnsiChar* str )										{ return ::atof( str ); }
		RED_FORCE_INLINE Double	StringToDouble( const Char* str )											{ return ::_wtof( str ); }


		//////////////////////////////////////////////////////////////////////////
		// Print Functions
		RED_FORCE_INLINE Int32 VSNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, va_list arg )	{ return ::vsnprintf_s( buffer, count, _TRUNCATE, format, arg ); }
		RED_FORCE_INLINE Int32 VSNPrintF( UniChar* buffer, size_t count, const UniChar* format, va_list arg )	{ return ::_vsnwprintf_s( buffer, count, _TRUNCATE, format, arg ); }

		                 Int32 SNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, ... );
		                 Int32 SNPrintF( UniChar* buffer, size_t count, const UniChar* format, ... );

		//////////////////////////////////////////////////////////////////////////
		// For use only by Red System
		namespace Internal
		{
			RED_FORCE_INLINE Bool FileOpen( FILE** handle, const AnsiChar* filename, const AnsiChar* mode )	{ *handle = ::_fsopen( filename, mode, _SH_DENYNO ); return *handle != NULL; }
			RED_FORCE_INLINE Bool FileOpen( FILE** handle, const UniChar* filename, const UniChar* mode )	{ *handle = ::_wfsopen( filename, mode, _SH_DENYNO ); return *handle != NULL; }
			RED_FORCE_INLINE Bool FileClose( FILE* handle )													{ return ::fclose( handle ) == 0; }
			RED_FORCE_INLINE Bool FileFlush( FILE* handle )													{ return ::fflush( handle ) == 0; }
			                Int32 FilePrintF( FILE* handle, const AnsiChar* format, ... );
			                Int32 FilePrintF( FILE* handle, const UniChar* format, ... );
			RED_FORCE_INLINE void FilePrint( FILE* handle, const AnsiChar* buffer )							{ ::fputs( buffer, handle ); }
			RED_FORCE_INLINE void FilePrint( FILE* handle, const UniChar* buffer )							{ ::fputws( buffer, handle ); }

			RED_FORCE_INLINE void EmergencyExit() { _exit( EXIT_SUCCESS ); }
		}
	}
}

#endif

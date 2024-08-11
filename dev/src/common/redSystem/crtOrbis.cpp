/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "crtOrbis.h"
#include "error.h"
#include <locale>

// Have to override default libc heap size. A lot of internal functions like printf's will crash on bigger allocations otherwise.
size_t sceLibcHeapSize = 1024*1024*16;

Red::System::Int32 Red::System::SNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = VSNPrintF( buffer, count, format, arglist );
	va_end( arglist );
	return retval;
}

Red::System::Int32 Red::System::SNPrintF( UniChar* buffer, size_t count, const UniChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = VSNPrintF( buffer, count, format, arglist );
	va_end( arglist );
	return retval;
}

Red::System::Int32 Red::System::StringCompareNoCase( const UniChar* a, const UniChar* b )
{
	RED_ASSERT( a != nullptr, TXT( "Cannot compare null strings. This will probably crash" ) );
	RED_ASSERT( b != nullptr, TXT( "Cannot compare null strings. This will probably crash" ) );

	std::locale thisLocale;		// This is copied from the global locale when default constructor used
	MemSize aLength = StringLength( a );
	MemSize bLength = StringLength( b );

	// Note! This assumes the strings are null terminated. If they are not, this will overflow
	MemSize searchLength = aLength > bLength ? bLength : aLength;
	MemSize strCounter = 0;
	while( strCounter <= searchLength )		// We actually test the first null character from either string
	{
		UniChar aLowercase = std::tolower( a[strCounter], thisLocale );
		UniChar bLowercase = std::tolower( b[strCounter], thisLocale );

		if( aLowercase > bLowercase )
			return 1;
		else if( aLowercase < bLowercase )
			return -1;

		++strCounter;
	}

	return 0;
}

Red::System::Int32 Red::System::StringCompareNoCase( const UniChar* a, const UniChar* b, size_t max )
{
	RED_ASSERT( a != nullptr, TXT( "Cannot compare null strings. This will probably crash" ) );
	RED_ASSERT( b != nullptr, TXT( "Cannot compare null strings. This will probably crash" ) );

	std::locale thisLocale;		// This is copied from the global locale when default constructor used
	MemSize aLength = StringLength( a ) + 1;
	MemSize bLength = StringLength( b ) + 1;

	MemSize searchLength = aLength > bLength ? bLength : aLength;
	searchLength = searchLength > max ? max : searchLength;

	MemSize strCounter = 0;
	while( strCounter < searchLength )		// We actually test the first null character from either string
	{
		UniChar aLowercase = std::tolower( a[strCounter], thisLocale );
		UniChar bLowercase = std::tolower( b[strCounter], thisLocale );

		if( aLowercase > bLowercase )
			return 1;
		else if( aLowercase < bLowercase )
			return -1;

		++strCounter;
	}

	return 0;
}

Red::System::Double Red::System::StringToDouble( const Char* str )
{
	//FIXME>>>:
	AnsiChar buf[1024];
	size_t strLength = StringLength( str );
	size_t charsWritten = 0;
	wcstombs_s( &charsWritten, buf, sizeof(buf), str, strLength );
	return StringToDouble( buf );
}

Red::System::Bool Red::System::Internal::FileOpen( FILE** handle, const UniChar* filename, const UniChar* mode )
{
	AnsiChar ansiFilename[ 256 ];
	AnsiChar ansiMode[ 16 ];

	WideCharToStdChar( ansiFilename, filename, ARRAY_COUNT( ansiFilename ) );
	WideCharToStdChar( ansiMode, mode, ARRAY_COUNT( ansiMode ) );

	return ::fopen_s( handle, ansiFilename, ansiMode ) == 0;
}

Red::System::Int32 Red::System::Internal::FilePrintF( FILE* handle, const AnsiChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = vfprintf_s( handle, format, arglist );
	va_end( arglist );
	return retval;
}

Red::System::Int32 Red::System::Internal::FilePrintF( FILE* handle, const UniChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = vfwprintf_s( handle, format, arglist );
	va_end( arglist );
	return retval;
}

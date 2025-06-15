/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "crtWindows.h"

using namespace Red::System;

Int32 Red::System::SNPrintF( AnsiChar* buffer, size_t count, const AnsiChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = VSNPrintF( buffer, count, format, arglist );
	va_end( arglist );
	return retval;
}

Int32 Red::System::SNPrintF( UniChar* buffer, size_t count, const UniChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = VSNPrintF( buffer, count, format, arglist );
	va_end( arglist );
	return retval;
}

#ifdef RED_PLATFORM_LINUX

#include "utility.h"

#include <codecvt>
#include <locale>

Bool Internal::FileOpen( FILE** handle, const UniChar* filename, const UniChar* mode )
{
	AnsiChar ansiFilename[ 256 ];
	AnsiChar ansiMode[ 16 ];

	WideCharToStdChar( ansiFilename, filename, ARRAY_COUNT( ansiFilename ) );
	WideCharToStdChar( ansiMode, mode, ARRAY_COUNT( ansiMode ) );

	*handle = fopen(ansiFilename, ansiMode);
	auto result = !( *handle );
	return result == 0;
}

void Internal::FilePrint( FILE* handle, const UniChar* buffer )
{
	std::u16string u16str(buffer);
	std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
	std::string utf8str = converter.to_bytes(u16str);

	::fputs( utf8str.c_str(), handle );
}

#else

Int32 Internal::FilePrintF( FILE* handle, const AnsiChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = vfprintf_s( handle, format, arglist );
	va_end( arglist );
	return retval;
}

Int32 Internal::FilePrintF( FILE* handle, const UniChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Int32 retval = vfwprintf_s( handle, format, arglist );
	va_end( arglist );
	return retval;
}

#endif

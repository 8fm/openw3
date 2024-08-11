/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "gatTTYWriter.h"

#if defined(RED_PLATFORM_WINPC)

CGatTTYWriter::CGatTTYWriter() : CommonOutputDevice()
	, m_verbose(false)
	, m_silent(false)
{
	SetSafeToCallOnCrash();

	m_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
}

CGatTTYWriter::~CGatTTYWriter()
{

}

void CGatTTYWriter::SetSilent(const Red::Bool silent)
{
	m_silent = silent;
}

void CGatTTYWriter::SetVerbose(const Red::Bool verbose)
{
	m_verbose = verbose;
}

void CGatTTYWriter::UpdateStatusText( const Char* txt )
{
	if ( m_hStdOut != INVALID_HANDLE_VALUE )
	{
		if ( 0 != Red::StringCompare( txt, m_statusText ) )
		{
			Red::StringCopy( m_statusText, txt, ARRAY_COUNT(m_statusText) );
			SetConsoleTitleW( m_statusText );
		}
	}
}

void CGatTTYWriter::WriteFormatted( const AnsiChar* message )
{
	// process status update
	{
		const AnsiChar* statusMessage = Red::StringSearch( message, "Status: " );
		if ( statusMessage )
		{
			UpdateStatusText( ANSI_TO_UNICODE( statusMessage + 8 ) );
			return;
		}
	}

	// no log output
	if ( m_silent )
		return;

	if ( !m_verbose )
	{
		const bool isGat = (NULL != strstr( message, "[Gat]" ));
		const bool isError = (NULL != strstr( message, "[Error]" ));
		const bool isInfo = (NULL != strstr( message, "[Info]" ));
		if (!isGat && !isError && !isInfo )
			return;
	}

	printf( message );
	fflush( stdout );
}

void CGatTTYWriter::WriteFormatted( const UniChar* message )
{
	// process status update
	{
		const Char* statusMessage = Red::StringSearch( message, TXT("Status: ") );
		if ( statusMessage )
		{
			UpdateStatusText( statusMessage + 8 );
			return;
		}
	}

	// no log output
	if ( m_silent)
		return;

	// in a non-verbose mode print only stuff from [Gat] or [Error]
	if ( !m_verbose )
	{
		const bool isGat = (NULL != wcsstr( message, L"[Gat]" ));
		const bool isError = (NULL != wcsstr( message, L"[Error]" ));
		const bool isInfo = (NULL != wcsstr( message, L"[Info]" ));
		if (!isGat && !isError && !isInfo)
			return;

		if ( isError )
		{
			SetConsoleTextAttribute( m_hStdOut, FOREGROUND_RED );
		}
		if ( isInfo )
		{
			SetConsoleTextAttribute( m_hStdOut, FOREGROUND_GREEN );
		}
	}

	wprintf( message );
	fflush( stdout );
}

#endif
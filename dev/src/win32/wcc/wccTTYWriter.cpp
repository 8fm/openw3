/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "wccTTYWriter.h"

const Red::CNameHash c_allowedMessageChannels[] = 
{
	Red::CNameHash( "WCC" ),
	Red::CNameHash( "Error" ),
	Red::CNameHash( "MemoryFramework" ),
};
const Uint32 c_allowedMessageChannelsCount = sizeof( c_allowedMessageChannels ) / sizeof( *c_allowedMessageChannels );

CWccTTYWriter::CWccTTYWriter() : CommonOutputDevice()
	, m_verbose(false)
	, m_silent(false)
{
	SetSafeToCallOnCrash();

	m_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
}

CWccTTYWriter::~CWccTTYWriter()
{

}

void CWccTTYWriter::SetSilent(const Red::Bool silent)
{
	m_silent = silent;
}

void CWccTTYWriter::SetVerbose(const Red::Bool verbose)
{
	m_verbose = verbose;
}

void CWccTTYWriter::UpdateStatusText( const Char* txt )
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

void CWccTTYWriter::WriteFormatted( const UniChar* message )
{
	wprintf( message );
	fflush( stdout );
}

void CWccTTYWriter::Flush()
{
	fflush( stdout );
}

void CWccTTYWriter::Write( const Red::System::Log::Message& message )
{
	// process status update
	{
		const Char* statusMessage = Red::StringSearch( message.text, TXT("Status: ") );
		if ( statusMessage )
		{
			UpdateStatusText( statusMessage + 8 );
			return;
		}
	}

	// no log output
	if ( m_silent )
		return;

	if( !m_verbose )
	{
		Bool filterPassed = false;

		if ( message.priority == Red::System::Log::P_Error )
		{
			filterPassed = true;
		}
		else
		{
			for( Uint32 c=0;c<c_allowedMessageChannelsCount;++c )
			{
				if( message.channelHash == c_allowedMessageChannels[ c ] )
				{
					filterPassed = true;
					break;
				}
			}
		}
		if( !filterPassed )
		{
			return;
		}
	}

	CommonOutputDevice::Write( message );
}

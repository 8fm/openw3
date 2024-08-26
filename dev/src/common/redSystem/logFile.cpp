/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "logFile.h"
#include "os.h"
#include "crt.h"
#include "error.h"

Red::System::Log::File::File( const Char* filename, Bool isPrimaryLogFile )
:	CommonOutputDevice()
,	m_file( nullptr )
{
	SetSafeToCallOnCrash();

#ifdef RED_PLATFORM_LINUX
	size_t size = StringLength(filename) + 1;
	AnsiChar* filenameAnsi = static_cast< AnsiChar* >( RED_ALLOCA( size * sizeof( AnsiChar ) ) );
	WideCharToStdChar( filenameAnsi, filename, size );
	Internal::FileOpen( &m_file, filenameAnsi, "w" );
#else
	Internal::FileOpen( &m_file, filename, TXT( "w" ) );
#endif

	if( isPrimaryLogFile )
	{
		Error::Handler::GetInstance()->SetLogFile( filename, this );
	}
}

Red::System::Log::File::~File()
{
	Close();
}

void Red::System::Log::File::WriteFormatted( const Char* message )
{
	if ( m_file )
	{
		Internal::FilePrint( m_file, message );
	}
}

void Red::System::Log::File::Close()
{
	if ( m_file )
	{
		Internal::FileClose( m_file );
		m_file = nullptr;
	}
}

void Red::System::Log::File::Flush()
{
	if ( m_file )
	{
		Internal::FileFlush( m_file );
	}
}

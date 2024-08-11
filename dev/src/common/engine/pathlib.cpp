#include "build.h"

#include "pathlib.h"

IMPLEMENT_RTTI_ENUM( EPathLibCollision );

namespace PathLib
{

static CLog g_log;

///////////////////////////////////////////////////////////////////////////////
// CLog
///////////////////////////////////////////////////////////////////////////////

CLog::CLog()
	: m_linesCount( 0 )
	, m_currentLine( 0 )
	, m_version( 0 )
	
{
}
void CLog::Log( ELineFlag flag, const Char* format, ... )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	va_list arglist;
	va_start(arglist, format);
	Char formattedBuf[4096];

	Red::System::VSNPrintF(formattedBuf,  ARRAY_COUNT(formattedBuf), format, arglist); 
	Write( flag, formattedBuf );
	m_version.Increment();
}


void CLog::Write( ELineFlag flag, const Char* text )
{
	if ( (*text) == 0 )
		return;

	// add line
	if ( ( ++m_currentLine) == s_maxLines )
		m_currentLine = 0;
	m_linesCount = Min( m_linesCount+1, s_maxLines );

	Uint32 len = 0;
	for ( ; text[ len ] == 0; ++len )
	{
		if ( text[ len ] == TXT('\n') )
		{
			m_lines[ m_currentLine ].Set( text, len );
			m_lineFlags[ m_currentLine ] = Int8(flag);
			text += len + 1;
			Write( flag, text );
			return;
		}
	}
	m_lines[ m_currentLine ] = text;
	m_lineFlags[ m_currentLine ] = Int8(flag);
}

CLog* CLog::GetInstance()
{
	return &g_log;
}

};		// namespace PathLib


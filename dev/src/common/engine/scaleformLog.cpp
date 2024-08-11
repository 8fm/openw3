/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "scaleformLog.h"

#ifdef USE_SCALEFORM

#include "../core/tokenizer.h"

#include "guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CScaleformLog
//////////////////////////////////////////////////////////////////////////
CScaleformLog::CScaleformLog()
{
#ifndef RED_FINAL_BUILD
	CTokenizer tokenizer( SGetCommandLine(), TXT(" ") );
	for ( Uint32 i = 0; i < tokenizer.GetNumTokens(); ++i )
	{
		String token = tokenizer.GetToken( i );
		if ( token == TXT("-flashtrace_prefix") )
		{
			m_traceFilterPrefix = tokenizer.GetToken( i + 1 );
			break;
		}
	}
#endif
}

// Mostly copy paste from CGUIManager. Change later if needs be.
void CScaleformLog::LogMessageVarg( SF::LogMessageId messageId, const SFChar* pfmt, va_list argList )
{
	SFChar buffer[2048];
	Red::System::VSNPrintF( buffer, ARRAY_COUNT( buffer ), pfmt, argList );

	// Trim last newline char if exist
	Int32 length = static_cast< Int32 >( Red::System::StringLength( buffer ) );
	if ( length > 0 && buffer[length - 1] == '\n' )
	{
		buffer[length - 1] = 0;
	}

	String message = FLASH_LOG_UTF8_TO_TXT( buffer );

	// Don't care about channels, only message type is important
	SF::LogMessageType messageType =  messageId.GetMessageType();

	// Print message to screen
	if ( messageType == SF::LogMessage_Text )
	{
#ifndef RED_FINAL_BUILD
		if ( ! m_traceFilterPrefix.Empty() && message.BeginsWith( m_traceFilterPrefix ) )
		{
			GUI_LOG( TXT("Scaleform Text: '%ls'"), message.AsChar() );
		}	
#else
		GUI_LOG( TXT("Scaleform Text: '%ls'"), message.AsChar() );
#endif
	}
	else if ( messageType == SF::LogMessage_Warning  )
	{
		GUI_WARN( TXT("Scaleform Warning: '%ls'"), message.AsChar() );
	}
	else if ( messageType == SF::LogMessage_Error )
	{
		GUI_ERROR( TXT("Scaleform Error: '%ls'"), message.AsChar() );
	}
	else if ( messageType == SF::LogMessage_Assert )
	{
		GUI_ERROR( TXT("Scaleform Assert: '%ls'"), message.AsChar() );
	}
	else
	{
		GUI_LOG( TXT("Unknown Scaleform Message: '%ls'"), message.AsChar() );
	}
}

#endif // USE_SCALEFORM

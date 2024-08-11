/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CScaleformLog
//////////////////////////////////////////////////////////////////////////
class CScaleformLog : public GFx::Log
{
#ifndef RED_FINAL_BUILD
private:
	String m_traceFilterPrefix;
#endif

public:
	CScaleformLog();

public:
	virtual void LogMessageVarg( SF::LogMessageId messageId, const SFChar* pfmt, va_list argList );
};

#endif // USE_SCALEFORM

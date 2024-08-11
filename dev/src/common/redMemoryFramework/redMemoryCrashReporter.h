/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef RED_MEMORY_CRASH_REPORTER_H
#define RED_MEMORY_CRASH_REPORTER_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "../redSystem/crashReportDataBuffer.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////////
// CrashReporter
//	Writes data from the memory manager on out-of-memory so it can be
//  passed to the crash reporter tool
class CrashReporter
{
public:
	CrashReporter();
	~CrashReporter();

	static CrashReporter& GetInstance();
	void Initialise();
	void WriteCrashData( const Red::System::Char* crashDataBuffer, Red::System::MemSize dataSize );

private:
	static Red::System::Uint32 ErrorHandlerDebugInfoCallback( Red::System::Char* buffer, Red::System::Uint32 size );

	Red::System::Error::CrashReportDataBuffer m_crashReportBuffer;
};

} }

#endif
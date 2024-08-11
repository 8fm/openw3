/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "redMemoryCrashReporter.h"
#include "../redSystem/error.h"

namespace Red { namespace MemoryFramework {

const Red::System::MemSize c_MaximumCrashReportSize = 1024u * 8u;		// Max data we write to a crash report

RED_DECLARE_DEBUG_CALLBACK_TAG( RedMemory );

//////////////////////////////////////////////////////////////////////
// Default CTor
//	Creates the crash reporter buffer
CrashReporter::CrashReporter()
	: m_crashReportBuffer( TXT( "RedMemory" ), c_MaximumCrashReportSize )
{
	RED_REGISTER_DEBUG_CALLBACK( RedMemory, ErrorHandlerDebugInfoCallback );
}

//////////////////////////////////////////////////////////////////////
// DTor
//	Doesn't do much...
CrashReporter::~CrashReporter()
{
	RED_UNREGISTER_DEBUG_CALLBACK( RedMemory );
}

//////////////////////////////////////////////////////////////////////
// ErrorHandlerDebugInfoCallback
//	Appends the contents of the crash report buffer to the error handler debug info
Red::System::Uint32 CrashReporter::ErrorHandlerDebugInfoCallback( Red::System::Char* buffer, Red::System::Uint32 size )
{
	Red::System::MemSize bufferSize = 0;
	void* crashBuffer = CrashReporter::GetInstance().m_crashReportBuffer.ReadBufferContents( bufferSize );
	if( bufferSize > 0 && crashBuffer != nullptr )
	{
		Red::System::MemSize bufferToWrite = Red::Math::NumericalUtils::Min( (Red::System::MemSize)size, bufferSize );
		if( bufferToWrite > 0 )
		{
			Red::System::Char* errorReportBuffer = reinterpret_cast< Red::System::Char* >( crashBuffer );
			Red::System::StringCopy( buffer, errorReportBuffer, bufferToWrite );
			return (Red::System::Uint32)bufferToWrite;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Initialise
//	Just used to touch the crash reporter internally so the file is mapped as soon as possible
void CrashReporter::Initialise()
{
}

//////////////////////////////////////////////////////////////////////
// GetInstance
//	Singleton .. not ideal but ok for now. One crash reporter for all memory managers
CrashReporter& CrashReporter::GetInstance()
{
	static CrashReporter s_instance;

	return s_instance;
}

//////////////////////////////////////////////////////////////////////
// OnOutOfMemory
//	Writes data from the memory manager to the crash report
void CrashReporter::WriteCrashData( const Red::System::Char* crashDataBuffer, Red::System::MemSize dataSize )
{
	m_crashReportBuffer.Write( crashDataBuffer, dataSize );
}

} }
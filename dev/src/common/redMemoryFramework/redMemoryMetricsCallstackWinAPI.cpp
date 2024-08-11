/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCallstack.h"
#include "../redMath/numericalutils.h"
#include "../redSystem/os.h"
#include <DbgHelp.h>

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////
// Win32 Callstack helper
namespace WinAPI {

Red::System::Uint32 WalkStack( MetricsCallstack::CallStackAddress* stack, Red::System::Uint32 size, Red::System::Uint32 framesToSkip, Red::System::Uint64& hash )
{
	// Temporary list of stack frame pointers used for capture
	VOID* callStackFramesTemp[ MetricsCallstack::k_largestStackFrame ] = { nullptr };
	DWORD callstackHash = 0u;
	USHORT framesCaptured = RtlCaptureStackBackTrace( framesToSkip, Red::Math::NumericalUtils::Min( size, 63u ), callStackFramesTemp, &callstackHash );

	// Copy stack frame 
	for( USHORT frame = 0; frame < framesCaptured; ++frame )
	{
		stack[ frame ] = reinterpret_cast< MetricsCallstack::CallStackAddress >( callStackFramesTemp[ frame ] );
	}

	hash = callstackHash;

	return framesCaptured;
}

void GetSymbolInfoForAddress( MetricsCallstack::CallStackAddress address,  Red::System::Char* symbol, Red::System::Int32 symLength,  Red::System::Char* file,  Red::System::Int32 fileLength )
{
	Red::System::StringCopy(symbol, TXT("<unknown-symbol>"), symLength);
	Red::System::StringCopy(file, TXT("<unknown-file>"), fileLength);

	IMAGEHLP_LINE64 lineInfo = {0};
	lineInfo.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
	DWORD displacement = 0;
	HANDLE thisProcess = GetCurrentProcess();

	BOOL result = SymGetLineFromAddr64( thisProcess, address, &displacement, &lineInfo ) == TRUE;

	if (result)
	{
		Red::System::AnsiChar* offset = Red::System::StringSearch(lineInfo.FileName, "\\src\\" );
		Red::System::AnsiChar* fileName;

		if (offset)
			fileName = offset;
		else
			fileName = lineInfo.FileName;

		Red::System::SNPrintF( file, fileLength, TXT( "%S(%d)" ), fileName, lineInfo.LineNumber );
	}

	// Symbol buffer struct is loaded in-place
	BYTE symbolBuffer[ sizeof(SYMBOL_INFO) + 1024 ];
	Red::System::MemoryZero( symbolBuffer, sizeof(symbolBuffer) );
	PSYMBOL_INFO symbolInfo = (PSYMBOL_INFO)symbolBuffer;
	symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbolInfo->MaxNameLen = 1024;
	DWORD64 displacement64 = 0;
	result = SymFromAddr( thisProcess, address, &displacement64, symbolInfo ) == TRUE;
	if (result)
	{
		Red::System::SNPrintF(symbol, symLength, TXT( "%S()+%d" ), symbolInfo->Name, displacement);
	}		
}

}

/////////////////////////////////////////////////////////////
// CTor
//	This should grab the callstack addresses on construction
MetricsCallstack::MetricsCallstack( Red::System::Int32 stackAddressesToIgnore )
	: m_callStackDepth(0)
{
	m_callStackDepth = WinAPI::WalkStack( m_callstack, k_largestStackFrame, stackAddressesToIgnore, m_hash );
}

/////////////////////////////////////////////////////////////
// GetAsString
//	Resolve a stack frame pointer to a string
void MetricsCallstack::GetAsString( Red::System::Int32 index, Red::System::Char* stringBuffer, Red::System::Int32 maxLength )
{
	RED_MEMORY_ASSERT( index < m_callStackDepth,  "No data at this stack-frame depth" );
	if( index < m_callStackDepth )
	{
		Red::System::Char filename[128];
		Red::System::Char symbol[128];
		WinAPI::GetSymbolInfoForAddress( m_callstack[index], symbol, 128, filename, 128 );

		Red::System::SNPrintF( stringBuffer, maxLength, TXT( "%s - %s" ), symbol, filename );
	}
	else
	{
		Red::System::SNPrintF( stringBuffer, maxLength, TXT( "< Unknown >" ) );
	}
}

} }
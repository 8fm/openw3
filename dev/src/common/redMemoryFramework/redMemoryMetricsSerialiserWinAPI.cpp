/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsSerialiser.h"
#include <DbgHelp.h>

namespace Red { namespace MemoryFramework {

const Red::System::AnsiChar c_platformIdentifier[] = "WINDOWS";

#ifdef RED_ARCH_X86
	const Red::System::AnsiChar c_architectureIdentifier[] = "32";
#else
	const Red::System::AnsiChar c_architectureIdentifier[] = "64";
#endif

//////////////////////////////////////////////////////////////
// EnumerateModuleCallback
//	Called for each loaded module
BOOL CALLBACK EnumerateLoadedModulesProc64( _In_ PCSTR moduleName, _In_ DWORD64 moduleBase, _In_ ULONG moduleSize, _In_opt_ PVOID userContext )
{
	RED_UNUSED( moduleSize );

	const char c_modulePrefix[] = "MOD_";									// Used to identify modules
	Red::System::Uint32 moduleNameLength = static_cast< Red::System::Uint32 >( Red::System::StringLength( moduleName ) );
	Red::System::Uint64 moduleSize64 = static_cast< Red::System::Uint64  >( moduleSize );
	OSAPI::FileWriter* fileWriter = reinterpret_cast< OSAPI::FileWriter* >( userContext );

	fileWriter->Write( c_modulePrefix, sizeof( c_modulePrefix )-1 );		// Don't write the terminator
	fileWriter->Write( &moduleSize64, sizeof( moduleSize64 ) );
	fileWriter->Write( &moduleNameLength, sizeof( moduleNameLength ) );
	fileWriter->Write( moduleName, Red::System::StringLength( moduleName ) );
	fileWriter->Write( &moduleBase, sizeof( moduleBase ) );

	return TRUE;
}

//////////////////////////////////////////////////////////////
// WriteDumpHeader_Platform
//	Windows-specific debug data
void MetricsSerialiser::WriteDumpHeader_Platform()
{
	const char c_modulePostfix[] = "_MOD";	// Mark the end of modules

	m_fileWriter.Write( c_platformIdentifier, sizeof( c_platformIdentifier )-sizeof(c_platformIdentifier[0]) );
	m_fileWriter.Write( c_architectureIdentifier, sizeof( c_architectureIdentifier )-sizeof(c_architectureIdentifier[0]) );

	// Enumerate the loaded modules
	HANDLE thisProcess = GetCurrentProcess();
	EnumerateLoadedModules64( thisProcess, EnumerateLoadedModulesProc64, (void*)( &m_fileWriter ) ); 

	m_fileWriter.Write( c_modulePostfix, sizeof( c_modulePostfix ) - 1 );
}

} }
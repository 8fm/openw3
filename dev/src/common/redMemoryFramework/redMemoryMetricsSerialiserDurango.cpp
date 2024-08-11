/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsSerialiser.h"

namespace Red { namespace MemoryFramework {

const Red::System::AnsiChar c_platformIdentifier[8] = "DURANGO";
const Red::System::AnsiChar c_architectureIdentifier[] = "64";

//////////////////////////////////////////////////////////////
// WriteDumpHeader_Platform
//	Durango-specific debug data
void MetricsSerialiser::WriteDumpHeader_Platform()
{
	const char c_modulePrefix[] = "MOD_";	
	const char c_modulePostfix[] = "_MOD";	// Mark the end of modules

	m_fileWriter.Write( c_platformIdentifier, sizeof( c_platformIdentifier )-sizeof(c_platformIdentifier[0]) );
	m_fileWriter.Write( c_architectureIdentifier, sizeof( c_architectureIdentifier )-sizeof(c_architectureIdentifier[0]) );

	HMODULE thisProcess = GetModuleHandle(NULL);
	const Red::System::Uint32 size = 256;
	char filename[ size ];
	GetModuleFileNameA( NULL, filename, size );
	
	Red::System::Uint64 moduleSize = 0;
	Red::System::Uint32 moduleNameLength = static_cast< Red::System::Uint32 >( Red::System::StringLength( filename ) );
	m_fileWriter.Write( c_modulePrefix, sizeof( c_modulePrefix )-1 );		// Don't write the terminator
	m_fileWriter.Write( &moduleSize, sizeof( moduleSize ) );
	m_fileWriter.Write( &moduleNameLength, sizeof( moduleNameLength ) );
	m_fileWriter.Write( filename, moduleNameLength );
	m_fileWriter.Write( &thisProcess, sizeof( thisProcess ) );

	m_fileWriter.Write( c_modulePostfix, sizeof( c_modulePostfix ) - 1 );
}

} }
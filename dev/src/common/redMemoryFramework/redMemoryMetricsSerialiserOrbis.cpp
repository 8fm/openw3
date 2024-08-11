/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsSerialiser.h"

namespace Red { namespace MemoryFramework {

const Red::System::AnsiChar c_platformIdentifier[8] = "ORBIS";
const Red::System::AnsiChar c_architectureIdentifier[] = "64";

//////////////////////////////////////////////////////////////
// WriteDumpHeader_Platform
//	Orbis-specific debug data
void MetricsSerialiser::WriteDumpHeader_Platform()
{
	const char c_modulePrefix[] = "MOD_";	
	const char c_modulePostfix[] = "_MOD";	// Mark the end of modules

	m_fileWriter.Write( c_platformIdentifier, sizeof( c_platformIdentifier ) );
	m_fileWriter.Write( c_architectureIdentifier, sizeof( c_architectureIdentifier )-sizeof(c_architectureIdentifier[0]) );
	m_fileWriter.Write( c_modulePrefix, sizeof( c_modulePrefix )-1 );

	Red::System::Uint64 moduleSize = 0;
	m_fileWriter.Write( &moduleSize, sizeof( moduleSize ) );

	size_t size = 64;
	size_t moduleCount = 0;
	
	SceKernelModule m_modules[ size ];

	if( sceKernelGetModuleList( m_modules, size, &moduleCount ) == SCE_OK )
	{
		SceKernelModuleInfo module;
		module.size = sizeof(SceKernelModuleInfo);
		if( sceKernelGetModuleInfo( m_modules[ 0 ], &module ) == SCE_OK ) // first module is always the game filename
		{
			Red::System::Uint32 moduleNameLength = static_cast< Red::System::Uint32 >( Red::System::StringLength( module.name ) );
			m_fileWriter.Write( &moduleNameLength, sizeof( moduleNameLength ) );
			m_fileWriter.Write( module.name, moduleNameLength );
		}	
	}

	m_fileWriter.Write( c_modulePostfix, sizeof( c_modulePostfix ) - 1 );		
}

} }
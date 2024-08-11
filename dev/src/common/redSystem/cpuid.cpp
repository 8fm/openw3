/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "cpuid.h"

#include "os.h"
#include "crt.h"
#include "utility.h"

#if defined( RED_COMPILER_MSC )
#	include <intrin.h>
#endif

using namespace Red::System;

CpuId::CpuId()
:	m_cpuCapabilityFlags( 0 )
{
#if defined( RED_COMPILER_MSC )
	int info[ 4 ];

	__cpuid( info, 0 );

	MemoryZero( m_vendor, sizeof( AnsiChar ) * ARRAY_COUNT( m_vendor ) );
	*reinterpret_cast< int* >( m_vendor )						= info[ 1 ];
	*reinterpret_cast< int* >( m_vendor + sizeof( int ) )		= info[ 3 ];
	*reinterpret_cast< int* >( m_vendor + sizeof( int ) * 2 )	= info[ 2 ];

	Bool processorIsIntel = StringCompareNoCase( m_vendor, "genuineintel" ) == 0;

	__cpuid( info, 1 );
	m_physicalCores = m_logicalCores = ( ( info[ 1 ] >> 16 ) & 0xff );
	
	m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 23 ) )? FLAG( CPU_MMX ) : 0;
	m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 25 ) )? FLAG( CPU_SSE1 ) : 0;
	m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 26 ) )? FLAG( CPU_SSE2 ) : 0;
	m_cpuCapabilityFlags |= ( info[ 2 ] & FLAG( 0 ) )? FLAG( CPU_SSE3 ) : 0;
	m_cpuCapabilityFlags |= ( info[ 2 ] & FLAG( 19 ) )? FLAG( CPU_SSE4_1 ) : 0;
	m_cpuCapabilityFlags |= ( info[ 2 ] & FLAG( 20 ) )? FLAG( CPU_SSE4_2 ) : 0;

	if( processorIsIntel )
	{
		m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 28 ) )? FLAG( CPU_Hyperthreading ) : 0;

		__cpuidex( info, 4, 0 );
		m_logicalCores = ( info[ 0 ] >> 26 ) + 1;

		if( GetCPUCapability( CPU_Hyperthreading ) )
		{
			m_physicalCores = m_logicalCores / 2;
		}
		else
		{
			m_physicalCores = m_logicalCores;
		}
	}


	__cpuid( info, 0x80000000 );

	if( info[ 0 ] >= 0x80000001 )
	{
		__cpuid( info, 0x80000001 );
		m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 30 ) )? FLAG( CPU_3DNowExt ) : 0;
		m_cpuCapabilityFlags |= ( info[ 3 ] & FLAG( 31 ) )? FLAG( CPU_3DNow ) : 0;
	}
#endif
}
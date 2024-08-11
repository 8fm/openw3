/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCallstack.h"

namespace Red { namespace MemoryFramework {

extern MemoryClass g_debugWriteMemoryClass;

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
///////////////////////////////////////////////////////////////////
// SetMetricsNameLookupTables
//	Pass a pool and class name lookup table so the metrics can report strings rather than labels
RED_INLINE void MetricsSerialiser::SetMetricsNameLookupTables( PoolNamesList* poolNames, MemoryClassNamesList* classNames )
{
	m_memoryPoolNames = poolNames;
	m_memoryClassNames = classNames;
}
#endif

///////////////////////////////////////////////////////////////////
//
//
RED_INLINE void MetricsSerialiser::WriteAddress( const void* address )
{
	Red::System::MemUint addressUintPtr = reinterpret_cast< Red::System::MemUint >( address );
	m_fileWriter.Write( &addressUintPtr, sizeof( addressUintPtr ) );
}

///////////////////////////////////////////////////////////////
// WriteAllocInfo
//	Writes an allocation / free
RED_INLINE void MetricsSerialiser::WriteAllocInfo( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	Red::System::Double time = m_timer.GetSeconds();
	Red::System::Uint32 size32 = static_cast< Red::System::Uint32 >( size );		// Max 4gig - 1 byte
	Red::System::Uint8 label8 = static_cast< Red::System::Uint8 >( label );
	Red::System::Uint8 memClass8 = static_cast< Red::System::Uint8 >( memoryClass );
	m_fileWriter.Write( &time, sizeof( time ) );
	m_fileWriter.Write( &label8, sizeof( label8 ) );
	m_fileWriter.Write( &memClass8, sizeof( memClass8 ) );
	WriteAddress( address );
	m_fileWriter.Write( &size32, sizeof( size32 ) );
	m_callstackWriter.WriteCallstack( callstack );
}

///////////////////////////////////////////////////////////////
// OnAreaAdded
//	Writes an allocation / free
RED_INLINE void MetricsSerialiser::OnAreaAdded( PoolLabel label, Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
	m_mutex.Acquire();
	const Red::System::AnsiChar prefix[2] = "G";
	m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
	Red::System::Uint8 label8 = static_cast< Red::System::Uint8 >( label );
	m_fileWriter.Write( &label8, sizeof( label8 ) );
	WriteAddress( reinterpret_cast< void* >( lowAddress ) );
	WriteAddress( reinterpret_cast< void* >( highAddress - lowAddress ) );
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////
// OnAreaRemoved
//	Writes an allocation / free
RED_INLINE void MetricsSerialiser::OnAreaRemoved( PoolLabel label, Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
	m_mutex.Acquire();
	const Red::System::AnsiChar prefix[2] = "H";
	m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
	Red::System::Uint8 label8 = static_cast< Red::System::Uint8 >( label );
	m_fileWriter.Write( &label8, sizeof( label8 ) );
	WriteAddress( reinterpret_cast< void* >( lowAddress ) );
	WriteAddress( reinterpret_cast< void* >( highAddress - lowAddress ) );
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////
// OnAllocation
//
RED_INLINE void MetricsSerialiser::OnAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack, AnsiChar* dbgString, Int32 dbgStringLength )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "A";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );	
		m_fileWriter.Write( &dbgStringLength, sizeof( dbgStringLength ) );
		if( dbgStringLength > 0 )
		{
			m_fileWriter.Write( dbgString, dbgStringLength * sizeof( dbgString[0] ) );
		}
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// OnStaticAllocation
//
RED_INLINE void MetricsSerialiser::OnStaticAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "B";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );	
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// OnOverflowAllocation
//
RED_INLINE void MetricsSerialiser::OnOverflowAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "C";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );	
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// OnFree
//
RED_INLINE void MetricsSerialiser::OnFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "D";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// OnStaticFree
//
RED_INLINE void MetricsSerialiser::OnStaticFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "E";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// OnOverflowFree
//
RED_INLINE void MetricsSerialiser::OnOverflowFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack )
{
	if( memoryClass == g_debugWriteMemoryClass || g_debugWriteMemoryClass == (MemoryClass)-1 )
	{
		m_mutex.Acquire();
		const Red::System::AnsiChar prefix[2] = "F";
		m_fileWriter.Write( prefix, sizeof( prefix )-sizeof( prefix[0] ) );
		WriteAllocInfo( label, memoryClass, address, size, callstack );
		m_mutex.Release();
	}
}

///////////////////////////////////////////////////////////////
// IsWritingDump
//
RED_INLINE Red::System::Bool	MetricsSerialiser::IsWritingDump() const
{
	return m_isWritingDump && m_fileWriter.IsFileOpen();
}

} }

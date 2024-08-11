/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsSerialiser.h"

namespace Red { namespace MemoryFramework {

MemoryClass g_debugWriteMemoryClass = (MemoryClass)-1;

///////////////////////////////////////////////////////////////
// CTor
//
MetricsSerialiser::MetricsSerialiser() :
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	 m_memoryPoolNames( nullptr )
	, m_memoryClassNames( nullptr ),
#endif
	m_isWritingDump(false)
	, m_callstackWriter( m_fileWriter )
{

}

///////////////////////////////////////////////////////////////
// DTor
//
MetricsSerialiser::~MetricsSerialiser()
{
	EndDump();
}

///////////////////////////////////////////////////////////////
// OnMemoryArea
//	Used when writing the dump header
void MetricsSerialiser::OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size )
{
	const Red::System::AnsiChar c_areaPrefix[] = "AREA_";
	m_fileWriter.Write( c_areaPrefix, sizeof(c_areaPrefix) - sizeof(c_areaPrefix[0]) );
	WriteAddress( reinterpret_cast< void* >( address ) );
	WriteAddress( reinterpret_cast< void* >( size ) );
}

///////////////////////////////////////////////////////////////
// BeginHeader
//	Allocator header writing (the serialiser does not have direct access to the allocators)
//	Note! This locks the mutex
void MetricsSerialiser::BeginHeader( )
{
	WriteDumpHeader();
	m_timer.Reset();
}

///////////////////////////////////////////////////////////////
// EndHeader
//	Release the mutex so we can start writing to the dumps
void MetricsSerialiser::EndHeader( )
{
	const Red::System::AnsiChar c_headerPostfix[] = "_HEAD";
	m_fileWriter.Write( c_headerPostfix, sizeof(c_headerPostfix)-sizeof(c_headerPostfix[0]));
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////
// BeginAllocatorHeader
//	Allocator header writing (the serialiser does not have direct access to the allocators)
void MetricsSerialiser::BeginAllocatorHeader( PoolLabel allocatorLabel )
{
	const Red::System::AnsiChar c_allocHeadPrefix[] = "POOL_";
	m_fileWriter.Write( c_allocHeadPrefix, sizeof(c_allocHeadPrefix)-sizeof(c_allocHeadPrefix[0]) );
	Red::System::Uint8 lbl8 = static_cast< Red::System::Uint8 >( allocatorLabel );
	m_fileWriter.Write( &lbl8, sizeof( lbl8 ) );
}

///////////////////////////////////////////////////////////////
// EndAllocatorHeader
//
void MetricsSerialiser::EndAllocatorHeader( )
{
	// May need this later
}

///////////////////////////////////////////////////////////////
// BeginDump
//	Note! The mutex is NOT release until EndHeader is called!
void MetricsSerialiser::BeginDump( const Red::System::Char* fileName )
{
	m_mutex.Acquire();
	RED_UNUSED( fileName );
	if( !m_isWritingDump && !m_fileWriter.IsFileOpen() )
	{
		// Open the file, clear write cache, etc
		OSAPI::EFileWriterResult result = m_fileWriter.OpenFile( fileName );
		if( result != OSAPI::FW_OK )
		{
			RED_MEMORY_HALT(  "Failed to open memory dump file for writing!" );
		}
		else
		{
			m_isWritingDump = true;
		}
	}
}

///////////////////////////////////////////////////////////////
// WriteDumpHeader
//
void MetricsSerialiser::WriteDumpHeader()
{
	WriteDumpHeader_Platform();

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	Red::System::Int32 nameCount = m_memoryPoolNames->GetNameCount();
	m_fileWriter.Write( &nameCount, sizeof(nameCount) );

	for( Red::System::Int32 poolIndex = 0; poolIndex < nameCount; poolIndex++ )
	{
		Red::System::AnsiChar poolName[32] = {'\0'};
		PoolLabel poolLabel = m_memoryPoolNames->GetLabelByIndex( poolIndex );
		m_memoryPoolNames->GetNameByLabel( poolLabel, poolName, 32 );

		Red::System::Uint8 labelByte = static_cast< Red::System::Uint8 >( poolLabel );
		m_fileWriter.Write( &labelByte, sizeof( labelByte ) );
		m_fileWriter.Write( poolName, sizeof( poolName ) );
	}

	nameCount = m_memoryClassNames->GetNameCount();
	m_fileWriter.Write( &nameCount, sizeof(nameCount) );
	for( Red::System::Int32 classIndex = 0; classIndex < nameCount; classIndex++ )
	{
		Red::System::AnsiChar className[32] = {'\0'};
		MemoryClass classId = m_memoryClassNames->GetLabelByIndex( classIndex );
		m_memoryClassNames->GetNameByLabel( classId, className, 32 );

		Red::System::Uint8 classByte = static_cast< Red::System::Uint8 >( classId );
		m_fileWriter.Write( &classByte, sizeof( classByte ) );
		m_fileWriter.Write( className, sizeof( className ) );
	}
#else
	const Red::System::Int32 nameCount = 0;
	m_fileWriter.Write( &nameCount, sizeof(nameCount) );		// pool count
	m_fileWriter.Write( &nameCount, sizeof(nameCount) );		// class count
#endif
}

///////////////////////////////////////////////////////////////
// EndDump
//
void MetricsSerialiser::EndDump( )
{
	m_mutex.Acquire();
	if( m_isWritingDump )
	{
		Flush();
		m_fileWriter.CloseFile();
		m_isWritingDump = false;
	}
	m_mutex.Release();
}

///////////////////////////////////////////////////////////////
// Flush
//
void MetricsSerialiser::Flush()
{
	m_fileWriter.Flush();
}

} } 
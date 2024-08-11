#include "build.h"
#include "triggerMaskBuffer.h"

CTriggerBitMask::CTriggerBitMask()
	: m_mask( NULL )
	, m_memorySize( 0 )
{
}

CTriggerBitMask::~CTriggerBitMask()
{
	if ( NULL != m_mask )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_mask );
		m_mask = NULL;
	}
}

void CTriggerBitMask::Reset()
{
	Red::System::MemoryZero( m_mask, m_memorySize );
}

void CTriggerBitMask::Resize( const Uint32 size )
{
	if ( NULL != m_mask )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_mask );
		m_mask = NULL;
	}

	// recreate buffers
	const Uint32 numWords = ( size + 31 ) / 32;
	m_memorySize = numWords * sizeof( Uint32 );
	m_mask = (Uint32*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TriggerSystem, m_memorySize );
	Red::System::MemoryZero( m_mask, m_memorySize );
}

CTriggerMaskBuffer::CTriggerMaskBuffer( const Uint32 initialSize )
	: m_capacity( 0 )
{
	Resize(initialSize);
}

CTriggerMaskBuffer::~CTriggerMaskBuffer()
{
}

void CTriggerMaskBuffer::Resize( const Uint32 newSize )
{
	// recreate buffers
	m_capacity = newSize;
	m_insideFlags.Resize( newSize );
	m_testedFlags.Resize( newSize );
	m_enterFlags.Resize( newSize );
	m_exitFlags.Resize( newSize );
	m_ignoreFlags.Resize( newSize );

	// resize the object collector 
	m_testedObjects.Resize( newSize );
	m_testedObjects.ClearFast();
}

void CTriggerMaskBuffer::Reset()
{
	for ( Uint32 i=0; i<m_testedObjects.Size(); ++i )
	{
		const Uint32 objectIndex = m_testedObjects[i];
		m_insideFlags.Clear( objectIndex );
		m_testedFlags.Clear( objectIndex );
		m_enterFlags.Clear( objectIndex );
		m_exitFlags.Clear( objectIndex );	
	}

	m_ignoreFlags.Reset();

	m_testedObjects.ClearFast();
}
/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////////////////////
// CTor
//
template < typename TSyncLock, class TScopedMemoryProtection >
TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::TLSFAllocatorBase( )
	: IAllocator( )
	, m_theAllocator( )
{
}

/////////////////////////////////////////////////////////////////////////////
// DTor
//
template < typename TSyncLock, class TScopedMemoryProtection >
TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::~TLSFAllocatorBase( )
{
}

/////////////////////////////////////////////////////////////////////////////
// IncreaseMemoryFootprint
//	Add areas if we can
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::OnOutOfMemory()
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	m_theAllocator.LogOutOfMemoryInfo();
}

/////////////////////////////////////////////////////////////////////////////
// IncreaseMemoryFootprint
//	Add areas if we can
template < typename TSyncLock, class TScopedMemoryProtection >
Red::System::Bool TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
{
	if( m_flags & Allocator_StaticSize )
	{
		return false;
	}
	else
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
		return m_theAllocator.IncreaseMemoryFootprint( areaCallback, sizeRequired );
	}
}

/////////////////////////////////////////////////////////////////////////////
// ReleaseFreeMemoryToSystem
//	Release any memory that we can
template < typename TSyncLock, class TScopedMemoryProtection >
Red::System::MemSize TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	return m_theAllocator.ReleaseFreeMemoryToSystem( areaCallback );
}

/////////////////////////////////////////////////////////////////////////////
// OwnsPointer
//	Returns true if this pool owns the pointer
template < typename TSyncLock, class TScopedMemoryProtection >
RED_INLINE Red::System::Bool TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::OwnsPointer( const void* ptr ) const
{
	if( m_flags & Allocator_StaticSize )
	{
		return m_theAllocator.PoolOwnsPointer( ptr );
	}
	else
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
		return m_theAllocator.PoolOwnsPointer( ptr );
	}
}

/////////////////////////////////////////////////////////////////////////////
// GetAllocationSize
//	Returns the size of the block pointed to by ptr or 0 if the pool doesn't own it
template < typename TSyncLock, class TScopedMemoryProtection >
RED_INLINE Red::System::MemSize TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::GetAllocationSize( const void* ptr ) const
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	return m_theAllocator.SizeOfOwnedBlock( ptr );
}

/////////////////////////////////////////////////////////////////////////////
// RequestAllocatorInfo
//	
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::RequestAllocatorInfo( AllocatorInfo& info )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	info.SetAllocatorBudget( m_theAllocator.GetMemoryBudget() );
	info.SetAllocatorTypeName( TXT( "TLSF" ) );
	info.SetPerAllocationOverhead( TLSF::c_UsedBlockHeaderSize );
}

/////////////////////////////////////////////////////////////////////////////
// Initialise
//
template < typename TSyncLock, class TScopedMemoryProtection >
EAllocatorInitResults TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
{
 	const CreationParameters& params = static_cast< const CreationParameters& >( parameters );
	m_flags = flags;	

	TLSFAllocatorImpl::EPoolCreationResults initResult = m_theAllocator.Initialise( params.m_initialSize, params.m_maximumSize, params.m_systemBlockSize, params.m_secondLevelGranularity, flags );
	if( initResult != TLSFAllocatorImpl::Pool_OK )
	{
		Release();
		switch( initResult )
		{
		case TLSFAllocatorImpl::Pool_OutOfMemory:
			return AllocInit_OutOfMemory;

		case TLSFAllocatorImpl::Pool_MemoryTooSmall:
		case TLSFAllocatorImpl::Pool_BadParameters:
			return AllocInit_BadParameters;

		case TLSFAllocatorImpl::Pool_OK:
			break;
		}
	}

	return AllocInit_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Release
//	Clean up the tlsf allocator and free any paged memory 
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::Release( )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	m_theAllocator.Release();
}

/////////////////////////////////////////////////////////////////////////////
// Allocate
//
template < typename TSyncLock, class TScopedMemoryProtection >
RED_INLINE void* TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	return m_theAllocator.Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
}

/////////////////////////////////////////////////////////////////////////////
// Reallocate
//
template < typename TSyncLock, class TScopedMemoryProtection >
RED_INLINE void* TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	return m_theAllocator.Reallocate( ptr, allocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
}

/////////////////////////////////////////////////////////////////////////////
// Free
//
template < typename TSyncLock, class TScopedMemoryProtection >
RED_INLINE EAllocatorFreeResults TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::Free( const void* ptr )
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	return m_theAllocator.Free( ptr );
}

/////////////////////////////////////////////////////////////////////////////
// DumpDebugOutput
//
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::DumpDebugOutput()
{
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	Red::System::Log::Manager::GetInstance().SetCrashModeActive( true );
	m_theAllocator.DumpDebugOutput();
	m_theAllocator.WalkHeapDebug();
	Red::System::Log::Manager::GetInstance().SetCrashModeActive( false );
}

/////////////////////////////////////////////////////////////////////////////
// DumpDebugOutput
//	Walk each large area of the allocator
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::WalkAllocator( AllocatorWalker* theWalker ) 
{ 
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	m_theAllocator.WalkAllocator( theWalker ); 
}

/////////////////////////////////////////////////////////////////////////////
// DumpDebugOutput
//	Walk all allocations in a particular area
template < typename TSyncLock, class TScopedMemoryProtection >
void TLSFAllocatorBase< TSyncLock, TScopedMemoryProtection >::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker ) 
{ 
	typename TSyncLock::TScopedLock lock( &m_syncLock );
	TScopedMemoryProtection lockMemoryForCpu( &m_theAllocator );
	m_theAllocator.WalkPoolArea( startAddress, size, theWalker ); 
}

} }
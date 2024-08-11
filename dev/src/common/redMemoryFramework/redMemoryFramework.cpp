/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryFramework.h"
#include "redMemoryManager.inl"
#include "redMemoryTlsfAllocator.inl"
#include "redMemorySmallBlockAllocator.inl"
#include "redMemoryGpuAllocator.inl"
#include "redMemoryVirtualAllocWrapper.inl"

namespace Red { namespace MemoryFramework {

////////////////////////////////////////////////////////////////////////
// Use these function to instantiate templated allocators. 

#define INSTANTIATE_ALLOCATOR( AllocatorType )		\
	template AllocatorType* AllocatorCreator::CreateAllocator< AllocatorType >();	\
	template void AllocatorCreator::DestroyAllocator< AllocatorType >( AllocatorType* );	\
	template EMemoryManagerResults MemoryManager::AddNamedPool< AllocatorType >( PoolLabel, const IAllocatorCreationParameters&, Red::System::Uint32 );		\
	template void* MemoryManager::Allocate< AllocatorType >( PoolLabel, MemoryClass, Red::System::MemSize, Red::System::MemSize );		\
	template EAllocatorFreeResults MemoryManager::Free< AllocatorType >( PoolLabel, MemoryClass, const void*, Red::System::MemSize* );		\
	template void* MemoryManager::Reallocate< AllocatorType >( PoolLabel, void*, MemoryClass, Red::System::MemSize, Red::System::MemSize );

#define INSTANTIATE_ALLOCATOR_TEMPLATES( AllocatorType )		\
	template class AllocatorType;								\
	template AllocatorType* AllocatorCreator::CreateAllocator< AllocatorType >();	\
	template void AllocatorCreator::DestroyAllocator< AllocatorType >( AllocatorType* );	\
	template EMemoryManagerResults MemoryManager::AddNamedPool< AllocatorType >( PoolLabel, const IAllocatorCreationParameters&, Red::System::Uint32 );		\
	template void* MemoryManager::Allocate< AllocatorType >( PoolLabel, MemoryClass, Red::System::MemSize, Red::System::MemSize );		\
	template EAllocatorFreeResults MemoryManager::Free< AllocatorType >( PoolLabel, MemoryClass, const void*, Red::System::MemSize* );		\
	template void* MemoryManager::Reallocate< AllocatorType >( PoolLabel, void*, MemoryClass, Red::System::MemSize, Red::System::MemSize );		

#define INSTANTIATE_REGION_ALLOCATOR( AllocatorType )	\
	template class AllocatorType;	\
	template AllocatorType* AllocatorCreator::CreateAllocator< AllocatorType >();	\
	template void AllocatorCreator::DestroyAllocator< AllocatorType >( AllocatorType* );	\
	template EMemoryManagerResults MemoryManager::AddNamedPool< AllocatorType >( PoolLabel, const IAllocatorCreationParameters&, Red::System::Uint32 );		\
	template MemoryRegionHandle MemoryManager::AllocateRegion< AllocatorType >( PoolLabel, MemoryClass, Red::System::MemSize, Red::System::MemSize, RegionLifetimeHint );		\
	template MemoryRegionHandle MemoryManager::SplitRegion< AllocatorType >( PoolLabel label, MemoryRegionHandle& baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitAlignment );	\
	template EAllocatorFreeResults MemoryManager::FreeRegion< AllocatorType >( PoolLabel, MemoryRegionHandle& );	\
	template void MemoryManager::UnlockRegion< AllocatorType >( PoolLabel, MemoryRegionHandle );	\
	template void MemoryManager::LockRegion< AllocatorType >( PoolLabel, MemoryRegionHandle );

	////////////////////////////////////////////////////////////////////////
	// Explicitly instantiate templated stuff here so it gets linked into this library
	INSTANTIATE_ALLOCATOR_TEMPLATES( TLSFAllocatorBase< CMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( TLSFAllocatorBase< CNullMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( TLSFAllocatorBase< CSpinlock > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( SmallBlockAllocator< CMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( SmallBlockAllocator< CSpinlock > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( SmallBlockAllocator< CNullMutex > );
	typedef TLSFAllocatorBase< CMutex, TLSFAllocatorImpl::ScopedGpuProtection > _InternalGpuTlsfLocked;
	typedef TLSFAllocatorBase< CNullMutex, TLSFAllocatorImpl::ScopedGpuProtection > _InternalGpuTlsfNoLock;
	INSTANTIATE_ALLOCATOR( _InternalGpuTlsfLocked );
	INSTANTIATE_ALLOCATOR( _InternalGpuTlsfNoLock );
	INSTANTIATE_REGION_ALLOCATOR( GpuAllocator< CMutex > );
	INSTANTIATE_REGION_ALLOCATOR( GpuAllocator< CNullMutex > );
	INSTANTIATE_ALLOCATOR( MultiAllocator );
	INSTANTIATE_ALLOCATOR_TEMPLATES( VirtualAllocWrapperAllocator< CMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( VirtualAllocWrapperAllocator< CNullMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( VirtualAllocWrapperAllocator< CSpinlock > );
#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
	INSTANTIATE_ALLOCATOR( DebugAllocator );
#endif
#ifdef RED_PLATFORM_ORBIS
	INSTANTIATE_ALLOCATOR_TEMPLATES( SmallBlockAllocator< CAdaptiveMutex > );
	INSTANTIATE_ALLOCATOR_TEMPLATES( TLSFAllocatorBase< CAdaptiveMutex > );
#endif
} } // namespace Red { namespace MemoryFramework {
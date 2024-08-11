/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_FRAMEWORK_NULL_ALLOCATOR_INL
#define _RED_MEMORY_FRAMEWORK_NULL_ALLOCATOR_INL
#pragma once

#include "redMemoryAllocatorInfo.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////////////
// CTor
//
NullAllocator::NullAllocator()
	: IAllocator()
{
}

//////////////////////////////////////////////////////////////////////////
// DTor
//
NullAllocator::~NullAllocator()
{
}

//////////////////////////////////////////////////////////////////////////
// IncreaseMemoryFootprint
//
Red::System::Bool NullAllocator::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
{
	RED_UNUSED( areaCallback );
	RED_UNUSED( sizeRequired );
	return false;
}

//////////////////////////////////////////////////////////////////////////
// ReleaseFreeMemoryToSystem
//
Red::System::MemSize NullAllocator::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
{
	RED_UNUSED( areaCallback );
	return 0u;
}

//////////////////////////////////////////////////////////////////////////
// RequestAllocatorInfo
//	
void NullAllocator::RequestAllocatorInfo( AllocatorInfo& info )
{
	info.SetAllocatorBudget( 0 );
	info.SetAllocatorTypeName( TXT( "Null" ) );
	info.SetPerAllocationOverhead( 0 );
}

//////////////////////////////////////////////////////////////////////////
// Initialise
//	
EAllocatorInitResults NullAllocator::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
{
	m_flags = flags;
	RED_UNUSED( parameters );
	return AllocInit_OK;
}

//////////////////////////////////////////////////////////////////////////
// Release
//
void NullAllocator::Release( )
{
}

//////////////////////////////////////////////////////////////////////////
// Allocate
//	Just returns a non-null value. DO NOT USE THE POINTER RETURNED!
void* NullAllocator::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
{ 
	RED_UNUSED( allocSize );
	RED_UNUSED( allocAlignment );
	allocatedSize = sizeof( Red::System::MemSize );
	return (void*)0xfeedface;
}

//////////////////////////////////////////////////////////////////////////
// Free
//	Does nothing
EAllocatorFreeResults NullAllocator::Free( void* ptr )
{
	RED_UNUSED( ptr );
	return Free_OK;	
}

//////////////////////////////////////////////////////////////////////////
// GetAllocationSize
//	...
Red::System::MemSize NullAllocator::GetAllocationSize( void* ptr ) const
{
	RED_UNUSED( ptr );
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// OwnsPointer
//	...
Red::System::Bool NullAllocator::OwnsPointer( void* ptr ) const
{
	RED_UNUSED( ptr );
	return false;
}

//////////////////////////////////////////////////////////////////////////
// Reallocate
//	Does nothing, returns a non-null value to indicate success
void* NullAllocator::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
{
	RED_UNUSED( ptr );
	RED_UNUSED( allocSize );
	RED_UNUSED( allocAlignment );

	freedSize = sizeof( Red::System::MemSize );
	allocatedSize = sizeof( Red::System::MemSize );

	return (void*)0xfacefeed;
}

} } // namespace Red { namespace MemoryFramework {

#endif
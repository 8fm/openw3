/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_REGION_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_REGION_ALLOCATOR_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "redMemoryAllocator.h"

// Use this to enable storage of debug strings in region handles.
// #define ENABLE_MEMORY_REGION_DEBUG_STRINGS

namespace Red { namespace MemoryFramework {

	enum RegionLifetimeHint
	{
		Region_Shortlived,
		Region_Longlived
	};

	// Region allocator is special case wrapper that hides 'normal' interface + exposes region interface
	class MemoryRegion
	{
	public:
		MemoryRegion() : m_alignedAddress( 0 ), m_alignedSize( 0 ) 
		{ 
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
			Red::System::StringCopy( m_debugString, "unknown", c_debugStringLength ); 
#endif
		}

		RED_INLINE void* GetRawPtr() const						{ return reinterpret_cast< void* >( m_alignedAddress ); }
		RED_INLINE Red::System::MemUint GetAddress() const		{ return m_alignedAddress; }
		RED_INLINE Red::System::MemSize	GetSize() const			{ return m_alignedSize; }
		RED_INLINE void SetMemoryClass( MemoryClass memoryClass )	{ m_memoryClass = memoryClass; }
		RED_INLINE MemoryClass GetMemoryClass() const			{ return m_memoryClass; }
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		RED_INLINE void SetDebugString( const UniChar* str ) 	
		{ 
			Red::System::MemSize strLength = Red::System::StringLength( str );
			const UniChar* srcBuffer = strLength < c_debugStringLength ? str : str + strLength - c_debugStringLength;
			Red::System::StringConvert( m_debugString, srcBuffer, c_debugStringLength ); 
		}
		RED_INLINE void SetDebugString( const AnsiChar* str )	
		{ 
			Red::System::MemSize strLength = Red::System::StringLength( str );
			const AnsiChar* srcBuffer = strLength < c_debugStringLength ? str : str + strLength - c_debugStringLength;
			Red::System::StringCopy( m_debugString, srcBuffer, c_debugStringLength ); 
		}
		RED_INLINE const AnsiChar* GetDebugString() const	 	{ return m_debugString; }
#endif

	private:
		// No copy / move
		MemoryRegion( const MemoryRegion& other );
		MemoryRegion( MemoryRegion&& other );

	protected:
		Red::System::MemUint m_alignedAddress;
		Red::System::Uint32 m_alignedSize;
		MemoryClass			m_memoryClass;
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		static const Uint32 c_debugStringLength = 32;
		AnsiChar			m_debugString[ c_debugStringLength ];
#endif
	};

	class MemoryRegionHandle
	{
	public:
		RED_INLINE MemoryRegionHandle()	
			: m_internalRegion( nullptr )	
		{ 
		}
		RED_INLINE MemoryRegionHandle( const MemoryRegion* theRegion ) 
			: m_internalRegion( theRegion )
		{ 
		}
		RED_INLINE MemoryRegionHandle( const MemoryRegionHandle& other ) 
			: m_internalRegion( other.m_internalRegion )
		{ 
		}
		RED_INLINE MemoryRegionHandle( MemoryRegionHandle&& other ) 
			: m_internalRegion( other.m_internalRegion )
		{ 
			other.m_internalRegion = nullptr;
		}
		RED_INLINE MemoryRegionHandle& operator=( const MemoryRegionHandle& other )	
		{ 
			if( &other != this )
			{
				m_internalRegion = other.m_internalRegion;
			}
			return *this;
		}
		RED_INLINE MemoryRegionHandle& operator=( MemoryRegionHandle&& other )	
		{ 
			if( &other != this )
			{
				m_internalRegion = other.m_internalRegion;
				other.m_internalRegion = nullptr;
			}
			return *this;
		}
		RED_INLINE void* GetRawPtr() const							{ return m_internalRegion->GetRawPtr(); }
		RED_INLINE Red::System::MemUint GetAddress() const			{ return m_internalRegion->GetAddress(); }
		RED_INLINE Red::System::MemSize	GetSize() const				{ return m_internalRegion->GetSize(); }
		RED_INLINE Bool IsValid() const								{ return m_internalRegion != nullptr && m_internalRegion->GetRawPtr() != nullptr; }
		RED_INLINE const MemoryRegion* GetRegionInternal() const	{ return m_internalRegion; }
		RED_INLINE MemoryClass GetMemoryClass() const				{ return m_internalRegion->GetMemoryClass(); }
		RED_INLINE void InternalSetMemoryClass( MemoryClass cls )	{ if( m_internalRegion ) const_cast< MemoryRegion* >( m_internalRegion )->SetMemoryClass( cls ); }
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
		RED_INLINE void SetDebugString( const UniChar* str ) 	
		{ 
			if( m_internalRegion )
			{
				const_cast< MemoryRegion* >( m_internalRegion )->SetDebugString( str );
			}
		}
		RED_INLINE void SetDebugString( const AnsiChar* str )	
		{ 
			if( m_internalRegion )
			{
				const_cast< MemoryRegion* >( m_internalRegion )->SetDebugString( str );
			}
		}
		RED_INLINE const AnsiChar* GetDebugString() const	 	{ return m_internalRegion ? m_internalRegion->GetDebugString() : ""; }
#endif

	private:
		const MemoryRegion* m_internalRegion;
	};

	class RegionAllocator : public IAllocator
	{
	public:
		RegionAllocator()			{ }
		virtual ~RegionAllocator()	{ }

		virtual MemoryRegionHandle RuntimeAllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint = Region_Longlived ) = 0;
		virtual EAllocatorFreeResults RuntimeFreeRegion( MemoryRegionHandle handle ) = 0;

		// chop off region AFTER given position, existing region is clipped
		// will return NULL if the block is to small for the split
		virtual MemoryRegionHandle RuntimeSplitRegion( MemoryRegionHandle baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitBlockAlignment ) = 0;

	private:
		void* RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		void* RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		EAllocatorFreeResults RuntimeFree( void* ptr );
		Red::System::MemSize RuntimeGetAllocationSize( void* ptr ) const;
		Red::System::Bool RuntimeOwnsPointer( void* ptr ) const;
	};

} }

#endif
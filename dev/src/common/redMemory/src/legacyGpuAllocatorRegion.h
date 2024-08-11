/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_LEGACY_GPU_ALLOCATOR_REGION_H_
#define _RED_MEMORY_LEGACY_GPU_ALLOCATOR_REGION_H_

namespace red
{
namespace memory
{
	// Region allocator is special case wrapper that hides 'normal' interface + exposes region interface
	class Region
	{
	public:
		Region() : m_alignedAddress( 0 ), m_alignedSize( 0 ) 
		{}

		RED_INLINE void* GetRawPtr() const						{ return reinterpret_cast< void* >( m_alignedAddress ); }
		RED_INLINE u64 GetAddress() const		{ return m_alignedAddress; }
		RED_INLINE u64 GetSize() const			{ return m_alignedSize; }

	private:
		// No copy / move
		Region( const Region& other );
		Region( Region&& other );

	protected:
		u64 m_alignedAddress;
		u32 m_alignedSize;
	};

	class RegionHandle
	{
	public:
		RED_INLINE RegionHandle()	
			: m_internalRegion( nullptr )	
		{ 
		}
		RED_INLINE RegionHandle( const Region* theRegion ) 
			: m_internalRegion( theRegion )
		{ 
		}
		RED_INLINE RegionHandle( const RegionHandle& other ) 
			: m_internalRegion( other.m_internalRegion )
		{ 
		}
		RED_INLINE RegionHandle( RegionHandle&& other ) 
			: m_internalRegion( other.m_internalRegion )
		{ 
			other.m_internalRegion = nullptr;
		}
		RED_INLINE RegionHandle& operator=( const RegionHandle& other )	
		{ 
			if( &other != this )
			{
				m_internalRegion = other.m_internalRegion;
			}
			return *this;
		}
		RED_INLINE RegionHandle& operator=( RegionHandle&& other )	
		{ 
			if( &other != this )
			{
				m_internalRegion = other.m_internalRegion;
				other.m_internalRegion = nullptr;
			}
			return *this;
		}
		RED_INLINE void* GetRawPtr() const							{ return m_internalRegion->GetRawPtr(); }
		RED_INLINE u64 GetAddress() const			{ return m_internalRegion->GetAddress(); }
		RED_INLINE u64	GetSize() const				{ return m_internalRegion->GetSize(); }
		RED_INLINE bool IsValid() const								{ return m_internalRegion != nullptr && m_internalRegion->GetRawPtr() != nullptr; }
		RED_INLINE const Region* GetRegionInternal() const	{ return m_internalRegion; }

	private:
		const Region* m_internalRegion;
	};
}
}

#endif

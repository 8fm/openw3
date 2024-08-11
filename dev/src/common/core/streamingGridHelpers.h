#pragma once

#include "math.h"

//-----------------------------

/// Element mask to use when collecting elements
class CStreamingEntryMask
{
public:
	CStreamingEntryMask( const Uint32 initialSize );
	~CStreamingEntryMask();

	// prepare for given number of objects (keeps current data)
	void Prepare( const Uint32 maxIndex );

	// get capacity
	RED_FORCE_INLINE const Uint32 GetCapacity() const
	{
		return m_maxIndex;
	}

	// set bit
	RED_FORCE_INLINE void Set( const Uint32 index )
	{
		RED_FATAL_ASSERT( index < m_maxIndex, "Accessing bit filed outside the range" );
		const Uint64 mask = ((Uint64)1) << (index & BIT_MASK);
		m_bits[ index >> BIT_SHIFT ] |= mask;
	}

	// clear bit
	RED_FORCE_INLINE void Clear( const Uint32 index )
	{
		RED_FATAL_ASSERT( index < m_maxIndex, "Accessing bit filed outside the range" );
		const Uint64 mask = ((Uint64)1) << (index & BIT_MASK);
		m_bits[ index >> BIT_SHIFT ] &= ~mask;
	}

	// clear all
	RED_FORCE_INLINE void ClearAll()
	{
		Red::MemoryZero( m_bits, m_maxIndex / 8 );
	}

	// test bit
	RED_FORCE_INLINE const Bool Test( const Uint32 index ) const
	{
		RED_FATAL_ASSERT( index < m_maxIndex, "Accessing bit filed outside the range" );
		const Uint64 mask = ((Uint64)1) << (index & BIT_MASK);
		return 0 != (m_bits[ index >> BIT_SHIFT ] & mask);
	}

private:
	static const Uint32 BIT_SHIFT = 6;
	static const Uint32 BIT_MASK = 63;

	Uint64*			m_bits;
	Uint32			m_maxIndex;
};

//-----------------------------

/// Position quantizer
class CStreamingPositionQuantizer
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_Engine, 16 );

public:
	CStreamingPositionQuantizer( Float worldSize );

	// Helper bullshit, waiting for RedMath to have this
	RED_ALIGNED_CLASS( VectorI, 16 )
	{
	public:
		union
		{
			__m128i	quad;

			struct 
			{
				Int32	x;
				Int32	y;
				Int32	z;
				Int32	w;
			};
		};

		RED_FORCE_INLINE VectorI()
		{
		}

		RED_FORCE_INLINE void SetScalar( Int32 v )
		{
			quad = _mm_set1_epi32( v );
		}

		RED_FORCE_INLINE void Add( const VectorI& v )
		{
			quad = _mm_add_epi32( quad, v.quad );
		}

		RED_FORCE_INLINE void SetAdd( const VectorI& a, const VectorI& b )
		{
			quad = _mm_add_epi32( a.quad, b.quad );
		}

		RED_FORCE_INLINE void Sub( const VectorI& v )
		{
			quad = _mm_sub_epi32( quad, v.quad );
		}

		RED_FORCE_INLINE void SetSub( const VectorI& a, const VectorI& b )
		{
			quad = _mm_sub_epi32( a.quad, b.quad );
		}

		RED_FORCE_INLINE void SetReplicateZ( const VectorI& v )
		{
			quad = _mm_shuffle_epi32( v.quad, _MM_SHUFFLE(2,2,2,2) );
		}
	};

	RED_FORCE_INLINE VectorI QuantizePosition( const Vector& v )
	{
		__m128 pos = _mm_set_ps( 0.0f, v.Z, v.Y, v.X );

		__m128 qpos = _mm_mul_ps( _mm_add_ps( pos, m_offset ), m_scale );
		qpos = _mm_max_ps( qpos, MinC );
		qpos = _mm_min_ps( qpos, MaxC );

		__m128i q = _mm_cvtps_epi32 ( qpos );
		return *(const VectorI*) &q;
	}

	RED_FORCE_INLINE VectorI QuantizePositionAndRadius( const Vector& v, const Float r )
	{
		__m128 pos = _mm_set_ps( r, v.Z, v.Y, v.X );

		__m128 qpos = _mm_mul_ps( _mm_add_ps( pos, m_offset ), m_scale );
		qpos = _mm_max_ps( qpos, MinC );
		qpos = _mm_min_ps( qpos, MaxC );

		__m128i q = _mm_cvtps_epi32 ( qpos );
		return *(const VectorI*) &q;
	}

private:
	static const __m128 MinC;
	static const __m128 MaxC;

	__m128	m_scale;
	__m128	m_offset;
};

//-----------------------------

/// Collector that is using static space
template< Uint32 N >
class TStreamingGridCollectorStack : public CStreamingGridCollector
{
public:
	TStreamingGridCollectorStack()
		: CStreamingGridCollector( &m_data[0], N )
	{}

private:
	Elem		m_data[ N ];
};

//-----------------------------


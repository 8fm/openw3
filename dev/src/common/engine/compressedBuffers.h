
#pragma once

#include "compressedRotation.h"
#include "animMath.h"

namespace
{
	void PackQuaternionIntoInt64( Uint32& valueA, Uint32& valueB, const AnimQuaternion& qin )
	{
		const Float f = 65535.f / 2.2f;
		const Float offset = -1.1f;
#ifdef USE_HAVOK_ANIMATION
		const hkReal fa = ( qin.m_vec( 0 ) - offset ) * f;
		const hkReal fb = ( qin.m_vec( 1 ) - offset ) * f;
		const hkReal fc = ( qin.m_vec( 2 ) - offset ) * f;
		const hkReal fd = ( qin.m_vec( 3 ) - offset ) * f;
#else
		const Float fa = ( qin.Quat.X - offset ) * f;
		const Float fb = ( qin.Quat.Y - offset ) * f;
		const Float fc = ( qin.Quat.Z - offset ) * f;
		const Float fd = ( qin.Quat.W - offset ) * f;
#endif
		RED_ASSERT( fa >= 0.0f && fa <= 65535.0f );
		RED_ASSERT( fb >= 0.0f && fb <= 65535.0f );
		RED_ASSERT( fc >= 0.0f && fc <= 65535.0f );
		RED_ASSERT( fd >= 0.0f && fd <= 65535.0f );

		union 
		{
			Uint32 u32;
			Uint16 u16[2];
		} uA, uB;

		uA.u16[0] = (Uint16)fa;
		uA.u16[1] = (Uint16)fb;
		uB.u16[0] = (Uint16)fc;
		uB.u16[1] = (Uint16)fd;

		valueA = uA.u32;
		valueB = uB.u32;
	}

	// dex's best guess implementation
	Uint32 PackQuaternionIntoUint32( const AnimQuaternion& qin )
	{
#ifdef USE_HAVOK_ANIMATION
		return hkVector4Util::packQuaternionIntoInt32( qin.m_vec );
#else
		const Float overScale = 1.1f; // quaternions are not that perfect...
		const Float base = 196736.5f; // large enough so we can convert back by direct cast (Havok did it like that)
		const Float scale = 128.0f / overScale;

		const Float x = base + (qin.Quat.X * scale);
		const Float y = base + (qin.Quat.Y * scale);
		const Float z = base + (qin.Quat.Z * scale);
		const Float w = base + (qin.Quat.W * scale);

		const Uint32 packedX = (reinterpret_cast<const Uint32&>(x) >> 6) & 0xFF;
		const Uint32 packedY = (reinterpret_cast<const Uint32&>(y) >> 6) & 0xFF;
		const Uint32 packedZ = (reinterpret_cast<const Uint32&>(z) >> 6) & 0xFF;
		const Uint32 packedW = (reinterpret_cast<const Uint32&>(w) >> 6) & 0xFF;

		return packedX | (packedY<<8) | (packedZ<<16) | (packedW<<24);
#endif
	}

	void UnpackInt64IntoQuaternion( const Uint32 valueA, const Uint32 valueB, AnimQuaternion& qout )
	{
		Int32 a = valueA;
		Int32 b = valueA >> 16;
		Int32 c = valueB;
		Int32 d = valueB >> 16;

		a &= 0xFFFF;
		c &= 0xFFFF;

		a -= 0x7FFF;
		b -= 0x7FFF;
		c -= 0x7FFF;
		d -= 0x7FFF;

		const Float f = 2.2f / 65535.f;
#ifdef USE_HAVOK_ANIMATION
		qout.m_vec(0) = f * (hkReal)a;
		qout.m_vec(1) = f * (hkReal)b;
		qout.m_vec(2) = f * (hkReal)c;
		qout.m_vec(3) = f * (hkReal)d;
#else
		qout.Quat.X = f * (Float)a;
		qout.Quat.Y = f * (Float)b;
		qout.Quat.Z = f * (Float)c;
		qout.Quat.W = f * (Float)d;
#endif
	}

	void UnpackUint32IntoQuaternion( const Uint32 val, AnimQuaternion& qout )
	{
#ifdef USE_HAVOK_ANIMATION
		hkVector4Util::unPackInt32IntoQuaternion( val, qout.m_vec );
#else
		const Int32 packedX = (val & 0xFF) - 128;
		const Int32 packedY = ((val>>8) & 0xFF) - 128;
		const Int32 packedZ = ((val>>8) & 0xFF) - 128;
		const Int32 packedW = ((val>>8) & 0xFF) - 128;
		const Float scale = 1.1f / 128.0f;
		qout.Quat.X = scale * packedX;
		qout.Quat.Y = scale * packedY;
		qout.Quat.Z = scale * packedZ;
		qout.Quat.W = scale * packedW;
#endif
	}
}

class CompressedQuaternionBuffer
{
	TDynArray< Uint32 >			m_data;
	ECompressedRotationType		m_type; // Hack

public:
	CompressedQuaternionBuffer() : m_type( CRT_32 ) {}

	RED_INLINE void SetType( ECompressedRotationType type ) { m_type = type; }

	RED_INLINE void Reserve( Uint32 size )
	{
		if ( m_type == CRT_64 )
		{
			m_data.Reserve( 2 * size );
		}
		else
		{
			m_data.Reserve( size );
		}
	}

	RED_INLINE Int32 Size() const
	{
		if ( m_type == CRT_64 )
		{
			return m_data.SizeInt() / 2;
		}
		else
		{
			return m_data.SizeInt();
		}
	}

	RED_INLINE Uint32 DataSize() const
	{
		return static_cast< Uint32 >( m_data.DataSize() );
	}

	RED_INLINE void AddQuaternion( const AnimQuaternion& quat )
	{
		if ( m_type == CRT_64 )
		{
			Uint32 a = 0, b = 0;
			PackQuaternionIntoInt64( a, b, quat );
			m_data.PushBack( a );
			m_data.PushBack( b );
		}
		else
		{
			m_data.PushBack( PackQuaternionIntoUint32( quat ) );
		}
	}
	RED_INLINE void GetQuaternion( const Int32 index, AnimQuaternion& quat ) const
	{
		if ( m_type != CRT_64 )
		{
			UnpackUint32IntoQuaternion( m_data[ index ], quat );
		}
		else
		{
			UnpackInt64IntoQuaternion( m_data[ 2*index ], m_data[ 2*index+1 ], quat );
		}
	}

	RED_INLINE void Clear()
	{
		m_data.Clear();
	}

	RED_INLINE void Serialize( IFile& file )
	{
		file << m_data;

		if ( file.GetVersion() >= VER_COMPR_ROT_TYPES )
		{
			if ( file.IsWriter() )
			{
				Int32 type = m_type;
				file << type;
			}
			else if ( file.IsReader() )
			{
				Int32 type = 0;
				file << type;
				m_type = (ECompressedRotationType)type;
			}
		}
	}
};

template< typename T >
class CompressedVectorBuffer
{
	TDynArray< T >		m_data;
	Float				m_offset;
	Float				m_scale;

public:
	RED_INLINE void Reserve( Uint32 size )
	{
		m_data.Reserve( size );
	}

	RED_INLINE void SetMinMax( Float min, Float max )
	{
		m_offset = min;
		m_scale = ( max - min ) / ( Float )NumericLimits< T >::Max();
	}

	RED_INLINE Int32 Size() const
	{
		return m_data.SizeInt();
	}

	RED_INLINE Uint32 DataSize() const
	{
		return static_cast< Uint32 >( m_data.DataSize() );
	}

	RED_INLINE void AddVector( const AnimVector4& vec )
	{
#ifdef USE_HAVOK_ANIMATION
		Float fx = ( vec( 0 ) - m_offset ) / m_scale;
		Float fy = ( vec( 1 ) - m_offset ) / m_scale;
		Float fz = ( vec( 2 ) - m_offset ) / m_scale;
#else
		Float fx = ( vec.X - m_offset ) / m_scale;
		Float fy = ( vec.Y - m_offset ) / m_scale;
		Float fz = ( vec.Z - m_offset ) / m_scale;
#endif
		ASSERT( fx >= 0.f && fx <= ( Float )NumericLimits< T >::Max() );
		ASSERT( fy >= 0.f && fy <= ( Float )NumericLimits< T >::Max() );
		ASSERT( fz >= 0.f && fz <= ( Float )NumericLimits< T >::Max() );

		T x = T( fx );
		T y = T( fy );
		T z = T( fz );

		m_data.PushBack( x );
		m_data.PushBack( y );
		m_data.PushBack( z );
	}

	RED_INLINE void GetVector( const Int32 index, AnimVector4& vec ) const
	{
#ifdef USE_HAVOK_ANIMATION
		vec.set( 
			m_scale * m_data[ 3 * index + 0 ] + m_offset,
			m_scale * m_data[ 3 * index + 1 ] + m_offset,
			m_scale * m_data[ 3 * index + 2 ] + m_offset );
#else
		vec.Set( m_scale * m_data[ 3 * index + 0 ] + m_offset,
				 m_scale * m_data[ 3 * index + 1 ] + m_offset,
				 m_scale * m_data[ 3 * index + 2 ] + m_offset, 
				 1.0f );
#endif
	}	

	RED_INLINE void Clear()
	{
		m_data.Clear();
		m_offset = 0.0f;
		m_scale = 0.0f;
	}

	RED_INLINE void Serialize( IFile& file )
	{
		file << m_data;
		file << m_offset;
		file << m_scale;
	}
};

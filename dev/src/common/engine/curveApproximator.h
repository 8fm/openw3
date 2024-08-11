/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CCurve;
class CurveParameter;

#define DEFAULT_NUM_SAMPLES 16

#if defined( RED_SSE )

// ** ************************************
//
template<>
RED_INLINE Vector Lerp( Float frac, const Vector & v0, const Vector & v1 )
{
	Vector v;

	__m128 v0m		= _mm_load_ps( v0.A );
	__m128 v1m		= _mm_load_ps( v1.A );

	_mm_store_ps( v.A, _mm_add_ps( v0m, _mm_mul_ps( _mm_load1_ps( &frac ), _mm_sub_ps( v1m, v0m ) ) ) );

	return v;
}

// ** ************************************
//
RED_INLINE Vector Clamp( const Vector & v )
{
	Vector vr;

	_mm_store_ps( vr.A, _mm_min_ps( _mm_set1_ps( 1.f ), _mm_max_ps( _mm_setzero_ps(), _mm_load_ps( v.A ) ) ) );

	return vr;
}

// ** ************************************
//
RED_INLINE Vector LerpClamp( Float frac, const Vector & v0, const Vector & v1 )
{
	Vector vr;

	__m128 v0m		= _mm_load_ps( v0.A );
	__m128 v1m		= _mm_load_ps( v1.A );
	__m128 lerped	= _mm_add_ps( v0m, _mm_mul_ps( _mm_load1_ps( &frac ), _mm_sub_ps( v1m, v0m ) ) );\

		_mm_store_ps( vr.A, _mm_min_ps( _mm_set1_ps( 1.f ), _mm_max_ps( _mm_setzero_ps(), lerped ) ) );

	return vr;
}

#else

// ** ************************************
//
RED_INLINE Vector LerpClamp( Float frac, const Vector & v0, const Vector & v1 )
{
	Vector vr;

	vr.X = v0.X + frac * ( v1.X - v0.X );
	vr.Y = v0.Y + frac * ( v1.Y - v0.Y );
	vr.Z = v0.Z + frac * ( v1.Z - v0.Z );
	vr.W = v0.W + frac * ( v1.W - v0.W );

	vr.X = Clamp<float>( vr.X, 0, 1 );
	vr.Y = Clamp<float>( vr.X, 0, 1 );
	vr.Z = Clamp<float>( vr.X, 0, 1 );
	vr.W = Clamp<float>( vr.X, 0, 1 );

	return vr;
}

#endif

namespace SampledCurveEvaluators
{
	// ** ************************************
	//
	template< typename T >
	RED_INLINE Int32	GetLowerIndex	( Float t, const TDynArray< T > & samples, Float & frac )
	{
		ASSERT( t > 0.f && t < 1.f );

		Float tScaled = t * Float( samples.Size() - 1 );
		Float flIndex = floorf( tScaled );
		
		frac = tScaled - flIndex;

		return (Int32) flIndex;
	}

	// ** ************************************
	//
	template< typename T >
	RED_INLINE Bool	BoundsEvaluator	( Float t, const TDynArray< T > & samples, T & val )
	{
		ASSERT( samples.Size() > 1 );

		if( t <= 0.f )
		{
			val = samples[ 0 ];

			return true;
		}

		if( t >= 1.f )
		{
			val = samples[ samples.Size() - 1 ];

			return true;
		}

		return false;
	}

	// ** ************************************
	//
	template< typename T >
	RED_INLINE T	SampleNearest	( Float t, const TDynArray< T > & samples )
	{
		T val = T();

		if( BoundsEvaluator( t, samples, val ) )
		{
			return val;
		}

		Float frac = 0.f;

		Int32 i = GetLowerIndex( t, samples, frac );

		return samples[ i ];
	}

	// ** ************************************
	//
	template< typename T >
	RED_INLINE T	SampleLinear	( Float t, const TDynArray< T > & samples )
	{
		T val = T();

		if( BoundsEvaluator( t, samples, val ) )
		{
			return val;
		}

		Float frac = 0.f;

		Int32 i = GetLowerIndex( t, samples, frac );

		return Lerp( frac, samples[ i ], samples[ i + 1 ] );
	}

	// ** ************************************
	//
	RED_INLINE Vector	SampleLinear( Float t, const TDynArray< Vector > & samples )
	{
		// FIXME: clamping to [0,0.999f] is a hack
		// FIXME: better to add two guard samples and clamp to [0,1]
		t = ::Clamp( t, 0.f, 0.999f ); //implement in SSE so that it there is no branch for sure
		Float frac = 0.f;
		Int32 i = GetLowerIndex( t, samples, frac );

		return Lerp( frac, samples[ i ], samples[ i + 1 ] );
	}


	// ** ************************************
	//
	RED_INLINE Vector	SampleLinearClamp( Float t, const TDynArray< Vector > & samples )
	{
		// FIXME: clamping to [0,0.999f] is a hack
		// FIXME: better to add two guard samples and clamp to [0,1]
		t = ::Clamp( t, 0.f, 0.999f ); //implement in SSE so that it there is no branch for sure
		Float frac = 0.f;
		Int32 i = GetLowerIndex( t, samples, frac );

		return LerpClamp( frac, samples[ i ], samples[ i + 1 ] );
	}
}

#if 0 // DEAD CODE!

// FIXME: at this point only lerp is serviced and at least nearest neighbour should be added here
// FIXME: add some more intelligent pre sampling scheme (e.g. non evenly placed sample points)
class CCurveApproximator : public Red::System::NonCopyable
{
public:

	TDynArray< Float >	m_samples;

public:

	CCurveApproximator						( const CCurve * curve, Uint32 numSamples = DEFAULT_NUM_SAMPLES );
	CCurveApproximator						();

	~CCurveApproximator						();

	RED_INLINE Float	GetValue			( Float t ) const 
	{ 
		return SampledCurveEvaluators::SampleLinear( t, m_samples );
	}

	void	GenerateApproximation			( const CCurve * curve, Uint32 numSamples );

	friend IFile & operator << ( IFile & file, CCurveApproximator & param )
	{
		file << param.m_samples;

		return file;
	}
};

// FIXME: seems like a hack - it's just an approximation counterpart of CurveParameter class
// FIXME: yet it is supposed to serve as a snapshot
class CCurveApproximatorParameter
{
private:

	CCurveApproximator * m_approximators[ 4 ];

	Uint32	m_count;

public:

	CCurveApproximatorParameter		( const CurveParameter * curveParameter, Uint32 numSamples = DEFAULT_NUM_SAMPLES );
	CCurveApproximatorParameter		();
	~CCurveApproximatorParameter	();

	void	GenerateApproximation	( const CurveParameter * curveParameter, Uint32 numSamples = DEFAULT_NUM_SAMPLES );

private:

	void	DeleteApproximators		();

public:

	RED_INLINE Uint32						GetApproximatorCount() const { return m_count; }
	RED_INLINE const CCurveApproximator *	GetApproximator		( Uint32 index ) const { return m_approximators[ index ]; }
	RED_INLINE CCurveApproximator *		GetApproximatorCount( Uint32 index ) { return m_approximators[ index ]; }

	friend IFile & operator << ( IFile & file, CCurveApproximatorParameter & param )
	{
		file << param.m_count;

		Uint32 count = param.m_count;
		Uint32 i = 0;

		while ( count-- )
		{
			if ( !param.m_approximators[i] )
			{
				param.m_approximators[ i ] = new CCurveApproximator();
			}

			file << *param.m_approximators[ i++ ];
		}

		return file;
	}

};

class CVecCurveApproximationParameter
{
private:
	TDynArray< Vector > m_samples;
	Uint32 m_count;

public:
	CVecCurveApproximationParameter( const CurveParameter * curveParameter, Uint32 numSamples = DEFAULT_NUM_SAMPLES );
	CVecCurveApproximationParameter();
	~CVecCurveApproximationParameter();

	void GenerateApproximation( const CurveParameter * curveParameter, Uint32 numSamples = DEFAULT_NUM_SAMPLES );

	RED_INLINE Vector GetValue( Float t ) const 
	{ 
		return SampledCurveEvaluators::SampleLinear( t, m_samples );
	}

	RED_INLINE Vector GetClampedValue( Float t ) const
	{
		return SampledCurveEvaluators::SampleLinearClamp( t, m_samples );
	}

	friend IFile & operator << ( IFile & file, CVecCurveApproximationParameter & param )
	{
		file << param.m_count;
		file << param.m_samples;

		return file;
	}
};

#endif
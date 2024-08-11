/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CSubframeInterpolator
{
	friend class CRenderProxy_Particles;

private:
	static const Uint32 TRANSFORM_COUNT_BITS	= 2;
	static const Uint32 TRANSFORM_COUNT			= 1 << TRANSFORM_COUNT_BITS;
	static const Uint32 TRANSFORM_MASK			= TRANSFORM_COUNT - 1;

	Matrix		m_localToWorld[TRANSFORM_COUNT];	//!< A sequence of last four transform matrices
	Uint32		m_transformIndex;					//!< Index of the newest matrix in the sequence

public:
	void Init( const Matrix& n0Transform )
	{
		for ( Uint32 i = 0; i < TRANSFORM_COUNT; ++i )
		{
			m_localToWorld[i] = n0Transform;
		}
		m_transformIndex = 0;
	}

	RED_INLINE Uint32 GetTransformIndex( Uint32 i ) const
	{
		return (m_transformIndex - i) & TRANSFORM_MASK;
	}

	void Update( const Matrix& n0Transform )
	{
		m_transformIndex++;
		m_localToWorld[ m_transformIndex & TRANSFORM_MASK ] = n0Transform;
	}

	Float GetAngleChange() const
	{
		const Vector& n3Pos = m_localToWorld[ GetTransformIndex( 3 ) ].GetTranslationRef();
		const Vector& n2Pos = m_localToWorld[ GetTransformIndex( 2 ) ].GetTranslationRef();
		const Vector& n1Pos = m_localToWorld[ GetTransformIndex( 1 ) ].GetTranslationRef();
		const Vector& n0Pos = m_localToWorld[ GetTransformIndex( 0 ) ].GetTranslationRef();

		// Get directions of all 3 consecutive line segments

		const Vector3 to0Vec = (n0Pos - n1Pos).Normalized3();
		const Vector3 to1Vec = (n1Pos - n2Pos).Normalized3();
		const Vector3 to2Vec = (n2Pos - n3Pos).Normalized3();

		// Get angles at points 1 and 2

		const Float angleAt1 = (to0Vec.IsZero() || to1Vec.IsZero()) ? 0.0f : acos( Vector::Dot3(to0Vec, to1Vec) );
		const Float angleAt2 = (to1Vec.IsZero() || to2Vec.IsZero()) ? 0.0f : acos( Vector::Dot3(to1Vec, to2Vec) );

		// Return average of 2 angles

		return (angleAt1 + angleAt2) * 0.5f;
	}

	Vector CalculateHermiteVector( Float t, const Vector& p0, const Vector& p1, const Vector& cp0, const Vector& cp1  ) const
	{
		const Float powT2 = t * t;
		const Float powT3 = powT2 * t;

		const Float h1 =  2 *	powT3 - 3 * powT2 + 1;          
		const Float h2 = -2 *	powT3 + 3 *	powT2;            
		const Float h3 =		powT3 - 2 *	powT2 + t;
		const Float h4 =		powT3 -		powT2;
		return p0 * h1 + p1 * h2 + cp0 * h3 + cp1 * h4;
	}

	//! Interpolate position, range 0 ( previous ) to 1 ( current )
	RED_INLINE void InterpolatePosition( Float frac, Vector& pos ) const
	{
		const Vector& n3Pos = m_localToWorld[ GetTransformIndex( 3 ) ].GetTranslationRef();
		const Vector& n2Pos = m_localToWorld[ GetTransformIndex( 2 ) ].GetTranslationRef();
		const Vector& n1Pos = m_localToWorld[ GetTransformIndex( 1 ) ].GetTranslationRef();
		const Vector& n0Pos = m_localToWorld[ GetTransformIndex( 0 ) ].GetTranslationRef();
		Vector ct0, ct1;

		ct0 = ( n1Pos - n3Pos ) * 0.5f;
		ct1 = ( n0Pos - n2Pos ) * 0.5f;

		pos = CalculateHermiteVector( frac, n2Pos, n1Pos, ct0, ct1 );
	}

	//! Interpolate axis, range 0 ( previous ) to 1 ( current )
	RED_INLINE void InterpolateAxis( Float frac, Vector& pos ) const
	{
#ifdef USE_HAVOK_ANIMATION
		EulerAngles interpolated = InterpolateEulerAngles( m_localToWorld[ GetTransformIndex( 2 ) ].ToEulerAngles(), m_localToWorld[ GetTransformIndex( 1 ) ].ToEulerAngles(), frac );
		pos = interpolated.ToMatrix().GetRow(2);
#else
		// Very slow...
		EulerAngles ea1 = m_localToWorld[ GetTransformIndex( 2 ) ].ToEulerAngles();
		EulerAngles ea2 = m_localToWorld[ GetTransformIndex( 1 ) ].ToEulerAngles();

		RedEulerAngles store = RedEulerAngles::InterpolateEulerAngles( reinterpret_cast< const RedEulerAngles& >( ea1 ), reinterpret_cast< const RedEulerAngles& >( ea2 ), frac );
		EulerAngles interpolated = reinterpret_cast< const EulerAngles& >( store );
		pos = interpolated.ToMatrix().GetRow(2);
#endif
	}

	//! Interpolate vector
	RED_INLINE void InterpolateVector( Float frac, const Vector& in, Vector& outVector ) const
	{
		const Vector prevPos = m_localToWorld[ GetTransformIndex( 1 ) ].TransformVector( in );
		const Vector curPos = m_localToWorld[ GetTransformIndex( 0 ) ].TransformVector( in );
		outVector = ::Lerp< Vector >( frac, prevPos, curPos );
	}
};
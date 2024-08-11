#include "build.h"

#include "curveData.h"
#include "../core/algorithms.h"
#include "mathUtils.h"
#include "object.h"
#include "bezier2d.h"

IMPLEMENT_RTTI_ENUM( ECurveBaseType );
IMPLEMENT_RTTI_ENUM( ECurveSegmentType );
IMPLEMENT_RTTI_ENUM( ECurveValueType );
IMPLEMENT_RTTI_CLASS( SCurveData );
IMPLEMENT_RTTI_CLASS( SCurveDataEntry );

TDynArray< SCurveDataEntry, MC_CurveData >::const_iterator CurveLowerBoundIndex( TDynArray< SCurveDataEntry, MC_CurveData >::const_iterator  begin, TDynArray< SCurveDataEntry, MC_CurveData >::const_iterator end, float val )
{
	TDynArray< SCurveDataEntry, MC_CurveData >::const_iterator endCopy = end;
	Uint32 count = static_cast< Uint32 >( end - begin );

	while( count > 0 )
	{
		Uint32 halfCount = count / 2;

		TDynArray< SCurveDataEntry, MC_CurveData >::const_iterator middle = begin + halfCount;
		if ( (*middle).time < val )
		{
			if ( ( middle + 1 < end && (*( middle + 1 )).time > val ) || middle + 1 == endCopy ) return middle;
			begin = middle+1;
			count = count - halfCount - 1;
		}
		else
		{
			count = halfCount;
		}	
	}

	return begin;
}


Float SCurveData::GetLengthUpToTime( Float time, Uint32 ) const
{
	Int32 index = GetLowerBoundIndex( time );
	if ( index == -1 ) return 0.0f;
	Float sum = 0.0f;
	Int32 max = Min( index, (Int32)(m_curveValues.Size() - 2 ) );
	for ( Int32 i = 0; i <= max; ++ i)
	{
		sum += GetSegmentLengthUpToTime( i < max ? m_curveValues[i + 1].time : time );
	}
	return sum;
}

Float SCurveData::GetSegmentLengthUpToTime( Float time, Uint32 probingDensity /*= 16*/ ) const
{
	ASSERT( m_valueType == CVT_Float );
	Int32 index = GetLowerBoundIndex( time );
	if ( index != -1 && time == m_curveValues[ index ].time ) --index;
	if ( index == (Int32)m_curveValues.Size() - 1 || index == -1 ) return 0;
	Float sum = 0.0f;
	Float val = GetFloatValue( m_curveValues[index].time );
	Float t = m_curveValues[ index ].time;
	Float dt = ( time - m_curveValues[ index ].time ) / probingDensity;
	for ( Uint32 i = 0; i < probingDensity; ++ i )
	{
		t += dt;
		Float newVal = GetFloatValue( t );
		sum += Abs( val - newVal );
		val = newVal;
	}
	return sum;
}

static bool CalcSmoothCurvePoint(
	bool  keepOutputXAtGivenT, 
	float point12_t,
	float point0_x, float point0_y, 
	float point1_x, float point1_y, 
	float point2_x, float point2_y, 
	float point3_x, float point3_y,
	float &out_x, float &out_y)
{
	struct Local
	{
		static inline void CalcBezier1v(float t, float p0, float p1, float p2, float &out)
		{
			out = (1-t)*(1-t)*p0 + 2*t*(1-t)*p1 + t*t*p2;
		}

		static inline void CalcBezier2v(float t, const float *p0, const float *p1, const float *p2, float *out)
		{
			CalcBezier1v(t, p0[0], p1[0], p2[0], out[0]);
			CalcBezier1v(t, p0[1], p1[1], p2[1], out[1]);
		}
	};

	const float smoothness = 0.5f;

	// test if parameters are valid

	//ac> I think it's below condition shouldn't be required.
	//if (point12_t < 0 || point12_t > 1)
	//    return false;

	if (point0_x >= point1_x || point1_x >= point2_x || point2_x >= point3_x)
		return false;

	// calculate bezier curve control points and interpolation value

	float bezier_t;
	float bezier_points[3][2];
	{//scope
		float unit_segs[3][2] = {
			{ point1_x-point0_x, point1_y-point0_y, },
			{ point2_x-point1_x, point2_y-point1_y, },
			{ point3_x-point2_x, point3_y-point2_y, }
		};
		for (int i=0; i<3; ++i)
		{
			float inv_len = 1.f / sqrtf( unit_segs[i][0]*unit_segs[i][0] + unit_segs[i][1]*unit_segs[i][1] );
			unit_segs[i][0] *= inv_len;
			unit_segs[i][1] *= inv_len;
		}

		float slopes[2] = { 
			(unit_segs[0][1]+unit_segs[1][1]) / (unit_segs[0][0]+unit_segs[1][0]),
			(unit_segs[2][1]+unit_segs[1][1]) / (unit_segs[2][0]+unit_segs[1][0])
		};

		float middle_controls_off[2] = {
			(0.5f * smoothness) * Min<float>(point2_x - point1_x, point1_x - point0_x),
			(0.5f * smoothness) * Min<float>(point3_x - point2_x, point2_x - point1_x)
		};

		float middle_controls[2][2] = { 
			{ point1_x + middle_controls_off[0],  point1_y + middle_controls_off[0] * slopes[0] },
			{ point2_x - middle_controls_off[1],  point2_y - middle_controls_off[1] * slopes[1] }
		};

		float middle_controls_center[2] = { 
			0.5f * (middle_controls[0][0] + middle_controls[1][0]), 
			0.5f * (middle_controls[0][1] + middle_controls[1][1]) 
		};

		float split_t = (middle_controls_center[0] - point1_x) / (point2_x - point1_x);
		if (point12_t < split_t)
		{
			bezier_t = point12_t / split_t;
			bezier_points[0][0] = point1_x;
			bezier_points[0][1] = point1_y;
			bezier_points[1][0] = middle_controls[0][0];
			bezier_points[1][1] = middle_controls[0][1];
			bezier_points[2][0] = middle_controls_center[0];
			bezier_points[2][1] = middle_controls_center[1];
		}
		else
		{
			bezier_t = split_t<0.9995f ? (point12_t-split_t) / (1-split_t) : 0.5f;
			bezier_points[0][0] = middle_controls_center[0];
			bezier_points[0][1] = middle_controls_center[1];
			bezier_points[1][0] = middle_controls[1][0];
			bezier_points[1][1] = middle_controls[1][1];
			bezier_points[2][0] = point2_x;
			bezier_points[2][1] = point2_y;
		}
	}

	// if function parameters indicates that output x should be
	// at fraction point12_t between points point1 and point2, then
	// we have to transform out bezier interpolation 't', so that it
	// matches requested output x value.

	if (keepOutputXAtGivenT)
	{
		double eps = 0.0001;

		double p0 = bezier_points[0][0];
		double p1 = bezier_points[1][0];
		double p2 = bezier_points[2][0];
		double pr = p0 + bezier_t * (p2 - p0);
		double a  = (p0 - 2 * p1 + p2);
		double b  = - 2 * p0 + 2 * p1;
		double c  = p0 - pr;

		if (abs(a) < eps)
			a = a>=0 ? eps : -eps;

		double delta = Max<double>(0, b*b - 4*a*c);
		double t0 = (-b+sqrt(delta))/(2*a);
		//double t1 = (-b-sqrt(delta))/(2*a);

		bezier_t = (float) t0;
	}

	// calculate point on a smooth curve

	if (keepOutputXAtGivenT)
	{
		float bezier_result_y;
		Local::CalcBezier1v(bezier_t, bezier_points[0][1], bezier_points[1][1], bezier_points[2][1], bezier_result_y);
		out_x = Max<float>(point1_x, Min<float>(point2_x, point1_x+point12_t*(point2_x-point1_x)));
		out_y = bezier_result_y;
	}
	else
	{
		float out[2];
		Local::CalcBezier2v(bezier_t, bezier_points[0], bezier_points[1], bezier_points[2], out);
		out_x = out[0];
		out_y = out[1];
	}

	return true;
}

static Float CalcSmoothCurvePointY(
	bool  keepOutputXAtGivenT, 
	float point12_t,
	float point0_x, float point0_y, 
	float point1_x, float point1_y, 
	float point2_x, float point2_y, 
	float point3_x, float point3_y,
	float defaultOnFailure )
{
	Float outX = 0;
	Float outY = defaultOnFailure;
	CalcSmoothCurvePoint( keepOutputXAtGivenT, point12_t, point0_x, point0_y, point1_x, point1_y, point2_x, point2_y, point3_x, point3_y, outX, outY );
	return outY;
}


void SCurveData::SetBaseType( ECurveBaseType baseType )
{
	ASSERT( !( m_baseType == CT_Segmented && m_valueType == CVT_Vector ), TXT("A Vector-valued curve cannot use the Segmented base type") );
	if ( !( m_baseType == CT_Segmented && m_valueType == CVT_Vector ) )
	{
		m_baseType = baseType;
	}
}

void SCurveData::Sort()
{
	const auto predicate = []( const SCurveDataEntry & left, const SCurveDataEntry& right) { return left.time < right.time; };
	::Sort( m_curveValues.Begin(), m_curveValues.End(), predicate );  
}

Int32 SCurveData::AddPoint( const Float time, const Float value, const ECurveSegmentType& curveType, Bool )
{
	ASSERT( m_valueType == CVT_Float, TXT("Cannot add a Float value to a Vector-valued curve") );
	if ( m_valueType != CVT_Float )
		return -1;

	Uint32 i;
	for ( i = 0; i < m_curveValues.Size() && m_curveValues[ i ].time < time; ++i );

	SCurveDataEntry entry = 
	{ 
		Vector( -1.0f/10.0f, 0.0f, +1.0f/10.0f, 0.0f ),
		time, 
		value,
		static_cast< Uint16 >( curveType ),
		static_cast< Uint16 >( curveType )
	};

	m_curveValues.Insert( i, entry );
	
	return i;
}

Int32 SCurveData::AddPoint( const Float time, const Float value, const Vector& tangents, const ECurveSegmentType& curveType, Bool )
{
	ASSERT( m_valueType == CVT_Float, TXT("Cannot add a Float value to a Vector-valued curve") );
	if ( m_valueType != CVT_Float )
		return -1;

	Uint32 i;
	for ( i = 0; i < m_curveValues.Size() && m_curveValues[ i ].time < time; ++i );

	SCurveDataEntry entry = 
	{ 
		tangents,
		time, 
		value,
		static_cast< Uint16 >( curveType ),
		static_cast< Uint16 >( curveType )
	};

	m_curveValues.Insert( i, entry );

	return i;
}

Int32 SCurveData::AddPoint( const Float time, const Vector& value, Bool )
{
	ASSERT( m_valueType == CVT_Vector, TXT("Cannot add a Vector value to a Float-valued curve") );
	if ( m_valueType != CVT_Vector )
		return -1;

	Uint32 i;
	for ( i = 0; i < m_curveValues.Size() && m_curveValues[ i ].time < time; ++i );

	SCurveDataEntry entry = { value, time };
	m_curveValues.Insert( i, entry );

	return i;
}


void SCurveData::SwapPoints( Uint32 a, Uint32 b )
{
	::Swap( m_curveValues[ a ], m_curveValues[ b ] );
}


Uint32 SCurveData::RevalidatePoint( Uint32 i )
{
	// If it's already in the right plce, do nothing!
	if ( ( i + 1 == Size() || m_curveValues[ i ].time <= m_curveValues[ i + 1 ].time ) && ( i == 0 || m_curveValues[ i - 1 ].time <= m_curveValues[ i ].time ) )
	{
		return i;
	}

	while ( i > 0 && m_curveValues[ i ].time < m_curveValues[ i - 1 ].time)
	{
		SwapPoints( i, i - 1 );
		--i;
	}

	while ( i + 1 < Size() && m_curveValues[ i ].time > m_curveValues[ i + 1 ].time )
	{
		SwapPoints( i, i + 1 );
		++i;
	}
	return i;
}


Uint32 SCurveData::SetTime( const Uint32& index, const Float& time, Bool revalidatePoint )
{
	m_curveValues[index].time = time;
	return revalidatePoint ? RevalidatePoint( index ) : index;
}


Int32 SCurveData::RemovePoint( const Float time )
{
	Int32 index = GetLowerBoundIndex( time );
	if ( index >= 0 && m_curveValues[ index ].time == time )
	{
		RemovePointAtIndex( index );
		return index;
	}
	return -1;
}

void SCurveData::RemovePointAtIndex( const Uint32& i )
{
	m_curveValues.RemoveAt( i );
}

Vector SCurveData::GetTangentValue( const Uint32& index, const Int32& tangentIndex ) const
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1, TXT("Invalid SCurveData tangent index") );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot get tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return Vector::ZEROS;

	if ( tangentIndex == 0 )
	{
		return Vector( m_curveValues[index].controlPoint.X, m_curveValues[index].controlPoint.Y, 0, 0 );
	}
	else
	{
		return Vector( m_curveValues[index].controlPoint.Z, m_curveValues[index].controlPoint.W, 0, 0 );
	}
}



Vector SCurveData::GetVectorValue( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const
{
	ASSERT( m_valueType == CVT_Vector, TXT("GetVectorValue() called on float-valued SCurveData") );

	if ( m_valueType != CVT_Vector ) return Vector::ZEROS;

	// If no control points, just return 0
	if ( m_curveValues.Size() == 0 )
	{
		lowerBoundIndex = -1;
		upperBoundIndex = -1;
		return Vector::ZEROS;
	}

	FindLowerUpperBounds( time, lowerBoundIndex, upperBoundIndex );

	Uint32 i1 = lowerBoundIndex;
	Uint32 i2 = upperBoundIndex;

	if ( i1 == i2 )
	{
		return m_curveValues[i1].controlPoint;
	}

	Float t1 = m_curveValues[i1].time;
	Float t2 = m_curveValues[i2].time;
	if ( m_loop && t2 <= t1 )
	{
		t2 += 1.0f;
		if ( time < t1 )
		{
			time += 1.0f;
		}
	}

	Float localTime = ( time - t1 ) / ( t2 - t1 );
	ASSERT( t1 < t2 );
	ASSERT( localTime >= 0.0f && localTime <= 1.0f );

	switch ( m_baseType )
	{
	case CT_Linear:
		{
			return ::Lerp( localTime, m_curveValues[i1].controlPoint, m_curveValues[i2].controlPoint );
		}

	case CT_Smooth:
		{
			// Bezier curve, with the help of two neighboring points.
			Uint32 i0 = ( i1 == 0 ? m_curveValues.Size() - 1 : i1 - 1 );
			Uint32 i3 = ( i2 + 1 ) % m_curveValues.Size();
			Float t0 = m_curveValues[i0].time;
			Float t3 = m_curveValues[i3].time;

			// Adjust times, if they are wrapping around.
			if (t0 >= t1) t0 -= 1.0f;
			if (t3 <= t1) t3 += 1.0f;
			localTime = (time - t1) / (t2 - t1);

			const Vector& v0 = m_curveValues[i0].controlPoint;
			const Vector& v1 = m_curveValues[i1].controlPoint;
			const Vector& v2 = m_curveValues[i2].controlPoint;
			const Vector& v3 = m_curveValues[i3].controlPoint;

			Vector result;
			result.X = CalcSmoothCurvePointY( true, localTime, t0, v0.X, t1, v1.X, t2, v2.X, t3, v3.X, v1.X );
			result.Y = CalcSmoothCurvePointY( true, localTime, t0, v0.Y, t1, v1.Y, t2, v2.Y, t3, v3.Y, v1.Y );
			result.Z = CalcSmoothCurvePointY( true, localTime, t0, v0.Z, t1, v1.Z, t2, v2.Z, t3, v3.Z, v1.Z );
			result.W = CalcSmoothCurvePointY( true, localTime, t0, v0.W, t1, v1.W, t2, v2.W, t3, v3.W, v1.W );
			return result;
		}

	case CT_Segmented:
		{
			HALT( "Should not have a Segmented Vector curve!" );
			return Vector();
		}

	default:
		HALT( "Invalid CurveBaseType? %i", m_baseType );
		return Vector();
	}
}
Vector SCurveData::GetVectorValue( Float time ) const
{
	Int32 lb, ub;
	return GetVectorValue( time, lb, ub );
}

Float SCurveData::GetFloatValue( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const
{
	ASSERT( m_valueType == CVT_Float );
	if ( m_valueType != CVT_Float ) return 0.0f;

	// If no control points, just return 0.0f
	if ( m_curveValues.Size() == 0 )
	{
		lowerBoundIndex = -1;
		upperBoundIndex = -1;
		return 1.0f;
	}

	FindLowerUpperBounds( time, lowerBoundIndex, upperBoundIndex );

	Uint32 i1 = lowerBoundIndex;
	Uint32 i2 = upperBoundIndex;

	if ( i1 == i2 )
	{
		return m_curveValues[i1].value;
	}

	Float t1 = m_curveValues[i1].time;
	Float t2 = m_curveValues[i2].time;
	if ( m_loop )
	{
		while ( t2 <= t1 )
		{
			t2 += 1.0f;
		}
		while ( time < t1 )
		{
			time += 1.0f;
		}
	}

	Float localTime = ( time - t1 ) / ( t2 - t1 );
	ASSERT( t1 < t2 );
	ASSERT( localTime >= 0.0f && localTime <= 1.0f );

	switch ( m_baseType )
	{
	case CT_Linear:
		{
			return ::Lerp( localTime, m_curveValues[i1].value, m_curveValues[i2].value );
		}

	case CT_Smooth:
		{
			// Bezier curve, with the help of two neighboring points.
			Uint32 i0, i3;
			if ( m_loop )
			{
				i0 = ( i1 == 0 ? m_curveValues.Size() - 1 : i1 - 1 );
				i3 = ( i2 + 1 ) % m_curveValues.Size();
			}
			else
			{
				i0 = ( i1 == 0 ? 0 : i1 - 1 );
				i3 = ( i2 == m_curveValues.Size() - 1 ? i2 : i2 + 1 );
			}
			Float t0 = m_curveValues[i0].time;
			Float t3 = m_curveValues[i3].time;

			// Adjust times, if they are wrapping around.
			while (t0 >= t1) t0 -= 1.0f;
			while (t3 <= t2) t3 += 1.0f;
			localTime = (time - t1) / (t2 - t1);

			return CalcSmoothCurvePointY( true, localTime, t0, m_curveValues[i0].value, t1, m_curveValues[i1].value, t2, m_curveValues[i2].value, t3, m_curveValues[i3].value, m_curveValues[i1].value );
		}

	case CT_Segmented:
		{
			// Constant
			if ( m_curveValues[i1].curveTypeR == CST_Constant )
			{
				return m_curveValues[i1].value;
			}
			// Interpolated
			else if ( m_curveValues[i1].curveTypeR == CST_Interpolate && m_curveValues[i2].curveTypeL == CST_Interpolate )
			{
				return ::Lerp( localTime, m_curveValues[i1].value, m_curveValues[i2].value );
			}
			// Bezier 2d
			else if ( m_curveValues[i1].curveTypeR == CST_Bezier2D && m_curveValues[i2].curveTypeR == CST_Bezier2D )
			{
				const Vector tan1 = GetTangentValue( i1, 1 );
				const Vector tan2 = GetTangentValue( i2, 0 );

				Bezier2D bezier;
				bezier.SetPointsAndHandlers( t1, m_curveValues[i1].value, tan1.X, tan1.Y, t2, m_curveValues[i2].value, tan2.X, tan2.Y );

				return bezier.Get( localTime );
			}
			// Bezier
			else
			{
				const Float wholeRangeTime = t2 - t1;
				const Float t0 = ( wholeRangeTime * GetTangentValue( i1, 1 ).Y / GetTangentValue( i1, 1 ).X );
				const Float t1 = ( wholeRangeTime * GetTangentValue( i2, 0 ).Y / GetTangentValue( i2, 0 ).X );

				return MathUtils::InterpolationUtils::Hermite1D( localTime, m_curveValues[i1].value, t0, t1, m_curveValues[i2].value );
			}
		}
	}

	return 0.0f;
}

Float SCurveData::GetAngleValue(Float time) const
{
	ASSERT( m_valueType == CVT_Float );
	if ( m_valueType != CVT_Float ) return 0.0f;

	// If no control points, just return 0.0f
	if ( m_curveValues.Size() == 0 )
	{
		return 1.0f;
	}

	Int32 lowerBoundIndex, upperBoundIndex;
	FindLowerUpperBounds( time, lowerBoundIndex, upperBoundIndex );

	Uint32 i1 = lowerBoundIndex;
	Uint32 i2 = upperBoundIndex;

	if ( i1 == i2 )
	{
		return m_curveValues[i1].value;
	}

	Float t1 = m_curveValues[i1].time;
	Float t2 = m_curveValues[i2].time;
	if ( m_loop )
	{
		while ( t2 <= t1 )
		{
			t2 += 1.0f;
		}
		while ( time < t1 )
		{
			time += 1.0f;
		}
	}

	Float localTime = ( time - t1 ) / ( t2 - t1 );
	ASSERT( t1 < t2 );
	ASSERT( localTime >= 0.0f && localTime <= 1.0f );

	switch ( m_baseType )
	{
	case CT_Linear:
		{
			return EulerAngles::NormalizeAngle( EulerAngles::Interpolate( m_curveValues[i1].value, m_curveValues[i2].value, localTime ) );
		}

	case CT_Smooth:
		{
			// Bezier curve, with the help of two neighboring points.
			Uint32 i0, i3;
			if ( m_loop )
			{
				i0 = ( i1 == 0 ? m_curveValues.Size() - 1 : i1 - 1 );
				i3 = ( i2 + 1 ) % m_curveValues.Size();
			}
			else
			{
				i0 = ( i1 == 0 ? 0 : i1 - 1 );
				i3 = ( i2 == m_curveValues.Size() - 1 ? i2 : i2 + 1 );
			}
			Float t0 = m_curveValues[i0].time;
			Float t3 = m_curveValues[i3].time;

			// Adjust times, if they are wrapping around.
			while (t0 >= t1) t0 -= 1.0f;
			while (t3 <= t2) t3 += 1.0f;
			localTime = (time - t1) / (t2 - t1);

			const Float v1 = EulerAngles::ToNearestAngle( m_curveValues[i1].value, m_curveValues[i0].value );
			const Float v2 = EulerAngles::ToNearestAngle( m_curveValues[i2].value, v1 );
			const Float v3 = EulerAngles::ToNearestAngle( m_curveValues[i3].value, v2 );

			return EulerAngles::NormalizeAngle( CalcSmoothCurvePointY( true, localTime, t0, m_curveValues[i0].value, t1, v1, t2, v2, t3, v3, v1 ) );
		}

	case CT_Segmented:
		{
			// Constant
			if ( m_curveValues[i1].curveTypeR == CST_Constant )
			{
				return m_curveValues[i1].value;
			}
			// Interpolated
			else if ( m_curveValues[i1].curveTypeR == CST_Interpolate && m_curveValues[i2].curveTypeL == CST_Interpolate )
			{
				return EulerAngles::NormalizeAngle( EulerAngles::Interpolate( m_curveValues[i1].value, m_curveValues[i2].value, localTime ) );
			}
			// Bezier
			else
			{
				const Float wholeRangeTime = t2 - t1;
				const Float t0 = ( wholeRangeTime * GetTangentValue( i1, 1 ).Y / GetTangentValue( i1, 1 ).X );
				const Float t1 = ( wholeRangeTime * GetTangentValue( i2, 0 ).Y / GetTangentValue( i2, 0 ).X );

				return EulerAngles::NormalizeAngle( MathUtils::InterpolationUtils::Hermite1D( localTime, m_curveValues[i1].value, t0, t1, EulerAngles::ToNearestAngle( m_curveValues[i2].value, m_curveValues[i1].value ) ) );
			}
		}
	}

	return 0.0f;
}

void SCurveData::FindLowerUpperBounds( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const
{
	Uint32 numValues = m_curveValues.Size();

	// If no control points, just return something...
	if ( numValues == 0 )
	{
		lowerBoundIndex = -1;
		upperBoundIndex = -1;
		return;
	}

	Uint32 i1, i2;

	// If we aren't looping, check for boundaries.
	if ( !m_loop )
	{
		// Time < first control point time, so get value from first control point
		if ( time <= m_curveValues[0].time )
		{
			lowerBoundIndex = 0;
			upperBoundIndex = 0;
			return;
		}

		// Time > last control point time, so get the value from last control point
		if ( time >= m_curveValues[numValues - 1].time )
		{
			upperBoundIndex = numValues - 1;
			lowerBoundIndex = upperBoundIndex;
			return;
		}

		Uint32 i = static_cast< Uint32 >( CurveLowerBoundIndex( m_curveValues.Begin(), m_curveValues.End(), time ) - m_curveValues.Begin() + 1 );
		// We already checked boundaries, so we know i != 0 and i != numValues.
		i1 = i - 1;
		i2 = i;
	}
	else
	{
		// LowerBoundIndex seems to have issues with boundaries...
		if ( time > m_curveValues.Back().time )
			time -= 1.0f;

		Uint32 i;
		if ( time <= m_curveValues[0].time )
			i = 0;
		else
			i = static_cast< Uint32 >( CurveLowerBoundIndex( m_curveValues.Begin(), m_curveValues.End(), time ) - m_curveValues.Begin() + 1 );

		i1 = ( i == 0 ? numValues - 1 : i - 1 );
		i2 = ( i == numValues ? 0 : i );
	}

	lowerBoundIndex = ( time == m_curveValues[i2].time ? i2 : i1 );
	upperBoundIndex = i2;
}


void SCurveData::SetControlPoint( const Uint32& index, const Int32& tangentIndex, const Vector& v)
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot set tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return;

	m_curveValues[index].controlPoint.A[tangentIndex<<1] = v.X;
	m_curveValues[index].controlPoint.A[1+(tangentIndex<<1)] = v.Y;
}
Vector SCurveData::GetControlPoint( const Uint32& index, const Int32& tangentIndex ) const
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot get tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return Vector::ZEROS;

	if ( tangentIndex == 0 )
	{
		return Vector( m_curveValues[index].controlPoint.X, m_curveValues[index].controlPoint.Y, 0, 0 );
	}
	else
	{
		return Vector( m_curveValues[index].controlPoint.Z, m_curveValues[index].controlPoint.W, 0, 0 );
	}
}

Float SCurveData::GetTimeAtIndex( Uint32 index )const
{
	return m_curveValues[ index ].time;
}

Int32 SCurveData::GetLowerBoundIndex( const Float& time ) const
{
	if ( m_curveValues.Size() == 0 || time < m_curveValues[0].time ) return -1;
	return static_cast< Int32 >( CurveLowerBoundIndex( m_curveValues.Begin(), m_curveValues.End(), time ) - m_curveValues.Begin() );
}
Int32 SCurveData::GetIndex( const Float& time ) const
{
	TDynArray<SCurveDataEntry, MC_CurveData>::const_iterator it = CurveLowerBoundIndex( m_curveValues.Begin(), m_curveValues.End(), time );
	if ( it == m_curveValues.End() || (*it).time != time )
	{
		return -1;
	}
	return static_cast< Int32 >( it - m_curveValues.Begin() );
}

void SCurveData::SetTangentType( const Uint32& index, const Int32& tangentIndex, ECurveSegmentType type )
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot set tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return;

	auto& curveType = (tangentIndex == 0) ? m_curveValues[index].curveTypeL : m_curveValues[index].curveTypeR;
	auto& curveTypeOther = (tangentIndex == 0) ? m_curveValues[index].curveTypeR : m_curveValues[index].curveTypeL;
	curveType = type;

	// For some curves types we have to force second type and/or control point to some value
	if ( type == CST_BezierSmooth ||
		type == CST_BezierSmoothLyzwiaz ||
		type == CST_BezierSmoothSymertric )
	{
		// Set other tangent type
		curveTypeOther = type;

		// Refresh other tangent value
		SetTangentValue( index, tangentIndex, GetTangentValue( index, tangentIndex ) );
	}
	else if ( type == CST_Bezier2D )
	{
		// Change other type if necessary
		ECurveSegmentType otherType = (ECurveSegmentType)curveTypeOther;
		if ( otherType == CST_BezierSmooth ||
			otherType == CST_BezierSmoothLyzwiaz ||
			otherType == CST_BezierSmoothSymertric ||
			otherType == CST_Bezier )
		{
			curveTypeOther = (Uint32)CST_Bezier2D;
		}
	}
	else
	{
		// Change other type if necessary
		ECurveSegmentType otherType = (ECurveSegmentType)curveTypeOther;
		if ( otherType == CST_BezierSmooth ||
			otherType == CST_BezierSmoothLyzwiaz ||
			otherType == CST_BezierSmoothSymertric )
		{
			curveTypeOther = (Uint32)CST_Bezier;
		}
	}
}
ECurveSegmentType SCurveData::GetTangentType( const Uint32& index, const Int32& tangentIndex ) const
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot get tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return CST_Interpolate;

	const auto& curveType = (tangentIndex == 0) ? m_curveValues[ index ].curveTypeL : m_curveValues[ index ].curveTypeR;
	return (ECurveSegmentType)curveType;
}

void SCurveData::SetTangentValue( const Uint32& index, const Int32& tangentIndex, const Vector& tangentValue )
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	ASSERT( m_valueType == CVT_Float, TXT("Cannot set tangents for Vector curves") );
	if ( m_valueType != CVT_Float ) return;

	Float oldLength = GetTangentValue( index, tangentIndex ).Mag3();

	// Set new tangent value
	m_curveValues[index].controlPoint.A[tangentIndex<<1] = ( tangentIndex == 0 ) ? Min( tangentValue.X, 0.0f ) : Max( tangentValue.X, 0.0f );
	m_curveValues[index].controlPoint.A[1 + (tangentIndex<<1)] = tangentValue.Y;

	// For some control points value we have to force other tangent value
	const auto& curveType = (tangentIndex == 0) ? m_curveValues[index].curveTypeL : m_curveValues[index ].curveTypeR;
	const auto& curveTypeOther = (tangentIndex == 0) ? m_curveValues[index ].curveTypeR : m_curveValues[index ].curveTypeL;
	RED_UNUSED( curveTypeOther );
	if ( curveType == CST_BezierSmooth )
	{
		// The same direction, not the same length
		ASSERT( curveTypeOther == CST_BezierSmooth );
		SetControlPoint( index, 1 - tangentIndex, GetControlPoint( index, tangentIndex ).Normalized3().Mul4( - GetControlPoint( index, 1 - tangentIndex ).Mag3() ) );
	}
	else if ( curveType == CST_BezierSmoothLyzwiaz )
	{
		// Sum of length of both tangents is the same as earlier
		ASSERT( curveTypeOther == CST_BezierSmoothLyzwiaz );
		Float newLength = Max( 0.1f, GetControlPoint( index, 1 - tangentIndex ).Mag3() + oldLength - GetControlPoint( index, tangentIndex ).Mag3() );
		SetControlPoint( index, 1 - tangentIndex, GetControlPoint( index, tangentIndex ).Normalized3().Mul4( -newLength ) );
	}
	else if ( curveType == CST_BezierSmoothSymertric )
	{
		// The same direction, the same length
		ASSERT( curveTypeOther == CST_BezierSmoothSymertric );
		SetControlPoint( index, 1 - tangentIndex, - GetControlPoint( index, tangentIndex ) );
	}
}

namespace
{

const CName c_times( TXT("times") );
const CName c_values( TXT( "values" ) );
const CName c_curveTypeL( TXT("left curve type") );
const CName c_curveTypeR( TXT("right curve type") );
const CName c_controlPoint( TXT("control points") );

}

template<> 
Bool TTypedClass< SCurveData, (EMemoryClass)SCurveData::MemoryClass, SCurveData::MemoryPool >::OnReadUnknownProperty( void* object, const CName& propName, const CVariant& propValue ) const
{
	SCurveData* curve = static_cast< SCurveData* >( object );
	
	if( propName == c_times )
	{
		TDynArray< Float, MC_CurveData > times;
		propValue.AsType( times );

		curve->m_curveValues.Resize( times.Size() );

		for( Uint32 index = 0, end = times.Size(); index != end; ++index  )
		{
			curve->m_curveValues[ index ].time = times[ index ];
		}

		return true;
	}
	else if( propName == c_values )
	{
		TDynArray< Float, MC_CurveData > values;
		propValue.AsType( values );

		curve->m_curveValues.Resize( values.Size() );

		for( Uint32 index = 0, end = values.Size(); index != end; ++index  )
		{
			curve->m_curveValues[ index ].value = values[ index ];
		}

		return true;
	}
	else if( propName == c_curveTypeL )
	{
		TDynArray< Uint32, MC_CurveData > curveType;
		propValue.AsType( curveType );

		curve->m_curveValues.Resize( curveType.Size() );

		for( Uint32 index = 0, end = curveType.Size(); index != end; ++index  )
		{
			curve->m_curveValues[ index ].curveTypeL = curveType[ index ];
		}

		return true;
	}
	else if( propName == c_curveTypeR )
	{
		TDynArray< Uint32, MC_CurveData > curveType;
		propValue.AsType( curveType );

		curve->m_curveValues.Resize( curveType.Size() );

		for( Uint32 index = 0, end = curveType.Size(); index != end; ++index  )
		{
			curve->m_curveValues[ index ].curveTypeR = curveType[ index ];
		}

		return true;
	}
	else if( propName == c_controlPoint )
	{
		TDynArray< Vector, MC_CurveData > controlPoints;
		propValue.AsType( controlPoints );

		curve->m_curveValues.Resize( controlPoints.Size() );

		for( Uint32 index = 0, end = controlPoints.Size(); index != end; ++index  )
		{
			curve->m_curveValues[ index ].controlPoint = controlPoints[ index ];
		}

		return true;
	}
	
	return false;
}

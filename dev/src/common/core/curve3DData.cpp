#include "build.h"
#include "object.h"
#include "curve3DData.h"

IMPLEMENT_RTTI_CLASS( SCurve3DData );

void SCurve3DData::SetControlPoint( const Float& time, const Vector& val, const Int32& tangentIndex )
{
	Vector value = val;
	Float f = tangentIndex == 0 ? -1.0f : 1.0f;
	{
		Int32 index = v[0].GetIndex( time );
		if ( index >= 0 )
		{
			v[0].SetTangentValue( index, tangentIndex, Vector( f, value.X, 0.0f ) );
		}
	}
	{
		Int32 index = v[1].GetIndex( time );
		if ( index >= 0 )
		{
			v[1].SetTangentValue( index, tangentIndex, Vector( f, value.Y, 0.0f ) );
		}
	}
	{
		Int32 index = v[2].GetIndex( time );
		if ( index >= 0 )
		{
			v[2].SetTangentValue( index, tangentIndex, Vector( f, value.Z, 0.0f ) );
		}
	}
}

Vector SCurve3DData::GetControlPoint( const Float& time, const Int32& tangentIndex ) const
{
	Vector r;
	{
		Int32 index = v[0].GetIndex( time );
		if ( index >= 0 )
		{
			r.X = v[0].GetControlPoint( index, tangentIndex ).Y / v[0].GetControlPoint( index, tangentIndex ).X;
			if ( Red::Math::NumericalUtils::IsNan( r.X ) || v[0].GetControlPoint( index, tangentIndex ).X == 0.0f ) r.X = 0.0f;
		}
	}
	{
		Int32 index = v[1].GetIndex( time );
		if ( index >= 0 )
		{
			r.Y = v[1].GetControlPoint( index, tangentIndex ).Y / v[1].GetControlPoint( index, tangentIndex ).X;
			if ( Red::Math::NumericalUtils::IsNan( r.Y ) || v[1].GetControlPoint( index, tangentIndex ).X == 0.0f ) r.Y = 0.0f;
		}
	}
	{
		Int32 index = v[2].GetIndex( time );
		if ( index >= 0 )
		{
			r.Z = v[2].GetControlPoint( index, tangentIndex ).Y / v[2].GetControlPoint( index, tangentIndex ).X;
			if ( Red::Math::NumericalUtils::IsNan( r.Z ) || v[2].GetControlPoint( index, tangentIndex ).X == 0.0f ) r.Z = 0.0f;
		}
	}
	if ( tangentIndex == 0 ) r *= -1.0f;
	return r;
}

void SCurve3DData::SetValue( const Float& time, const Vector& value )
{
	{
		Int32 index = v[0].GetIndex( time );
		if ( index >= 0 )
		{
			v[0].m_curveValues[index].value = value.X;
		}
		else
		{
			v[0].AddPoint( time, value.X );
		}
	}
	{
		Int32 index = v[1].GetIndex( time );
		if ( index >= 0 )
		{
			v[1].m_curveValues[index].value = value.Y;
		}
		else
		{
			v[1].AddPoint( time, value.Y );
		}
	}
	{
		Int32 index = v[2].GetIndex( time );
		if ( index >= 0 )
		{
			v[2].m_curveValues[index].value = value.Z;
		}
		else
		{
			v[2].AddPoint( time, value.Z );
		}
	}
}

Vector SCurve3DData::GetValue( const Float& time ) const
{
	return Vector(
		v[0].GetFloatValue( time ),
		v[1].GetFloatValue( time ),
		v[2].GetFloatValue( time )
		);
}
void SCurve3DData::RemovePoint( const Float& time )
{
	v[0].RemovePoint( time );
	v[1].RemovePoint( time );
	v[2].RemovePoint( time );
}

void SCurve3DData::AddPoint( const Float& time, const Vector& value )
{
	v[0].AddPoint( time, value.X );
	v[1].AddPoint( time, value.Y );
	v[2].AddPoint( time, value.Z );
}

void SCurve3DData::GetKeyframes( TDynArray<Float>& frames) const
{
	Uint32 ids[3] = { 0, 0, 0 };
	Uint32 n = v[0].Size() + v[1].Size() + v[2].Size();
	for ( Uint32 i = 0; i < n; ++i )
	{
		Uint32 min = 10;
		if ( v[0].Size() > ids[0] ) min = 0;
		if ( v[1].Size() > ids[1] && ( min == 10 || v[1].m_curveValues[ids[1]].time < v[min].m_curveValues[ids[min]].time ) ) min = 1;
		if ( v[2].Size() > ids[2] && ( min == 10 || v[2].m_curveValues[ids[2]].time < v[min].m_curveValues[ids[min]].time ) ) min = 2;
		Float value = v[min].m_curveValues[ids[min]].time;
		if ( frames.Size() == 0 || value != frames[frames.Size()-1] ) frames.PushBack( value );
		++ids[min];
	}
}

Float SCurve3DData::GetStart() const
{
	Float min = NumericLimits< Float >::Max();
	if ( v[0].m_curveValues.Size() ) min = Min( v[0].m_curveValues[0].time, min );
	if ( v[1].m_curveValues.Size() ) min = Min( v[1].m_curveValues[0].time, min );
	if ( v[2].m_curveValues.Size() ) min = Min( v[2].m_curveValues[0].time, min );
	return min;
}

Float SCurve3DData::GetEnd() const
{
	Float max = NumericLimits< Float >::Min();
	if ( v[0].m_curveValues.Size() ) max = Max( v[0].m_curveValues[ v[0].m_curveValues.Size() - 1 ].time, max );
	if ( v[1].m_curveValues.Size() ) max = Max( v[1].m_curveValues[ v[1].m_curveValues.Size() - 1 ].time, max );
	if ( v[2].m_curveValues.Size() ) max = Max( v[2].m_curveValues[ v[2].m_curveValues.Size() - 1 ].time, max );
	return max;
}
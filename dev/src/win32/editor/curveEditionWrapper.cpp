/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "curveEditionWrapper.h"
#include "../../common/engine/curve.h"
#include "../../common/core/xmlFile.h"

// USE THIS ONLY WHILE INITIALISING CCurveEditionWrapper ! ! !
// Otherwise you will corrupt the stack.
void CCurveEditionWrapper::CreateControlPoints()
{
	m_controlPoints.ClearPtr();
	for ( Uint32 i = 0; i < m_data.Size(); ++i )
	{
		m_controlPoints.PushBack( new CCurveEditionWrapper::ControlPoint( i, m_data, this ) );
	}
}

void CCurveEditionWrapper::ControlPoint::SetTime( const Float time )
{
	Uint32 index = m_data.SetTime( m_index, time );
	if ( index != -1 && m_index != index)
	{
		if ( m_curve )
		{
			for ( Uint32 i = index; i < m_index; ++i )
			{	
				++m_curve->m_controlPoints[i]->m_index;
			}
			for ( Uint32 i = m_index + 1; i <= index; ++i )
			{
				--m_curve->m_controlPoints[i]->m_index;
			}
			m_curve->SortControlPoints();
			m_index = index;
		}
	}
}

ECurveSegmentType CCurveEditionWrapper::ControlPoint::GetTangentType( const Int32& tangentIndex ) const
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	return (ECurveSegmentType)m_data.GetTangentType( m_index, tangentIndex );
}

void CCurveEditionWrapper::ControlPoint::SetTangentType( const Int32& tangentIndex, ECurveSegmentType type )
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	m_data.SetTangentType( m_index, tangentIndex, type );
}

void CCurveEditionWrapper::ControlPoint::SetTangentValue( const Int32& tangentIndex, const Vector& tangentValue )
{
	ASSERT( tangentIndex == 0 || tangentIndex == 1 );
	m_data.SetTangentValue( m_index, tangentIndex, tangentValue);
}

CCurveEditionWrapper::CCurveEditionWrapper( SCurveData& data, const Color& color )
	: m_color( color )
	, m_data( data )
{
	CreateControlPoints();
}

CCurveEditionWrapper::CCurveEditionWrapper( SCurveData& data, const Float value, const Color& color )
	: m_color( color )
	, m_data( data )
{
	CreateControlPoints();
	CCurveEditionWrapper::ControlPoint* controlPoint = AddControlPoint( 0.0f, value );
	controlPoint->SetTangentType( CCurveEditionWrapper::ControlPoint::CD_In, CST_Constant );
	controlPoint->SetTangentType( CCurveEditionWrapper::ControlPoint::CD_Out, CST_Constant );
}

CCurveEditionWrapper::~CCurveEditionWrapper()
{
	// Delete control points table
	m_controlPoints.ClearPtr();
}

Float CCurveEditionWrapper::GetValue( const Float& time ) const
{
	return m_data.GetFloatValue( time );
}

void CCurveEditionWrapper::SortControlPoints()
{
	CCurveControlPointCompareFunc pred;
	Sort( m_controlPoints.Begin(), m_controlPoints.End(), pred );
}


CCurveEditionWrapper::ControlPoint* CCurveEditionWrapper::AddControlPoint( const Float time, const Float value, const ECurveSegmentType& curveType, bool addToCurve/* = true*/ )
{
	Uint32 index;
	if ( addToCurve )
	{
		index = m_data.AddPoint( time, value, curveType );
	}
	else
	{
		index = m_data.GetIndex( time );
		ASSERT( index != -1 );
	}
	CCurveEditionWrapper::ControlPoint* controlPoint = new CCurveEditionWrapper::ControlPoint( index, m_data, this );
	m_controlPoints.Insert( index, controlPoint );
	for ( Uint32 i = index + 1; i < m_controlPoints.Size(); ++i )
	{
		++m_controlPoints[i]->m_index;
	}
	return controlPoint;
}

Bool CCurveEditionWrapper::RemoveControlPoint( CCurveEditionWrapper::ControlPoint* controlPoint, bool removeFromCurve /*= true*/ )
{
	if ( &controlPoint->m_data != &m_data || controlPoint->m_index >= m_data.Size() )
	{
		return false;
	}
	if ( removeFromCurve ) m_data.RemovePointAtIndex( controlPoint->m_index );
	for ( Uint32 i = controlPoint->m_index; i < m_controlPoints.Size(); ++i )
	{
		-- m_controlPoints[i]->m_index;
	}
	m_controlPoints.Remove( controlPoint );
	return true;
}
void CCurveEditionWrapper::Clear()
{
	m_controlPoints.ClearPtr();
	m_data.Clear();
}

void CCurveEditionWrapper::RemapControlPoints( Float oldRangeMin, Float oldRangeMax, Float newRangeMin, Float newRangeMax )
{
	ASSERT( oldRangeMax > oldRangeMin );
	ASSERT( newRangeMax > newRangeMin );
	Float scale = ( newRangeMax - newRangeMin ) / ( oldRangeMax - oldRangeMin );

	for( Uint32 i=0; i<m_controlPoints.Size(); i++ )
	{
		if( m_controlPoints[i] )
		{
			Float t = m_controlPoints[i]->GetTime();
			m_controlPoints[i]->SetTime( ( t - oldRangeMin ) * scale + newRangeMin );
		}
	}
}

Bool CCurveEditionWrapper::GetValuesMinMax( Float& minValue, Float& maxValue ) const
{
	if( m_controlPoints.Size() == 0 )
	{
		return false;
	}

	minValue = NumericLimits< Float >::Max();
	maxValue = -NumericLimits< Float >::Max();

	for( Uint32 i=0; i<m_controlPoints.Size(); i++ )
	{
		if( m_controlPoints[i] )
		{
			Float v = m_controlPoints[i]->GetValue();
			if( v < minValue )
				minValue = v;

			if( v > maxValue )
				maxValue = v;
		}
	}

	return true;
}

Bool CCurveEditionWrapper::GetTimesMinMax( Float& minValue, Float& maxValue ) const
{
	if( m_controlPoints.Size() == 0 )
	{
		return false;
	}

	minValue = NumericLimits< Float >::Max();
	maxValue = -NumericLimits< Float >::Max();

	for( Uint32 i=0; i<m_controlPoints.Size(); i++ )
	{
		if( m_controlPoints[i] )
		{
			Float v = m_controlPoints[i]->GetTime();
			if( v < minValue )
				minValue = v;

			if( v > maxValue )
				maxValue = v;
		}
	}

	return true;
}

void CCurveEditionWrapper::GetApproximationSamples( TDynArray< Float >& samples ) const
{
	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( CurveApproximation::NUM_SAMPLES - 1 );

	samples.Clear();

	for( Uint32 i = 0; i < CurveApproximation::NUM_SAMPLES; ++i, t += dt )
	{
		samples.PushBack( GetValue( t ) );
	}
}

void CCurveEditionWrapper::GetApproximationSamples( Float* samples ) const
{
	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( CurveApproximation::NUM_SAMPLES - 1 );

	for( Uint32 i = 0; i < CurveApproximation::NUM_SAMPLES; ++i, t += dt )
	{
		samples[ i ] = GetValue( t );
	}
}

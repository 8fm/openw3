/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "curveSimple.h"
#include "object.h"

IMPLEMENT_ENGINE_CLASS( SSimpleCurvePoint );
IMPLEMENT_ENGINE_CLASS( SSimpleCurve );

// --------------------------------------------------------------------------

Bool						SSimpleCurve::s_graphTimeDisplayEnable	= false;
Float						SSimpleCurve::s_graphTimeDisplayValue	= 0.5f;

Bool						SSimpleCurve::s_graphGlobalSelectionEnable = true;
Float						SSimpleCurve::s_graphMinTimeSelection = 0.0f;
Float						SSimpleCurve::s_graphMaxTimeSelection = 0.0f;

// --------------------------------------------------------------------------

SSimpleCurve::SSimpleCurve ()
	: SCurveBase( CVT_Float, true )
{
	SetCurveType( CT_Smooth );
}

SSimpleCurve::SSimpleCurve ( ESimpleCurveType type, Float scalarEditScale, Float scalarEditOrigin )
	: SCurveBase( type == SCT_Float ? CVT_Float : CVT_Vector, true )
{
	m_simpleCurveType = type;
	SetCurveType( CT_Smooth );
	SetScalarEditScale( scalarEditScale );
	SetScalarEditOrigin( scalarEditOrigin );
}

SSimpleCurve::SSimpleCurve ( ESimpleCurveType type, const SSimpleCurvePoint &controlPoint, Float scalarEditScale, Float scalarEditOrigin )
	: SCurveBase( type == SCT_Float ? CVT_Float : CVT_Vector, true )
{
	m_simpleCurveType = type;
	SetCurveType( CT_Smooth );
	SetScalarEditScale( scalarEditScale );
	SetScalarEditOrigin( scalarEditOrigin );
	AddPoint( controlPoint.m_time, controlPoint.m_value );
}

SSimpleCurve& SSimpleCurve::Reset( ESimpleCurveType type, Float scalarEditScale, Float scalarEditOrigin )
{
	// Only rebuild the curve completely if the curve type has changed
	if( m_simpleCurveType != type )
	{
		m_data = SCurveData( type == SCT_Float ? CVT_Float : CVT_Vector );
	}
	else
	{
		// This does not resize the internal point arrays
		Clear();
	}
	m_data.m_loop = true;

	m_simpleCurveType = type;
	SetCurveType( CT_Smooth );
	SetScalarEditScale( scalarEditScale );
	SetScalarEditOrigin( scalarEditOrigin );
	return *this;
}


void SSimpleCurve::ImportDayPointValue( const SSimpleCurve &srcCurve, const SSimpleCurve &destCurve, Float lerpFactor, Float time, ECurvePointInterpolateMode interpolationMode /*= CPI_LINEAR*/ )
{
	SetInterpolatedValue( srcCurve, destCurve, lerpFactor, time, interpolationMode );
	m_simpleCurveType = (srcCurve.GetSimpleCurveType() == destCurve.GetSimpleCurveType()) ? srcCurve.GetSimpleCurveType() : SCT_Vector;
}

// --------------------------------------------------------------------------

namespace
{
	const CName c_curveSimpletimes( TXT("dataTimes") );
	const CName c_curveSimplevalues( TXT( "dataValues" ) );
	const CName c_curveSimplecurveTypeL( TXT("dataCurveType0") );
	const CName c_curveSimplecurveTypeR( TXT("dataCurveType1") );
	const CName c_curveSimplecontrolPoint( TXT("dataControlPoints") );
}

template<> Bool TTypedClass<SSimpleCurve, (EMemoryClass)SSimpleCurve::MemoryClass, SSimpleCurve::MemoryPool>::OnReadUnknownProperty( void* object, const CName& propName, const CVariant& propValue ) const
{
	SSimpleCurve* curve = (SSimpleCurve*)object;
	if ( propName == CNAME( ControlPoints ) )
	{
		TDynArray<SSimpleCurvePoint, MC_CurveData> points;
		if ( propValue.AsType(points) )
		{
			for ( Uint32 i = 0; i < points.Size(); ++i )
			{
				// Add as a vector. SCurveBase will convert as appropriate.
				curve->AddPoint( points[i].m_time, points[i].m_value );
			}
		}

		return true;
	}
	else if( propName == c_curveSimpletimes )
	{
		TDynArray< Float, MC_CurveData > times;
		propValue.AsType( times );

		curve->m_data.m_curveValues.Resize( times.Size() );
		
		for( Uint32 index = 0, end = times.Size(); index != end; ++index  )
		{
			curve->m_data.m_curveValues[ index ].time = times[ index ];
		}

		return true;
	}
	else if( propName == c_curveSimplevalues )
	{
		TDynArray< Float, MC_CurveData > values;
		propValue.AsType( values );

		curve->m_data.m_curveValues.Resize( values.Size() );
		
		for( Uint32 index = 0, end = values.Size(); index != end; ++index  )
		{
			curve->m_data.m_curveValues[ index ].value = values[ index ];
		}

		return true;
	}
	else if( propName == c_curveSimplecurveTypeL )
	{
		TDynArray< Uint32, MC_CurveData > curveType;
		propValue.AsType( curveType );

		curve->m_data.m_curveValues.Resize( curveType.Size() );
	
		for( Uint32 index = 0, end = curveType.Size(); index != end; ++index  )
		{
			curve->m_data.m_curveValues[ index ].curveTypeL = curveType[ index ];
		}

		return true;
	}
	else if( propName == c_curveSimplecurveTypeR )
	{
		TDynArray< Uint32, MC_CurveData > curveType;
		propValue.AsType( curveType );

		curve->m_data.m_curveValues.Resize( curveType.Size() );
		
		for( Uint32 index = 0, end = curveType.Size(); index != end; ++index  )
		{
			curve->m_data.m_curveValues[ index ].curveTypeR = curveType[ index ];
		}

		return true;
	}
	else if( propName == c_curveSimplecontrolPoint )
	{
		TDynArray< Vector, MC_CurveData > controlPoints;
		propValue.AsType( controlPoints );

		curve->m_data.m_curveValues.Resize( controlPoints.Size() );

		for( Uint32 index = 0, end = controlPoints.Size(); index != end; ++index  )
		{
			curve->m_data.m_curveValues[ index ].controlPoint = controlPoints[ index ];
		}

		return true;
	}

	return false;
}
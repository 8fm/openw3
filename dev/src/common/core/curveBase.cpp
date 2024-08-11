/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "curveBase.h"
#include "xmlFile.h"

IMPLEMENT_RTTI_ENUM( ECurvePointInterpolateMode );
IMPLEMENT_RTTI_ENUM( ESimpleCurveType );

SCurveBase::SCurveBase( ECurveValueType valueType, Bool loop )
	: m_data( valueType )
	, m_ScalarEditScale (1.f)
	, m_ScalarEditOrigin (0.f)
#ifndef NO_EDITOR
	, m_useDayTime( true )
#endif
{
	m_data.m_loop = loop;

	m_simpleCurveType = (valueType == CVT_Float ? SCT_Float : SCT_Vector);
}

SCurveBase::~SCurveBase()
{
}


SCurveBase& SCurveBase::Clear()
{
	m_data.Clear();
	return *this;
}


Int32 SCurveBase::AddPoint( Float newTime, const Vector &newValue, bool allowReplacement /*= true*/ )
{
	newTime = AdaptTime( newTime );

	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.AddPoint( newTime, newValue, allowReplacement );
	}
	else
	{
		return m_data.AddPoint( newTime, newValue.W, CST_Interpolate, allowReplacement );
	}
}

Int32 SCurveBase::AddPoint( Float newTime, Float newValue, const ECurveSegmentType& segType /*= CST_Interpolate*/, Bool allowReplacement /*= false*/ )
{
	newTime = AdaptTime( newTime );

	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.AddPoint( newTime, Vector( 0, 0, 0, newValue ), allowReplacement );
	}
	else
	{
		return m_data.AddPoint( newTime, newValue, segType, allowReplacement );
	}
}


void SCurveBase::SetValue( Uint32 index, const Vector &newValue )
{
	ASSERT( index < m_data.Size() );
	if ( index < m_data.Size() )
	{
		if ( m_data.GetValueType() == CVT_Vector )
		{
			m_data.m_curveValues[index].controlPoint = newValue;
		}
		else
		{
			m_data.m_curveValues[index].value = newValue.W;
		}
	}
}

void SCurveBase::SetValue( Uint32 index, Float newValue )
{
	ASSERT( index < m_data.Size() );
	if ( index < m_data.Size() )
	{
		if ( m_data.GetValueType() == CVT_Vector )
		{
			m_data.m_curveValues[index].controlPoint.Set4( 0, 0, 0, newValue );
		}
		else
		{
			m_data.m_curveValues[index].value = newValue;
		}
	}
}


Int32 SCurveBase::RemovePointAtTime( Float time )
{
	time = AdaptTime( time );
	return m_data.RemovePoint( time );
}

void SCurveBase::RemovePointAtIndex( Uint32 index )
{
	m_data.RemovePointAtIndex( index );
}

Uint32 SCurveBase::SetPointTime( Uint32 index, Float time )
{
	return m_data.SetTime( index, time );
}

void SCurveBase::SetTangentType( const Uint32& index, const Int32& tangentIndex, ECurveSegmentType type )
{
	// Only float-valued curves can use tangents.
	if ( m_data.GetValueType() == CVT_Float )
	{
		m_data.SetTangentType( index, tangentIndex, type );
	}
}

ECurveSegmentType SCurveBase::GetTangentType( const Uint32& index, const Int32& tangentIndex ) const
{
	if ( m_data.GetValueType() != CVT_Float )
	{
		return CST_Interpolate;
	}
	return m_data.GetTangentType( index, tangentIndex );
}


void SCurveBase::SetTangentValue( const Uint32& index, const Int32& tangentIndex, const Vector& tangentValue )
{
	// Only float-valued curves can use tangents.
	if ( m_data.GetValueType() == CVT_Float )
	{
		m_data.SetTangentValue( index, tangentIndex, tangentValue );
	}
}

Vector SCurveBase::GetTangentValue( const Uint32& index, const Int32& tangentIndex ) const
{
	if ( m_data.GetValueType() != CVT_Float )
	{
		return Vector::ZEROS;
	}
	return m_data.GetTangentValue( index, tangentIndex );
}


SCurveTangents SCurveBase::GetTangent( Uint32 index ) const
{
	if ( m_data.GetValueType() == CVT_Float )
		return SCurveTangents( m_data.m_curveValues[index].controlPoint, m_data.m_curveValues[index].curveTypeL, m_data.m_curveValues[index].curveTypeR );
	else
		return SCurveTangents();
}


void SCurveBase::SetTangent( Uint32 index, const SCurveTangents& tangents )
{
	if ( m_data.GetValueType() != CVT_Float )
	{
		return;
	}

	m_data.m_curveValues[index].curveTypeL = (ECurveSegmentType)tangents.m_segmentTypes[0];
	m_data.m_curveValues[index].curveTypeR = (ECurveSegmentType)tangents.m_segmentTypes[1];
	m_data.m_curveValues[index].controlPoint = tangents.m_tangents; 
}

Vector SCurveBase::GetValueAtIndex( Uint32 index ) const
{
	ASSERT( index < m_data.Size() );
	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.m_curveValues[index].controlPoint;
	}
	else
	{
		return Vector( 0, 0, 0, m_data.m_curveValues[index].value );
	}
}

Float SCurveBase::GetFloatValueAtIndex( Uint32 index ) const
{
	ASSERT( index < m_data.Size() );
	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.m_curveValues[index].controlPoint.W;
	}
	else
	{
		return m_data.m_curveValues[index].value;
	}
}

// The times given here are not affected by the loop setting.
void SCurveBase::RemapControlPoints( Float oldTimeMin, Float oldTimeMax, Float newTimeMin, Float newTimeMax )
{
	ASSERT( oldTimeMax > oldTimeMin );
	ASSERT( newTimeMax > newTimeMin );
	Float scale = ( newTimeMax - newTimeMin ) / ( oldTimeMax - oldTimeMin );

	for( Uint32 i=0; i< m_data.m_curveValues.Size(); i++ )
	{
		m_data.m_curveValues[i].time = ( m_data.m_curveValues[i].time - oldTimeMin ) * scale + newTimeMin;
	}
}

Vector SCurveBase::GetValue( Float time ) const
{
	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.GetVectorValue( time );
	}
	else
	{
		return Vector( 0, 0, 0, m_data.GetFloatValue( time ) );
	}
}

Float SCurveBase::GetFloatValue( Float time ) const
{
	if ( m_data.GetValueType() == CVT_Vector )
	{
		return m_data.GetVectorValue( time ).W;
	}
	else
	{
		return m_data.GetFloatValue( time );
	}
}


// Returns -1 if there is no control point at the given time.
Int32 SCurveBase::GetIndex( Float time ) const
{
	time = AdaptTime( time );

	return m_data.GetIndex( time );
}

Float SCurveBase::GetDuration() const
{
	if ( IsEmpty() ) return 0.0f;

	return m_data.m_curveValues[m_data.Size() - 1].time;
}



Bool SCurveBase::GetValuesMinMax( Vector& minValue, Vector& maxValue ) const
{
	if( m_data.Size() == 0 )
	{
		return false;
	}

	minValue.Set4( NumericLimits< Float >::Max() );
	maxValue.Set4( -NumericLimits< Float >::Max() );

	if ( m_data.GetValueType() == CVT_Vector )
	{
		for( Uint32 i=0; i< m_data.Size(); i++ )
		{
			const Vector& v = m_data.m_curveValues[i].controlPoint;
			minValue = Vector::Min4( minValue, v );
			maxValue = Vector::Max4( maxValue, v );
		}
	}
	else
	{
		for( Uint32 i=0; i< m_data.Size(); i++ )
		{
			Float v = m_data.m_curveValues[i].value;
			minValue.W = ::Min( minValue.W, v );
			maxValue.W = ::Max( maxValue.W, v );
		}
	}

	return true;
}

Bool SCurveBase::GetFloatValuesMinMax( Float& minValue, Float& maxValue ) const
{
	if( m_data.Size() == 0 )
	{
		return false;
	}

	minValue = NumericLimits< Float >::Max();
	maxValue = -NumericLimits< Float >::Max();

	if ( m_data.GetValueType() == CVT_Vector )
	{
		for( Uint32 i=0; i< m_data.Size(); i++ )
		{
			const Vector& v = m_data.m_curveValues[i].controlPoint;
			minValue = ::Min( minValue, v.W );
			maxValue = ::Max( maxValue, v.W );
		}
	}
	else
	{
		for( Uint32 i=0; i< m_data.Size(); i++ )
		{
			Float v = m_data.m_curveValues[i].value;
			minValue = ::Min( minValue, v );
			maxValue = ::Max( maxValue, v );
		}
	}

	return true;
}

Bool SCurveBase::GetTimesMinMax( Float& minValue, Float& maxValue ) const
{
	if( m_data.Size() == 0 )
	{
		return false;
	}

	minValue = m_data.m_curveValues[0].time;
	maxValue = m_data.m_curveValues[m_data.Size() - 1].time;

	return true;
}


void SCurveBase::SetInterpolatedValue( const SCurveBase &srcCurve, const SCurveBase &destCurve, Float lerpFactor, Float time, ECurvePointInterpolateMode interpolationMode /*= CPI_LINEAR*/ )
{
	Vector valueA = srcCurve.GetValue(time);
	Vector valueB = destCurve.GetValue(time);

	// If either curve is a non-vector type (SegmentedFloat), copy the vector data from the other.
	if ( srcCurve.m_data.GetValueType() == CVT_Float && destCurve.m_data.GetValueType() == CVT_Vector )
	{
		// Set3 only modifies XYZ, so we keep the same scalar value.
		valueA.Set3( valueB );
	}
	else if (srcCurve.m_data.GetValueType() == CVT_Vector && destCurve.m_data.GetValueType() == CVT_Float )
	{
		valueB.Set3( valueA );
	}

	// Adjust the interpolation factor, based on what type of interpolation we're using.
	Float finalFactor = lerpFactor;

	switch ( interpolationMode )
	{
	case CPI_LINEAR:
		break;
	case CPI_X2:
		finalFactor *= finalFactor;
		break;
	case CPI_X3:
		finalFactor *= finalFactor * finalFactor;
		break;
	case CPI_X2_INV:
		finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor);
		break;
	case CPI_X3_INV:
		finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor)*(1-finalFactor);
		break;
	case CPI_X2_HISTERESIS:
		{
			if ( valueA.W < valueB.W )
			{
				finalFactor *= finalFactor;
			}
			else
			{
				finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor);
			}
		}
		break;
	case CPI_X3_HISTERESIS:
		{
			if ( valueA.W < valueB.W )
			{
				finalFactor *= finalFactor * finalFactor;
			}
			else
			{
				finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor)*(1-finalFactor);
			}
		}
		break;
	case CPI_X2_INV_HISTERESIS:
		{
			if ( valueA.W < valueB.W )
			{
				finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor);
			}
			else
			{
				finalFactor *= finalFactor;
			}
		}
		break;
	case CPI_X3_INV_HISTERESIS:
		{
			if ( valueA.W < valueB.W )
			{
				finalFactor = 1.0f - (1-finalFactor)*(1-finalFactor)*(1-finalFactor);
			}
			else
			{
				finalFactor *= finalFactor * finalFactor;
			}
		}
		break;
	}

	Vector valueResult = ::Lerp(lerpFactor, valueA, valueB);

	// TODO: Change value type accordingly?

	Clear();

	AddPoint(time, valueResult, true);
}



void SCurveBase::GetApproximationSamples( TDynArray< Vector >& samples, Uint32 numSamples ) const
{
	samples.ResizeFast( numSamples );
	GetApproximationSamples( samples.TypedData(), numSamples );
}

void SCurveBase::GetApproximationSamples( TDynArray< Float >& samples, Uint32 numSamples ) const
{
	samples.ResizeFast( numSamples );
	GetApproximationSamples( samples.TypedData(), numSamples );
}

void SCurveBase::GetApproximationSamples( Vector* samples, Uint32 numSamples ) const
{
	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( numSamples - 1 );

	for( Uint32 i = 0; i < numSamples; ++i, t += dt )
	{
		samples[ i ] = GetValue( t );
	}
}

void SCurveBase::GetApproximationSamples( Float* samples, Uint32 numSamples ) const
{
	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( numSamples - 1 );

	for( Uint32 i = 0; i < numSamples; ++i, t += dt )
	{
		samples[ i ] = GetFloatValue( t );
	}
}




Bool SCurveBase::ConvertValueFromType( Float time, Vector &value, ESimpleCurveType srcType ) const
{
	if ( srcType == m_simpleCurveType )
	{
		return true;
	}

	if ( srcType == SCT_ColorScaled )
	{
		if ( m_simpleCurveType == SCT_Float )
		{
			value.X = value.Y = value.Z = 0;
		}
		else if ( m_simpleCurveType == SCT_Color )
		{
			value.W = 1;
		}
		else
		{
			HALT(  "Curve conversion type mismatch." );
		}

		return true;
	}

	if ( srcType == SCT_Float && m_simpleCurveType == SCT_ColorScaled )
	{
		Float val = value.W;
		value = GetValue( time );
		value.W = val;
		return true;
	}

	if ( srcType == SCT_Color && m_simpleCurveType == SCT_ColorScaled )
	{
		value.W = GetValue( time ).W;
		return true;
	}

	ASSERT( srcType != SCT_Vector );

	return false;
}

Bool SCurveBase::CanImportFromType( ESimpleCurveType srcType ) const
{
	ESimpleCurveType destType = m_simpleCurveType;

	// Do not import from Vector type
	if ( srcType == SCT_Vector )
	{
		return false;
	}

	// Cannot import from Color type to Float type
	if ( srcType == SCT_Color && destType == SCT_Float )
	{
		return false;
	}

	// Cannot import from Float type to Color type
	if ( srcType == SCT_Float && destType == SCT_Color )
	{
		return false;
	}

	return true;
}

Bool SCurveBase::SerializeXML( IXMLFile& file, Bool merge /*= false*/ )
{
	if ( file.IsReader() )
	{
		if ( !file.BeginNode( TXT("curve") ) )
		{
			return false;
		}

		ECurveBaseType newBaseType = m_data.GetBaseType();
		String valueType;
		String curveType;
		String simpleCurveType;
		String editScale;
		String editOrigin;

		if ( file.Attribute( TXT("curveType"), curveType ) )
		{
			::FromString< Int32 >( curveType, (Int32&)(newBaseType) );
		}

		if ( !file.Attribute( TXT("type"), simpleCurveType ) )
		{
			return false;
		}

		if ( !file.Attribute( TXT("editScale"), editScale ) )
		{
			return false;
		}

		if ( !file.Attribute( TXT("editOrigin"), editOrigin ) )
		{
			return false;
		}

		ESimpleCurveType srcType;
		Float newScale;
		Float newOrigin;
		::FromString< Int32 >( simpleCurveType, *(Int32 *)(&srcType) );
		::FromString< Float >( editScale, newScale );
		::FromString< Float >( editOrigin, newOrigin );

		if ( !CanImportFromType( srcType) )
		{
			WARN_CORE( TXT("Simple curve types mismatch.") );
			return false;
		}

		// do not change the m_simpleCurveType
		SetScalarEditScale( newScale );
		SetScalarEditOrigin( newOrigin );

		if ( !merge )
		{
			// Replace points
			Clear();
		}

		SetCurveType( newBaseType );

		const Uint32 numPoints = file.GetChildCount();
		for ( Uint32 i = 0; i < numPoints; ++i )
		{
			file.BeginNode( TXT("point") );

			String timeString;
			String valueString;
			file.Attribute( TXT("time"), timeString );
			file.Attribute( TXT("value"), valueString );

			Float newTime;
			Vector newValue;
			::FromString< Float >( timeString, newTime );
			::FromString< Vector >( valueString, newValue );
			ConvertValueFromType( newTime, newValue, srcType );

			if ( m_data.GetValueType() == CVT_Vector )
			{
				AddPoint( newTime, newValue );
			}
			else
			{
				AddPoint( newTime, newValue.W );
			}

			if ( m_data.GetValueType() == CVT_Float )
			{
				String segString, tanString;
				Uint32 segType;
				Vector tanValue;

				if ( file.Attribute( TXT("type0"), segString ) &&
					file.Attribute( TXT("tan0"), tanString ) )
				{
					::FromString< Uint32 >( segString, segType );
					::FromString< Vector >( tanString, tanValue );
					SetTangentType( i, 0, (ECurveSegmentType)segType );
					SetTangentValue( i, 0, tanValue );
				}

				if ( file.Attribute( TXT("type1"), segString ) &&
					file.Attribute( TXT("tan1"), tanString ) )
				{
					::FromString< Uint32 >( segString, segType );
					::FromString< Vector >( tanString, tanValue );
					SetTangentType( i, 1, (ECurveSegmentType)segType );
					SetTangentValue( i, 1, tanValue );
				}
			}

			file.EndNode();
		}

		file.EndNode();
	}
	else
	{
		file.BeginNode( TXT("curve") );

		String valueType = ::ToString< Int32 >( (Int32)m_data.GetValueType() );
		String curveType = ::ToString< Int32 >( (Int32)GetCurveType() );
		String simpleCurveType  = ::ToString< Int32 >( (Int32)GetSimpleCurveType() );
		String editScale  = ::ToString< Float >( GetScalarEditScale() );
		String editOrigin = ::ToString< Float >( GetScalarEditOrigin() );

		file.Attribute( TXT("valueType"), valueType );
		file.Attribute( TXT("curveType"), curveType );
		file.Attribute( TXT("type"), simpleCurveType );
		file.Attribute( TXT("editScale"), editScale );
		file.Attribute( TXT("editOrigin"), editOrigin );

		const Uint32 numPoints = GetNumPoints();
		for ( Uint32 i = 0; i < numPoints; ++i )
		{
			file.BeginNode( TXT("point") );
			String timeString = ::ToString< Float >( m_data.m_curveValues[i].time );
			String valueString;

			if ( m_data.GetValueType() == CVT_Vector )
			{
				valueString = ::ToString< Vector >( m_data.m_curveValues[i].controlPoint );
			}
			else
			{
				valueString = ::ToString< Vector >( Vector( 0, 0, 0, m_data.m_curveValues[i].value ) );
			}

			file.Attribute( TXT("time"), timeString );
			file.Attribute( TXT("value"), valueString );

			// We only need to save out tangent data if we're using it.
			if ( m_data.GetValueType() == CVT_Float )
			{
				String tanString, segString;

				segString = ::ToString< Uint32 >( GetTangentType( i, 0 ) );
				tanString = ::ToString< Vector >( GetTangentValue( i, 0 ) );
				file.Attribute( TXT("type0"), segString );
				file.Attribute( TXT("tan0"), tanString );

				segString = ::ToString< Uint32 >( GetTangentType( i, 1 ) );
				tanString = ::ToString< Vector >( GetTangentValue( i, 1 ) );
				file.Attribute( TXT("type1"), segString );
				file.Attribute( TXT("tan1"), tanString );
			}
			file.EndNode();
		}

		file.EndNode();
	}

	return true;
}

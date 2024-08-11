#include "build.h"
#include "multiCurve.h"
#include "../core/mathUtils.h"
#include "../core/curveData.h"
#include "node.h"

IMPLEMENT_ENGINE_CLASS( SMultiCurvePosition );
IMPLEMENT_ENGINE_CLASS( SMultiCurve );
IMPLEMENT_ENGINE_CLASS( SCurveEaseParam );
IMPLEMENT_RTTI_ENUM( ECurveType );
IMPLEMENT_RTTI_ENUM( ECurveInterpolationMode );
IMPLEMENT_RTTI_ENUM( ECurveManualMode );
IMPLEMENT_RTTI_ENUM( ECurveRelativeMode );

SMultiCurve::SMultiCurve() :
	m_type( ECurveType_Uninitialized ),
	m_color( Color( 255, 50, 50, 255 ) ),
	m_positionInterpolationMode( ECurveInterpolationMode_Automatic ),
	m_positionManualMode( ECurveManualMode_BezierSymmetric ),
	m_rotationInterpolationMode( ECurveInterpolationMode_Linear ),
	m_totalTime( 1.0f ),
	m_automaticPositionInterpolationSmoothness( 1.0f ),
	m_automaticRotationInterpolationSmoothness( 1.0f ),
	m_enableConsistentNumberOfControlPoints( true ),
	m_enableAutomaticTimeByDistanceRecalculation( false ),
	m_enableAutomaticTimeRecalculation( false ),
	m_enableAutomaticRotationFromDirectionRecalculation( false ),
	m_translationRelativeMode( ECurveRelativeMode_InitialTransform ),
	m_rotationRelativeMode( ECurveRelativeMode_InitialTransform ),
	m_scaleRelativeMode( ECurveRelativeMode_InitialTransform ),
	m_parent( NULL ),
	m_hasInitialParentTransform( false )
{
}

/*
Cctor.

Compiler generated cctor would also copy m_parent, m_hasInitialParentTransform and m_initialParentTransform - we don't want that.
*/
SMultiCurve::SMultiCurve( const SMultiCurve& other )
: m_type( other.m_type )
, m_showFlags( other.m_showFlags )
, m_color( other.m_color )
, m_positionInterpolationMode( other.m_positionInterpolationMode )
, m_positionManualMode( other.m_positionManualMode )
, m_automaticPositionInterpolationSmoothness( other.m_automaticPositionInterpolationSmoothness )
, m_rotationInterpolationMode( other.m_rotationInterpolationMode )
, m_automaticRotationInterpolationSmoothness( other.m_automaticRotationInterpolationSmoothness )
, m_totalTime( other.m_totalTime )
, m_enableConsistentNumberOfControlPoints( other.m_enableConsistentNumberOfControlPoints )
, m_enableAutomaticTimeByDistanceRecalculation( other.m_enableAutomaticTimeByDistanceRecalculation )
, m_enableAutomaticTimeRecalculation( other.m_enableAutomaticTimeRecalculation )
, m_enableAutomaticRotationFromDirectionRecalculation( other.m_enableAutomaticRotationFromDirectionRecalculation )
, m_curves( other.m_curves )
, m_leftTangents( other.m_leftTangents )
, m_rightTangents( other.m_rightTangents )
, m_easeParams( other.m_easeParams )
, m_translationRelativeMode( other.m_translationRelativeMode )
, m_rotationRelativeMode( other.m_rotationRelativeMode )
, m_scaleRelativeMode( other.m_scaleRelativeMode )
, m_parent( nullptr )
, m_hasInitialParentTransform( false )
{}

void SMultiCurve::SetTransformationRelativeMode( ECurveRelativeMode mode )
{
	SetTranslationRelativeMode( mode );
	SetRotationRelativeMode( mode );
	SetScaleRelativeMode( mode );
}

void SMultiCurve::SetTranslationRelativeMode( ECurveRelativeMode mode )
{
	m_translationRelativeMode = mode;
}

void SMultiCurve::SetRotationRelativeMode( ECurveRelativeMode mode )
{
	m_rotationRelativeMode = mode;
}

void SMultiCurve::SetScaleRelativeMode( ECurveRelativeMode mode )
{
	m_scaleRelativeMode = mode;
}

ECurveType SMultiCurve::GetCurveType() const
{
	return m_type;
}

void SMultiCurve::RecalculateBezierTangents()
{
	m_leftTangents.ClearFast();
	m_rightTangents.ClearFast();
	ValidateTangents();
	PostChange();
}

Float SMultiCurve::ToNormalizedTimeAtControlPoint( Float time ) const
{
	time /= m_totalTime;
	time = Clamp( time, 0.0f, 1.0f );
	return time;
}

Float SMultiCurve::FromNormalizedTimeAtControlPoint( Float time ) const
{
	time = Clamp( time, 0.0f, 1.0f );
	time *= m_totalTime;
	return time;
}

Float SMultiCurve::ToNormalizedTime( Float time ) const
{
	time = ToNormalizedTimeAtControlPoint( time );

	if ( !HasEaseParams() )
	{
		return time;
	}

	// Remap time based on ease in/out parameters

	Int32 i1, i2;
	m_curves[ 0 ].FindLowerUpperBounds( time, i1, i2 );
	if ( i1 == i2 )
	{
		return time;
	}
	else
	{
		// Get times for control points

		const auto & entry = m_curves[ 0 ].m_curveValues;

		Float t1 = entry[ i1 ].time;
		Float t2 = entry[ i2 ].time;
		if ( m_curves[ 0 ].IsLoop() )
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

		const Float timeRange = ( t2 - t1 );
		const Float localTime = ( time - t1 ) / timeRange;
		ASSERT( t1 < t2 );
		ASSERT( localTime >= 0.0f && localTime <= 1.0f );

		// Remap time using Bezier curve

		const Float tangent1 = timeRange * m_easeParams[ i1 ].m_easeOut;
		const Float tangent2 = timeRange * m_easeParams[ i2 ].m_easeIn;

		return MathUtils::InterpolationUtils::Hermite1D( localTime, t1, tangent1, tangent2, t2 );
	}
}

void SMultiCurve::ValidateTangents()
{
	if ( m_positionInterpolationMode != ECurveInterpolationMode_Manual )
	{
		m_leftTangents.Clear();
		m_rightTangents.Clear();
		return;
	}

	const Uint32 numControlPoints = Size();

	if ( m_leftTangents.Size() == numControlPoints && m_rightTangents.Size() == numControlPoints )
	{
		return;
	}

	m_leftTangents.Resize( numControlPoints );
	m_rightTangents.Resize( numControlPoints );

	for ( Uint32 i = 0; i < numControlPoints; i++ )
	{
		Vector curveDirection;
		CalculateTangentFromCurveAtControlPoint( i, curveDirection );
		curveDirection *= m_automaticPositionInterpolationSmoothness;

		m_leftTangents[i] = -curveDirection;
		m_rightTangents[i] = curveDirection;
	}
}

void SMultiCurve::SetPositionInterpolationMode( ECurveInterpolationMode mode )
{
	ASSERT( HasPosition() );

	if ( m_positionInterpolationMode == mode )
	{
		return;
	}
	m_positionInterpolationMode = mode;

	ECurveBaseType baseType = CT_Linear;
	switch ( mode )
	{
	case ECurveInterpolationMode_Linear:
		baseType = CT_Linear;
		break;
	case ECurveInterpolationMode_Automatic:
	case ECurveInterpolationMode_Manual:
		baseType = CT_Smooth; // This will make position look different between 2D and 3D editors (3D uses SMultiCurve, 2D uses SCurveData interpolation methods)
		break;
	}

	m_curves[0].SetBaseType( baseType );
	m_curves[1].SetBaseType( baseType );
	m_curves[2].SetBaseType( baseType );

	ValidateTangents();
	PostChange();
}

ECurveInterpolationMode SMultiCurve::GetPositionInterpolationMode() const
{
	return m_positionInterpolationMode;
}

void SMultiCurve::SetPositionManualMode( ECurveManualMode mode )
{
	m_positionManualMode = mode;
	PostChange();
}

void SMultiCurve::SetRotationInterpolationMode( ECurveInterpolationMode mode )
{
	ASSERT( HasRotation() );

	if ( m_rotationInterpolationMode == mode )
	{
		return;
	}
	m_rotationInterpolationMode = mode;

	ECurveBaseType baseType = CT_Linear;
	switch ( mode )
	{
	case ECurveInterpolationMode_Linear:
		baseType = CT_Linear;
		break;
	case ECurveInterpolationMode_Automatic:
	case ECurveInterpolationMode_Manual:
		baseType = CT_Smooth; // This will make rotation look different between 2D and 3D editors (3D uses SMultiCurve, 2D uses SCurveData interpolation methods)
		break;
	}

	SCurveData* curves = GetRotationCurves();
	curves[0].SetBaseType( baseType );
	curves[1].SetBaseType( baseType );
	curves[2].SetBaseType( baseType );

	PostChange();
}

ECurveInterpolationMode SMultiCurve::GetRotationInterpolationMode() const
{
	return m_rotationInterpolationMode;
}

ECurveManualMode SMultiCurve::GetPositionManualMode() const
{
	return m_positionManualMode;
}

Bool SMultiCurve::HasRotation() const
{
	return m_type == ECurveType_EulerAngles || m_type == ECurveType_EngineTransform;
}

Bool SMultiCurve::HasPosition() const
{
	return m_type == ECurveType_Vector || m_type == ECurveType_EngineTransform;
}

Bool SMultiCurve::HasScale() const
{
	return m_type == ECurveType_EngineTransform;
}

Bool SMultiCurve::IsEditableIn3D() const
{
	return m_type == ECurveType_Vector || m_type == ECurveType_EngineTransform;
}

Bool SMultiCurve::IsEditableIn2D() const
{
	return m_type == ECurveType_Vector || m_type == ECurveType_EulerAngles || m_type == ECurveType_Float;
}

void SMultiCurve::SetLooping( Bool loop )
{
	for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		it->SetLoop( loop );
	}

	if ( m_positionInterpolationMode == ECurveInterpolationMode_Automatic )
	{
		RecalculateBezierTangents();
	}

	PostChange();
}

Bool SMultiCurve::IsLooping() const
{
	return m_curves[0].IsLoop();
}

Uint32 SMultiCurve::GetNumCurves() const
{
	return m_curves.Size();
}

SCurveData* SMultiCurve::GetCurveData( Uint32 index )
{
	return &m_curves[ index ];
}

Float SMultiCurve::GetTotalTime() const
{
	return m_totalTime;
}

Uint32 SMultiCurve::GetNumBaseDataStreams() const
{
	switch ( m_type )
	{
	case ECurveType_Float: return 1;
	case ECurveType_EulerAngles: return 3;
	case ECurveType_Vector: return 3;
	case ECurveType_EngineTransform: return 9;
	}
	return 0;
}


Uint32 SMultiCurve::GetNumExtraDataStreams() const
{
	return m_curves.Size() - GetNumBaseDataStreams();
}

void SMultiCurve::GetExtraDataValues( Float time, Float* result ) const
{
	const Float normalizedTime = ToNormalizedTime( time );

	Uint32 numExtraDataStreams;
	const SCurveData* curves = GetExtraDataCurves(numExtraDataStreams);

	for ( Uint32 i = 0; i < numExtraDataStreams; i++ )
		result[i] = curves[i].GetFloatValue( normalizedTime );
}

void SMultiCurve::GetControlPointExtraDataValues( Uint32 index, Float* result ) const
{
	Uint32 numExtraDataStreams;
	const SCurveData* curves = GetExtraDataCurves(numExtraDataStreams);

	for ( Uint32 i = 0; i < numExtraDataStreams; i++ )
		result[i] = curves[i].GetFloatValueAtIndex( index );
}

void SMultiCurve::SetExtraDataValues( Uint32 index, const Float* values )
{
	Uint32 numExtraDataStreams;
	SCurveData* curves = GetExtraDataCurves(numExtraDataStreams);

	for ( Uint32 i = 0; i < numExtraDataStreams; i++ )
		curves[i].SetValueAtIndex( index, values[i] );
}

Float SMultiCurve::GetFloat( Float time ) const
{
	ASSERT( m_type == ECurveType_Float );
	return m_curves[0].GetFloatValue( ToNormalizedTime( time ) );
}

void SMultiCurve::GetPosition( Float time, Vector& result ) const
{
	ASSERT( m_type == ECurveType_Vector || m_type == ECurveType_EngineTransform );

	Float normalizedTime = ToNormalizedTime( time );
	
	Int32 i1, i2;
	m_curves[0].FindLowerUpperBounds( normalizedTime, i1, i2 );

	if ( i1 == i2 )
	{
		GetControlPointPosition( i1, result );
		return;
	}

	const auto& entry = m_curves[ 0 ].m_curveValues;

	Float t1 = entry[ i1 ].time;
	Float t2 = entry[ i2 ].time;
	if ( m_curves[ 0 ].IsLoop() )
	{
		while ( t2 <= t1 )
		{
			t2 += 1.0f;
		}
		while ( normalizedTime < t1 )
		{
			normalizedTime += 1.0f;
		}
	}

	const Float localTime = ( normalizedTime - t1 ) / ( t2 - t1 );
	ASSERT( t1 < t2 );
	ASSERT( localTime >= 0.0f && localTime <= 1.0f );

	Vector pos1, pos2;
	GetControlPointPosition( i1, pos1 );
	GetControlPointPosition( i2, pos2 );

	switch ( m_positionInterpolationMode )
	{
		case ECurveInterpolationMode_Linear:
		{
			result = pos1 + ( pos2 - pos1 ) * localTime;
			break;
		}
		case ECurveInterpolationMode_Automatic:
		{
			Vector t1, t2;
			CalculateTangentFromCurveAtControlPoint( i1, t1 );
			CalculateTangentFromCurveAtControlPoint( i2, t2 );

			t1 *= m_automaticPositionInterpolationSmoothness;
			t2 *= m_automaticPositionInterpolationSmoothness;

			result = MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( pos1, t1, pos2, t2, localTime );
			break;
		}
		case ECurveInterpolationMode_Manual:
		{
			switch ( m_positionManualMode )
			{
				case ECurveManualMode_Bezier:
				case ECurveManualMode_BezierSymmetric:
				case ECurveManualMode_BezierSymmetricDirection:
				{
					result = MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( pos1, m_rightTangents[ i1 ], pos2, -m_leftTangents[ i2 ], localTime );
					break;
				}
			}
			break;
		}
	}
}

void SMultiCurve::GetPositionInSegment( Uint32 startIndex, Float localTime, Vector& result ) const
{
	ASSERT( m_type == ECurveType_Vector || m_type == ECurveType_EngineTransform );

	const Uint32 endIndex = GetNextIndex( startIndex );

	Vector pos1, pos2;
	GetControlPointPosition( startIndex, pos1 );
	GetControlPointPosition( endIndex, pos2 );

	switch ( m_positionInterpolationMode )
	{
		case ECurveInterpolationMode_Linear:
		{
			result = pos1 + ( pos2 - pos1 ) * localTime;
			break;
		}
		case ECurveInterpolationMode_Automatic:
		{
			Vector t1, t2;
			CalculateTangentFromCurveAtControlPoint( startIndex, t1 );
			CalculateTangentFromCurveAtControlPoint( endIndex, t2 );

			t1 *= m_automaticPositionInterpolationSmoothness;
			t2 *= m_automaticPositionInterpolationSmoothness;

			result = MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( pos1, t1, pos2, t2, localTime );
			break;
		}
		case ECurveInterpolationMode_Manual:
		{
			switch ( m_positionManualMode )
			{
				case ECurveManualMode_Bezier:
				case ECurveManualMode_BezierSymmetric:
				case ECurveManualMode_BezierSymmetricDirection:
				{
					result = MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( pos1, m_rightTangents[ startIndex ], pos2, -m_leftTangents[ endIndex ], localTime );
					break;
				}
			}
			break;
		}
	}
}

void SMultiCurve::GetInterpolationControlPoints( Vector& p1, Vector& p2, Vector& p3, Vector& p4, Int32 edgeIdx ) const
{
	const Uint32 nVertices	= Size();
	const Bool closed		= IsLooping() && nVertices > 2;
	const Uint32 nEdges		= closed ? nVertices : nVertices - 1;

	ASSERT( edgeIdx >= 0 && edgeIdx < (Int32)nEdges );

	GetControlPointPosition(
		(edgeIdx > 0 )
		? ( edgeIdx - 1 )
		: closed
		? ( nVertices - 1 )
		: edgeIdx,
		p1 );
	GetControlPointPosition( edgeIdx, p2 );
	GetControlPointPosition( (edgeIdx + 1) % nVertices, p3 );
	GetControlPointPosition(
		closed
		? ( (edgeIdx + 2) % nVertices )
		: Min<Int32>( nVertices-1, edgeIdx+2 ),
		p4 );
}

void SMultiCurve::SetAutomaticInterpolationSmoothness( Float smoothness )
{
	m_automaticPositionInterpolationSmoothness = smoothness;
}

void SMultiCurve::GetRotation( Float time, EulerAngles& result ) const
{
	ASSERT( HasRotation() );

	const Float normalizedTime = ToNormalizedTime( time );

	if ( const SCurveData* curves = GetRotationCurves() )
	{
		result.Roll = curves[0].GetAngleValue( normalizedTime );
		result.Pitch = curves[1].GetAngleValue( normalizedTime );
		result.Yaw = curves[2].GetAngleValue( normalizedTime );
	}

	if ( m_automaticRotationInterpolationSmoothness != 1.0f && HasPosition() )
	{
		EulerAngles curveAngles;
		CalculateAnglesFromCurveDirection( time, curveAngles );

		result = EulerAngles::Interpolate( result, curveAngles, 1.0f - Clamp( m_automaticRotationInterpolationSmoothness, 0.0f, 1.0f ) );
		result.Normalize();
	}
}

void SMultiCurve::GetTransform( Float time, EngineTransform& result ) const
{
	if ( HasPosition() )
	{
		Vector pos;
		GetPosition( time, pos );
		result.SetPosition( pos );
	}

	if ( HasRotation() )
	{
		EulerAngles rot;
		GetRotation( time, rot );
		result.SetRotation( rot );
	}

	if ( HasScale() )
	{
		Vector scale;
		GetScale( time, scale );
		result.SetScale( scale );
	}
}

void SMultiCurve::GetScale( Float time, Vector& result ) const
{
	ASSERT( HasScale() );

	time = ToNormalizedTime( time );

	if ( const SCurveData* curves = GetScaleCurves() )
	{
		result.Set3( curves[0].GetFloatValue( time ), curves[1].GetFloatValue( time ), curves[2].GetFloatValue( time ) );
	}
}

void SMultiCurve::Reset()
{
	m_type = ECurveType_Uninitialized;
	m_enableConsistentNumberOfControlPoints = true;
	m_enableAutomaticTimeByDistanceRecalculation = false;
	m_enableAutomaticTimeRecalculation = false;
	m_enableAutomaticRotationFromDirectionRecalculation = false;
	m_curves.Clear();
	m_leftTangents.Clear();
	m_rightTangents.Clear();
	m_easeParams.Clear();
	m_translationRelativeMode = ECurveRelativeMode_InitialTransform;
	m_rotationRelativeMode = ECurveRelativeMode_None;
	m_scaleRelativeMode = ECurveRelativeMode_None;
}

void SMultiCurve::SetCurveType( ECurveType curveType, const void* initialValue, Bool insertEndPoints, Uint32 numExtraDataStreams )
{
	if (m_type == curveType)
	{
		return;
	}
	m_type = curveType;
	m_curves.ClearFast();

	// Set up curve with initial values

	switch ( curveType )
	{
		case ECurveType_Float:
		{
			m_enableAutomaticRotationFromDirectionRecalculation = false;
			m_enableAutomaticTimeByDistanceRecalculation = false;

			m_curves.Resize( 1 );
			m_curves[0].SetBaseType( CT_Smooth );

			if ( insertEndPoints )
			{
				const Float value = initialValue ? *(const Float*) initialValue : 0.0f;

				m_curves[0].AddPoint( 0, value, CST_BezierSmooth );
				m_curves[0].AddPoint( 0.99f, value, CST_BezierSmooth );
			}
		}
		break;

		case ECurveType_Vector:
		{
			m_enableAutomaticRotationFromDirectionRecalculation = false;
			m_enableAutomaticTimeByDistanceRecalculation = false;

			m_curves.Resize( 3 );
			for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
			{
				it->SetBaseType( CT_Smooth );
			}

			if ( insertEndPoints )
			{
				Vector value( 0.0f, 0.0f, 0.0f, 0.0f );
				if ( initialValue )
				{
					value = *(const Vector*) initialValue;
				}

				AddControlPoint( 0.0f, value );

				value += Vector(1.0f, 0.0f, 0.0f);
				AddControlPoint( 0.999f, value );
			}
		}
		case ECurveType_EulerAngles:
		{
			m_enableAutomaticRotationFromDirectionRecalculation = false;
			m_enableAutomaticTimeByDistanceRecalculation = false;

			m_curves.Resize( 3 );
			for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
			{
				it->SetBaseType( CT_Smooth );
			}

			if ( insertEndPoints )
			{
				EngineTransform value;
				if ( initialValue )
				{
					if ( curveType == ECurveType_EulerAngles )
					{
						const EulerAngles& angles = *(const EulerAngles*) initialValue;
						value.SetRotation( angles );
					}
				}

				AddControlPoint( 0.0f, value );

				EulerAngles temp = value.GetRotation();
				temp.Yaw += 180.0f;
				value.SetRotation( temp );
				AddControlPoint( 0.999f, value );
			}
		}
		break;

		case ECurveType_EngineTransform:
		{
			m_curves.Resize( 9 );
			for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
			{
				it->SetBaseType( CT_Smooth );
			}

			if ( insertEndPoints )
			{
				EngineTransform value;
				if ( initialValue )
				{
					value = *(const EngineTransform*) initialValue;
				}

				AddControlPoint( 0.0f, value );

				value.SetPosition( value.GetPosition() + Vector(0.0f, 3.0f, 0.0f) );
				AddControlPoint( 0.5f, value );

				value.SetPosition( value.GetPosition() + Vector(3.0f, 0.0f, 0.0f) );
				AddControlPoint( 0.999f, value );
			}
		}
		break;

		default:
			break;
	}

	// Add extra data streams

	for ( Uint32 i = 0; i < numExtraDataStreams; i++ )
	{
		SCurveData data;
		data.SetBaseType( CT_Smooth );

		m_curves.PushBack( data );
	}
}

Uint32 SMultiCurve::Size() const
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	return m_curves[0].Size();
}

Float SMultiCurve::GetControlPointTime( Uint32 index ) const
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	return FromNormalizedTime( m_curves[0].m_curveValues[ index ].time );
}

void SMultiCurve::GetControlPointPosition( Uint32 index, Vector& result ) const
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( HasPosition() );
	
	if ( const SCurveData* curves = GetPositionCurves() )
	{
		result.Set3( curves[0].GetFloatValueAtIndex( index ), curves[1].GetFloatValueAtIndex( index ), curves[2].GetFloatValueAtIndex( index ) );
	}
}

void SMultiCurve::GetControlPointTransform( Uint32 index, EngineTransform& result ) const
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	if ( const SCurveData* curves = GetPositionCurves() )
	{
		const Vector pos( curves[0].GetFloatValueAtIndex( index ), curves[1].GetFloatValueAtIndex( index ), curves[2].GetFloatValueAtIndex( index ) );
		result.SetPosition( pos );
	}

	if ( const SCurveData* curves = GetRotationCurves() )
	{
		const EulerAngles rot( curves[0].GetFloatValueAtIndex( index ), curves[1].GetFloatValueAtIndex( index ), curves[2].GetFloatValueAtIndex( index ) );
		result.SetRotation( rot );
	}

	if ( const SCurveData* curves = GetScaleCurves() )
	{
		const Vector scale( curves[0].GetFloatValueAtIndex( index ), curves[1].GetFloatValueAtIndex( index ), curves[2].GetFloatValueAtIndex( index ) );
		result.SetScale( scale );
	}
}

void SMultiCurve::GetControlPointTangent( Uint32 index, Uint32 tangentIndex, Vector& result )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( HasPosition() );
	ASSERT( m_positionInterpolationMode == ECurveInterpolationMode_Manual );

	ValidateTangents();
	
	result = (tangentIndex ? m_rightTangents : m_leftTangents)[ index ];
}

void SMultiCurve::SetControlPointTangent( Uint32 index, Uint32 tangentIndex, const Vector& value )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( HasPosition() );
	ASSERT( m_positionInterpolationMode == ECurveInterpolationMode_Manual );

	ValidateTangents();

	// Update appropriate tangent vector

	(tangentIndex ? m_rightTangents : m_leftTangents)[ index ] = value;

	// Update the other tangent vector

	Vector& otherTangent = ( ( 1 - tangentIndex ) ? m_rightTangents : m_leftTangents)[ index ];
	switch ( m_positionManualMode )
	{
		case ECurveManualMode_BezierSymmetric:
			otherTangent = -value;
			break;
		case ECurveManualMode_BezierSymmetricDirection:
		{
			// Preserve length
			const Float otherTangentLength = otherTangent.Mag3();
			otherTangent = -value.Normalized3() * otherTangentLength;
		}
		break;
		default:
			break;
	}
}

void SMultiCurve::SetControlPointRotation( Uint32 index, const EulerAngles& value )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	if ( SCurveData* curves = GetRotationCurves() )
	{
		curves[0].SetValueAtIndex( index, value.Roll );
		curves[1].SetValueAtIndex( index, value.Pitch );
		curves[2].SetValueAtIndex( index, value.Yaw );
	}

	PostChange();
}

void SMultiCurve::SetControlPointTransform( Uint32 index, const EngineTransform& value )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	if ( SCurveData* curves = GetPositionCurves() )
	{
		curves[0].SetValueAtIndex( index, value.GetPosition().X );
		curves[1].SetValueAtIndex( index, value.GetPosition().Y );
		curves[2].SetValueAtIndex( index, value.GetPosition().Z );
	}

	if ( SCurveData* curves = GetRotationCurves() )
	{
		curves[0].SetValueAtIndex( index, value.GetRotation().Roll );
		curves[1].SetValueAtIndex( index, value.GetRotation().Pitch );
		curves[2].SetValueAtIndex( index, value.GetRotation().Yaw );
	}

	if ( SCurveData* curves = GetScaleCurves() )
	{
		curves[0].SetValueAtIndex( index, value.GetScale().X );
		curves[1].SetValueAtIndex( index, value.GetScale().Y );
		curves[2].SetValueAtIndex( index, value.GetScale().Z );
	}

	PostChange();
}

Uint32 SMultiCurve::AddControlPoint( Float time, const Vector& value )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	
	EngineTransform transform;
	transform.SetPosition( value );

	return AddControlPoint( time, transform );
}

Uint32 SMultiCurve::AddControlPoint( Float time, const EngineTransform& value )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	time = ToNormalizedTimeAtControlPoint( time );

	Int32 newControlPointIndex = -1;

	if ( SCurveData* curves = GetPositionCurves() )
	{
		newControlPointIndex = curves[0].AddPoint( time, value.GetPosition().X );
		curves[1].AddPoint( time, value.GetPosition().Y );
		curves[2].AddPoint( time, value.GetPosition().Z );
	}

	if ( SCurveData* curves = GetRotationCurves() )
	{
		newControlPointIndex = curves[0].AddPoint( time, value.GetRotation().Roll );
		curves[1].AddPoint( time, value.GetRotation().Pitch );
		curves[2].AddPoint( time, value.GetRotation().Yaw );
	}

	if ( SCurveData* curves = GetScaleCurves() )
	{
		newControlPointIndex = curves[0].AddPoint( time, value.GetScale().X );
		curves[1].AddPoint( time, value.GetScale().Y );
		curves[2].AddPoint( time, value.GetScale().Z );
	}

	Uint32 numExtraDataCurves;
	if ( SCurveData* curves = GetExtraDataCurves( numExtraDataCurves ) )
	{
		for ( Uint32 i = 0; i < numExtraDataCurves; i++ )
		{
			curves[i].AddPoint( time, curves[i].GetFloatValue( time ) );
		}
	}

	PostControlPointAdded( newControlPointIndex );
	PostChange();

	return (Uint32) newControlPointIndex;
}

Uint32 SMultiCurve::FindNearestControlPoint( const Vector& pos ) const
{
	Float nearestDistanceSqr = FLT_MAX;
	Uint32 nearestIndex = 0;
	for ( Uint32 i = 0; i < Size(); i++ )
	{
		Vector controlPointPos;
		GetControlPointPosition( i, controlPointPos );

		const Float distanceSqr = controlPointPos.DistanceSquaredTo( pos );
		if ( distanceSqr < nearestDistanceSqr )
		{
			nearestIndex = i;
			nearestDistanceSqr = distanceSqr;
		}
	}
	return nearestIndex;
}

Uint32 SMultiCurve::FindNearestEdgeIndex( const Vector& pos ) const
{
	Int32 closestEdge = -1;
	Float closestDistance = FLT_MAX;

	const Uint32 numEdges = IsLooping() ? Size() : ( Size() - 1 );

	for ( Uint32 i = 0; i < numEdges; i++ )
	{
		Vector a, b;
		GetControlPointPosition( i, a );
		GetControlPointPosition( ( i + 1 ) == Size() ? 0 : ( i + 1 ), b );

		const Float distanceToEdge = pos.DistanceToEdge( a, b );
		if ( distanceToEdge < closestDistance )
		{
			closestDistance = distanceToEdge;
			closestEdge = i;
		}
	}

	return closestEdge;
}

Uint32 SMultiCurve::AddControlPoint( const Vector& pos )
{
	const Uint32 nearestEdgeIndex = FindNearestEdgeIndex( pos ) + 1;
	return AddControlPointAt( nearestEdgeIndex, &pos );
}

Uint32 SMultiCurve::AddControlPointAt( Uint32 index, const Vector* pos )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	// Figure out the time to add new point at

	Float timeBefore = index > 0 ? GetControlPointTime( index - 1 ) : -0.1f;
	Float timeAfter = index < Size() ? GetControlPointTime( index ) : ( GetTotalTime() + 0.1f );
	if ( timeBefore >= timeAfter )
	{
		if ( m_enableAutomaticTimeByDistanceRecalculation )
		{
			RecalculateTimeByDistance();
		}
		else if ( m_enableAutomaticTimeRecalculation )
		{
			RecalculateTime();
		}

		timeBefore = index > 0 ? GetControlPointTime( index - 1 ) : -0.1f;
		timeAfter = index < Size() ? GetControlPointTime( index ) : ( GetTotalTime() + 0.1f );
		ASSERT( timeBefore < timeAfter );
	}

	const Float newTime = ( timeBefore + timeAfter ) * 0.5f;

	// Add control point

	Int32 newControlPointIndex = -1;
	if ( pos )
	{
		// Move previous control points's time back if collides

		if ( index > 0 && ToNormalizedTimeAtControlPoint( GetControlPointTime( index - 1 ) ) == ToNormalizedTimeAtControlPoint( newTime ) )
		{
			const Float timeBefore2 = index > 1 ? GetControlPointTime( index - 2 ) : 0.0f;
			const Float newTime2 = ( timeBefore2 + timeBefore ) * 0.5f;
			SetControlPointTime( index - 1, newTime2 );
		}

		newControlPointIndex = AddControlPoint( newTime, *pos );
	}
	else
	{
		for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
		{
			newControlPointIndex = it->AddPoint( newTime, it->GetFloatValue( newTime ) );
		}
	}

	PostControlPointAdded( newControlPointIndex );
	PostChange();

	return (Uint32) newControlPointIndex;
}

Uint32 SMultiCurve::AddControlPointAt( Uint32 index, const EngineTransform& transform )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	// Figure out the time to add new point at

	Float timeBefore = index > 0 ? GetControlPointTime( index - 1 ) : -0.1f;
	Float timeAfter = index < Size() ? GetControlPointTime( index ) : ( GetTotalTime() + 0.1f );
	if ( timeBefore >= timeAfter )
	{
		if ( m_enableAutomaticTimeByDistanceRecalculation )
		{
			RecalculateTimeByDistance();
		}
		else if ( m_enableAutomaticTimeRecalculation )
		{
			RecalculateTime();
		}

		timeBefore = index > 0 ? GetControlPointTime( index - 1 ) : -0.1f;
		timeAfter = index < Size() ? GetControlPointTime( index ) : ( GetTotalTime() + 0.1f );
		ASSERT( timeBefore < timeAfter );
	}

	const Float newTime = ( timeBefore + timeAfter ) * 0.5f;

	// Add control point

	const Uint32 newControlPointIndex = AddControlPoint( newTime, transform );

	PostControlPointAdded( newControlPointIndex );
	PostChange();

	return (Uint32) newControlPointIndex;
}

void SMultiCurve::PostControlPointAdded( Uint32 newControlPointIndex )
{
	if ( m_positionInterpolationMode == ECurveInterpolationMode_Manual )
	{
		Vector curveDirection;
		CalculateTangentFromCurveAtControlPoint( newControlPointIndex, curveDirection );
		curveDirection *= m_automaticPositionInterpolationSmoothness;

		m_leftTangents.Insert( newControlPointIndex, -curveDirection );
		m_rightTangents.Insert( newControlPointIndex, curveDirection );
	}

	if ( HasEaseParams() )
	{
		Float easeIn = 1.0f;
		Float easeOut = 1.0f;

		// Maintain smooth ends

		if ( newControlPointIndex == 0 )
		{
			easeOut = 0.0f;
			if ( Size() >= 2 )
			{
				m_easeParams[ newControlPointIndex + 1 ].m_easeIn = 1.0f;
			}
		}
		else if ( newControlPointIndex == Size() - 1 )
		{
			easeIn = 0.0f;
			if ( Size() >= 2 )
			{
				m_easeParams[ newControlPointIndex ].m_easeOut = 1.0f;
			}
		}

		m_easeParams.Insert( newControlPointIndex, SCurveEaseParam( easeIn, easeOut ) );
	}
}

void SMultiCurve::PostControlPointRemoved( Uint32 removedControlPointIndex )
{
	if ( m_positionInterpolationMode == ECurveInterpolationMode_Manual && removedControlPointIndex < m_leftTangents.Size() )
	{
		m_leftTangents.RemoveAt( removedControlPointIndex );
		m_rightTangents.RemoveAt( removedControlPointIndex );
	}

	if ( HasEaseParams() )
	{
		// Maintain smooth ends

		if ( removedControlPointIndex == 0 )
		{
			if ( Size() )
			{
				m_easeParams[ removedControlPointIndex + 1 ].m_easeOut = 0.0f;
			}
		}
		else if ( removedControlPointIndex == Size() )
		{
			if ( Size() )
			{
				m_easeParams[ removedControlPointIndex - 1 ].m_easeIn = 0.0f;
			}
		}

		m_easeParams.RemoveAt( removedControlPointIndex );
	}
}

void SMultiCurve::CalculateTangentFromCurveAtControlPoint( Uint32 index, Vector& tangent ) const
{
	const Uint32 lastControlPointIndex = Size() - 1;

	const Uint32 prevIndex = ( index > 0 ) ? ( index - 1 ) : ( IsLooping() ? lastControlPointIndex : 0 );
	const Uint32 nextIndex = ( index < lastControlPointIndex ) ? ( index + 1 ) : ( IsLooping() ? 0 : lastControlPointIndex );

	Vector p0, p1;
	GetControlPointPosition( prevIndex, p0 );
	GetControlPointPosition( nextIndex, p1 );
	tangent = ( p1 - p0 ).Normalized3();
}

void SMultiCurve::CalculateTangentFromCurveDirection( Float time, Vector& tangent ) const
{
	const Float epsilon = 0.001f;

	Vector p0, p1;
	GetPosition( Max( time - epsilon, 0.f ), p0 );
	GetPosition( Min( time + epsilon, GetTotalTime() ), p1 );
	tangent = ( p1 - p0 ).Normalized3();
}

void SMultiCurve::CalculateAbsoluteTangentFromCurveDirection( Float time, Vector& tangent ) const
{
	Vector localTangent;
	CalculateTangentFromCurveDirection( time, localTangent );

	// Transform to absolute space

	Matrix absoluteMatrix;
	GetAbsoluteMatrix( absoluteMatrix );
	absoluteMatrix.FullInvert();
	absoluteMatrix.Transpose();

	tangent = absoluteMatrix.TransformVector( localTangent ).Normalized3();
}

void SMultiCurve::CalculateAbsoluteTangentFromCurveDirection( const SMultiCurvePosition & curvePosition, Vector& tangent ) const
{
	const Int32 nEdges = IsLooping() ? Size() : Size() - 1;
	Float time = 0.0;
	if ( nEdges == curvePosition.m_edgeIdx )
	{
		time = 1.0f;
	}
	else
	{
		const Float timeA = GetControlPointTime( curvePosition.m_edgeIdx );
		const Float timeB = GetControlPointTime( curvePosition.m_edgeIdx + 1 );
		time = timeA + ( timeB - timeA ) * curvePosition.m_edgeAlpha;
	}

	CalculateAbsoluteTangentFromCurveDirection( time, tangent );
}

// hacky implementation of above function to get rid of above functions unaccuracy
void SMultiCurve::CalculateAbsolute2DTangentFromCurveDirection( const SMultiCurvePosition & curvePosition, Vector2& outTangent, Float minPrecision ) const
{
	const Int32 nEdges = IsLooping() ? Size() : Size() - 1;
	Float time = 0.0;
	if ( nEdges == curvePosition.m_edgeIdx )
	{
		time = 1.0f;
	}
	else
	{
		const Float timeA = GetControlPointTime( curvePosition.m_edgeIdx );
		const Float timeB = GetControlPointTime( curvePosition.m_edgeIdx + 1 );
		time = timeA + ( timeB - timeA ) * curvePosition.m_edgeAlpha;
	}

	// compute transformation matrix
	Matrix absoluteMatrix;
	GetAbsoluteMatrix( absoluteMatrix );
	absoluteMatrix.FullInvert();
	absoluteMatrix.Transpose();

	// compute tangent
	Vector2 tangent;
	
	Float maxEpsilon = 1.f;
	Float minEpsilon = 0.0001;
	Float minPrecisionSq = minPrecision*minPrecision;
	

	Float epsilon = 0.001f;
	do 
	{
		Vector p0, p1;
		GetPosition( Max( time - epsilon, 0.f ), p0 );
		GetPosition( Min( time + epsilon, GetTotalTime() ), p1 );
		absoluteMatrix.TransformVector( p0 );
		absoluteMatrix.TransformVector( p1 );
		tangent = absoluteMatrix.TransformVector( p1 - p0 ).AsVector2();
		Float tangentLenSq = tangent.SquareMag();
		if ( tangentLenSq < (0.002f * 0.002f) )
		{
			// need larger step
			minEpsilon = epsilon;
			if ( ( maxEpsilon - minEpsilon ) < 0.00025f )
			{
				break;
			}
			epsilon = Min( maxEpsilon, epsilon * 2.f );
			continue;
		}
		else if ( tangentLenSq > minPrecisionSq )
		{
			// need smaller step
			maxEpsilon = epsilon;
			if ( ( maxEpsilon - minEpsilon ) < 0.00025f )
			{
				break;
			}
			epsilon = (epsilon + minEpsilon) * 0.5f;
			continue;
		}

		break;

	} while ( true );

	outTangent = tangent.Normalized();
}

void SMultiCurve::DirectionToEulerAngles( const Vector& direction, EulerAngles& angles )
{
	angles.Yaw = direction.Y != 0.0f ? RAD2DEG( -atan2f( direction.X, direction.Y ) ) : 0.0f;
	const Float lenSqr = direction.X * direction.X + direction.Y * direction.Y;
	angles.Pitch = ( lenSqr != 0.0f && direction.Z != 0.0f ) ? RAD2DEG( atan2f( direction.Z, sqrtf( lenSqr ) ) ) : 0.0f;
	angles.Roll = 0.0f;
}

void SMultiCurve::CalculateAnglesFromCurveDirection( Float time, EulerAngles& angles ) const
{
	Vector tangent;
	CalculateTangentFromCurveDirection( time, tangent );
	DirectionToEulerAngles( tangent, angles );
}

void SMultiCurve::RemoveControlPoint( Uint32 index )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );

	for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		it->RemovePointAtIndex( index );
	}

	PostControlPointRemoved( index );
	PostChange();
}

void SMultiCurve::EnableAutomaticTimeByDistanceRecalculation( Bool enable )
{
	m_enableAutomaticTimeByDistanceRecalculation = enable;
	m_enableAutomaticTimeRecalculation = false;
	if ( enable )
	{
		RecalculateTimeByDistance();
	}
}

void SMultiCurve::EnableAutomaticTimeRecalculation( Bool enable )
{
	m_enableAutomaticTimeRecalculation = enable;
	m_enableAutomaticTimeByDistanceRecalculation = false;
	if ( enable )
	{
		RecalculateTime();
	}
}

void SMultiCurve::EnableAutomaticRotationFromDirectionRecalculation( Bool enable )
{
	m_enableAutomaticRotationFromDirectionRecalculation = enable;
	if ( enable )
	{
		RecalculateRotationFromDirection();
	}
}

void SMultiCurve::RecalculateRotationFromDirection()
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( HasRotation() );
	ASSERT( HasPosition() );

	SCurveData* curves = GetRotationCurves();
	ASSERT( curves );

	const Uint32 numControlPoints = Size();
	for ( Uint32 i = 0; i < numControlPoints; i++ )
	{
		Vector direction;
		CalculateTangentFromCurveAtControlPoint( i, direction );

		EulerAngles rotation;
		DirectionToEulerAngles( direction, rotation );

		curves[0].SetValueAtIndex( i, rotation.Roll );
		curves[1].SetValueAtIndex( i, rotation.Pitch );
		curves[2].SetValueAtIndex( i, rotation.Yaw );
	}
}

void SMultiCurve::RecalculateTimeByDistance( const TDynArray< Uint32 >* _indices, Uint32 numApproximationSegmentsBetweenControlPoints )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( IsEditableIn3D() );

	if ( _indices && _indices->Size() < Size() ) // Only recalculate for selected indices
	{
		if ( _indices->Empty() )
		{
			return;
		}

		const TDynArray< Uint32 >& indices = *_indices;

		// Find first not included index

		Uint32 offset = 0;
		for ( Uint32 i = 0; i < indices.Size(); i++ )
		{
			if ( indices[i] != i )
			{
				offset = GetNextIndex( i );
				break;
			}
		}

		// Process contiguous curve parts 

		Uint32 start = 0;
		while ( start < indices.Size() )
		{
			// Determine contiguous index set

			Uint32 end = start;
			while ( end < indices.Size() && GetNextIndex( indices[ ( offset + end ) % indices.Size() ] ) == indices[ ( offset + end + 1 ) % indices.Size() ] )
			{
				end++;
			}

			// Calculate length of the curve part

			const Uint32 firstIndex = GetPrevIndex( indices[ ( offset + start ) % indices.Size() ] );
			Float length = CalculateSegmentLength( firstIndex, numApproximationSegmentsBetweenControlPoints );
			for ( Uint32 i = start; i <= end; i++ )
			{
				const Uint32 index = indices[ ( offset + i ) % indices.Size() ];
				length += CalculateSegmentLength( index, numApproximationSegmentsBetweenControlPoints );
			}

			if ( length == 0.0f )
			{
				return;
			}
			const Float totalLength = length;

			const Uint32 lastIndex = GetNextIndex( indices[ ( offset + end ) % indices.Size() ] );

			// Distribute control point times evenly

			const Float firstTime = GetControlPointTime( firstIndex );
			Float lastTime = GetControlPointTime( lastIndex );
			if ( firstTime > lastTime )
			{
				lastTime += m_totalTime;
			}

			const Float lengthScale = ( ( lastTime - firstTime ) / m_totalTime ) / totalLength;

			length = CalculateSegmentLength( firstIndex, numApproximationSegmentsBetweenControlPoints );
			for ( Uint32 i = start; i <= end; i++ )
			{
				const Uint32 index = indices[ ( offset + i ) % indices.Size() ];

				Float newTime = firstTime + length * lengthScale;
				if ( newTime > m_totalTime )
				{
					newTime -= m_totalTime;
				}
				SetControlPointTime( index, newTime );

				length += CalculateSegmentLength( i, numApproximationSegmentsBetweenControlPoints );
			}

			// Go to next index set

			start = end + 1;
		}
	}
	else // Recalculate for all vertices - simpler
	{
		// Determine total curve length

		const Float totalLength = CalculateLength( numApproximationSegmentsBetweenControlPoints );
		if ( totalLength == 0.0f )
		{
			return;
		}

		const float totalLengthInversed = 1.0f / totalLength;

		// Update times for all control points

		const Uint32 numControlPoints = Size();

		Float length = 0.0f;
		for ( Uint32 i = 0; i < numControlPoints; i++ )
		{
			SetControlPointTime( i, FromNormalizedTimeAtControlPoint( length * totalLengthInversed ) );
			length += CalculateSegmentLength( i, numApproximationSegmentsBetweenControlPoints );
		}
	}
}

void SMultiCurve::RecalculateTime()
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( IsEditableIn3D() );

	const Uint32 numControlPoints = Size();

	Float currentTime = 0.0f;
	const Float timeStep = 1.0f / (Float) ( numControlPoints - ( IsLooping() ? 0 : 1 ) );
	for ( Uint32 i = 0; i < numControlPoints; i++ )
	{
		SetControlPointTime( i, FromNormalizedTimeAtControlPoint( currentTime ) );
		currentTime += timeStep;
	}
}

void SMultiCurve::SetControlPointTime( Uint32 i, Float time )
{
	time = ToNormalizedTimeAtControlPoint( time );

	for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		(*it).SetTime( i, time, false );
	}
}

void SMultiCurve::EnableConsistentNumberOfControlPoints( Bool enable )
{
	m_enableConsistentNumberOfControlPoints = enable;

	if ( m_enableConsistentNumberOfControlPoints )
	{
		// Get all unique times

		TDynArray<Float> times;
		for ( auto it = m_curves.Begin(); it != m_curves.End(); ++it )
		{
			for ( auto it2 = it->m_curveValues.Begin(); it2 != it->m_curveValues.End(); it2++ )
			{
				times.PushBackUnique( it2->time );
			}
		}

		Sort(times.Begin(), times.End());

		// Create keyframes for all times for all curves

		TDynArray<SCurveData> curvesCopy = m_curves;

		for ( Uint32 i = 0; i < m_curves.Size(); i++ )
		{
			SCurveData& srcCurve = curvesCopy[ i ];
			SCurveData& dstCurve = m_curves[ i ];

			dstCurve.Clear();
			for ( auto it = times.Begin(); it != times.End(); ++it )
			{
				dstCurve.AddPoint( *it, srcCurve.GetFloatValue( *it ), CST_BezierSmooth );
			}
		}
	}
}

void SMultiCurve::Translate( const Vector& translation )
{
	ASSERT( m_enableConsistentNumberOfControlPoints );
	ASSERT( HasPosition() );

	SCurveData* curves = GetPositionCurves();

	for ( Uint32 i = 0; i < 3; i++ )
	{
		SCurveData& curve = curves[i];
		const Float offset = (&translation.X)[i];
		for ( Uint32 j = 0; j < curve.Size(); j++ )
		{
			curve.SetValueAtIndex( j, curve.GetFloatValueAtIndex(j) + offset );
		}
	}

	PostChange();
}

void SMultiCurve::PostChange()
{
	if ( m_enableAutomaticTimeByDistanceRecalculation )
	{
		RecalculateTimeByDistance();
	}
	else if ( m_enableAutomaticTimeRecalculation )
	{
		RecalculateTime();
	}

	if ( m_enableAutomaticRotationFromDirectionRecalculation )
	{
		RecalculateRotationFromDirection();
	}
}

void SMultiCurve::SetFromPoints( const TDynArray<Vector>& points, const TDynArray<Float>* times )
{
	ASSERT( !times || times->Size() == points.Size() );

	SetCurveType( ECurveType_Vector, NULL, false );
	EnableAutomaticTimeByDistanceRecalculation( false );

	Float dummyTime = 0.0f;
	const Float dummyTimeStep = m_totalTime / (Float) points.Size();
	for ( Uint32 i = 0; i < points.Size(); i++ )
	{
		AddControlPoint( times ? (*times)[ i ] : dummyTime, points[ i ] );
		dummyTime += dummyTimeStep;
	}

	if ( !times )
	{
		EnableAutomaticTimeByDistanceRecalculation( true );
	}
}

Float SMultiCurve::CalculateSegmentLength( Uint32 startIndex, Uint32 numApproximationSegmentsBetweenControlPoints ) const
{
	ASSERT( 0 <= startIndex && startIndex < Size() );
	ASSERT( numApproximationSegmentsBetweenControlPoints >= 1 );

	const Uint32 endIndex = GetNextIndex( startIndex );

	if ( numApproximationSegmentsBetweenControlPoints == 1 )
	{
		Vector startPos, endPos;
		GetControlPointPosition( startIndex, startPos );
		GetControlPointPosition( endIndex, endPos );
		return startPos.DistanceTo( endPos );
	}
	else
	{
		const Float timeStep = 1.0f / ( ( Float ) numApproximationSegmentsBetweenControlPoints );
		Float localTime = 0.0f;

		Float totalLength = 0.0f;
		Vector prevPos;
		GetControlPointPosition( startIndex, prevPos );
		for ( Uint32 i = 0; i < numApproximationSegmentsBetweenControlPoints; i++ )
		{
			Vector nextPos;
			if ( i + 1 < numApproximationSegmentsBetweenControlPoints )
			{
				localTime += timeStep;
				GetPositionInSegment( startIndex, localTime, nextPos );
			}
			else
			{
				GetControlPointPosition( endIndex, nextPos );
			}

			totalLength += prevPos.DistanceTo( nextPos );

			prevPos = nextPos;
		}
		return totalLength;
	}
}

Float SMultiCurve::CalculateLength( Uint32 numApproximationSegmentsBetweenControlPoints ) const
{
	const Uint32 numSegments = Size() - ( IsLooping() ? 0 : 1 );

	Float totalLength = 0.0f;
	for ( Uint32 i = 0; i < numSegments; i++ )
	{
		totalLength += CalculateSegmentLength( i, numApproximationSegmentsBetweenControlPoints );
	}

	//LOG_ENGINE( TXT("curve total length = %f (num segments: %u)"), totalLength, numApproximationSegmentsBetweenControlPoints );
	return totalLength;
}

void SMultiCurve::EnableEaseParams( Bool enable )
{
	if ( enable && !HasEaseParams() )
	{
		m_easeParams.Resize( Size() );
		for ( auto it = m_easeParams.Begin(); it != m_easeParams.End(); ++it )
		{
			it->Reset();
		}

		if ( !IsLooping() && Size() > 0 )
		{
			// Start and end smoothly if not looping

			m_easeParams[0].m_easeOut = 0.0f;
			m_easeParams.Back().m_easeIn = 0.0f;
		}
	}
	else if ( !enable && HasEaseParams() )
	{
		m_easeParams.Clear();
	}
}

void SMultiCurve::SetParent( CNode* parent )
{
	m_parent = parent;

	UpdateInitialTransformFromParent();
}

void SMultiCurve::UpdateInitialTransformFromParent( Bool force ) const
{
	if ( !m_hasInitialParentTransform || force )
	{
		m_hasInitialParentTransform = true;
		if ( m_parent )
		{
			m_initialParentTransform = m_parent->GetTransform();
		}
		else
		{
			m_initialParentTransform.Identity();
		}
	}
}

void SMultiCurve::GetAbsolutePosition( Float time, Vector& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	EngineTransform absoluteTransform;
	ToAbsoluteTransform( localTransform, absoluteTransform );

	result = absoluteTransform.GetPosition();
}

void SMultiCurve::GetAbsoluteRotation( Float time, EulerAngles& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	EngineTransform absoluteTransform;
	ToAbsoluteTransform( localTransform, absoluteTransform );

	result = absoluteTransform.GetRotation();
}

void SMultiCurve::GetAbsoluteTransform( Float time, EngineTransform& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	ToAbsoluteTransform( localTransform, result );
}

void SMultiCurve::GetAbsoluteControlPointPosition( Uint32 index, Vector& result ) const
{
	EngineTransform localTransform;
	GetControlPointTransform( index, localTransform );

	EngineTransform absoluteTransform;
	ToAbsoluteTransform( localTransform, absoluteTransform );

	result = absoluteTransform.GetPosition();
}

void SMultiCurve::GetAbsoluteControlPointTransform( Uint32 index, EngineTransform& result ) const
{
	EngineTransform localTransform;
	GetControlPointTransform( index, localTransform );

	ToAbsoluteTransform( localTransform, result );
}

Float SMultiCurve::GetAlphaOnEdge( const Vector3& point, Uint32 edgeIdx, Vector& outClosestSpot, Float epsilon ) const
{
	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

	Vector pointLocal;
	ToLocalPosition( point, pointLocal );

	return MathUtils::InterpolationUtils::HermiteClosestPoint( p1, p2, p3, p4, pointLocal, outClosestSpot, epsilon );
}
void SMultiCurve::GetClosestPointOnCurve( const Vector& point, SMultiCurvePosition& outCurvePos, Vector& outClosestSpot, Float epsilon ) const
{
	Vector pointLocal;
	ToLocalPosition( point, pointLocal );
	Int32 edgeIdx = FindNearestEdgeIndex( pointLocal );

	const Int32 nVertices	= Size();
	const Bool closed		= IsLooping() && nVertices > 2;
	const Int32 nEdges		= closed ? nVertices : nVertices - 1;

	Int32  step       = 0;
	Int32  prevStep;
	Vector closestPoint;
	Float edgeAlpha;

	do {

		Vector p1, p2, p3, p4;
		GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

		prevStep = step;
		edgeAlpha = MathUtils::InterpolationUtils::HermiteClosestPoint( p1, p2, p3, p4, pointLocal, closestPoint, epsilon );

		Int32 step = ( edgeAlpha < 0.f ) ? -1
			: ( edgeAlpha > 1.f ) ?  1
			:                        0;

		if ( !closed )
		{
			if ( step > 0 && edgeIdx + step > (Int32)nEdges - 1 )
			{
				edgeIdx   = nEdges - 1;
				edgeAlpha = 1.f;
				break;
			}
			else if ( step < 0 && edgeIdx + step < 0 )
			{
				edgeIdx   = 0;
				edgeAlpha = 0.f;
				break;
			}
		}

		edgeIdx = ( edgeIdx + step ) % nEdges;
		if ( edgeIdx < 0 )
			edgeIdx += nEdges;

	} while ( step != 0 && step != -prevStep ); // step != -prevStep - avoid returning to the edge that we previously came from

	// assign result
	outCurvePos.m_edgeIdx = edgeIdx;
	outCurvePos.m_edgeAlpha = edgeAlpha;

	ToAbsolutePosition( closestPoint, outClosestSpot );
}

void SMultiCurve::GetPointOnCurveInDistance( SMultiCurvePosition& curvePosition, Float distance, Vector& outComputedSpot, Bool &isEndOfPath ) const
{
	Uint32 nVertices		= Size();
	Bool closed				= IsLooping() && nVertices > 2;
	Uint32 nEdges			= closed ? nVertices : nVertices - 1;
	isEndOfPath				= false;

	Int32 edgeIdx = curvePosition.m_edgeIdx;
	Float edgeAlpha = curvePosition.m_edgeAlpha;

	ASSERT( edgeIdx >= 0 && edgeIdx < (Int32)nEdges );
	ASSERT( edgeAlpha >= 0.f && edgeAlpha <= 1.f );
	ASSERT( distance != 0.f );

	Vector closestPoint;

	do {
		Vector p2, p3;
		GetControlPointPosition( edgeIdx, p2);
		GetControlPointPosition( (edgeIdx + 1) % nVertices, p3 );

		Float alpha = distance > 0.f ? 1.f - edgeAlpha : edgeAlpha;
		Float edgeLenght = ( p3 - p2 ).Mag3();
		Float edgeLengthLeft = edgeLenght * alpha;

		if ( edgeLengthLeft >= MAbs( distance ) )
		{
			Vector p1, p4;

			GetControlPointPosition(
				edgeIdx > 0 
				? ( edgeIdx - 1 )
				: closed ? ( nVertices-1 ) : ( edgeIdx ),
				p1 );
			GetControlPointPosition(
				closed
				? ( (edgeIdx + 2) % nVertices )
				: Min<Int32>( nVertices-1, edgeIdx+2 ),
				p4 );

			edgeAlpha = edgeAlpha + distance / edgeLengthLeft * alpha;
			closestPoint = MathUtils::InterpolationUtils::HermiteInterpolate( p1, p2, p3, p4, edgeAlpha );
			break;
		}

		if ( distance > 0 )
			distance -= edgeLengthLeft;
		else
			distance += edgeLengthLeft;

		if ( distance > 0.f )
		{
			if ( !closed && edgeIdx + 1 > (Int32)nEdges - 1 )
			{
				edgeIdx   = nEdges - 1;
				edgeAlpha = 1.f;
				closestPoint = p3;
				isEndOfPath  = true;
				break;
			}
			edgeAlpha = 0.f;
			if ( ++edgeIdx == (Int32)nEdges )
				edgeIdx = 0;
		}
		else
			if ( distance < 0.f )
			{
				if ( !closed && edgeIdx - 1 < 0 )
				{
					edgeIdx   = 0;
					edgeAlpha = 0.f;
					closestPoint = p2;
					isEndOfPath  = true;
					break;
				}
				edgeAlpha = 1.f;
				if ( --edgeIdx < 0 )
					edgeIdx += nEdges;
			}

	} while ( true );

	curvePosition.m_edgeIdx = edgeIdx;
	curvePosition.m_edgeAlpha = edgeAlpha;

	ToAbsolutePosition( closestPoint, outComputedSpot );
}

void SMultiCurve::GetCurvePoint( const SMultiCurvePosition& curvePosition, Vector& outComputedSpot ) const
{
	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, curvePosition.m_edgeIdx );

	ToAbsolutePosition( MathUtils::InterpolationUtils::HermiteInterpolate( p1, p2, p3, p4, curvePosition.m_edgeAlpha ), outComputedSpot );
}

void SMultiCurve::CorrectCurvePoint( const Vector& point, SMultiCurvePosition& inOutCurvePosition, Vector& outClosestSpot, Float epsilon ) const
{
	struct Local
	{
		static Float DistanceToEdge( const Vector& pointLocal, Int32 edgeIdx, const SMultiCurve& curve )
		{
			Vector a, b;
			curve.GetControlPointPosition( edgeIdx, a );
			curve.GetControlPointPosition( (edgeIdx + 1) % curve.Size(), b );

			return pointLocal.DistanceToEdge( a, b );
		}
	};

	Vector pointLocal;
	ToLocalPosition( point, pointLocal );

	const Int32 nVertices	= Size();
	const Bool closed		= IsLooping() && nVertices > 2;
	const Int32 nEdges		= closed ? nVertices : nVertices - 1;

	Int32 edgeIdx			= inOutCurvePosition.m_edgeIdx;
	Float edgeAlpha			= inOutCurvePosition.m_edgeAlpha;

	Float currEdgeDist	= Local::DistanceToEdge( pointLocal, edgeIdx, *this );
	Float bestDist		= currEdgeDist;
	Bool movedForward	= false;

	// try to move forward
	while ( edgeIdx + 1 < nEdges )
	{
		Int32 tryEdge	= edgeIdx + 1;
		Float tryDist	= Local::DistanceToEdge( pointLocal, tryEdge, *this );
		if ( tryDist	>= bestDist )
		{
			break;
		}
		edgeIdx			= tryEdge;
		bestDist		= tryDist;
		movedForward	= true;
	}
	if ( !movedForward )
	{
		while ( edgeIdx > 0 )
		{
			Int32 tryEdge	= edgeIdx-1;
			Float tryDist	= Local::DistanceToEdge( pointLocal, tryEdge, *this );
			if ( tryDist	>= bestDist )
			{
				break;
			}
			edgeIdx		= tryEdge;
			bestDist	= tryDist;
		}
	}

	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

	Vector closestPoint;
	edgeAlpha = MathUtils::InterpolationUtils::HermiteClosestPoint( p1, p2, p3, p4, pointLocal, closestPoint, epsilon );

	inOutCurvePosition.m_edgeAlpha = edgeAlpha;
	inOutCurvePosition.m_edgeIdx = edgeIdx;

	ToAbsolutePosition( closestPoint, outClosestSpot );
}

void SMultiCurve::GetAbsoluteTransform( EngineTransform& result ) const
{
	RED_WARNING_ONCE( GIsEditor && !GIsEditorGame, "This should only be used while editing the curve!" );

	// Update initial parent transform when moving things around in the editor

	UpdateInitialTransformFromParent( true );

	if ( !m_parent )
	{
		return;
	}

	// Get absolute parent transform

	EngineTransform absoluteParentTransform;

#ifndef NO_EDITOR
	if ( m_parent->GetClass()->GetName() == CName( TXT("CStoryScenePreviewPlayer") ) ) // HACK: Curves attached to CStoryScenePlayer use 'fake' attachment
																				// (without this curves would not display correctly in dialog editor's
																				// preview mode with world loaded)
	{
		extern EngineTransform CStoryScenePlayer_GetAbsoluteTransform( CNode* storyScenePlayer );
		absoluteParentTransform = CStoryScenePlayer_GetAbsoluteTransform( m_parent );
	}
	else
#endif
	{
		absoluteParentTransform = EngineTransform( m_parent->GetLocalToWorld() );
	}

	// Set up transformation components as necessary

	switch ( m_translationRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
	case ECurveRelativeMode_CurrentTransform:
		result.SetPosition( absoluteParentTransform.GetPosition() );
		break;
	}

	switch ( m_rotationRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
	case ECurveRelativeMode_CurrentTransform:
		result.SetRotation( absoluteParentTransform.GetRotation() );
		break;
	}

	switch ( m_scaleRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
	case ECurveRelativeMode_CurrentTransform:
		result.SetScale( absoluteParentTransform.GetScale() );
		break;
	}
}

void SMultiCurve::GetAbsoluteMatrix( Matrix& result ) const
{
	EngineTransform transform;
	GetAbsoluteTransform( transform );

	transform.CalcLocalToWorld( result );
}

void SMultiCurve::ToAbsoluteMatrix( const EngineTransform& localTransform, Matrix& result ) const
{
	Matrix rootMatrix;
	GetAbsoluteMatrix( rootMatrix );

	Matrix localMatrix;
	localTransform.CalcLocalToWorld( localMatrix );

	result = Matrix::Mul( rootMatrix, localMatrix );
}

void SMultiCurve::ToAbsoluteTransform( const EngineTransform& localTransform, EngineTransform& result ) const
{
	Matrix absoluteMatrix;
	ToAbsoluteMatrix( localTransform, absoluteMatrix );
	result.Init( absoluteMatrix );
}

void SMultiCurve::ToAbsolutePosition( const Vector& localPosition, Vector& result ) const
{
	Matrix absoluteMatrix;
	GetAbsoluteMatrix( absoluteMatrix );
	result = absoluteMatrix.TransformPoint( localPosition );
}

void SMultiCurve::GetRootTransform( EngineTransform& result ) const
{
	if ( !m_parent )
	{
		return;
	}

	// Get (local) parent transform

	const EngineTransform currentParentTransform = m_parent->GetTransform();

	// Set up transformation components as necessary

	switch ( m_translationRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
		result.SetPosition( m_initialParentTransform.GetPosition() );
		break;
	case ECurveRelativeMode_CurrentTransform:
		result.SetPosition( currentParentTransform.GetPosition() );
		break;
	}

	switch ( m_rotationRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
		result.SetRotation( m_initialParentTransform.GetRotation() );
		break;
	case ECurveRelativeMode_CurrentTransform:
		result.SetRotation( currentParentTransform.GetRotation() );
		break;
	}

	switch ( m_scaleRelativeMode )
	{
	case ECurveRelativeMode_InitialTransform:
		result.SetScale( m_initialParentTransform.GetScale() );
		break;
	case ECurveRelativeMode_CurrentTransform:
		result.SetScale( currentParentTransform.GetScale() );
		break;
	}
}

void SMultiCurve::ToRootTransform( const EngineTransform& localTransform, EngineTransform& result ) const
{
	EngineTransform rootTransform;
	GetRootTransform( rootTransform );

	result.Mul( rootTransform, localTransform );
}

void SMultiCurve::ToRootPosition( const Vector& localPosition, Vector& result ) const
{
	EngineTransform rootTransform;
	GetRootTransform( rootTransform );

	result = rootTransform.TransformPoint( localPosition );
}

void SMultiCurve::ToLocalMatrix( const EngineTransform& inAbsoluteTransform, Matrix& result ) const
{
	Matrix absoluteMatrix;
	GetAbsoluteMatrix( absoluteMatrix );
	const Matrix invAbsoluteMatrix = absoluteMatrix.FullInvert();

	Matrix inAbsoluteMatrix;
	inAbsoluteTransform.CalcLocalToWorld( inAbsoluteMatrix );

	result = Matrix::Mul( invAbsoluteMatrix, inAbsoluteMatrix );
}

void SMultiCurve::ToLocalTransform( const EngineTransform& absoluteTransform, EngineTransform& result ) const
{
	Matrix localMatrix;
	ToLocalMatrix( absoluteTransform, localMatrix );
	result.Init( localMatrix );
}

void SMultiCurve::ToLocalPosition( const Vector& absolutePosition, Vector& localPosition ) const
{
	Matrix absoluteMatrix;
	GetAbsoluteMatrix( absoluteMatrix );
	const Matrix invAbsoluteMatrix = absoluteMatrix.FullInvert();

	localPosition = invAbsoluteMatrix.TransformPoint( absolutePosition );
}

void SMultiCurve::GetRootPosition( Float time, Vector& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	EngineTransform rootTransform;
	ToRootTransform( localTransform, rootTransform );

	result = rootTransform.GetPosition();
}

void SMultiCurve::GetRootRotation( Float time, EulerAngles& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	EngineTransform rootTransform;
	ToRootTransform( localTransform, rootTransform );

	result = rootTransform.GetRotation();
}

void SMultiCurve::GetRootTransform( Float time, EngineTransform& result ) const
{
	EngineTransform localTransform;
	GetTransform( time, localTransform );

	ToRootTransform( localTransform, result );
}

void SMultiCurve::GetRootControlPointTransform( Uint32 index, EngineTransform& result ) const
{
	EngineTransform localTransform;
	GetControlPointTransform( index, localTransform );

	ToRootTransform( localTransform, result );
}

void SMultiCurve::GetRootControlPointPosition( Uint32 index, Vector& result ) const
{
	EngineTransform localTransform;
	GetControlPointTransform( index, localTransform );

	EngineTransform rootTransform;
	ToRootTransform( localTransform, rootTransform );

	result = rootTransform.GetPosition();
}

#ifndef NO_EDITOR

TDynArray< ICurveChangeListener* > SMultiCurve::m_changeListeners;

void SMultiCurve::RegisterChangeListener( ICurveChangeListener* listener )
{
	m_changeListeners.PushBackUnique( listener );
}

void SMultiCurve::UnregisterChangeListener( ICurveChangeListener* listener )
{
	m_changeListeners.Remove( listener );
}

#ifndef NO_EDITOR
void SMultiCurve::PostCurveChanged( SMultiCurve* curve )
{
	for ( auto it = m_changeListeners.Begin(); it != m_changeListeners.End(); ++it )
	{
		(*it)->OnCurveChanged( curve );
	}
}
#endif

#endif

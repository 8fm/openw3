/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "classBuilder.h"
#include "math.h"
#include "curveBase.h"

struct SSimpleCurvePoint
{
	DECLARE_RTTI_STRUCT( SSimpleCurvePoint );

public:
	RED_INLINE static Float GetValueScalar( const Vector &value )	
	{ 
		return value.W; 
	}

	RED_INLINE static Color GetValueColor( const Vector &value )
	{ 
		return Color ( 
			(Uint8) Clamp( value.X, 0.f, 255.f ),
			(Uint8) Clamp( value.Y, 0.f, 255.f ),
			(Uint8) Clamp( value.Z, 0.f, 255.f ) );
	}

	RED_INLINE static Vector GetValueColorScaled( const Vector &value, bool normalizedColor )
	{ 
		const Float scale = (normalizedColor ? 1.f/255 : 1.f) * Max(0.f, value.W);
		return Vector ( 
			scale * Clamp( value.X, 0.f, 255.f ), 
			scale * Clamp( value.Y, 0.f, 255.f ), 
			scale * Clamp( value.Z, 0.f, 255.f ),
			1.f );
	}

	RED_INLINE static Vector GetValueColorScaledGammaToLinear( const Vector &value, bool normalizedColor )
	{ 
		const Float normalization = (normalizedColor ? 1.f/255 : 1.f);
		const Float scale = Max(0.f, value.W);
		return Vector ( 
			scale * powf( normalization * Clamp( value.X, 0.f, 255.f ), 2.2f), 
			scale * powf( normalization * Clamp( value.Y, 0.f, 255.f ), 2.2f ), 
			scale * powf( normalization * Clamp( value.Z, 0.f, 255.f ), 2.2f ),
			1.f );
	}

	RED_INLINE static Vector GetValueColorGammaToLinear( const Vector &value, bool normalizedColor )
	{ 
		const Float normalization = (normalizedColor ? 1.f/255 : 1.f);
		return Vector ( 
			powf( normalization * Clamp( value.X, 0.f, 255.f ), 2.2f), 
			powf( normalization * Clamp( value.Y, 0.f, 255.f ), 2.2f ), 
			powf( normalization * Clamp( value.Z, 0.f, 255.f ), 2.2f ),
			1.f );
	}

	RED_INLINE static Vector GetValueColorNotScaled( const Vector &value, bool normalizedColor )
	{ 
		const Float normalization = (normalizedColor ? 1.f/255 : 1.f);
		return Vector ( 
			normalization * Clamp( value.X, 0.f, 255.f ), 
			normalization * Clamp( value.Y, 0.f, 255.f ), 
			normalization * Clamp( value.Z, 0.f, 255.f ),
			1.f );
	}

	RED_INLINE static Vector BuildValue( const Color &colorRGB, Float scalar )
	{
		return Vector ( (Float)colorRGB.R, (Float)colorRGB.G, (Float)colorRGB.B, scalar );
	}

	RED_INLINE static Vector BuildValueScalar( Float scalar )
	{
		return BuildValue( Color::BLACK, scalar );
	}

public:
	RED_INLINE SSimpleCurvePoint ()
		:	m_value (0, 0, 0, 0)
		, m_time (0)
		
	{}

	RED_INLINE SSimpleCurvePoint ( Float newTime, const Vector &newValue )
		:  m_value (newValue),
		m_time (newTime)
	{}
	
	RED_INLINE Bool operator==( const SSimpleCurvePoint &other ) const
	{
		return m_time == other.m_time && m_value == other.m_value;
	}

	RED_INLINE Bool operator!=( const SSimpleCurvePoint &other ) const
	{
		return !operator==(other);
	}

	RED_INLINE SSimpleCurvePoint& Set( Float time, const Color &colorRGB, Float scalar )
	{
		m_time = time;
		m_value = BuildValue(colorRGB, scalar);
		return *this;
	}

	RED_INLINE SSimpleCurvePoint& SetValue( const Color &colorRGB, Float scalar )
	{
		m_value = BuildValue(colorRGB, scalar);
		return *this;
	}

	RED_INLINE SSimpleCurvePoint& SetValueScalar( Float scalar )
	{
		m_value = BuildValue(Color::BLACK, scalar);
		return *this;
	}

	RED_INLINE Float GetTime() const
	{
		return m_time;
	}

	RED_INLINE Float GetScalar() const 
	{ 
		return GetValueScalar( m_value );		
	}

	RED_INLINE Float GetScalarClampMin( Float valueMin ) const 
	{ 
		return Max( valueMin, GetValueScalar( m_value ) );
	}

	RED_INLINE Float GetScalarClampMax( Float valueMax ) const 
	{ 
		return Min( valueMax, GetValueScalar( m_value ) );
	}

	RED_INLINE Float GetScalarClamp( Float valueMin, Float valueMax ) const 
	{ 
		return Clamp( GetValueScalar( m_value ), valueMin, valueMax );
	}

	RED_INLINE Color GetColor() const 
	{ 
		return GetValueColor( m_value );		
	}

	RED_INLINE Vector	GetColorScaled( bool normalizedColor ) const 
	{ 
		return GetValueColorScaled( m_value, normalizedColor );	
	}

	RED_INLINE Vector	GetColorScaledGammaToLinear( bool normalizedColor ) const 
	{ 
		return GetValueColorScaledGammaToLinear( m_value, normalizedColor );	
	}

	RED_INLINE Vector	GetColorGammaToLinear( bool normalizedColor ) const 
	{ 
		return GetValueColorGammaToLinear( m_value, normalizedColor );	
	}

	RED_INLINE Vector	GetColorNotScaled( bool normalizedColor ) const 
	{ 
		return GetValueColorNotScaled( m_value, normalizedColor );	
	}

	RED_INLINE Bool IsColorScaledBlack() const
	{
		Vector scaledColor = GetColorScaled( false );
		return 0.f == Vector::Dot3( scaledColor, Vector::ONES );
	}

	RED_INLINE Bool IsColorScaledWhite() const
	{
		Vector scaledColor = GetColorScaled( false );
		return 255.f == scaledColor.X && 255.f == scaledColor.Y && 255.f == scaledColor.Z;
	}

	Vector	m_value;
	Float	m_time;
};

BEGIN_CLASS_RTTI( SSimpleCurvePoint );
	PROPERTY( m_value );	
	PROPERTY( m_time );
END_CLASS_RTTI();


struct SSimpleCurve : public SCurveBase
{
	DECLARE_RTTI_STRUCT( SSimpleCurve );

public:
	// --------------------------------
	static Bool		s_graphTimeDisplayEnable;
	static Float	s_graphTimeDisplayValue;

	static Bool		s_graphGlobalSelectionEnable;
	static Float	s_graphMinTimeSelection;
	static Float	s_graphMaxTimeSelection;
	// --------------------------------
	
public:
	SSimpleCurve ();
	SSimpleCurve ( ESimpleCurveType type, Float scalarEditScale = 1.f, Float scalarEditOrigin = 1.f );
	SSimpleCurve ( ESimpleCurveType type, const SSimpleCurvePoint &controlPoint, Float scalarEditScale = 1.f, Float scalarEditOrigin = 1.f );
	
	SSimpleCurve &operator=( const SSimpleCurve& );

	/// Resets with given type
	SSimpleCurve& Reset( ESimpleCurveType type, Float scalarEditScale, Float scalarEditOrigin );

	/// Calculates curve with interpolated value for given time. This curve's SimpleCurveType is changed to either
	/// the same as the sources (if both have the same type), or a general Vector if they have different types.
	/// Otherwise, this is the same as SetInterpolatedValue() from SCurveBase.
	void ImportDayPointValue( const SSimpleCurve &srcCurve, const SSimpleCurve &destCurve, Float lerpFactor, Float time, ECurvePointInterpolateMode interpolationMode = CPI_LINEAR );


	/// Returns cached point
	/** This function is meant to be used when operation on merged simple curves, which have only one control point */
	RED_INLINE SSimpleCurvePoint GetCachedPoint() const
	{
		if ( IsEmpty() )
			return SSimpleCurvePoint( 0.f, Vector ( 0.f, 0.f, 0.f, 0.f ) );
		else
			return SSimpleCurvePoint( m_data.m_curveValues[0].time, GetValueAtIndex( 0 ) );
	}
};

BEGIN_CLASS_RTTI( SSimpleCurve );
	// Manually specify name to match existing curves.
	PROPERTY_NAME( m_simpleCurveType, TXT("CurveType") );

	PROPERTY( m_ScalarEditScale );
	PROPERTY( m_ScalarEditOrigin );

	// Serialize the CurveData. This is not done by CurveData itself, not by CurveBase, so that neither needs to be RTTI.
	PROPERTY_NAME( m_data.m_curveValues, TXT( "dataCurveValues" ) )
	PROPERTY_NAME( m_data.m_baseType, TXT("dataBaseType") );
END_CLASS_RTTI();

// Custom handler for unknown properties. This lets us translate from old-style TDynArray<SSimpleCurvePoint> data
// to the SCurveData.
template<> Bool TTypedClass<SSimpleCurve, (EMemoryClass)SSimpleCurve::MemoryClass, SSimpleCurve::MemoryPool>::OnReadUnknownProperty( void* object, const CName& propName, const CVariant& propValue ) const;

namespace RedPropertyBuilder
{
	RED_INLINE CProperty* CreateRangedProperty
		(
		CClass* parentClass,
		size_t offset,
		const CName& name,
		const CName& typeName,
		const String& hint,
		Uint32 flags,
		SSimpleCurve* tag,
		Float minValue,
		Float maxValue,
		const String& customEditor,
		Bool customEditorArray
		)
	{
		return CreateRangedProperty( parentClass, offset, name, typeName, hint, flags, static_cast<SCurveBase*>( tag ), minValue, maxValue, customEditor, customEditorArray );
	}
}

RED_INLINE SSimpleCurve & SSimpleCurve::operator=( const SSimpleCurve& s )
{
	if( &s != this )
	{
		m_ScalarEditScale = s.m_ScalarEditScale;
		m_ScalarEditOrigin = s.m_ScalarEditOrigin;
		m_simpleCurveType = s.m_simpleCurveType;

		SCurveBase::operator=( s );
	}
	return *this;
}

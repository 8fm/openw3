/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "enumBuilder.h"
#include "math.h"
#include "curveData.h"

class IXMLFile;

// TODO: Get rid of this, make type templated, and might be good?
enum ESimpleCurveType
{
	SCT_Float,
	SCT_Vector,
	SCT_Color,
	SCT_ColorScaled,
};
BEGIN_ENUM_RTTI( ESimpleCurveType );
	ENUM_OPTION( SCT_Float );
	ENUM_OPTION( SCT_Vector );
	ENUM_OPTION( SCT_Color );
	ENUM_OPTION( SCT_ColorScaled );
END_ENUM_RTTI();




enum ECurvePointInterpolateMode
{
	CPI_LINEAR,
	CPI_X2,
	CPI_X3,
	CPI_X2_INV,
	CPI_X3_INV,
	CPI_X2_HISTERESIS,
	CPI_X3_HISTERESIS,
	CPI_X2_INV_HISTERESIS,
	CPI_X3_INV_HISTERESIS,
};

BEGIN_ENUM_RTTI( ECurvePointInterpolateMode );
ENUM_OPTION( CPI_LINEAR );
ENUM_OPTION( CPI_X2 );
ENUM_OPTION( CPI_X3 );
ENUM_OPTION( CPI_X2_INV );
ENUM_OPTION( CPI_X3_INV );
ENUM_OPTION( CPI_X2_HISTERESIS );
ENUM_OPTION( CPI_X3_HISTERESIS );
ENUM_OPTION( CPI_X2_INV_HISTERESIS );
ENUM_OPTION( CPI_X3_INV_HISTERESIS );
END_ENUM_RTTI();


// Contains information about a control point's tangents when working with a SegmentedFloat curve. Not used internally,
// but allows outside users to easily save and restore tangent values.
struct SCurveTangents
{
	Vector m_tangents;
	Uint32 m_segmentTypes[2];

	SCurveTangents()
		: m_tangents( -0.1f, 0, 0.1f, 0 )
	{
		m_segmentTypes[0] = CST_Interpolate;
		m_segmentTypes[1] = CST_Interpolate;
	}
	SCurveTangents( const Vector& tans, Uint32 seg0, Uint32 seg1 )
		: m_tangents( tans )
	{
		m_segmentTypes[0] = seg0;
		m_segmentTypes[1] = seg1;
	}
};


// Provides a common base for curves. Supports a few types of curves, defined by the ECurveBaseType enumeration.
// The curve can be looping, or not. A looping curve's parameter value is always in the range [0, 1), any values
// outside this range are wrapped around. A non-looping curve can have arbitrary range of parameter values.
//
// As SCurveBase is not registered with the RTTI, it is up to subclasses to handle serialization of all data. This
// allows existing data using SSimpleCurve and CCurve to still work, since those two classes require different
// serialization formats.
struct SCurveBase
{
protected:
	SCurveData m_data;

	Float m_ScalarEditScale;
	Float m_ScalarEditOrigin;
	ESimpleCurveType m_simpleCurveType;

#ifndef NO_EDITOR
	Bool					m_useDayTime;	//!< HACK-ish flag: "Day Time" curves should synchronize and show time range.
											//!< Others would be better with range 0-1. Default is true.
public:
	RED_INLINE void SetUseDayTime( Bool useDayTime ) { m_useDayTime = useDayTime; }

	RED_INLINE Bool UseDayTime() const { return m_useDayTime; }
#else
public:
	// Empty stub, has no effect for non-editor builds.
	RED_INLINE void SetUseDayTime( Bool ) {}
#endif

public:
	SCurveBase( ECurveValueType valueType = CVT_Float, Bool loop = false );
	virtual ~SCurveBase();

	SCurveBase& Clear();
	RED_INLINE Uint32	Size() const { return m_data.Size(); };

		/// Returns type of this curve
	RED_INLINE ESimpleCurveType GetSimpleCurveType() const { return m_simpleCurveType; }


	RED_INLINE void SetScalarEditScale( Float newScale ) { m_ScalarEditScale = Max( 0.001f, newScale ); }
	RED_INLINE void SetScalarEditOrigin( Float newOrigin ) { m_ScalarEditOrigin = newOrigin; }
	RED_INLINE Float GetScalarEditScale() const { return m_ScalarEditScale; }
	RED_INLINE Float GetScalarEditOrigin() const { return m_ScalarEditOrigin; }
	Bool SerializeXML( IXMLFile& file, Bool merge = false );
	/// Conversion from different curve's type
	Bool CanImportFromType( ESimpleCurveType srcType ) const;
	Bool ConvertValueFromType( Float time, Vector &value, ESimpleCurveType srcType ) const;


	RED_INLINE SCurveData& GetCurveData() { return m_data; }
	RED_INLINE const SCurveData& GetCurveData() const { return m_data; }

	RED_INLINE ECurveValueType GetValueType() const { return m_data.GetValueType(); }

	// A looped curve always has time values in [0,1). Any times outside this range are wrapped around.
	RED_INLINE void	SetLoop( Bool loop ) { m_data.m_loop = loop; }
	RED_INLINE Bool	IsLoop() const { return m_data.m_loop; }
	RED_INLINE Float	AdaptTime( Float time ) const { return ( IsLoop() ? time - floorf( time ) : time ); }

	// Returns true if the curve is using tangent information (i.e. is segmented)
	RED_INLINE Bool	UsesTangents() const { return m_data.GetBaseType() == CT_Segmented; }

	// Change the curve type.
	RED_INLINE void			SetCurveType( ECurveBaseType type ) { m_data.SetBaseType( type ); }
	RED_INLINE ECurveBaseType	GetCurveType() const { return m_data.GetBaseType(); }
	

	// Add new control point (replaces existing with exact time point if allowed - otherwise always fails). If this is
	// used with a Float curve, only the scalar field (W channel) will be used.
	// It is safe to use the Vector form with a Float curve, or the Float form with a Vector curve. SCurveBase will do
	// an internal translation.
	Int32 AddPoint( Float newTime, const Vector &newValue, bool allowReplacement = true );

	// Add a float-valued control point. This should be used for Float curves, but if used on a Vector curve will add
	// a control point with the Scalar value (W channel) set, and the rest set to 0.
	Int32 AddPoint( Float newTime, Float newValue, const ECurveSegmentType& segType = CST_Interpolate, Bool allowReplace = false );

	// Set the value of a given control point. Using the Vector form with a Float curve will discard all but the
	// scalar field (W channel).
	void SetValue( Uint32 index, const Vector &newValue );
	void SetValue( Uint32 index, Float newValue );

	// Remove the control point at the given time parameter. Returns the index of the removed point. If no control point matches the
	// time exactly, nothing is removed and -1 is returned.
	Int32 RemovePointAtTime( Float time );

	// Remove the given control point. Assumes that index < Size().
	void RemovePointAtIndex( Uint32 index );
	RED_INLINE void RemovePointAtIndex( Int32 index )
	{
		ASSERT( index >= 0 );
		return RemovePointAtIndex( (Uint32)index );
	}

	// Changes the time value for a control point, without changing its value (or tangents). Returns the point's new index.
	Uint32 SetPointTime( Uint32 index, Float time );

	// Set the type of tangent used by a given control point. This is ignored for non-tangent curves (Vector).
	// tangentIndex is 0 for the left tangent, 1 for the right (left goes to previous control point, right goes to next).
	// Depending on what type of tangent is set, this might modify the control point's other tangent as well.
	void SetTangentType( const Uint32& index, const Int32& tangentIndex, ECurveSegmentType type );
	ECurveSegmentType GetTangentType( const Uint32& index, const Int32& tangentIndex ) const;

	// Set the tangent value for a control point. This is ignored for non-tangent curves. Depending on the type of tangent,
	// the control point's other tangent value may be modified as well.
	void SetTangentValue( const Uint32& index, const Int32& tangentIndex, const Vector& tangentValue );
	Vector GetTangentValue( const Uint32& index, const Int32& tangentIndex ) const;

	void SetTangent( Uint32 index, const SCurveTangents& tangent );
	SCurveTangents GetTangent( Uint32 index ) const;

	Vector GetValueAtIndex( Uint32 index ) const;
	Float GetFloatValueAtIndex( Uint32 index ) const;

	RED_INLINE Uint32	GetNumPoints() const { return m_data.Size(); }
	RED_INLINE Bool	IsEmpty() const { return m_data.Size() == 0; }

	// Remap the control points' time values, from one range to another. Does not do any adjustments
	// for looping curves, so this should probably just be used for non-looping ones.
	void	RemapControlPoints( Float oldTimeMin, Float oldTimeMax, Float newTimeMin, Float newTimeMax );

	Vector	GetValue( Float time ) const;
	Float	GetFloatValue( Float time ) const;

	// Returns -1 if there is no control point at the given time.
	Int32		GetIndex( Float time ) const;

	Float	GetDuration() const;

	// Finds the minimum and maximum control point values. For Vectors, this is a per-element min/max operation.
	Bool GetValuesMinMax( Vector& minValue, Vector& maxValue ) const;
	Bool GetFloatValuesMinMax( Float& minValue, Float& maxValue ) const;

	// Finds the minimum and maximum control point times.
	Bool GetTimesMinMax( Float& minValue, Float& maxValue ) const;


	// Calculates curve with interpolated value for given time. Any existing curve data is replaced by a single
	// control point. If one curve is Float and the other is Vector, the result will be a Vector curve with
	// vector value (XYZ) equal to the Vector input, and scalar (W) interpolated between the two.
	void SetInterpolatedValue( const SCurveBase &srcCurve, const SCurveBase &destCurve, Float lerpFactor, Float time, ECurvePointInterpolateMode interpolationMode = CPI_LINEAR );


	void GetApproximationSamples( TDynArray< Vector >& samples, Uint32 numSamples ) const;
	void GetApproximationSamples( Vector* samples, Uint32 numSamples ) const;

	void GetApproximationSamples( TDynArray< Float >& samples, Uint32 numSamples ) const;
	void GetApproximationSamples( Float* samples, Uint32 numSamples ) const;


	RED_INLINE SCurveBase& operator=( const SCurveBase& other )
	{
		if ( this != &other )
		{
			m_data = other.m_data;
		}
		return *this;
	}
};

/// Curve range property - used as a specialization for the CRangeProperty that can handle curves
class CCurveRangedProperty : public CProperty
{
protected:
	Float	m_rangeMin;		//!< Minimal value of property's values
	Float	m_rangeMax;		//!< Maximum value of property's values

public:
	CCurveRangedProperty( IRTTIType *type, 
		CClass* owner, 			   
		Uint32 offset, 			   
		const CName& name, 
		const String& hint, 
		Uint32 flags,
		SCurveBase* /* tag */, 
		Float rangeMin = -FLT_MAX,
		Float rangeMax = FLT_MAX, 
		const String &editorType = String::EMPTY,
		Bool arrayCustomEditor = false
		)
		: CProperty( type, owner, offset, name, hint, flags, editorType, arrayCustomEditor )
		, m_rangeMin( rangeMin )
		, m_rangeMax( rangeMax )
	{
	}

	//! Get min range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMin() const
	{
		return m_rangeMin;
	}

	//! Get max range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMax() const
	{
		return m_rangeMax;
	}
};

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
		struct SCurveBase* tag,
		Float minValue,
		Float maxValue,
		const String& customEditor,
		Bool customEditorArray
	)
	{
		IRTTIType* type = SRTTI::GetInstance().FindType( typeName );
		if ( !type )
		{
			WARN_CORE
			(
				TXT( "Trying to create property %s in class %s - type %s unknown!" ),
				name.AsString().AsChar(),
				parentClass ? parentClass->GetName().AsString().AsChar() : TXT( "<unknown>" ),
				typeName.AsString().AsChar()
			);

			return NULL;
		}

		return new CCurveRangedProperty
		(
			type,
			parentClass,
			static_cast< Uint32 >( offset ),
			name,
			hint,
			flags | PF_Native,
			tag,
			minValue,
			maxValue,
			customEditor,
			customEditorArray
		);
	}	
};

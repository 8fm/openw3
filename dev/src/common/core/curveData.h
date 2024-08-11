#pragma once

#include "enumBuilder.h"
#include "classBuilder.h"
#include "dynarray.h"
#include "math.h"

enum ECurveValueType
{
	CVT_Float,
	CVT_Vector
};
BEGIN_ENUM_RTTI( ECurveValueType );
ENUM_OPTION( CVT_Float );
ENUM_OPTION( CVT_Vector );
END_ENUM_RTTI();

enum ECurveBaseType
{
	CT_Linear,		//<! A series of control points, linearly interpolated.
	CT_Smooth,		//<! A series of control points, interpolated with a smooth curve.
	CT_Segmented	//<! A series of values, where each consecutive pair can be interpolated in a different way. Vector values are not supported.
};
BEGIN_ENUM_RTTI( ECurveBaseType );
ENUM_OPTION( CT_Linear );
ENUM_OPTION( CT_Smooth );
ENUM_OPTION( CT_Segmented );
END_ENUM_RTTI();


enum ECurveSegmentType
{
	CST_Constant,
	CST_Interpolate,
	CST_Bezier,
	CST_BezierSmooth,
	CST_BezierSmoothLyzwiaz,
	CST_BezierSmoothSymertric,
	CST_Bezier2D,
};

BEGIN_ENUM_RTTI( ECurveSegmentType );
	ENUM_OPTION( CST_Constant );
	ENUM_OPTION( CST_Interpolate );
	ENUM_OPTION( CST_Bezier );
	ENUM_OPTION( CST_BezierSmooth );
	ENUM_OPTION( CST_BezierSmoothLyzwiaz );
	ENUM_OPTION( CST_BezierSmoothSymertric );
	ENUM_OPTION( CST_Bezier2D );
END_ENUM_RTTI();

struct SCurveData;

class ICurveDataOwner
{
public:
	virtual TDynArray< SCurveData* >* GetCurvesData() = 0;
	virtual SCurveData* GetCurveData() = 0;
	virtual void OnCurveChanged() {}
	virtual Bool UseCurveData() const { return true; }
};

struct SCurveDataEntry
{
	DECLARE_RTTI_STRUCT( SCurveDataEntry );

	Vector controlPoint;
	Float time;
	Float value;
	Uint16 curveTypeL;
	Uint16 curveTypeR;
};

BEGIN_CLASS_RTTI( SCurveDataEntry )
	PROPERTY( time );
	PROPERTY( controlPoint );
	PROPERTY( value );
	PROPERTY( curveTypeL );
	PROPERTY( curveTypeR );
END_CLASS_RTTI();

struct SCurveData
{
	DECLARE_RTTI_STRUCT_WITH_ALLOCATOR( SCurveData, MC_CurveData );

	TDynArray< SCurveDataEntry, MC_CurveData > m_curveValues;  
	ECurveValueType		m_valueType;
	ECurveBaseType		m_baseType;							//<! This should not be modified directly. Use SetBaseType() instead. It is public only for ease of RTTI serialization.

	Bool				m_loop;								//<! When a curve is looping, time values are assumed to be in the range [0, 1).

	SCurveData( ECurveValueType valueType = CVT_Float ) 
		: m_valueType( valueType )
		, m_baseType( valueType == CVT_Vector ? CT_Smooth : CT_Segmented )
		, m_loop( false )
	{
	}

	SCurveData( SCurveData && other )
		:	m_curveValues( std::move( other.m_curveValues ) ) ,
			m_valueType( std::move( other.m_valueType ) ),
			m_baseType( std::move( other.m_baseType )  ),
			m_loop( std::move( other.m_loop ) )
	{}

	ECurveValueType		GetValueType() const { return m_valueType; }

	void				SetBaseType( ECurveBaseType baseType );
	ECurveBaseType		GetBaseType() const { return m_baseType; }

	// A looped curve always has time values in [0,1). Any times outside this range are wrapped around.
	RED_INLINE void	SetLoop( Bool loop ) { m_loop = loop; }
	RED_INLINE Bool	IsLoop() const { return m_loop; }
	RED_INLINE Float	AdaptTime( Float time ) const { return ( IsLoop() ? time - floorf( time ) : time ); }

	// Returns true if the curve is using tangent information (i.e. is segmented)
	RED_INLINE Bool	UsesTangents() const { return GetBaseType() == CT_Segmented; }

	// Add a control point to the curve. Only valid for Float-valued curves. Returns the new point's index.
	Int32				AddPoint( const Float time, const Float value, const ECurveSegmentType& curveType = CST_Interpolate, Bool allowReplacement = false );
	// Add a control point to the curve. Only valid for Float-valued curves. Returns the new point's index.
	Int32				AddPoint( const Float time, const Float value, const Vector& tangents, const ECurveSegmentType& curveType = CST_Interpolate, Bool allowReplacement = false );
	// Add a control point to the curve. Only valid for Vector-valued curves. Returns the new point's index.
	Int32				AddPoint( const Float time, const Vector& value, Bool allowReplacement = false );
	
	// Remove the control point at the given time. Returns -1 if not exact match (nothing was removed), or the index
	// of the removed point.
	Int32				RemovePoint( const Float time );
	void				RemovePointAtIndex( const Uint32& index );
	void				Sort();

	// Modify the time of the given control point. Returns the new index (in case it changed positions with another point).
	//
	// Setting a control point to the same time as another control point results in semi-strange behavior (ends up with a
	// discontinuous curve with one point off the rest of the curve. Which point is separated depends partly on which
	// direction the modified point came from).
	Uint32				SetTime( const Uint32& index, const Float& time, Bool revalidatePoint = true );

	// Use m_times to determine number of control points. All curve types use it, while some types might not use other fields.
	RED_INLINE Uint32	Size() const { return m_curveValues.Size(); };

	Float				GetMinTime() const { return m_curveValues.Size() > 0 ? m_curveValues[0].time : 0.0f; }
	Float				GetMaxTime() const { return m_curveValues.Size() > 0 ? m_curveValues[m_curveValues.Size()-1].time : 0.0f; }

	void				SetTangentType( const Uint32& index, const Int32& tangentIndex, ECurveSegmentType type );
	ECurveSegmentType	GetTangentType( const Uint32& index, const Int32& tangentIndex ) const;
	void				SetControlPoint( const Uint32& index, const Int32& tangentIndex, const Vector& v);
	Vector				GetControlPoint( const Uint32& index, const Int32& tangentIndex ) const;
	Float 				GetTimeAtIndex( Uint32 index )const;

	void				SetTangentValue( const Uint32& index, const Int32& tangentIndex, const Vector& tangentValue );
	Vector				GetTangentValue( const Uint32& index, const Int32& tangentIndex ) const;

	// Get a Vector value for the given time. Only valid for Vector-valued curves.
	Vector				GetVectorValue( Float time ) const;

	Vector				GetVectorValue( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const;

	// Get a Float value for the given time. Only valid for Float-valued curves.
	RED_INLINE Float	GetFloatValue( Float time ) const { Int32 l,b; return GetFloatValue( time, l, b ); }
	Float				GetFloatValue( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const;

	// Gets angle value (in degrees) interpolated using properietary "nearest-angle method"
	Float				GetAngleValue( Float time ) const;

	Float GetFloatValueAtIndex( Uint32 index ) const
	{
		ASSERT( m_valueType == CVT_Float );
		return m_curveValues[index].value;
	}
	const Vector& GetVectorValueAtIndex( Uint32 index ) const
	{
		ASSERT( m_valueType == CVT_Vector );
		return m_curveValues[index].controlPoint;
	}

	void SetValueAtIndex( Uint32 index, Float value )
	{
		ASSERT( m_valueType == CVT_Float );
		m_curveValues[index].value = value;
	}
	void SetValueAtIndex( Uint32 index, const Vector& value )
	{
		ASSERT( m_valueType == CVT_Vector );
		m_curveValues[index].controlPoint = value;
	}

	void Clear() { m_curveValues.ClearFast(); }

	Float GetLengthUpToTime( Float time, Uint32 probingDensity = 16 ) const;

	// Get the index of the control point at the given time. Returns -1 if there is no exact match.
	Int32 GetIndex( const Float& time ) const;

	// returns index from m_times for which m_times[index] is greatest value <= time or -1 if m_times.Size() == 0 || time < m_times[0]
	Int32 GetLowerBoundIndex( const Float& time ) const;

	void FindLowerUpperBounds( Float time, Int32& lowerBoundIndex, Int32& upperBoundIndex ) const;

	SCurveData& operator=(const SCurveData& other)
	{
		if ( this != &other )
		{
			m_valueType = other.m_valueType;
			m_baseType = other.m_baseType;
			m_loop = other.m_loop;
			m_curveValues = other.m_curveValues;
		}
		return *this;
	}

	SCurveData & operator=( SCurveData && other )
	{
		SCurveData( std::move( other ) ).Swap( *this );
		return *this;
	}

	void Swap( SCurveData & other )
	{
		::Swap( m_valueType, other.m_valueType );
		::Swap( m_baseType, other.m_baseType );
		::Swap( m_loop, other.m_loop );
		m_curveValues.SwapWith( other.m_curveValues );
	}

	static Bool IsBezierType( ECurveSegmentType type ) { return type == CST_Bezier || type == CST_BezierSmooth || type == CST_BezierSmoothLyzwiaz || type == CST_BezierSmoothSymertric || type == CST_Bezier2D; }

	void ConvertToNewFormat();

private:
	Float GetSegmentLengthUpToTime( Float time, Uint32 probingDensity = 16 ) const;

	void SwapPoints( Uint32 a, Uint32 b );
	Uint32 RevalidatePoint( Uint32 index );
};

BEGIN_NODEFAULT_CLASS_RTTI( SCurveData );
	PROPERTY_NAME( m_curveValues, TXT( "Curve Values" ) );
	PROPERTY_EDIT_NAME( m_valueType, TXT("value type"), TXT("value type") );
	PROPERTY_EDIT_NAME( m_baseType, TXT("type"), TXT("type") );
	PROPERTY_EDIT_NAME( m_loop, TXT("is looped"), TXT("is looped") );
END_CLASS_RTTI();

template<> 
Bool TTypedClass< SCurveData, (EMemoryClass)SCurveData::MemoryClass, SCurveData::MemoryPool >::OnReadUnknownProperty( void* object, const CName& propName, const CVariant& propValue ) const;
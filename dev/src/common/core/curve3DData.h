#pragma once

#include "curveData.h"

struct SCurve3DData
{
	SCurveData v[3];

	// ================ methods to probe curve values
	Vector				GetValue( const Float& time ) const;
	// returns time of first key frame
	Float				GetStart() const;
	// returns time of last key frame
	Float				GetEnd() const;
	// returns all key frames
	void				GetKeyframes( TDynArray<Float>& frames) const;

	// ================ methods to manipulate curve value
	// adds new point to curve
	void				AddPoint( const Float& time, const Vector& value );
	// removes point from curve
	void				RemovePoint( const Float& time );
	// sets new values for each component of curve at given time. If given component don't have key frame at given time, then it is added
	void				SetValue( const Float& time, const Vector& value );
	// sets bezier control points
	void				SetControlPoint( const Float& time, const Vector& value, const Int32& tangentIndex );
	// gets bezier control points
	Vector				GetControlPoint( const Float& time, const Int32& tangentIndex ) const;
	
	// ================ Utils
	// checks if given curve is one of the 3D curve components
	Bool				Contains( SCurveData* curve ) { return curve == &v[0] || curve == &v[1] || curve == &v[2]; }

	DECLARE_RTTI_STRUCT( SCurve3DData );
};

BEGIN_NODEFAULT_CLASS_RTTI( SCurve3DData );
PROPERTY_EDIT_NAME( v[0], TXT("curveX"), TXT("curve X") );
PROPERTY_EDIT_NAME( v[1], TXT("curveY"), TXT("curve Y") );
PROPERTY_EDIT_NAME( v[2], TXT("curveZ"), TXT("curve Z") );
END_CLASS_RTTI();
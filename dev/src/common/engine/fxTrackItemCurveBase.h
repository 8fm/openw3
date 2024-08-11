/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItem.h"

class CurveParameter;

/// Track item based on curve
class CFXTrackItemCurveBase : public CFXTrackItem
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CFXTrackItemCurveBase, CFXTrackItem );

protected:
	CurveParameter*		m_curveParameter;		//!< Curve

public:
	CFXTrackItemCurveBase( Uint32 numCurves, CName curvesName = CName::NONE );
	virtual ~CFXTrackItemCurveBase();

	//! This track item supports curves
	virtual Bool SupportsCurve() { return true; }

	//! Get editable curve parameter
	virtual CurveParameter *GetCurveParameter() { return m_curveParameter; }

	//! Get editable curve parameter
	virtual const CurveParameter *GetCurveParameter() const { return m_curveParameter; }

	//! Get editable curve
	virtual CCurve *GetCurve( Uint32 i = 0 );

	//! Get curve for read only mode
	virtual const CCurve *GetCurve( Uint32 i = 0 ) const;

	//! Serialize data
	virtual void OnSerialize( IFile& file );

public:
	//! Sets the same value for all curve control points
	void SetCurveValue( Uint32 curveIdx, Float value );

	//! Evaluate curve parameter
	Float GetCurveValue( Uint32 curveIdx, Float time ) const;

	//! Evaluate curve parameter
	Float GetCurveValue( Float time ) const;

	//! Calculate interpolated color
	Color GetColorFromCurve( Float time ) const;

	//! Calculate interpolated vector
	Vector GetVectorFromCurve( Float time ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CFXTrackItemCurveBase );
	PARENT_CLASS( CFXTrackItem );
END_CLASS_RTTI();

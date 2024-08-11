/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxTrackItemCurveBase.h"

/// Fullscreen effect for brightness modification
class CFXTrackItemBrightnessTint : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemBrightnessTint, CFXTrackItemCurveBase, 0 );

private:
	Color	m_color;		//!< Tint color
	Float	m_range;		//!< Effect range

public:
	CFXTrackItemBrightnessTint();

	//! Get the name of the track item
	virtual String GetName() const { return TXT("Brightness/tint"); }

	//! Change track item name
	virtual void SetName( const String &name ) {}

	//! Get name of the curve
	virtual String GetCurveName( Uint32 i = 0 ) const;

	//! Get number of curves
	virtual Uint32 GetCurvesCount() const { return 2; }

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemBrightnessTint );
	PARENT_CLASS( CFXTrackItemCurveBase );
	PROPERTY_EDIT( m_color, TXT("Color") );
	PROPERTY_EDIT( m_range, TXT("Range") );
END_CLASS_RTTI();

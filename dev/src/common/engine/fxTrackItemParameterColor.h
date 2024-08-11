/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"

/// Curve based color editor
class CFXTrackItemParameterColor : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemParameterColor, CFXTrackItemCurveBase, 0 );

private:
	CName		m_parameterName;		//!< Name of the edited parameter
	Bool		m_restoreAtEnd;			//!< Restore parameter to default value after this block ends

public:
	CFXTrackItemParameterColor();

	//! This parameter supports curve editor
	virtual Bool SupportsCurve() { return true; }

	//! This parameter supports color editor
	virtual Bool SupportsColor() { return true; }

	//! Get track name
	virtual String GetName() const { return m_parameterName.AsString(); }

	//! Change track name
	virtual void SetName( const String& name ) { m_parameterName = CName( name ); }

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemParameterColor );
	PARENT_CLASS( CFXTrackItemCurveBase );
	PROPERTY_CUSTOM_EDIT( m_parameterName, TXT("Parameter name"), TXT("EffectParameterColorList") );
	PROPERTY_EDIT( m_restoreAtEnd, TXT("Restore parameter to default value after this track item ends") );
END_CLASS_RTTI();

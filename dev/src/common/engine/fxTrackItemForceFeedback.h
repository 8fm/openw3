/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItem.h"

/// Rumble event
class CFXTrackItemForceFeedback : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemForceFeedback, CFXTrackItem, 0 );

private:
	Float	m_highFrequencyMotorSpeed;	//!< Speed of the high frequency in-pad motor
	Float	m_lowFrequencyMotorSpeed;	//!< Speed of the low frequency in-pad motor

public:
	CFXTrackItemForceFeedback();

	//! Get track item name
	virtual String GetName() const { return TXT("Rumble"); }

	//! Change name of track item
	virtual void SetName( const String& name ) {}

public:
	//! Spawn play data, called on 
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemForceFeedback );
PARENT_CLASS( CFXTrackItem );
PROPERTY_EDIT_RANGE( m_highFrequencyMotorSpeed, TXT( "Set speed of the HIGH frequency motor, ranges [0.0-1.0]. 0.0 Means no motor use." ), 0.0f, 1.0f );
PROPERTY_EDIT_RANGE( m_lowFrequencyMotorSpeed, TXT( "Set speed of the LOW frequency motor, ranges [0.0-1.0]. 0.0 Means no motor use." ), 0.0f, 1.0f );
END_CLASS_RTTI();

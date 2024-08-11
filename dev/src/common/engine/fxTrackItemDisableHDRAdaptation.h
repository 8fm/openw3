/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxTrackItemCurveBase.h"

//! Disable HDR adaptation for the duration of this track item
class CFXTrackItemDisableHDRAdaptation : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemDisableHDRAdaptation, CFXTrackItem, 0 );

public:
	CFXTrackItemDisableHDRAdaptation();

	//! Get the name of the track item
	virtual String GetName() const { return TXT("Disable HDR adaptation"); }

	//! Change name
	virtual void SetName( const String &name ) {}

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemDisableHDRAdaptation );
	PARENT_CLASS( CFXTrackItem );
END_CLASS_RTTI();

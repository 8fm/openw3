/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxTrackItemCurveBase.h"

/// Full screen blur track item in the FX
class CFXTrackItemFullscreenBlur : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemFullscreenBlur, CFXTrackItemCurveBase, 0 );

public:
	CFXTrackItemFullscreenBlur();

	virtual String GetName() const { return TXT("Fullscreen Blur"); }

	virtual void SetName( const String &name ) {}

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemFullscreenBlur );
	PARENT_CLASS( CFXTrackItemCurveBase );
END_CLASS_RTTI();

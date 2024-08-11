/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItem.h"

/// Pause event
class CFXTrackItemPause : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemPause, CFXTrackItem, 0 );

public:
	CFXTrackItemPause();

	//! Get track name
	virtual String GetName() const { return TXT("Pause"); }

	//! Set track name
	virtual void SetName( const String& name ) {}

	//! Is this a zero-time track item
	virtual Bool IsTick() { return true; }

public:
	//! Spawn play data, called on 
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemPause );
	PARENT_CLASS( CFXTrackItem );
END_CLASS_RTTI();

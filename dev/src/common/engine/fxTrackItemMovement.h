/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"
#include "fxSpawner.h"

/// Component movement item
class CFXTrackItemMovement : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemMovement, CFXTrackItem, 0 );

public:
	CFXTrackItemMovement();

	//! Change name of track item
	virtual void SetName( const String& name );

	//! Get name of track item
	virtual String GetName() const;

	//! Get the number of curves
	virtual Uint32 GetCurvesCount() const { return 3; }

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemMovement );
PARENT_CLASS( CFXTrackItem );
END_CLASS_RTTI();

/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemPause.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemPause );

CFXTrackItemPause::CFXTrackItemPause()
{
	// A tick
	m_timeDuration = 0.0f;
}

IFXTrackItemPlayData* CFXTrackItemPause::OnStart( CFXState& fxState ) const
{
	return NULL;
}
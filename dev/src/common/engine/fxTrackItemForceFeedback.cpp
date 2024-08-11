/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemForceFeedback.h"
#include "game.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemForceFeedback );

CFXTrackItemForceFeedback::CFXTrackItemForceFeedback()
{
	
}

IFXTrackItemPlayData* CFXTrackItemForceFeedback::OnStart( CFXState& fxState ) const
{
	Float low =	Clamp< Float >( m_lowFrequencyMotorSpeed, 0.0f, 1.0f );
	Float high = Clamp< Float >( m_highFrequencyMotorSpeed, 0.0f, 1.0f );

	// That's all to do here
	GGame->VibrateController( low, high, GetTimeDuration() );
	
	// No play data
	return NULL;
}

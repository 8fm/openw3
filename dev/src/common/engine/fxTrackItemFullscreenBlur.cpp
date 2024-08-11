/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxTrackItemFullscreenBlur.h"
#include "fxTrackGroup.h"
#include "environmentManager.h"
#include "game.h"
#include "world.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemFullscreenBlur );

RED_DEFINE_STATIC_NAME( FullscreenBlur );

/// Runtime player for radial blur
class CFXTrackItemFullscreenBlurPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemFullscreenBlur*	m_trackItem;		//!< Data

public:
	CFXTrackItemFullscreenBlurPlayData( CComponent* component, const CFXTrackItemFullscreenBlur* trackItem )
		: IFXTrackItemPlayData( component, trackItem )
		, m_trackItem( trackItem )
		, m_intensity( 0.0f )
	{
	};

	~CFXTrackItemFullscreenBlurPlayData()
	{
		FullscreenBlurDisable();
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		m_intensity = m_trackItem->GetCurveValue( fxState.GetCurrentTime() );
		FullscreenBlurSet();
	}

	void FullscreenBlurSet()
	{
		// TODO: This line could go into constructor, but environment manager can change during the game (can't it?)
		CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;

		if ( envManager )
		{
			// Set new blur params
			envManager->GetGameEnvironmentParams().m_fullscreenBlurIntensity = Clamp( m_intensity, 0.0f, 1.0f );
		}
	}

	void FullscreenBlurDisable()
	{
		CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
		if ( envManager )
		{
			// Set new blur params
			envManager->GetGameEnvironmentParams().m_fullscreenBlurIntensity = Clamp( 0.0f, 0.0f, 1.0f );
		}
	}

	// Blur params
	Float m_intensity;
};

//////////////////////////////////////////////////////////////////////////

CFXTrackItemFullscreenBlur::CFXTrackItemFullscreenBlur()
	: CFXTrackItemCurveBase( 1, CNAME(FullscreenBlur) )
{
}

IFXTrackItemPlayData* CFXTrackItemFullscreenBlur::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( component )
	{
		return new CFXTrackItemFullscreenBlurPlayData( component, this );
	}

	// Not applied
	return NULL;
}

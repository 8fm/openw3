/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxTrackItemDisableHDRAdaptation.h"
#include "fxTrackGroup.h"
#include "game.h"
#include "environmentManager.h"
#include "world.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemDisableHDRAdaptation );

/// Runtime player for radial blur
class CFXTrackItemDisableHDRAdaptationPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemDisableHDRAdaptation*	m_trackItem;		//!< Data

public:
	CFXTrackItemDisableHDRAdaptationPlayData( CComponent* component, const CFXTrackItemDisableHDRAdaptation* trackItem )
		: IFXTrackItemPlayData( component, trackItem )
		, m_trackItem( trackItem )
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			GGame->GetActiveWorld()->GetEnvironmentManager()->DisableHDR();
		}
	};

	~CFXTrackItemDisableHDRAdaptationPlayData()
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			GGame->GetActiveWorld()->GetEnvironmentManager()->EnableHDR();
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
	}
};

//////////////////////////////////////////////////////////////////////////

CFXTrackItemDisableHDRAdaptation::CFXTrackItemDisableHDRAdaptation()
	: CFXTrackItem()
{
}

IFXTrackItemPlayData* CFXTrackItemDisableHDRAdaptation::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( component )
	{
		return new CFXTrackItemDisableHDRAdaptationPlayData( component, this );
	}

	// Not applied
	return NULL;
}

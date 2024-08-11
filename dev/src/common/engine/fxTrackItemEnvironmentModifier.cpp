/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxTrackItemEnvironmentModifier.h"
#include "fxTrack.h"
#include "fxTrackGroup.h"
#include "environmentManager.h"
#include "fxTrackItem.h"
#include "game.h"
#include "world.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemEnvironmentModifier );

/// Runtime player for radial blur
class CFXTrackItemEnvironmentModifierPlayData : public IFXTrackItemPlayData
{
private:
	CRenderBaseLightParams* m_params;
	CEnvironmentManager::SBalanceOverride* m_balanceOverride;

public:
	CFXTrackItemEnvironmentModifierPlayData( CComponent* component, const CFXTrackItemEnvironmentModifier* trackItem, CRenderBaseLightParams* params, const Vector& ambient, CEnvironmentManager::SBalanceOverride* balanceOverride )
		: IFXTrackItemPlayData( component, trackItem )
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			CEnvironmentManager* mgr = GGame->GetActiveWorld()->GetEnvironmentManager();

			mgr->SetBaseLightingOverride( params );
			mgr->SetAmbientOverride( ambient );
			mgr->SetBalanceOverride( balanceOverride );
		}
		m_params = params;
		m_balanceOverride = balanceOverride;
	};

	~CFXTrackItemEnvironmentModifierPlayData()
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			CEnvironmentManager* mgr = GGame->GetActiveWorld()->GetEnvironmentManager();

			mgr->SetBaseLightingOverride( NULL );
			mgr->SetAmbientOverride( Vector::ZEROS );
			mgr->SetBalanceOverride( NULL );
		}

		if ( m_params )
		{
			delete m_params;
		}
		if ( m_balanceOverride )
		{
			delete m_balanceOverride;
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
	}
};

//////////////////////////////////////////////////////////////////////////

CFXTrackItemEnvironmentModifier::CFXTrackItemEnvironmentModifier()
	: CFXTrackItem()
	, m_sunLightBrightness( 1.0f )
	, m_ambientOverrideBrightness( 1.0f )
	, m_parametricBalanceLowScale( 1.0f )
	, m_parametricBalanceMidScale( 1.0f )
	, m_parametricBalanceHighScale( 1.0f )
	, m_overrideBalancing( false )
{
} 

IFXTrackItemPlayData* CFXTrackItemEnvironmentModifier::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( component )
	{
		CRenderBaseLightParams* params = new CRenderBaseLightParams;
		params->m_lightDirection = m_lightDirection.Normalized3();

		{
			const Vector sunLightColor = m_sunLightDiffuse.ToVector() * m_sunLightBrightness;
			params->m_sunLightDiffuse = sunLightColor;
			params->m_sunLightDiffuseLightSide = sunLightColor;
			params->m_sunLightDiffuseLightOppositeSide = sunLightColor;
		}

		//params->m_hdrAdaptationDisabled = m_hdrAdaptationDisabled;

		CEnvironmentManager::SBalanceOverride* balanceOverride = NULL;
		if ( m_overrideBalancing )
		{
			balanceOverride = new CEnvironmentManager::SBalanceOverride;
			balanceOverride->m_parametricBalanceLowOverride = Vector(m_parametricBalanceLow.R, m_parametricBalanceLow.G, m_parametricBalanceLow.B, m_parametricBalanceLowScale );
			balanceOverride->m_parametricBalanceMidOverride = Vector(m_parametricBalanceMid.R, m_parametricBalanceMid.G, m_parametricBalanceMid.B, m_parametricBalanceMidScale );
			balanceOverride->m_parametricBalanceHighOverride = Vector(m_parametricBalanceHigh.R, m_parametricBalanceHigh.G, m_parametricBalanceHigh.B, m_parametricBalanceHighScale );
		}

		return new CFXTrackItemEnvironmentModifierPlayData( component, this, params, m_ambientOverride.ToVector() * m_ambientOverrideBrightness, balanceOverride );
	}

	// Not applied
	return NULL;
}

/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxTrackItemRadialBlur.h"
#include "fxTrackGroup.h"
#include "curve.h"
#include "game.h"
#include "environmentGameParams.h"
#include "environmentManager.h"
#include "world.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemRadialBlur );

RED_DEFINE_STATIC_NAME( RadialBlur );

CFXTrackItemRadialBlur::CFXTrackItemRadialBlur()
	: CFXTrackItemCurveBase( 4, CNAME(RadialBlur) )
	, m_trackComponentPosition( true )
	, m_distanceFromCamera( 8.0f )
	, m_centerMultiplier( 0.0f )
{
	// Initialize curves with their default values
#ifndef NO_EDITOR
	SetCurveValue( 0, 0.0f );
	SetCurveValue( 1, 0.0f );
	SetCurveValue( 2, 8.0f );
	SetCurveValue( 3, 5.0f );
#endif
}

String CFXTrackItemRadialBlur::GetCurveName( Uint32 i /* = 0 */ ) const
{
	switch( i )
	{
	case 0:
		return TXT("Blur Amount");
	case 1:
		return TXT("Wave Amount");
	case 2:
		return TXT("Wave Speed");
	case 3:
		return TXT("Wave Freq");
	default:
		return String::EMPTY;
	}
}

/// Runtime player for radial blur
class CFXTrackItemRadialBlurPlayData : public IFXTrackItemPlayData
{
private:
	// Blur params
	Vector							m_blurSourcePos;
	Float							m_blurAmount;
	Float							m_sineWaveAmount;
	Float							m_sineWaveSpeed;
	Float							m_sineWaveFreq;

	CEntity*						m_ownerEntity;
	const CurveParameter*			m_curves;
											// blurAmountCurve;
											// sineWaveAmountCurve;
											// sineWaveSpeedCurve;
											// sineWaveFreqCurve;

	Bool							m_trackComponentPosition;
	Float							m_distanceFromCamera;
	Float							m_centerMultiplier;
	CEnvRadialBlurParameters*		m_radialBlurParams;

public:
	const CFXTrackItemRadialBlur*	m_trackItem;		//!< Data

public:
	CFXTrackItemRadialBlurPlayData( CComponent* component, const CFXTrackItemRadialBlur* trackItem, Bool trackComponentPosition, Float distanceFromCamera, Float centerMultiplier )
		: IFXTrackItemPlayData( component, trackItem )
		, m_trackItem( trackItem )
		, m_trackComponentPosition( trackComponentPosition )
		, m_distanceFromCamera( distanceFromCamera )
		, m_radialBlurParams( NULL )
		, m_centerMultiplier( centerMultiplier )
	{
		m_ownerEntity = component->GetEntity();

		m_curves = m_trackItem->GetCurveParameter();

		m_blurSourcePos = Vector(0,0,0);

		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			m_radialBlurParams = GGame->GetActiveWorld()->GetEnvironmentManager()->GetRadialBlurManager().GenerateNewRadialBlurParams();
		}
	};

	~CFXTrackItemRadialBlurPlayData()
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			GGame->GetActiveWorld()->GetEnvironmentManager()->GetRadialBlurManager().RemoveRadialBlurParams( m_radialBlurParams );
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		if ( m_trackComponentPosition )
		{
			CComponent* component = (CComponent*)( m_node );
			if ( component )
			{
				m_blurSourcePos  = component->GetWorldPosition();
			}
		}
		else
		{
			if ( GGame && GGame->GetActiveWorld() )
			{
				m_blurSourcePos = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
			}
		}

		// Evaluate curve value
		const Float currentTime = fxState.GetCurrentTime();
		const Float normalizedTime = ( currentTime - m_trackItem->GetTimeBegin() ) / m_trackItem->GetTimeDuration();
		m_blurAmount     = m_curves->GetCurveValue( 0, normalizedTime );
		m_sineWaveAmount = m_curves->GetCurveValue( 1, normalizedTime );
		m_sineWaveSpeed  = m_curves->GetCurveValue( 2, normalizedTime );
		m_sineWaveFreq   = m_curves->GetCurveValue( 3, normalizedTime );

		RadialBlurSet();
	}

	//! Called when entity is partially unstreamed, causing certain components to be destroyed
	virtual void OnPreComponentStreamOut( CComponent* component )
	{
		if ( m_node && m_node->AsComponent() == component )
		{
			m_node = NULL;
		}
	}

	void RadialBlurSet()
	{
		if ( m_radialBlurParams )
		{
			// Init new blur params
			CEnvRadialBlurParameters newBlurParams;
			newBlurParams.m_radialBlurSource = m_blurSourcePos;
			newBlurParams.m_radialBlurAmount = m_blurAmount;
			newBlurParams.m_sineWaveAmount = m_sineWaveAmount;
			newBlurParams.m_sineWaveSpeed = m_sineWaveSpeed;
			newBlurParams.m_sineWaveFreq = m_sineWaveFreq;
			newBlurParams.m_centerMultiplier = m_centerMultiplier;
			newBlurParams.m_distance = m_distanceFromCamera;

			*m_radialBlurParams = newBlurParams;
		}
	}           

};

IFXTrackItemPlayData* CFXTrackItemRadialBlur::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( component )
	{
		return new CFXTrackItemRadialBlurPlayData( component, this, m_trackComponentPosition, m_distanceFromCamera, m_centerMultiplier );
	}

	// Not applied
	return NULL;
}

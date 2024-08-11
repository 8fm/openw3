/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxTrackItemBrightnessTint.h"
#include "fxTrackGroup.h"
#include "curve.h"
#include "game.h"
#include "environmentManager.h"
#include "world.h"
#include "entity.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemBrightnessTint );

RED_DEFINE_STATIC_NAME( BrightnessTint );

CFXTrackItemBrightnessTint::CFXTrackItemBrightnessTint()
	: CFXTrackItemCurveBase( 2, CNAME(BrightnessTint) )
	, m_color( Color::WHITE )
	, m_range( 10.0f )
{
#ifndef NO_EDITOR
	// Initialize curves with their default values
	SetCurveValue( 0, 1.0f );
	SetCurveValue( 1, 0.0f );
#endif
}

String CFXTrackItemBrightnessTint::GetCurveName( Uint32 i /* = 0 */ ) const
{
	switch( i )
	{
	case 0:
		return TXT("Brightness");
	case 1:
		return TXT("Color amount");
	default:
		return String::EMPTY;
	}
}

/// Runtime player for radial blur
class CFXTrackItemBrightnessTintPlayData : public IFXTrackItemPlayData
{
private:
	// Params
	Color							m_color;
	Float							m_brightness;
	Float							m_tint;
	Float							m_range;

	const CurveParameter*			m_curves;
										// brightnessCurve   
										// tintCurve			

	CEnvBrightnessTintParameters*	m_brightnessTintParams;

	const CFXTrackItemBrightnessTint*	m_trackItem;		//!< Data

public:
	CFXTrackItemBrightnessTintPlayData( CComponent* component, const CFXTrackItemBrightnessTint* trackItem, const Color& color, Float range )
		: IFXTrackItemPlayData( component, trackItem )
		, m_color( color )
		, m_trackItem( trackItem )
		, m_range( range )
	{
		m_curves = m_trackItem->GetCurveParameter();

		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			m_brightnessTintParams = GGame->GetActiveWorld()->GetEnvironmentManager()->GetBrightnessTintManager().GenerateNewParams();
		}
	};

	~CFXTrackItemBrightnessTintPlayData()
	{
		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
		{
			GGame->GetActiveWorld()->GetEnvironmentManager()->GetBrightnessTintManager().RemoveParams( m_brightnessTintParams );
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			return;
		}

		// Evaluate curve value
		const Float currentTime = fxState.GetCurrentTime();
		const Float normalizedTime = ( currentTime - m_trackItem->GetTimeBegin() ) / m_trackItem->GetTimeDuration();
		
		const Vector position = fxState.GetEntity()->GetWorldPosition();
		const Float distance = position.DistanceTo( GGame->GetActiveWorld()->GetCameraPosition() );
		const Float value = 1.0f - Clamp( distance / m_range, 0.0f, 1.0f );

		m_brightness    = Lerp( value, 1.0f, m_curves->GetCurveValue( 0, normalizedTime ) );
		m_tint			= Lerp( value, 0.0f, m_curves->GetCurveValue( 1, normalizedTime ) );

		Set();
	}

	void Set()
	{
		if ( m_brightnessTintParams )
		{
			// Init new blur params
			CEnvBrightnessTintParameters newParams;
			newParams.m_brightness = m_brightness;
			newParams.m_tint = Lerp<Vector>( m_tint, Vector::ONES, m_color.ToVector() );

			*m_brightnessTintParams = newParams;
		}
	}           

};

IFXTrackItemPlayData* CFXTrackItemBrightnessTint::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( component )
	{
		return new CFXTrackItemBrightnessTintPlayData( component, this, m_color, m_range );
	}

	// Not applied
	return NULL;
}

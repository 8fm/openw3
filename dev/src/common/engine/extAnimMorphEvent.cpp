
#include "build.h"
#include "extAnimMorphEvent.h"
#include "morphedMeshComponent.h"
#include "animatedComponent.h"
#include "componentIterator.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimMorphEvent );

CExtAnimMorphEvent::CExtAnimMorphEvent()
	: CExtAnimDurationEvent()
	, m_invertWeight( false )
	, m_useCurve( false )
{

}

CExtAnimMorphEvent::CExtAnimMorphEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_invertWeight( false )
	, m_useCurve( false )
{
	// Morphing can be heavy so we always want to stop it properly
	m_alwaysFiresEnd = true;
}

void CExtAnimMorphEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	SetMorphWeight( m_invertWeight ? 1.f : 0.f, component );
}

void CExtAnimMorphEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( m_duration > 0.f )
	{
		Float w = 0.f;

		if ( m_useCurve )
		{
			const Float time01 = Clamp( ( info.m_animInfo.m_localTime - m_startTime ) / m_duration, 0.f, 1.f );
			w = m_curve.GetFloatValue( time01 );			
		}
		else
		{
			w = Clamp( ( info.m_animInfo.m_localTime - m_startTime ) / m_duration, 0.f, 1.f );
		}

		SetMorphWeight( m_invertWeight ? 1.f - w : w, component );
	}
}

void CExtAnimMorphEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	SetMorphWeight( m_invertWeight ? 0.f : 1.f, component );
}

void CExtAnimMorphEvent::SetMorphWeight( Float w, CAnimatedComponent* component ) const
{
	CEntity* e = component->GetEntity();

	for ( ComponentIterator< CMorphedMeshComponent > it( e ); it; ++it )
	{
		CMorphedMeshComponent* m = *it;

		if ( m->GetMorphComponentId() == m_morphComponentId )
		{
			m->SetMorphRatio( w );

			// Should we handle only one morph component with the same name?
			// break;
		}
	}
}

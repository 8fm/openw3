/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

/// property anim play event
#include "build.h"
#include "fxTrackItemPlayPropertyAnim.h"
#include "actor.h"
#include "../engine/fxTrackGroup.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemPlayPropertyAnim );

CFXTrackItemPlayPropertyAnim::CFXTrackItemPlayPropertyAnim()
	: m_restoreAtEnd( true )
	, m_propertyAnimationName( CName::NONE )
	, m_loopCount( 1 )
{
}

/// Runtime player for item effect
class CFXTrackItemPlayPropertyAnimData : public IFXTrackItemPlayData
{
public:
	CName						m_propertyAnimName;		//!< Name of the effect
	CAnimatedPropertyCapture	m_initialValues;
	Bool						m_restoreAtEnd;
	CEntity*					m_ent;
	Uint32						m_loopCount;

public:
	CFXTrackItemPlayPropertyAnimData( const CFXTrackItemPlayPropertyAnim* trackItem, CEntity* ent, CComponent* component, CName propertyAnimName, Bool restoreAtEnd, Uint32 loopCounter )
		: IFXTrackItemPlayData( component, trackItem )
		, m_propertyAnimName( propertyAnimName )
		, m_restoreAtEnd( restoreAtEnd )
		, m_ent( ent )
		, m_loopCount( loopCounter )
	{
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( m_ent );
		if ( gameplayEntity && m_propertyAnimName )
		{
			if ( m_restoreAtEnd )
			{
				m_initialValues.Init( gameplayEntity->GetPropertyAnimationSet() );
				m_initialValues.CaptureAnimatedProperties( m_propertyAnimName );
			}
			gameplayEntity->PlayPropertyAnimation( m_propertyAnimName, m_loopCount, trackItem->GetTimeDuration() );
		}
	}

	~CFXTrackItemPlayPropertyAnimData()
	{
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( m_ent );
		if ( gameplayEntity )
		{
			gameplayEntity->StopPropertyAnimation( m_propertyAnimName, false );
			if ( m_restoreAtEnd )
			{
				m_initialValues.RestoreAnimatedProperties();
			}
		}
	}

	virtual void OnStop()
	{
	}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
	}
};

IFXTrackItemPlayData* CFXTrackItemPlayPropertyAnim::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState ); 
	if ( !component ) 
	{
		return nullptr;
	}
	CEntity* entity = fxState.GetEntity();
	ASSERT( entity );
	return new CFXTrackItemPlayPropertyAnimData( this, entity, component, m_propertyAnimationName, m_restoreAtEnd, m_loopCount );
}

void CFXTrackItemPlayPropertyAnim::SetName( const String& name )
{
}

String CFXTrackItemPlayPropertyAnim::GetName() const
{
	return TXT("Property anim play");
}

CName CFXTrackItemPlayPropertyAnim::GetPropertyAnimName() const
{
	return m_propertyAnimationName; 
}
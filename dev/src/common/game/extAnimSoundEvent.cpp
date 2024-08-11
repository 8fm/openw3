/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimSoundEvent.h"

#include "actorAnimationEventFilter.h"
#include "../engine/soundStartData.h"
#include "../engine/soundEmitter.h"
#include "../engine/soundSystem.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimSoundEvent );

CExtAnimSoundEvent::CExtAnimSoundEvent()
	: m_soundEventName()
	, m_maxDistance( 30.0f )
	, m_filterCooldown( 0.0f )
	, m_bone( CNAME( Trajectory ) )
	, m_filter( false )
	, m_speed(-1.f)
	, m_decelDist(0.f)
{
	m_reportToScript = false;
}

CExtAnimSoundEvent::CExtAnimSoundEvent( const CName& eventName,	const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_soundEventName()
	, m_maxDistance( 30.0f )
	, m_filterCooldown( 0.0f )
	, m_bone( CNAME( Trajectory ) )
	, m_filter( false )
	, m_useDistanceParameter( false )
	, m_speed(-1.f)
	, m_decelDist(0.f)
{
	m_reportToScript = false;
}

void CExtAnimSoundEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	PC_SCOPE( SoundEvent );

	if ( m_soundEventName.Empty() || info.m_alpha == 0.f ) return;

	if ( component == nullptr )
	{
		GSoundSystem->SoundEvent( m_soundEventName.AsChar() );
		// Play sound with no actor positioning (2D)
		return;
	}

	CEntity *entity = Cast< CEntity >( component->GetParent() );
	ASSERT( entity );

	CSoundEmitterComponent* soundEmitterComponent = CSoundEmitterComponent::GetSoundEmitterIfMaxDistanceIsntReached( entity, m_maxDistance );
	if( !soundEmitterComponent ) return;

	// TODO - do not do it per call
	const Int32 bone = m_bone != CName::NONE ? component->FindBoneByName( m_bone ) : -1;

	if( CActor* actor = Cast< CActor >( entity ) )
	{
		if( ( actor->GetHideReason() & CEntity::HR_Scene ) == CEntity::HR_Scene )
		{
			return;
		}

		if( m_filter && actor->UsesAnimationEventFilter() )
		{
			if( !actor->GetAnimationEventFilter()->CanTriggerEvent( GetSoundEventName(), bone, m_filterCooldown ) )
			{
				return;
			}
		}
	}

	const Int32 switchCount = m_switchesToUpdate.SizeInt();
	for ( Int32 i=0; i!=switchCount; ++i )
	{
		const StringAnsi& switchName = m_switchesToUpdate[ i ];
		soundEmitterComponent->CopySwitchFromRoot( switchName.AsChar(), bone );
	}
	const Int32 parametersCount = m_parametersToUpdate.SizeInt();
	for ( Int32 i=0; i!=parametersCount; ++i )
	{
		const StringAnsi& paramterName = m_parametersToUpdate[ i ];
		soundEmitterComponent->CopyPrameterFromRoot( paramterName.AsChar(), bone );
	}

	if(m_useDistanceParameter)
	{
		soundEmitterComponent->SoundParameter("distance", (soundEmitterComponent->GetWorldPosition() - GSoundSystem->GetListenerPosition()).Mag3(), 0.f, bone);
	}

	if(m_speed >= 0.f)
	{
		soundEmitterComponent->AuxiliarySoundEvent(m_soundEventName.AsChar(), m_speed, m_decelDist, bone);
	}
	else
	{
		soundEmitterComponent->SoundEvent( m_soundEventName.AsChar(), bone );

	}

}

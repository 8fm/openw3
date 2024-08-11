/*
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimMaterialBasedFxEvent.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimMaterialBasedFxEvent );

CExtAnimMaterialBasedFxEvent::CExtAnimMaterialBasedFxEvent()
	: m_bone( CNAME( Trajectory ) )
	, m_vfxKickup( false )
{

}

CExtAnimMaterialBasedFxEvent::CExtAnimMaterialBasedFxEvent(const CName& eventName, const CName& animationName, Float startTime, const String& trackName)
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_bone( CNAME( Trajectory ) )
	, m_vfxKickup( false )
	, m_vfxFootstep( false )
{

}

void CExtAnimMaterialBasedFxEvent::Process(const CAnimationEventFired& info, CAnimatedComponent* component) const
{
	component->MarkProcessPostponedEvents();
}

void CExtAnimMaterialBasedFxEvent::ProcessPostponed(const CAnimationEventFired& info, CAnimatedComponent* component) const
{
	if ( m_vfxKickup || m_vfxFootstep )
	{
		if ( CEntity* entity = component->GetEntity() )
		{
			if ( const CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( component ) )
			{
				if ( const SPhysicalMaterial* material = mac->GetCurrentStandPhysicalMaterial() )
				{
					if ( m_vfxKickup && material->m_particleBodyroll )
					{
						entity->PlayEffect( material->m_particleBodyroll, m_bone, entity );
					}
					if ( m_vfxFootstep && material->m_particleFootsteps )
					{
						entity->PlayEffect( material->m_particleFootsteps, m_bone, entity );
					}
				}
			}
		}
	}
}

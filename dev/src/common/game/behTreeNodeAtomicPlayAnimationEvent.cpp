/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicPlayAnimationEvent.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicPlayAnimationEventDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicPlayAnimationEventInstance::CBehTreeNodeAtomicPlayAnimationEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_eventStateName( def.m_eventStateName.GetVal( context ) )
	, m_eventResetTriggerName( def.m_eventResetTriggerName )
	, m_eventRunning( false )
    , m_shouldForceEvent( def.m_shouldForceEvent )
	, m_eventRepeated( false )
{
}

Bool CBehTreeNodeAtomicPlayAnimationEventInstance::Activate()
{
	if( !FireEvent() )
	{
		DebugNotifyActivationFail();
		return false;
	}
	m_stateCompleted = false;

	return Super::Activate();
}

Bool CBehTreeNodeAtomicPlayAnimationEventInstance::FireEvent()
{
	CActor* actor = m_owner->GetActor();

    if( m_shouldForceEvent )
    {
        if ( !actor->RaiseBehaviorForceEvent( m_eventStateName ) )
        {
            return false;
        }
    }
    else
    {
        if ( !actor->RaiseBehaviorEvent( m_eventStateName ) )
        {
            return false;
        }
    }

	m_eventDelay = 3; // wait a little as we may be ticked less often
	m_eventRunning = false;

	return true;
}

void CBehTreeNodeAtomicPlayAnimationEventInstance::Update()
{
	if ( !m_eventRunning )
	{
		if ( m_eventDelay > 0 )
		{
			-- m_eventDelay;
		}
		else
		{
			Complete( BTTO_FAILED );
		}
	}
	else if ( m_stateCompleted )
	{
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodeAtomicPlayAnimationEventInstance::Deactivate()
{
	m_eventDelay = 0;
	m_eventRunning = false;
	m_eventRepeated = false;
 	Super::Deactivate();
}

Bool CBehTreeNodeAtomicPlayAnimationEventInstance::OnEvent( CBehTreeEvent& e )
{
	switch( e.m_eventType )
	{
	case BTET_AET_Tick:
		{
			if(e.m_eventName == CNAME ( EndTask ) )
			{
				Complete( BTTO_SUCCESS );
			}
		}
		break;
	case BTET_GameplayEvent:
		if ( e.m_eventName == CNAME( AI_AnimStateActivated ) )
		{
			if( static_cast< SGameplayEventParamCName* >(e.m_gameplayEventData.m_customData)->m_value == m_eventStateName )
			{
				m_eventRunning = true;
				m_stateCompleted = false;
			}
		}
		else if ( e.m_eventName == CNAME( AI_AnimStateDeactivated ) )
		{
			if( static_cast< SGameplayEventParamCName* >(e.m_gameplayEventData.m_customData)->m_value == m_eventStateName )
			{
				if( m_eventRepeated )
				{
					m_eventRepeated = false;
				}
				else
				{
					m_stateCompleted = true;
				}
			}
		}
		else if ( e.m_eventName == m_eventResetTriggerName )
		{
			Bool res = FireEvent();
			if( res )
			{
				m_eventRepeated = true;
			}
		}
		break;
	default:
		break;
	}

	return false;
}


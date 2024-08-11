/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicDamageReaction.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "baseDamage.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicDamageReactionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicDamageReactionInstance::CBehTreeNodeAtomicDamageReactionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_delay( def.m_delay )
	, m_activationDelay( -1.f )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BeingHit );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void CBehTreeNodeAtomicDamageReactionInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BeingHit );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeAtomicDamageReactionInstance::OnListenedEvent( CBehTreeEvent& e )
{
	CBaseDamage* damageData = e.m_gameplayEventData.Get< CBaseDamage >();
	if ( damageData->m_hitReactionAnimRequested )
	{
		if ( m_isActive )
		{
			Bool res = FireEvent();
			if(res)
			{
				m_eventRepeated = true;
			}
		}
		else
		{
			m_eventRunning = true;
			m_activationDelay = m_owner->GetLocalTime() + m_delay;
		}

		return true;
	}
	return false;
}
Bool CBehTreeNodeAtomicDamageReactionInstance::IsAvailable()
{
	Bool toRet = ( Super::IsAvailable() && (m_isActive || m_activationDelay >= m_owner->GetLocalTime()) );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}

	return toRet;
}
void CBehTreeNodeAtomicDamageReactionInstance::Complete( eTaskOutcome outcome )
{
	if ( outcome == BTTO_SUCCESS )
	{
		m_activationDelay = -1.f;
	}
	Super::Complete( outcome );
}
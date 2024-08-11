/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicPlayAnimation.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicPlayAnimationDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicPlayAnimationInstance::CBehTreeNodeAtomicPlayAnimationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
	, m_animationName( def.m_animationName.GetVal( context ) )
	, m_slotName( def.m_slotName.GetVal( context ) )
	, m_blendIn( def.m_blendInTime.GetVal( context ) )
	, m_blendOut( def.m_blendOutTime.GetVal( context ) )
{

}

Bool CBehTreeNodeAtomicPlayAnimationInstance::Activate()
{
	CActor* actor = m_owner->GetActor();
	if ( !actor->ActionPlaySlotAnimation( m_slotName, m_animationName, m_blendIn, m_blendOut ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	return Super::Activate();
}

void CBehTreeNodeAtomicPlayAnimationInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	if ( actor->GetActionType() != ActorAction_Animation )
	{
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodeAtomicPlayAnimationInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	if ( actor )
	{
		actor->ActionCancelAll();
	}
	Super::Deactivate();
}
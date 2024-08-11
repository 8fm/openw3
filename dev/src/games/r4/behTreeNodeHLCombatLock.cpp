/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeHLCombatLock.h"

#include "../../common/game/behTreeInstance.h"
#include "behTreeDecoratorHLCombat.h"

IBehTreeNodeDecoratorInstance* CBehTreeNodeProlongHLCombatDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return m_requestCombatActivationOnEvent
		? new CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance( *this, owner, context, parent )
		: new CBehTreeNodeProlongHLCombatDecoratorInstance( *this, owner, context, parent );

}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeProlongHLCombatDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeProlongHLCombatDecoratorInstance::CBehTreeNodeProlongHLCombatDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	 : Super( def, owner, context, parent )
	 , m_combatLock( owner, IBehTreeNodeHLCombatBaseInstance::GetCombatLockName() )
	 , m_lockIsUp( false )
{}

void CBehTreeNodeProlongHLCombatDecoratorInstance::LockUp()
{
	if( !m_lockIsUp )
	{
		m_combatLock->ModifyCounter( 1 );
		m_lockIsUp = true;
	}
}

void CBehTreeNodeProlongHLCombatDecoratorInstance::LockDown()
{
	if( m_lockIsUp )
	{
		m_combatLock->ModifyCounter( -1 );
		m_lockIsUp = false;
	}
}

Bool CBehTreeNodeProlongHLCombatDecoratorInstance::Activate()
{
	if( Super::Activate() )
	{
		LockUp();
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodeProlongHLCombatDecoratorInstance::Deactivate()
{
	LockDown();
	Super::Deactivate();
}

Bool CBehTreeNodeProlongHLCombatDecoratorInstance::IsAvailable()
{
	Bool parentAvailable = Super::IsAvailable();
	if( !parentAvailable )
	{
		LockDown();
	}
	return parentAvailable;
}

Int32 CBehTreeNodeProlongHLCombatDecoratorInstance::Evaluate()
{
	Int32 parentEvaluate = Super::Evaluate();
	if( parentEvaluate <= 0 )
	{
		LockDown();
	}
	return parentEvaluate;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance::CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( CombatActivated );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( CombatActivated );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );
	Super::OnDestruction();
}

void CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance::MarkDirty()
{
	if ( !m_isActive )
	{
		LockUp();
	}
	Super::MarkDirty();
}

Bool CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( CombatActivated ) )
	{
		LockDown();
		return false;
	}
	return Super::OnListenedEvent( e );
}

IBehTreeNodeDecoratorInstance* CBehTreeNodeNotifyCombatActivationDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new CBehTreeNodeNotifyCombatActivationDecoratorInstance( *this, owner, context, parent );
}

Bool CBehTreeNodeNotifyCombatActivationDecoratorInstance::Activate()
{
	if ( Super::Activate() )
	{
		m_owner->GetActor()->SignalGameplayEvent( CNAME( CombatActivated ) );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
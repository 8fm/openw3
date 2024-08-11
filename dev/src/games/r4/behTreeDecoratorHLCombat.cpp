#include "build.h"
#include "behTreeDecoratorHLCombat.h"

#include "r4BehTreeInstance.h"
#include "w3GenericVehicle.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeHLCombatDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeHLAnimalCombatDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorHLDangerDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorHLDangerTamableDefinition )

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeHLOffenceBaseInstance
////////////////////////////////////////////////////////////////////////
CName IBehTreeNodeHLOffenceBaseInstance::GetCombatLockName()
{
	return CNAME( CombatLock );
}
CName IBehTreeNodeHLOffenceBaseInstance::GetCombatRequestEventName()
{
	return CNAME( AI_RequestCombatEvaluation );
}

IBehTreeNodeHLOffenceBaseInstance::IBehTreeNodeHLOffenceBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_combatLock( owner, GetCombatLockName() )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = GetCombatRequestEventName();
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}

void IBehTreeNodeHLOffenceBaseInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = GetCombatRequestEventName();
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}
 
Bool IBehTreeNodeHLOffenceBaseInstance::Check()
{
	return m_combatLock->GetCounter() > 0 || ConditionCheck();
}


Bool IBehTreeNodeHLOffenceBaseInstance::IsAvailable()
{
	if( Check() )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}

Int32 IBehTreeNodeHLOffenceBaseInstance::Evaluate()
{
	if( Check() )
	{
		if( m_priority <= 0 )
		{
			DebugNotifyAvailableFail();
		}
		return m_priority;
	}

	DebugNotifyAvailableFail();
	return -1;
}

Bool IBehTreeNodeHLOffenceBaseInstance::Activate()
{
	if ( CActor* actor = m_owner->GetActor() )
	{
		if ( CAnimatedComponent* ac = actor->GetRootAnimatedComponent() )
		{
			// update every frame!
			ac->SetSkipUpdateAndSampleFramesLimitDueToAI( 0 );
		}
	}
	Bool b = Super::Activate();
	return b;
}

void IBehTreeNodeHLOffenceBaseInstance::Deactivate()
{
	if ( CActor* actor = m_owner->GetActor() )
	{
		if ( CAnimatedComponent* ac = actor->GetRootAnimatedComponent() )
		{
			// reset limit
			ac->SetSkipUpdateAndSampleFramesLimitDueToAI();
		}
	}
	Super::Deactivate();
}

Bool IBehTreeNodeHLOffenceBaseInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( !m_isActive )
	{
		if ( e.m_eventName == GetCombatRequestEventName() )
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeHLCombatInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeHLCombatInstance::CBehTreeNodeHLCombatInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaDataPtr( owner )
{}


Bool CBehTreeNodeHLCombatInstance::ConditionCheck()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( !npc )
	{
		return false;
	}

	if ( npc->IsInDanger() )
	{
		return true;
	}

	return false;
}

Bool CBehTreeNodeHLCombatInstance::Activate()
{
	if ( !Super::Activate() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_owner->SetIsInCombat( true );

	return true;
}

void CBehTreeNodeHLCombatInstance::Deactivate()
{
	Super::Deactivate();

	m_owner->SetIsInCombat( false );
	m_guardAreaDataPtr->ClearTargetNoticedAtGuardArea();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeHLAnimalCombatInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeHLAnimalCombatInstance::ConditionCheck()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( !npc )
	{
		return false;
	}

	if ( npc->IsSeeingNonFriendlyNPC() )
	{
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////
// IBehTreeDecoratorBaseHLDangerInstance
////////////////////////////////////////////////////////////////////////
Bool IBehTreeDecoratorBaseHLDangerInstance::Activate()
{
	// If this node can be activated
	if ( Check() )
	{
		// activate the children nodes :
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}

Bool IBehTreeDecoratorBaseHLDangerInstance::ConditionCheck()
{
	CNewNPC *const npc = m_owner->GetNPC();
	if ( npc == nullptr )
	{
		return false;
	}
	const CActor * enemy = nullptr;
	if ( GetNeutralIsDanger() == false )
	{
		enemy = npc->IsInDanger();
	}
	else
	{
		enemy = npc->IsSeeingNonFriendlyNPC();
	}

	if ( enemy )
	{
		return true;

	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerInstance
CBehTreeDecoratorHLDangerInstance::CBehTreeDecoratorHLDangerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent ) 
	, m_neutralIsDanger( def.m_neutralIsDanger.GetVal( context ) )
{

}
Bool CBehTreeDecoratorHLDangerInstance::GetNeutralIsDanger()const
{
	return m_neutralIsDanger;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerInstance
CBehTreeDecoratorHLDangerTamableInstance::CBehTreeDecoratorHLDangerTamableInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent ) 
	, m_tempNeutralIsDangerTime( 0.0f )
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_RequestCombatEvaluation );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_NeutralIsDanger );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}

void CBehTreeDecoratorHLDangerTamableInstance::OnDestruction()
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_RequestCombatEvaluation );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_NeutralIsDanger );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

Bool CBehTreeDecoratorHLDangerTamableInstance::IsCharmed()const
{
	CActor *const actor = m_owner->GetActor();
	if ( actor == nullptr )
	{
		return false;
	}
	CName horseAttitude = actor->GetAttitudeGroup();
	return horseAttitude == CNAME( animals_charmed );
}
Bool CBehTreeDecoratorHLDangerTamableInstance::GetNeutralIsDanger()const
{
	CActor *const actor = m_owner->GetActor();

	if ( ComponentIterator< W3HorseComponent >( actor ) && W3HorseComponent::IsTamed( actor ) )
	{
		return false;
	}

	if ( IsCharmed() )
	{
		return false;
	}

	if ( m_neutralIsDanger )
	{
		return true;
	}

	if ( m_tempNeutralIsDangerTime >= m_owner->GetLocalTime() )
	{
		return true;
	}
	return false;
}


Bool CBehTreeDecoratorHLDangerTamableInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_NeutralIsDanger ) )
	{
		SGameplayEventParamFloat* params	= e.m_gameplayEventData.Get< SGameplayEventParamFloat >();
		if ( params )
		{
			m_tempNeutralIsDangerTime	=  GetOwner()->GetLocalTime() + params->m_value;
		}
	}
	return CBehTreeDecoratorHLDangerInstance::OnListenedEvent( e );
}
			


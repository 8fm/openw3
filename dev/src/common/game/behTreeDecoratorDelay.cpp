#include "build.h"
#include "behTreeDecoratorDelay.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeActivationDelayDecoratorDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeActivationDelayDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeActivationDelayDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeActivationDelayDecoratorInstance::CBehTreeNodeActivationDelayDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_activationTimeout( 0.f )
	, m_delayOnSuccess( def.m_delayOnSuccess.GetVal( context ) )
	, m_delayOnFailure( def.m_delayOnFailure.GetVal( context ) )
	, m_delayOnInterruption( def.m_delayOnInterruption.GetVal( context ) )
{}

void CBehTreeNodeActivationDelayDecoratorInstance::OnSubgoalCompleted( eTaskOutcome outcome ) 
{
	if ( outcome == BTTO_SUCCESS )
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnSuccess );
	}
	else
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnFailure );
	}
	Super::OnSubgoalCompleted( outcome );
}

Bool CBehTreeNodeActivationDelayDecoratorInstance::Activate()
{
	if ( !Super::Activate() )
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnFailure );
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}

Bool CBehTreeNodeActivationDelayDecoratorInstance::Interrupt()
{
	if ( Super::Interrupt() )
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnInterruption );
		return true;
	}
	return false;
}

void CBehTreeNodeActivationDelayDecoratorInstance::Deactivate()
{
	if ( m_child->IsActive() )
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnInterruption );
	}
	Super::Deactivate();
}

Int32 CBehTreeNodeActivationDelayDecoratorInstance::Evaluate()
{
	if ( m_activationTimeout )
	{
		if ( m_activationTimeout > m_owner->GetLocalTime() )
		{
			DebugNotifyAvailableFail();
			return -1;
		}

		m_activationTimeout = 0.f;
	}
	return Super::Evaluate();
}

Bool CBehTreeNodeActivationDelayDecoratorInstance::IsAvailable()
{
	if ( m_activationTimeout )
	{
		if ( m_activationTimeout > m_owner->GetLocalTime() )
		{
			DebugNotifyAvailableFail();
			return false;
		}

		m_activationTimeout = 0.f;
	}
	return Super::IsAvailable();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDelayActivationDecoratorDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeDelayActivationDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDelayActivationDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeDelayActivationDecoratorInstance::CBehTreeNodeDelayActivationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_delay( def.m_delay.GetVal( context ) )
	, m_nextActivationTime( -1.0f )
{
	m_deactivationDelay = m_delay + def.m_activationWindow != -1.0f ? def.m_activationWindow : NumericLimits< Float >::Max();
}

void CBehTreeNodeDelayActivationDecoratorInstance::Deactivate()
{
	m_nextActivationTime 		= -1.0;
	Super::Deactivate();
}

Int32 CBehTreeNodeDelayActivationDecoratorInstance::Evaluate()
{
	if ( !m_isActive )
	{
		if ( m_nextActivationTime == -1.0f )
		{
			StartTimer();

			DebugNotifyAvailableFail();
			return -1;
		}

		if ( !Check() )
		{
			DebugNotifyAvailableFail();
			return -1;
		}
	}

	Int32 priority = Super::Evaluate();
	if ( priority <= 0 )
	{
		DebugNotifyAvailableFail();
		m_nextActivationTime = -1.f;
	}
	return priority;
}

Bool CBehTreeNodeDelayActivationDecoratorInstance::IsAvailable()
{
	if ( !m_isActive )
	{
		if ( m_nextActivationTime == -1.0f )
		{
			StartTimer();

			DebugNotifyAvailableFail();
			return false;
		}

		if ( !Check() )
		{
			DebugNotifyAvailableFail();
			return false;
		}
	}

	if ( !Super::IsAvailable() )
	{
		DebugNotifyAvailableFail();
		m_nextActivationTime = -1.f;
		return false;
	}
	return true;
}
Bool CBehTreeNodeDelayActivationDecoratorInstance::Check()
{
	const Float localTime = GetOwner()->GetLocalTime();
	
	if ( m_nextActivationTime > localTime )
	{
		return false;
	}
	else if ( m_deactivationTime < localTime )
	{
		m_nextActivationTime = -1.f;
		return false;
	}
	return true;
}
void CBehTreeNodeDelayActivationDecoratorInstance::StartTimer()
{
	const Float localTime = GetOwner()->GetLocalTime();

	m_nextActivationTime = localTime + m_delay;
	m_deactivationTime = m_nextActivationTime + m_deactivationDelay;
}
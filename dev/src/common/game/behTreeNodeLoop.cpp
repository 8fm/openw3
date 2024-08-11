#include "build.h"

#include "behTreeNodeLoop.h"
#include "behTreeInstance.h"

IMPLEMENT_RTTI_ENUM( EBTLoopMode );

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeLoopDecoratorDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeLoopDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeLoopDecoratorInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeLoopDecoratorInstance::Activate()
{
	m_iterationsCount = 0;
	if ( Super::Activate() )
	{
		return true;
	}

	if ( m_onFailed == BTLM_Continue )
	{
		if ( (++m_iterationsCount) == m_maxIterations )
		{
			DebugNotifyActivationFail();
			return false;
		}
		m_reactivationTime = m_owner->GetLocalTime() + m_reactivationDelay;
		return IBehTreeNodeInstance::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeLoopDecoratorInstance::Update()
{
	if ( m_child->IsActive() )
	{
		m_child->Update();
	}
	else
	{
		if ( m_reactivationTime < m_owner->GetLocalTime() )
		{
			// reactivate
			if ( m_child->Activate() )
			{
			}
			else if ( m_onFailed == BTLM_Continue )
			{
				// reactivation failed
				if ( (++m_iterationsCount) == m_maxIterations )
				{
					Complete( BTTO_FAILED );
				}
				else
				{
					m_reactivationTime = m_owner->GetLocalTime() + m_reactivationDelay;
				}
			}
			else
			{
				Complete( (m_onFailed == BTLM_ReportCompleted) ? BTTO_SUCCESS : BTTO_FAILED );
			}
		}
	}
}
void CBehTreeNodeLoopDecoratorInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	EBTLoopMode action = (outcome == BTTO_FAILED) ? m_onFailed : m_onCompleted;
	switch ( action )
	{
	case BTLM_Continue:
		if ( (++m_iterationsCount) == m_maxIterations )
		{
			Complete( (outcome == BTTO_FAILED) ? BTTO_FAILED  : BTTO_SUCCESS );
			return;
		}
		else if ( m_reactivationDelay > 0.f )
		{
			m_reactivationTime = m_owner->GetLocalTime() + m_reactivationDelay;
		}
		else
		{
			if ( !m_child->Activate() )
			{
				if ( m_onFailed == BTLM_ReportFailed )
				{
					Complete( BTTO_FAILED );
					return;
				}
				m_reactivationTime = m_owner->GetLocalTime() + m_reactivationDelay;
				return;
			}
		}
		break;
	case BTLM_ReportCompleted:
		Complete( BTTO_SUCCESS );
		break;
	case BTLM_ReportFailed:
	default:
		Complete( BTTO_FAILED );
		break;
	}
}

IBehTreeNodeInstance* CBehTreeNodeLoopDecoratorInstance::GetActiveChild() const
{
	return m_child->IsActive() ? m_child : nullptr;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeLoopWithTimelimitDecoratorDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance* CBehTreeNodeLoopWithTimelimitDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeLoopWithTimelimitDecoratorInstance
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeLoopWithTimelimitDecoratorInstance::Activate()
{
	m_currentTimeLimit = m_owner->GetLocalTime() + m_timeLimitDuration;
	return Super::Activate();
}
void CBehTreeNodeLoopWithTimelimitDecoratorInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( m_currentTimeLimit >= m_owner->GetLocalTime() )
	{
		// behave like normal loop decorator
		Super::OnSubgoalCompleted( outcome );
	}
	else
	{
		// complete itself
		IBehTreeNodeDecoratorInstance::OnSubgoalCompleted( outcome );
	}
}
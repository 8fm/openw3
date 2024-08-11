#include "build.h"
#include "r4BehTreeInstance.h"

#include "behTreeScriptTicketAlgorithm.h"
#include "ticketSystem.h"

IMPLEMENT_ENGINE_CLASS( CR4BehTreeInstance );

void CR4BehTreeInstance::SetCombatTarget( const THandle< CActor >& node, Bool registerAsAttacker )
{
	if ( m_actor )
	{
		CCombatDataComponent* prevTargetData = m_targetData;

		if ( prevTargetData )
		{
			prevTargetData->UnregisterAttacker( m_actor );
		}
		if ( registerAsAttacker )
		{
			CActor* target = node.Get();
			CCombatDataComponent* newTargetData = (m_targetData = target);
			if ( newTargetData )
			{
				newTargetData->RegisterAttacker( m_actor );
			}
		}
		else
		{
			m_targetData.Clear();
		}

		if ( m_reachabilityQuery )
		{
			m_reachabilityQuery->OnCombatTargetChanged();
		}
	}
	
	// TODO: Remove previous targeting system (implemented on actor)
	TBaseClass::SetCombatTarget( node, registerAsAttacker );
}
void CR4BehTreeInstance::ClearCombatTarget()
{
	if ( m_actor )
	{
		CCombatDataComponent* targetData = m_targetData;

		if ( targetData )
		{
			targetData->UnregisterAttacker( m_actor );
		}
		m_targetData.Clear();
	}

	// TODO: Remove previous targeting system (implemented on actor)
	TBaseClass::ClearCombatTarget();
}

Bool CR4BehTreeInstance::IsCombatTargetReachable()
{
	if ( !m_reachabilityQuery )
	{
		m_reachabilityQuery = new CAIReachabilityQuery( m_actor );
	}

	CActor* combatTarget = m_combatTarget.Get();
	if ( combatTarget )
	{
		return m_reachabilityQuery->QueryReachable( combatTarget->GetWorldPositionRef(), m_toleranceDistanceRequiredByCombat, GetLocalTime() );
	}
	
	return false;
}

void CR4BehTreeInstance::OnCombatTargetDestroyed()
{
	m_targetData.Clear();

	TBaseClass::OnCombatTargetDestroyed();
}

Bool CR4BehTreeInstance::IsInCombat() const
{
	return m_isInCombat;
}
void CR4BehTreeInstance::SetIsInCombat( Bool inCombat )
{
	if ( inCombat != m_isInCombat )
	{
		m_isInCombat = inCombat;

		TBaseClass::SetIsInCombat( inCombat );
	}
}

void CR4BehTreeInstance::DescribeTicketsInfo( TDynArray< String >& info )
{
	CCombatDataComponent* combatData = GetCombatData();
	if ( !combatData )
	{
		return;
	}
	const auto& ticketSourceList = combatData->GetTicketSourceList();
	for ( auto it = ticketSourceList.Begin(), end = ticketSourceList.End(); it != end; ++it )
	{
		CName ticketName = it->m_first;
		CTicketSource* ticketSource = it->m_second;

		ticketSource->DisposeTimeoutRequests();

		info.PushBack(
			String::Printf( TXT("Ticket %s: base:%d, requests:%d/%d, acq:%d/%d importance:%d, ovr:%d"),
				ticketName.AsChar(),
				ticketSource->GetBasePool(),
				ticketSource->GetQueuedRequestsCount(),
				ticketSource->GetRequests(),
				ticketSource->GetAcquisitionsCount(),
				ticketSource->GetAcquired(),
				Uint32( ticketSource->GetMinimalImportance() ),
				ticketSource->GetOverridesCount()
			) );
	}
}
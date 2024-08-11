/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionShouldPursueCombatTarget.h"

#include "r4BehTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionShouldPursueCombatTargetDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionShouldPursueCombatTargetInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionShouldPursueCombatTargetInstance::CBehTreeNodeConditionShouldPursueCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaDataPtr( owner )
	, m_testTimeout( 0.f )
	, m_lastDecision( false )
	, m_allowPursueInCombat( def.m_allowPursueInCombat )
	, m_testReachability( def.m_testReachability )
{
	m_allowPursueInDistanceSq = def.m_allowPursueInDistance.GetVal( context );
	m_allowPursueInDistanceSq *= m_allowPursueInDistanceSq;
}

Bool CBehTreeNodeConditionShouldPursueCombatTargetInstance::GuardAreaTest()
{
	CActor* target = m_owner->GetCombatTarget().Get();
	if ( !target )
	{
		return false;
	}

	CBehTreeGuardAreaData* data = m_guardAreaDataPtr.Get();
	CAreaComponent* guardArea = data->GetGuardArea();

	if ( guardArea )
	{
		Bool usePursueArea = false;
		const Vector& targetPos = target->GetWorldPositionRef();

		if ( m_allowPursueInCombat )
		{
			if ( data->HasNoticedTargetAtGuardArea( target ) )
			{
				usePursueArea = true;
			}
			else
			{
				CActor* me = m_owner->GetActor();
				const Vector& myPos = me->GetWorldPositionRef();
				// check if we are very close
				// check if we are in guard area
				if ( (myPos - targetPos).SquareMag3() < m_allowPursueInDistanceSq || (!data->IsRetreating() && !data->IsInGuardArea( me->GetWorldPositionRef(), guardArea  )) )
				{
					usePursueArea = true;
				}
			}
		}

		if ( usePursueArea )
		{
			if ( !data->IsInPursueArea( targetPos, guardArea ) )
			{
				return false;
			}
		}
		else
		{
			if ( !data->IsInGuardArea( targetPos, guardArea ) )
			{
				return false;
			}
		}

		data->NoticeTargetAtGuardArea( target );		
	}

	return true;
}
Bool CBehTreeNodeConditionShouldPursueCombatTargetInstance::ReachabilityTest()
{
	return !m_testReachability || CR4BehTreeInstance::Get( m_owner )->IsCombatTargetReachable();
}

Bool CBehTreeNodeConditionShouldPursueCombatTargetInstance::ConditionCheck()
{
	Float time = m_owner->GetLocalTime();
	if ( m_testTimeout > time )
	{
		return m_lastDecision;
	}
	m_testTimeout = time + 1.33333f;
	
	m_lastDecision = GuardAreaTest() && ReachabilityTest();

	return m_lastDecision;
}
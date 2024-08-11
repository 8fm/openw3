/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeFleeReaction.h"

#include "behTreeInstance.h"
#include "commonGame.h"
#include "gameWorld.h"


const Float CBehTreeNodeFleeReactionInstance::s_updateInterval = 1.f;

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeFleeReactionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{	
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeFleeReactionInstance::CBehTreeNodeFleeReactionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicMoveToInstance( def, owner, context, parent )
	, m_guardAreaDataPtr( owner )
	, m_fleeDistance( def.m_fleeDistance.GetVal( context ) )	
	, m_fleeDistanceSqrt( m_fleeDistance * m_fleeDistance )
	, m_queryRadiusRatio( def.m_queryRadiusRatio )
	, m_surrenderDistance( def.m_surrenderDistance.GetVal( context ) )
	, m_useCombatTarget( def.m_useCombatTarget )
{
}

CNode* CBehTreeNodeFleeReactionInstance::GetTarget()
{
	return m_useCombatTarget
		? m_owner->GetCombatTarget().Get() 
		: m_owner->GetActionTarget().Get();
}

Bool CBehTreeNodeFleeReactionInstance::FindSpot( Bool isAvailabilityTest )
{
	Float currTime = m_owner->GetLocalTime();
	if ( currTime <= m_activationTimeout )
	{
		return false;
	}

	LazyCreateNavigationQuery();

	switch( m_navigationQuery->GetQueryState() )
	{
	case QueryRequest::STATE_SETUP:
		m_navigationQuery->Dispose();
		// no break
	case QueryRequest::STATE_DISPOSED:
		// we need to setup&run query
		{
			CNode* target = GetTarget();

			if ( !target )
			{
				m_activationTimeout = currTime + 5.f;
				return false;
			}

			CActor* actor = m_owner->GetActor();
			const Vector& actorPos = actor->GetWorldPositionRef();
			Vector2 diff = actor->GetWorldPositionRef().AsVector2() - target->GetWorldPositionRef().AsVector2();
			Float dist = diff.Normalize();
			if ( isAvailabilityTest && dist < m_surrenderDistance )
			{
				m_activationTimeout = currTime + 2.f;
				return false;
			}

			CBehTreeGuardAreaData& guardAreaData = *m_guardAreaDataPtr;

			CAreaComponent* guardArea = guardAreaData.GetPursuitArea();
			Float tolerance = 0.f;
			if ( !guardArea )
			{
				guardArea = guardAreaData.GetGuardArea();
				tolerance = guardAreaData.GetPursuitRange();
				if ( !guardArea )
				{
					m_activationTimeout = currTime + 5.f;
					return false;
				}
			}

			CWorld* world = actor->GetLayer()->GetWorld();

			Vector desiredPos = actorPos;
			desiredPos += diff * m_fleeDistance;

			if ( !m_navigationQuery->RefreshSetup( world, actor, guardArea, desiredPos, m_fleeDistance * m_queryRadiusRatio, tolerance ) )
			{
				return false;
			}
			m_navigationQuery->Submit( *world->GetPathLibWorld() );
		}
		if ( isAvailabilityTest )
		{
			m_parent->MarkParentSelectorDirty();
		}
		break;

	case QueryRequest::STATE_ONGOING:
		// query is on the way
		if ( isAvailabilityTest )
		{
			m_parent->MarkParentSelectorDirty();
		}
		break;
	case QueryRequest::STATE_COMPLETED_SUCCESS:
		return true;
	case QueryRequest::STATE_COMPLETED_FAILURE:
		m_navigationQuery->Dispose();
		m_activationTimeout = m_owner->GetLocalTime() + 5.f;
		break;
	default:
		ASSERT( false );
		ASSUME( false );
	}
	return false;
}

Bool CBehTreeNodeFleeReactionInstance::Activate()
{
	if ( !FindSpot() )
	{
		return false;
	}

	m_nextUpdate = m_owner->GetLocalTime() + s_updateInterval;
	
	return Super::Activate();
}

void CBehTreeNodeFleeReactionInstance::CompleteWithFailure()
{	
	Complete( IBehTreeNodeInstance::BTTO_FAILED );
}

void CBehTreeNodeFleeReactionInstance::CompleteWithSuccess()
{
	Complete( IBehTreeNodeInstance::BTTO_SUCCESS );
}


void CBehTreeNodeFleeReactionInstance::LazyCreateNavigationQuery()
{
	if ( !m_navigationQuery )
	{
		m_navigationQuery = new QueryRequest();
	}
}

void CBehTreeNodeFleeReactionInstance::Update()
{
	Float currentTime = m_owner->GetLocalTime();
	if( m_nextUpdate <= currentTime )
	{	
		CNode* target = GetTarget();
		if ( !target )
		{
			Complete( BTTO_FAILED );
			return;
		}
		Vector3 targetPos = target->GetWorldPositionRef().AsVector3();
		Vector3 currPos = m_owner->GetActor()->GetWorldPositionRef().AsVector3();

		Float distSq = ( targetPos - currPos ).SquareMag();

		if ( distSq < m_surrenderDistance*m_surrenderDistance )
		{
			Complete( BTTO_FAILED );
			m_activationTimeout = currentTime + 2.f;
			return;
		}
		else if ( distSq > m_fleeDistanceSqrt )
		{
			Complete( BTTO_SUCCESS );
			m_activationTimeout = currentTime + 2.f;
			return;
		}

		if ( FindSpot() )
		{
			UpdateTargetAndHeading();
		}
		m_nextUpdate = currentTime + s_updateInterval;			
	}
	Super::Update();
}

void CBehTreeNodeFleeReactionInstance::Complete( eTaskOutcome outcome )
{
	// it should be handled by separate delay decorator - but here we have sanity test
	if ( outcome == BTTO_FAILED )
	{
		m_activationTimeout = m_owner->GetLocalTime() + 5.f;
	}

	Super::Complete( outcome );
}

Bool CBehTreeNodeFleeReactionInstance::IsAvailable()
{
	return m_isActive || FindSpot( true );
}

Int32 CBehTreeNodeFleeReactionInstance::Evaluate()
{
	if ( m_isActive || FindSpot( true ) )
	{
		return m_priority;
	}
	return -1;
}

Bool CBehTreeNodeFleeReactionInstance::ComputeTargetAndHeading()
{
	if ( !FindSpot() )
	{
		return false;
	}

	m_target = m_navigationQuery->GetComputedPosition();
	m_heading = 0;

	m_navigationQuery->Dispose();

	return true;
}

/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicGuardRetreat.h"

#include "../engine/pathlibWorld.h"
#include "../engine/areaComponent.h"

#include "aiSpawnTreeParameters.h"
#include "behTreeInstance.h"
#include "movableRepresentationPathAgent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorGuardRetreatDefinition )



//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeDecoratorGuardRetreatInstance::CBehTreeNodeDecoratorGuardRetreatInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaPtr( owner )
	, m_delayReactivation( 0.f )
	, m_isAvailableWhenInPursuitRange( def.m_isAvailableWhenInPursuitRange )
{
	SBehTreeEvenListeningData eChange;
	eChange.m_eventName = CNAME( AI_Change_Guard_Area );
	eChange.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eChange, this );
}

Bool CBehTreeNodeDecoratorGuardRetreatInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	outTarget = m_retreatRequest->GetComputedPosition();
	outHeading = 0.f;

	return true;
}

void CBehTreeNodeDecoratorGuardRetreatInstance::OnDestruction()
{
	SBehTreeEvenListeningData eChange;
	eChange.m_eventName = CNAME( AI_Change_Guard_Area );
	eChange.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eChange, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeDecoratorGuardRetreatInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( AI_Change_Guard_Area ) )
	{
		return true;
	}

	return false;
}

void CBehTreeNodeDecoratorGuardRetreatInstance::Complete( eTaskOutcome outcome )
{
	if ( outcome == BTTO_FAILED )
	{
		m_delayReactivation = m_owner->GetLocalTime() + 2.5f;
		m_retreatRequest->Dispose();
	}

	Super::Complete( outcome );
}

void CBehTreeNodeDecoratorGuardRetreatInstance::LazyCreateRequest()
{
	if( !m_retreatRequest )
	{
		m_retreatRequest = new CQueryReacheableSpotInAreaRequest();
	}
}

Bool CBehTreeNodeDecoratorGuardRetreatInstance::CheckCondition()
{
	if ( m_delayReactivation > m_owner->GetLocalTime() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	CActor* actor = m_owner->GetActor();
	CBehTreeGuardAreaData* data = m_guardAreaPtr.Get();
	CAreaComponent* guardArea = data->GetGuardArea();
	if ( !guardArea )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	const Vector& pos = actor->GetWorldPositionRef();

	if ( m_isActive || m_isAvailableWhenInPursuitRange )
	{
		// Is retreating to guard area
		Bool inGuard = data->IsInGuardArea( pos, guardArea );
		if ( !inGuard )
		{
			return true;
		}
	}
	else
	{
		// Can pursue up to pursue area
		Bool inPursuit = data->IsInPursueArea( pos, guardArea );
		if ( !inPursuit )
		{
			return true;
		}
	}

	DebugNotifyAvailableFail();
	return false;
}

Bool CBehTreeNodeDecoratorGuardRetreatInstance::FindSpot()
{
	LazyCreateRequest();
	switch( m_retreatRequest->GetQueryState() )
	{
	case CQueryReacheableSpotInAreaRequest::STATE_COMPLETED_FAILURE:
		m_delayReactivation = m_owner->GetLocalTime() + 5.f;
		m_retreatRequest->Dispose();
		break;
	case CQueryReacheableSpotInAreaRequest::STATE_DISPOSED:
	case CQueryReacheableSpotInAreaRequest::STATE_SETUP:
		{
			CAreaComponent* guardArea = m_guardAreaPtr->GetGuardArea();
			if ( !guardArea )
			{
				return false;
			}
			CActor* actor = m_owner->GetActor();
			CWorld* world = actor->GetLayer()->GetWorld();
			// we assume guard area is not null as it just past CheckCondition()
			m_retreatRequest->RefreshSetup( world, actor, guardArea, guardArea->GetBoundingBox().GetMassCenter() );
			m_retreatRequest->Submit( *world->GetPathLibWorld() );

		}
		break;
	case CQueryReacheableSpotInAreaRequest::STATE_ONGOING:
		break;
	case CQueryReacheableSpotInAreaRequest::STATE_COMPLETED_SUCCESS:
		return true;
	}
	return false;
}
Bool CBehTreeNodeDecoratorGuardRetreatInstance::IsAvailable()
{
	return CheckCondition() && FindSpot();
}

Int32 CBehTreeNodeDecoratorGuardRetreatInstance::Evaluate()
{
	if ( CheckCondition() && FindSpot() )
	{
		return m_priority;
	}
	return -1;
}

Bool CBehTreeNodeDecoratorGuardRetreatInstance::Activate()
{
	if ( !FindSpot() )
	{
		return false;
	}
	CBehTreeGuardAreaData* data = m_guardAreaPtr.Get();
	data->ClearTargetNoticedAtGuardArea();
	if ( Super::Activate() )
	{
		data->SetIsRetreating( true );
		return true;
	}
	return false;
}

void CBehTreeNodeDecoratorGuardRetreatInstance::Deactivate()
{
	Super::Deactivate();
	m_guardAreaPtr->SetIsRetreating( false );
}
/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDecoratorFindLandingSpot.h"

#include "../../common/engine/pathlibTerrain.h"
#include "../../common/engine/pathlibWorld.h"

#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/game/movementAdjustor.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeLandingDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeTakeOffDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeFindLandingSpotDecoratorDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeLandingDecoratorInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeLandingDecoratorInstance::CBehTreeNodeLandingDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_landingMaxHeight( def.m_landingMaxHeight.GetVal( context ) )
	, m_landingForwardOffset( def.m_landingForwardOffset.GetVal( context ) )
	, m_tolerationDistance( def.m_tolerationDistance.GetVal( context ) )
	, m_spotValidityTimeout( 0.f )
	, m_spotComputationSuccess( false )
	, m_isAdjustingPosition( false )
	, m_landed( false )
{
}

CName CBehTreeNodeLandingDecoratorInstance::AnimEventAdjustName()
{
	return CNAME( AI_Adjust );
}

CName CBehTreeNodeLandingDecoratorInstance::AnimEventLandName()
{
	return CNAME( AI_Land );
}

Bool CBehTreeNodeLandingDecoratorInstance::ShouldAdjustPosition()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	// test if we have physics data streamed from around
	if ( !mac->IsPhysicalRepresentationEnabled() )
	{
		return true;
	}
	// test if we are currently on a proper way to desired spot
	CPathAgent* pathAgent = mac->GetPathAgent();
	CPathLibWorld* pathlib = pathAgent->GetPathLib();
	PathLib::AreaId& areaId = pathAgent->CacheAreaId();
	Vector3 position = pathAgent->GetPosition();
	Float minZ = position.Z - m_landingMaxHeight;
	Float maxZ = position.Z;
	if ( !pathlib->ComputeHeightTop( position.AsVector2(), minZ, maxZ, position.Z, areaId ) )
	{
		return true;
	}

	if ( !pathlib->TestLocation( areaId, position, pathAgent->GetCollisionFlags() ) )
	{
		return true;
	}

	return false;
}

Bool CBehTreeNodeLandingDecoratorInstance::ComputeSpot()
{
	Float time = m_owner->GetLocalTime();
	if ( time < m_spotValidityTimeout )
	{
		return m_spotComputationSuccess;
	}

	m_spotComputationSuccess = false;

	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}
	CPathAgent* pathAgent = mac->GetPathAgent();
	CPathLibWorld* pathlib = pathAgent->GetPathLib();

	const Vector3& pos = pathAgent->GetPosition();
	Vector2 posForward = mac->GetWorldForward().AsVector2();

	// first find suitable navigation spot
	Vector3 desiredSpot = pos;
	desiredSpot.AsVector2() += posForward * m_landingForwardOffset;
	PathLib::AreaId& areaId = pathAgent->CacheAreaId();
	Float minZ = pos.Z - m_landingMaxHeight;
	Float maxZ = pos.Z;
	if ( !pathlib->ComputeHeightTop( desiredSpot.AsVector2(), minZ, maxZ, desiredSpot.Z, areaId ) )
	{
		return false;
	}

	if ( !pathlib->TestLocation( pathAgent->CacheAreaId(), desiredSpot, pathAgent->GetCollisionFlags() ) )
	{
		if ( pathlib->FindSafeSpot( areaId, desiredSpot, m_tolerationDistance, pathAgent->GetPersonalSpace(), desiredSpot, &minZ, &maxZ ) )
		{
			return false;
		}
	}

	m_computedSpot = desiredSpot;
	m_spotComputationSuccess = true;
	
	return true;
}


Bool CBehTreeNodeLandingDecoratorInstance::IsAvailable()
{
	if ( !m_isActive && !ComputeSpot() )
	{
		DebugNotifyAvailableFail();

		return false;
	}

	return Super::IsAvailable();
}

Int32 CBehTreeNodeLandingDecoratorInstance::Evaluate()
{
	if ( !m_isActive && !ComputeSpot() )
	{
		return -1;
	}

	return Super::Evaluate();
}

Bool CBehTreeNodeLandingDecoratorInstance::Activate()
{
	if ( !ComputeSpot() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_landed = false;
	return Super::Activate();
}

Bool CBehTreeNodeLandingDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == AnimEventAdjustName() && e.m_eventType != BTET_GameplayEvent && e.m_eventType != BTET_AET_DurationEnd )
	{
		if ( !m_isAdjustingPosition && ShouldAdjustPosition() )
		{
			CActor* actor = m_owner->GetActor();
			CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
			CMovementAdjustor* adjustor = mac->GetMovementAdjustor();
			adjustor->CreateNewRequest( CNAME( Landing ) )
				->BindToEventAnimInfo( *e.m_animEventData.m_eventAnimInfo )
				->AdjustLocationVertically( true )
				->SlideTo( m_computedSpot );

			m_isAdjustingPosition = true;
		}
		

		return false;
	}
	if ( e.m_eventName == AnimEventLandName() )
	{
		CBehTreeFlightData::DisableFlightState( m_owner->GetActor() );
		m_landed = true;

		return false;
	}

	return Super::OnEvent( e );
}

void CBehTreeNodeLandingDecoratorInstance::Complete( eTaskOutcome outcome )
{
	if ( !m_landed && outcome == BTTO_SUCCESS )
	{
		CBehTreeFlightData::DisableFlightState( m_owner->GetActor() );
	}

	Super::Complete( outcome );
}
///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTakeOffDecoratorInstance
///////////////////////////////////////////////////////////////////////////////
CName CBehTreeNodeTakeOffDecoratorInstance::AnimEventTakeOffName()
{
	return CNAME( AI_TakeOff );
}

Bool CBehTreeNodeTakeOffDecoratorInstance::Activate()
{
	m_hadTakeOff = false;
	return Super::Activate();
}

Bool CBehTreeNodeTakeOffDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == AnimEventTakeOffName() )
	{
		CBehTreeFlightData::EnableFlightState( m_owner->GetActor() );
		m_hadTakeOff = true;

		return false;
	}

	return Super::OnEvent( e );
}

void CBehTreeNodeTakeOffDecoratorInstance::Complete( eTaskOutcome outcome )
{
	if ( !m_hadTakeOff && outcome == BTTO_SUCCESS )
	{
		CBehTreeFlightData::EnableFlightState( m_owner->GetActor() );
	}

	Super::Complete( outcome );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeFindLandingSpotDecoratorInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeFindLandingSpotDecoratorInstance::CBehTreeNodeFindLandingSpotDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaDataPtr( owner )
	, m_desiredLandingSpotDistance( def.m_desiredLandingSpotDistance.GetVal( context ) )
	, m_findSpotDistance( def.m_findSpotDistance.GetVal( context ) )
{

}

CBehTreeNodeFindLandingSpotDecoratorInstance::EQueryStatus CBehTreeNodeFindLandingSpotDecoratorInstance::StartQuery()
{
	// compute desired spot
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return STATUS_FAILURE;
	}

	CPathAgent* pathAgent = mac->GetPathAgent();
	CPathLibWorld* pathlib = pathAgent->GetPathLib();

	const Vector& pos = mac->GetWorldPositionRef();
	Vector heading = mac->GetWorldForward();

	Vector3 desiredSpot = pos.AsVector3();
	desiredSpot.AsVector2() += heading.AsVector2() * m_desiredLandingSpotDistance;
	
	PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;

	pathlib->ComputeHeightTop( desiredSpot.AsVector2(), pos.Z - 20.f, pos.Z + 2.f, desiredSpot.Z, areaId );

	// first of all check if we are flying over terrain. If so, its quite cool as we can guess 
	//PathLib::CTerrainAreaDescription* terrainArea = pathlib->GetTerrainAreaAtPosition( pos.AsVector3() );
	//Float terrainHeight;
	//terrainArea->ComputeHeight( pos.AsVector2(), terrainHeight );
	

	SClosestSpotFilter filter;
	filter.m_maxDistance = m_findSpotDistance;
	filter.m_zDiff = 4.f;
	filter.m_awayFromCamera = false;
	filter.m_onlyReachable = false;
	filter.m_noRoughTerrain = true;
	filter.m_noInteriors = true;
	filter.m_limitToBaseArea = false;
	filter.m_limitedPrecision = true;

	CAreaComponent* guardArea = m_guardAreaDataPtr->GetGuardArea();			// possibly nullptr

	CBehTreePositioningRequest* data = m_requestPtr.Get();
	CPositioningFilterRequest* request = data->GetQuery();

	if ( !request->Setup( filter, GGame->GetActiveWorld(), desiredSpot, actor, &desiredSpot, guardArea ) )
	{
		return STATUS_FAILURE;
	}
	data->SetValidFor( m_owner->GetLocalTime() + m_queryValidFor );
	
	return STATUS_IN_PROGRESS;
}
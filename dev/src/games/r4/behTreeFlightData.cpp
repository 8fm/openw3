/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeFlightData.h"

#include "../../common/engine/areaComponent.h"
#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/behTreeInstance.h"
#include "../../common/game/movementAdjustor.h"
#include "../../common/game/movingPhysicalAgentComponent.h"

#include "behTreeNodeAtomicFlight.h"
#include "r4Enums.h"
#include "volumePathManager.h"


IMPLEMENT_ENGINE_CLASS( CBehTreeFlightData )

RED_DEFINE_STATIC_NAME( ChangeStance )

CBehTreeFlightData::CBehTreeFlightData()
	: m_pathManager( GCommonGame->GetSystem< CVolumePathManager >() )
	, m_lastPositionCoords( 0xffff, 0xffff, 0xffff )
	, m_currentPathDestinationCoords( 0xffff, 0xffff, 0xffff )
	, m_isEmergencyMode( false )
	, m_isPathFollowing( false )
	, m_forceReachTarget( false )
	, m_currentDestination( Vector::ZEROS )
	, m_pathDistanceFromDesiredTarget( 0.f )
{
}

void CBehTreeFlightData::Activate( CActor* actor )
{
	m_lastProperPos = actor->GetWorldPositionRef().AsVector3();
	m_currentDestination = m_lastProperPos + actor->GetWorldForward().AsVector3();
}

void CBehTreeFlightData::Deactivate( CActor* actor )
{
	m_forceReachTarget = false;
}

// Initial implementation. CTRL+C CTRL+V from scripts VolumetricMove::CalculateBehaviorVariables()
void CBehTreeFlightData::SetupBehaviorVariables( CActor* actor, const Vector& destination )
{
	Float flySpeed, flyPitch, flyYaw;

	Vector actorPos = actor->GetWorldPositionRef();
	Vector actorHeading = actor->GetWorldForward();

	Vector actorToDestination = destination - actorPos;
	Float actorToDestinationDistance = actorToDestination.Mag3();
	Vector actorToDestinationNormalized = actorToDestinationDistance > 0.f ? actorToDestination * ( 1.f / actorToDestinationDistance ) : Vector::ZEROS;
	Float COS60 = 0.5f;

	// Calculate Fly Speed
	if ( actorToDestinationNormalized.Dot3( actorHeading ) < COS60 )
	{
		// lower speed when angle is greater than
		flySpeed = 0.5f;
	}
	else
	{
		flySpeed = 1.f;
	}

	// Calculate Pitch
	// acos( Dot( actorToDestinationNormalized, actorToDestinationNormalized2D ) ) is basically 1 - Z^2 
	flyPitch = acos( 1.f - actorToDestinationNormalized.Z * actorToDestinationNormalized.Z );

	flyPitch = RAD2DEG( flyPitch )/90;

	if ( m_isEmergencyMode )
	{
		flyPitch *= 2.f;
	}
	//Float turnSpeedScale = 2.75f;
	//flyPitch = flyPitch * powf( turnSpeedScale, flyPitch );
	flyPitch = Min( flyPitch, 1.f );

	if ( actorToDestinationNormalized.Z < 0.f )
	{
		flyPitch = - flyPitch; 
	}

	// Calculate Yaw
	Float headingActorToDestination = EulerAngles::YawFromXY( actorToDestination.X, actorToDestination.Y );
	Float heading = EulerAngles::YawFromXY( actorHeading.X, actorHeading.Y );

	flyYaw = EulerAngles::AngleDistance( heading, headingActorToDestination );


	if ( m_isEmergencyMode )
	{
		flyYaw *= 2.f;
	}

	flyYaw = flyYaw / 180.f;

	// ??
	//flyYaw = flyYaw * powf( turnSpeedScale , Abs( flyYaw ) );

	Clamp( flyYaw, -1.f, 1.f );

	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	auto* behaviorStack = mac->GetBehaviorStack();

	behaviorStack->SetBehaviorVariable( CNAME( FlyYaw ), flyYaw );
	behaviorStack->SetBehaviorVariable( CNAME( FlyPitch ), flyPitch );
	behaviorStack->SetBehaviorVariable( CNAME( FlySpeed ), flySpeed );
}

Bool CBehTreeFlightData::PlotPath( IBehTreeNodeAtomicFlightInstance* node, const Vector3& sourcePos, const Vector3& desiredPosition )
{
	CAreaComponent* area = node->GetAreaEncompassingMovement();
	if ( area && area->TestPointOverlap( sourcePos ) )
	{
		CVolumePathManager::CClosestAcceptablePointInBoundingsRequest request( sourcePos, m_desiredPosition, m_detailedPath, CVolumePathManager::IRequest::WATER_ABOVE, FLT_MAX, area->GetBoundingBox() );
		if ( !m_pathManager->PlotPath( request ) )
		{
			return false;
		}
		m_pathDistanceFromDesiredTarget = request.GetResultDistanceFromDesiredPosition();
	}
	else
	{
		CVolumePathManager::CClosestAcceptablePointRequest request( sourcePos, m_desiredPosition, m_detailedPath, CVolumePathManager::IRequest::WATER_ABOVE, FLT_MAX );
		if ( !m_pathManager->PlotPath( request ) )
		{
			return false;
		}
		m_pathDistanceFromDesiredTarget = request.GetResultDistanceFromDesiredPosition();
	}
	
	if ( !m_pathManager->SimpifyPath( m_detailedPath, m_currentPath ) || m_currentPath.Size() < 2 )
	{
		return false;
	}
	m_currentPath.PopBackFast();

	return true;
}

Bool CBehTreeFlightData::UpdateFlight( IBehTreeNodeAtomicFlightInstance* node )
{
	// some general processing data
	CBehTreeInstance* owner = node->GetOwner();
	CActor* actor = owner->GetActor();
	const Vector& actorPos = actor->GetWorldPositionRef().AsVector3();
	Vector3 sourcePos = actorPos;
	Coordinate currentCoords = m_pathManager->CoordinateFromPosition( sourcePos );
	Coordinate destinationCoords = m_pathManager->CoordinateFromPosition( m_desiredPosition );

	// update emergency mode
	if ( currentCoords != m_lastPositionCoords )
	{
		// check for emergency mode
		Uint32 availableNeighbours;
		// get node at coordinate
		if ( m_pathManager->GetCollisionRepulsion( sourcePos, m_emergencyRepulsion, availableNeighbours ) )
		{
			if ( (availableNeighbours & CVolumePathManager::NODE_AVAILABLE) == 0 )
			{
				sourcePos = m_lastProperPos;
				m_isEmergencyMode = true;
			}
			else
			{
				m_lastProperPos = sourcePos;
				m_lastPositionCoords = currentCoords;
			}

			if ( ( availableNeighbours & CVolumePathManager::ALL_DIRECTIONS ) == CVolumePathManager::ALL_DIRECTIONS )
			{
				// possibly turn off emergency mode
				m_isEmergencyMode = false;
			}
			else
			{
				// turn on emergency mode
				m_isEmergencyMode = true;
			}
		}
	}

	// update path finding
	Bool computePath = false;
	if ( m_currentPath.Empty() || destinationCoords != m_currentPathDestinationCoords )
	{
		computePath = true;
	}
	else
	{
		computePath = m_pathManager->IsPathfindingNeeded( sourcePos, m_currentPath[ 0 ] );
	}

	if ( computePath )
	{
		// compute new path

		// test for common trivial case
		if ( !m_pathManager->IsPathfindingNeeded( sourcePos, m_desiredPosition ) )
		{
			m_currentPath.ResizeFast( 1 );
			m_currentPath[ 0 ] = m_desiredPosition;
			m_currentDestination = m_desiredPosition;
			m_pathDistanceFromDesiredTarget = 0.f;
		}
		else
		{
			if ( !PlotPath( node, sourcePos, m_desiredPosition  ) )
			{
				return false;
			}
			m_currentDestination = m_currentPath.Back();
		}
		m_currentPathDestinationCoords = destinationCoords;
	}
	else
	{
		// update path following
		const Float ACCEPT_WAYPOINT_DISTANCE = 2.f;

		while ( m_currentPath.Size() > 1 && !m_pathManager->IsPathfindingNeeded( sourcePos, m_currentPath[ m_currentPath.Size() - 2 ] ) )
		{
			m_currentPath.PopBackFast();
		}

		if ( m_currentPath.Empty() )
		{
			// out of path?
			return false;
		}

		m_currentDestination = m_currentPath.Back();
	}
	
	// update steering
	SetupBehaviorVariables( actor, m_currentDestination );

	// update emergency mode
	if ( m_isEmergencyMode )
	{
		if ( !m_emergencyRepulsion.IsAlmostZero() )
		{
			// absolute tmpshit

			//const Float MIN_TURN_RATE = 0.5f;
			const Float REPULSION_RATE = 1.f;

			CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
			CMovementAdjustor* movementAdjustor = mac->GetMovementAdjustor();
			
			movementAdjustor->AddOneFrameTranslationVelocity( m_emergencyRepulsion * REPULSION_RATE );
		}
	}

	return true;
}

void CBehTreeFlightData::SetDesiredPosition( const Vector& newDestination )
{
	m_desiredPosition = newDestination.AsVector3();
}

void CBehTreeFlightData::EnableFlightState( CActor* actor )
{
	CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
	if ( !mac )
	{
		return;
	}

	mac->SetPhysicalRepresentationRequest( CMovingPhysicalAgentComponent::Req_On, CMovingAgentComponent::LS_Scripts );
	mac->SetAnimatedMovement( true );
	mac->SnapToNavigableSpace( false );
	mac->SetGravity( false );

	// hack! call npc script function
	{
		Bool stateChanged = false;
		ENpcStance stance = NS_Fly;
		CallFunctionRet< Bool >( actor, CNAME( ChangeStance ), stance, stateChanged );
	}
}

void CBehTreeFlightData::DisableFlightState( CActor* actor )
{
	CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
	if ( !mac )
	{
		return;
	}

	mac->SetPhysicalRepresentationRequest( CMovingPhysicalAgentComponent::Req_Off, CMovingAgentComponent::LS_Scripts );
	mac->SetAnimatedMovement( false );
	mac->SnapToNavigableSpace( true );
	mac->SetGravity( true );

	// hack! call npc script function
	{
		Bool stateChanged = false;
		ENpcStance stance = NS_Normal;
		CallFunctionRet< Bool >( actor, CNAME( ChangeStance ), stance, stateChanged );
	}
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeFlightData::CInitializer
///////////////////////////////////////////////////////////////////////////////
CName CBehTreeFlightData::CInitializer::GetItemName() const
{
	return CNAME( FLIGHT_DATA );
}

IRTTIType* CBehTreeFlightData::CInitializer::GetItemType() const
{
	return CBehTreeFlightData::GetStaticClass();
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeFlightDataPtr
///////////////////////////////////////////////////////////////////////////////
CBehTreeFlightDataPtr::CBehTreeFlightDataPtr( CAIStorage* storage )
	: Super( CBehTreeFlightData::CInitializer(), storage )
{

}


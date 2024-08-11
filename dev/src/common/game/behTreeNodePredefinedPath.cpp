#include "build.h"
#include "behTreeNodePredefinedPath.h"

#include "../core/gameSave.h"

#include "../engine/tagManager.h"
#include "../engine/pathComponent.h"
#include "../engine/renderFrame.h"
#include "../engine/pathlibAgent.h"

#include "behTreeInstance.h"
#include "movableRepresentationPathAgent.h"
#include "movementGoal.h"
#include "explorationScriptSupport.h"
#include "movingPhysicalAgentComponent.h"
#include "expComponent.h"
#include "doorComponent.h"

RED_DEFINE_STATIC_NAME( isInExploration )
RED_DEFINE_STATIC_NAME( beforeExplorationPosition )
RED_DEFINE_STATIC_NAME( beforeExplorationRotation )


const Float CBehTreeNodePredefinedPathInstance:: MAX_EXPLORATION_PATH_DISTANCE_SQRT = 1.0f * 1.0f;
const Float CBehTreeNodePredefinedPathInstance:: MAX_EXPLORATION_NPC_DISTANCE_SQRT = 1.f * 1.f;

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathDefinition
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodePredefinedPathDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePredefinedPathInstance::CBehTreeNodePredefinedPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeCustomSteeringInstance( def, owner, context, parent )
	, m_pathName( def.m_pathName.GetVal( context ) )
	, m_pathMargin( def.m_pathMargin.GetVal( context ) )
	, m_tolerance( def.m_tolerance.GetVal( context ) ) 
	, m_defaultTolerance( def.m_tolerance.GetVal( context ) ) 
	, m_upThePath( def.m_upThePath.GetVal( context ) )
	, m_arrivalDistance( def.m_arrivalDistance.GetVal( context ) )
	, m_distanceFromStart( 0.0f )
	, m_useExplorations( true )// def.m_useExplorations.GetVal( context ) )
	, m_isInExploration( false )
	, m_explorationAction( NULL )
	, m_lastUsedExploration( NULL )
	, m_timeToSwitchOnCollisionAfterExploration( -1.0f )
	, m_lastExplorationUsageTime( 0 )
	, m_previousInteractionPriority( InteractionPriorityTypeNotSet )
{
	
}

Bool CBehTreeNodePredefinedPathInstance::Activate()
{
	CPathComponent* pathComponent = m_pathComponent.Get();
	if ( !pathComponent )
	{
		CNode* node = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_pathName );
		if ( !node )
		{
			// invalid tag
			DebugNotifyActivationFail();
			return false;
		}
		if ( node->IsA< CPathComponent >() )
		{
			pathComponent = static_cast< CPathComponent* >( node );
		}
		else
		{
			CEntity* entity = Cast< CEntity >( node );
			if ( entity )
			{
				pathComponent = entity->FindComponent< CPathComponent >();
			}
		}

		if ( !pathComponent || pathComponent->GetCurve().Size() == 0 )
		{
			// no path component in given node
			DebugNotifyActivationFail();
			return false;
		}
		m_pathComponent = pathComponent;
	}

	m_pathEdgeIdx = 0;
	m_pathEdgeAlpha = 0.f;
	
	CActor* actor = m_owner->GetActor();

	pathComponent->GetClosestPointOnPath( actor->GetWorldPositionRef(), m_pathEdgeIdx, m_pathEdgeAlpha );

	m_isCompleted	= false;
	m_isSuccess		= false;

	SMultiCurve& curve = pathComponent->GetCurve();
	curve.GetAbsoluteControlPointPosition( m_upThePath ? ( curve.Size() - 1 ) : 0, m_destinationPosition );

	// E3 2013 DEMO HACK BEGIN
	CMovingAgentComponent* movingAgent = actor->GetMovingAgentComponent();
	if ( movingAgent )
	{
		movingAgent->GetPathAgent()->AddCollisionFlags( PathLib::CT_NO_OBSTACLES );
	}
	// E3 2013 DEMO HACK END
	
	if( movingAgent )
	{						
		m_wasSnappedOnActivate = movingAgent->IsSnapedToNavigableSpace();
		movingAgent->SnapToNavigableSpace( false );			

		CMovingPhysicalAgentComponent* movingPhysicalAgent = Cast< CMovingPhysicalAgentComponent >( movingAgent );
		if ( movingPhysicalAgent )
		{
			m_previousInteractionPriority = movingPhysicalAgent->GetInteractionPriority();
			movingPhysicalAgent->SetInteractionPriority( InteractionPriorityTypeMax );
		}
	}

	m_movementSwitchedBackToWalking = false;

	RestoreExplorationState();

	actor->FreezeOnFailedPhysicalRequest( true );

	return Super::Activate();
}

void CBehTreeNodePredefinedPathInstance::RenewPathFollow()
{
	CPathComponent* pathComponent = m_pathComponent.Get();
	if ( !pathComponent )
	{
		CNode* node = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_pathName );
		if ( !node )
		{
			// invalid tag
			DebugNotifyActivationFail();
			return;
		}
		if ( node->IsA< CPathComponent >() )
		{
			pathComponent = static_cast< CPathComponent* >( node );
		}
		else
		{
			CEntity* entity = Cast< CEntity >( node );
			if ( entity )
			{
				pathComponent = entity->FindComponent< CPathComponent >();
			}
		}

		if ( !pathComponent || pathComponent->GetCurve().Size() == 0 )
		{
			// no path component in given node
			DebugNotifyActivationFail();
			return;
		}
		m_pathComponent = pathComponent;
	}

	m_pathEdgeIdx = 0;
	m_pathEdgeAlpha = 0.f;

	CActor* actor = m_owner->GetActor();

	pathComponent->GetClosestPointOnPath( actor->GetWorldPositionRef(), m_pathEdgeIdx, m_pathEdgeAlpha );

	m_isCompleted = false;
	m_isSuccess = false;

	SMultiCurve& curve = pathComponent->GetCurve();
	curve.GetAbsoluteControlPointPosition( m_upThePath ? ( curve.Size() - 1 ) : 0, m_destinationPosition );

	// E3 2013 DEMO HACK BEGIN
	CMovingAgentComponent* movingAgent = actor->GetMovingAgentComponent();
	if ( movingAgent )
	{
		movingAgent->GetPathAgent()->AddCollisionFlags( PathLib::CT_NO_OBSTACLES );
	}

	if ( !actor->ActionCustomSteer( this, GetMoveType(), m_moveSpeed, MFA_EXIT ) )
	{
		DebugNotifyActivationFail();
		return;
	}
}

void CBehTreeNodePredefinedPathInstance::Deactivate()
{
	// E3 2013 DEMO HACK BEGIN
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* movingAgent = actor->GetMovingAgentComponent();
	if ( movingAgent )
	{
		CPathAgent *const pathAgent = movingAgent->GetPathAgent();
		if ( pathAgent )
		{
			pathAgent->RemoveCollisionFlags( PathLib::CT_NO_OBSTACLES );
		}
	}
	// E3 2013 DEMO HACK END

	if( m_isInExploration || m_timeToSwitchOnCollisionAfterExploration > 0.0f )
	{					
		DisableExplorationWalking();
	}

	if( movingAgent )
	{
		if ( m_wasSnappedOnActivate )
		{
			movingAgent->SnapToNavigableSpace( true );
		}

		CMovingPhysicalAgentComponent* movingPhysicalAgent = Cast< CMovingPhysicalAgentComponent >( movingAgent );
		if ( movingPhysicalAgent && m_previousInteractionPriority != movingPhysicalAgent->GetInteractionPriority() )
		{
			movingPhysicalAgent->SetInteractionPriority( m_previousInteractionPriority );
		}
	}

	actor->FreezeOnFailedPhysicalRequest( false );

	Super::Deactivate();
}

void CBehTreeNodePredefinedPathInstance::Update()
{	
	if( m_isUsingDoors )
	{
		UpdateDoorsMovement();
	}

	if( m_isInExploration )
	{
		UpdateExploration();
	}
	else
	{
		SwitchToExplorationIfNeeded();
	}

	if( m_timeToSwitchOnCollisionAfterExploration > 0 && m_timeToSwitchOnCollisionAfterExploration < m_owner->GetLocalTime() )
	{
		DisableExplorationWalking();		
	}

	// checking if the actor has reached the end :
	if ( m_isCompleted )
	{
		Complete( m_isSuccess ? BTTO_SUCCESS : BTTO_FAILED );
	}	
}

void CBehTreeNodePredefinedPathInstance::RestoreExplorationState()
{
	if( m_isInExploration && m_restoreExplorationState )
	{
		m_restoreExplorationState	= false;
		m_isInExploration			= false;

		CActor* ownerActor = m_owner->GetActor();	
		ownerActor->Teleport( m_beforeExplorationPosition, m_beforeExplorationRotation );	
	}
}

void CBehTreeNodePredefinedPathInstance::SwitchToExplorationIfNeeded()
{
	if( !m_useExplorations )
		return;

	if( m_lastExplorationUsageTime  + 1 < m_owner->GetLocalTime() )
	{
		m_lastUsedExploration = nullptr;
	}

	SExplorationQueryContext eContext;
	SExplorationQueryToken explorationToken = GCommonGame->QueryExplorationSync( m_owner->GetActor(), eContext );

	if( explorationToken.IsValid() && m_lastUsedExploration != explorationToken.GetExploration() )
	{	

		CPathComponent* pathComponent = m_pathComponent.Get();

		if( !pathComponent )
		{
			return;
		}
			
		const IExploration* expCmp = explorationToken.GetExploration();
		Matrix mat;
		Vector p0, p1;
		expCmp->GetMatWS( mat );
		expCmp->GetEdgeWS( p0, p1 );
		const Vector& expCenter = mat.GetTranslationRef();

		Vector3 closestOnPath = pathComponent->GetClosestPointOnPath( expCenter );
		Vector3 distanceVec;
		{
			Vector2 closestPointOnExploration;
			MathUtils::GeometryUtils::DistancePointToLine2D( closestOnPath.AsVector2(), p0.AsVector2(), p1.AsVector2(), closestPointOnExploration );
			distanceVec.AsVector2() = closestOnPath.AsVector2() - closestPointOnExploration;
		}
		
		distanceVec.Z = closestOnPath.Z - expCenter.Z;

		float distanceSqr = distanceVec.AsVector2().SquareMag();

		static const Float maxExplorationToleranceAffectingDistanceSq = 4.f;
		m_tolerance = Min( 1.f, distanceSqr / maxExplorationToleranceAffectingDistanceSq ) * m_defaultTolerance;

		if( Abs( distanceVec.Z ) >  3.2f )
		{
			return;
		}

		if( distanceSqr > MAX_EXPLORATION_PATH_DISTANCE_SQRT )
		{
			return;
		}

		CActor* ownerActor = m_owner->GetActor();

		const Vector& npcPos = ownerActor->GetWorldPositionRef();
		distanceSqr = MathUtils::GeometryUtils::DistanceSqrPointToLine2D( npcPos.AsVector2(), p0.AsVector2(), p1.AsVector2() );
		if( distanceSqr > MAX_EXPLORATION_NPC_DISTANCE_SQRT )
		{
			return;
		}

		// determine if path crossing is in front of us
		Vector npcPositionOnPath = pathComponent->CorrectPathPoint( npcPos, m_pathEdgeIdx, m_pathEdgeAlpha );

		Vector2 curveTangent2d;
		SMultiCurvePosition npcCurvePos;
		npcCurvePos.m_edgeIdx = m_pathEdgeIdx;
		npcCurvePos.m_edgeAlpha = m_pathEdgeAlpha;
		SMultiCurve& pathCurve = pathComponent->GetCurve();
		pathCurve.CalculateAbsolute2DTangentFromCurveDirection( npcCurvePos, curveTangent2d );

		if ( curveTangent2d.AsVector2().IsAlmostZero() )
		{
			return;
		}

		if ( !m_upThePath )
		{
			curveTangent2d = -curveTangent2d;
		}
		if ( ( p1.AsVector2() - p0.AsVector2() ).IsAlmostZero( 0.1f ) )
		{
			// threat exploration as a point
			Vector2 expDir = expCenter.AsVector2() - npcPositionOnPath.AsVector2();
			// the path is behind us
			if ( expDir.Dot( curveTangent2d ) < 0.f )
			{
				return;
			}
		}
		else
		{
			Vector2 rearPos = npcPos.AsVector2() - curveTangent2d;
			Vector2 frontPos = npcPos.AsVector2() + curveTangent2d;

			Float ratioEdge, ratioPath;
			if ( !MathUtils::GeometryUtils::TestIntersectionLineLine2D( p0, p1, frontPos, rearPos, 0, 1, ratioEdge, ratioPath ) )
			{
				return;
			}
			// we are not hitting edge!
			if ( ratioEdge < NumericLimits< Float >::Epsilon() || ratioEdge > 1.f - NumericLimits< Float >::Epsilon() )
			{
				// legacy compatibility
				if ( ( closestOnPath.AsVector2() - expCenter.AsVector2() ).SquareMag() > MAX_EXPLORATION_PATH_DISTANCE_SQRT )
				{
					return;
				}
				//Vector2 spot = rearPos.AsVector2() + (npcTangent.AsVector2()) * ratioPath * 2.f;
				//if ( ( spot - npcPos.AsVector2() ).SquareMag() > MAX_EXPLORATION_NPC_DISTANCE_SQRT * (2.f*2.f) )
				//{
				//	return;
				//}
			}
			// the path is behind us
			if ( ratioPath < 0.5f )
			{
				return;
			}
		}

		// test if are meant to cross this madafakin curve
		{
			const Float CURVEPOINT_DIST_LIMIT_SQ = 3.f * 3.f;

			Bool isOk = false;
			Bool isEndOfPath = false;
			Vector2 segStart = p0.AsVector2();
			Vector2 segDiff = p1.AsVector2() - p0.AsVector2();
			SMultiCurvePosition curvePos = npcCurvePos;

			Float plySide = segDiff.CrossZ( npcPositionOnPath.AsVector2() - segStart );

			// iterate through path points, until we are far enough from the npc curve position or
			// path point is on other side of exploration
			for ( Float testDistance = (m_upThePath ? 1.f : -1.f); !isEndOfPath; testDistance += ( m_upThePath ? 1.f : -1.f )  )
			{
				Vector futurePos;
				pathCurve.GetPointOnCurveInDistance( curvePos, testDistance, futurePos, isEndOfPath );
				
				// combine cross products, and see if they have opposite signs
				Float crossZ = segDiff.CrossZ( futurePos.AsVector2() - segStart );
				if ( crossZ * plySide <= 0.f )
				{
					isOk = true;
					break;
				}

				Float dist2DSq = ( futurePos.AsVector2() - npcPositionOnPath.AsVector2() ).SquareMag();
				if ( dist2DSq > CURVEPOINT_DIST_LIMIT_SQ )
				{
					break;
				}
			}

			if ( !isOk )
			{
				return;
			}
		}
	

		ownerActor->ActionTraverseExploration( explorationToken );
		m_explorationAction	= ownerActor->GetActionExploration();		

		m_traverser = m_explorationAction->GetTraverser();
		if( m_traverser.Get() )
		{
			m_isInExploration			= true;
			m_beforeExplorationPosition = ownerActor->GetWorldPosition();
			m_beforeExplorationRotation = ownerActor->GetWorldRotation();			
			m_lastUsedExploration		= explorationToken.GetExploration();
			EnableExplorationWalking();
			m_prevLocalTime = m_owner->GetLocalTime();
		}		
	}
	else
	{
		m_tolerance = m_defaultTolerance;
	}
}

Bool CBehTreeNodePredefinedPathInstance::IsSavingState() const
{	
	return m_isInExploration;
}

void CBehTreeNodePredefinedPathInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Bool >( CNAME( isInExploration ), m_isInExploration );
	writer->WriteValue< Vector3 >( CNAME( beforeExplorationPosition ), m_beforeExplorationPosition);
	writer->WriteValue< EulerAngles >( CNAME( beforeExplorationRotation ), m_beforeExplorationRotation );		
}

Bool CBehTreeNodePredefinedPathInstance::LoadState( IGameLoader* reader )
{
	m_isInExploration = false;
	reader->ReadValue< Bool >( CNAME( isInExploration ), m_isInExploration );
	reader->ReadValue< Vector3 >( CNAME( beforeExplorationPosition ), m_beforeExplorationPosition );
	reader->ReadValue< EulerAngles >( CNAME( beforeExplorationRotation ), m_beforeExplorationRotation );		
	m_restoreExplorationState = m_isInExploration;
	return true;
}

void CBehTreeNodePredefinedPathInstance::UpdateExploration()
{
	EActorActionResult status = m_explorationAction->GetStatus();	
	CMovingPhysicalAgentComponent* mpac = m_mac.Get();
	if( status == ActorActionResult_InProgress )
	{
		if( m_traverser.Get() )
		{
			Float ct		= m_owner->GetLocalTime();
			Float dt		= ct - m_prevLocalTime;
			dt				= dt > 0.04f ? 0.04f : dt;			
			m_prevLocalTime = ct;
						
			if( mpac )
			{
				mpac->ForceRestartFrameSkipping();
			}

			m_traverser.Get()->Update( dt );		
		}	
		else
		{
			DisableExplorationWalking();
			RenewPathFollow();
			m_isInExploration = false;	
		}
	}	

	if( status == ActorActionResult_Succeeded || status == ActorActionResult_Failed )
	{		
		if( mpac )
		{				
			// bez sensu przelaczanie
			mpac->SetAnimatedMovement( false );					
			mpac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_JobTree );
			mpac->EnableStaticCollisions( true );						
			mpac->InvalidatePhysicsCache();	
			m_movementSwitchedBackToWalking = true;
		}
		m_timeToSwitchOnCollisionAfterExploration = m_owner->GetLocalTime() + 0.75f;		
		RenewPathFollow();

		m_isInExploration = false;		
		m_lastExplorationUsageTime = m_owner->GetLocalTime();
		//Complete( IBehTreeNodeInstance::BTTO_FAILED );
	}
}

void CBehTreeNodePredefinedPathInstance::EnableExplorationWalking()
{
	CActor* ownerActior = m_owner->GetActor();
	m_mac = Cast< CMovingPhysicalAgentComponent >( ownerActior->GetMovingAgentComponent() );
	CMovingPhysicalAgentComponent* mpac = m_mac.Get();
	if ( mpac )
	{			
		mpac->SetAnimatedMovement( true );		
		mpac->EnableDynamicCollisions( false );
		mpac->EnableStaticCollisions( false );
		// bez sensu przelaczanie
		mpac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_JobTree );
		mpac->EnableCharacterCollisions( false );
		mpac->InvalidatePhysicsCache();
		mpac->SetEnabledFeetIK( false, 0.f );
	}
	m_movementSwitchedBackToWalking = false;
}

void CBehTreeNodePredefinedPathInstance::DisableExplorationWalking()
{
	CActor* ownerActior = m_owner->GetActor();

	CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( ownerActior->GetMovingAgentComponent() );
	if ( mpac )
	{					
		if( !m_movementSwitchedBackToWalking )
		{
			mpac->SetAnimatedMovement( false );
			mpac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_JobTree );		
			mpac->EnableStaticCollisions( true );
		}		
		mpac->EnableCharacterCollisions( true );
		mpac->EnableDynamicCollisions( true );
		mpac->InvalidatePhysicsCache();
		mpac->SetEnabledFeetIK( true, 0.f );
	}

	m_timeToSwitchOnCollisionAfterExploration = -1.0f;	
}

// IMovementTargeter interface
void CBehTreeNodePredefinedPathInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{	
	goal.SetFulfilled( false );

	if( m_isInExploration )
		return;

	const Float MIN_DIST = 0.5f;
	const Float MAX_DIST = 1.5f;
	CPathComponent* pathComponent = m_pathComponent.Get();
	// sanity check
	if ( !pathComponent )
	{
		m_isCompleted = true;
		return;
	}

	// test distance calculation
	const Vector& position = agent.GetWorldPositionRef();

	// Update current path position
	Vector positionOnPath		= pathComponent->CorrectPathPoint( position, m_pathEdgeIdx, m_pathEdgeAlpha );//pathComponent->GetClosestPointOnPath( position, m_pathEdgeIdx, m_pathEdgeAlpha );
	
	Float distanceFromPath = ( position.AsVector2() - positionOnPath.AsVector2() ).Mag();

	// tightening calculation
	Float tighteningRatio = .01f;
	if ( distanceFromPath > m_tolerance )
	{
		tighteningRatio = ( distanceFromPath - m_tolerance ) / m_pathMargin;
		tighteningRatio = ::Min( 1.f, tighteningRatio );
	}

	// logical step distance
	Float referencePointDistance = MAX_DIST - (MAX_DIST - MIN_DIST) * tighteningRatio;
	if ( !m_upThePath )
	{
		// move path backwards
		referencePointDistance = -referencePointDistance;
	}
		
	Int32 newIndex = m_pathEdgeIdx;
	Float newAlpha = m_pathEdgeAlpha;
	Bool isEndOfPath = false;	
	Vector followPoint;
	Vector2 diff;
	Float followDistance;

	do
	{	
		followPoint = pathComponent->GetNextPointOnPath( newIndex, newAlpha, referencePointDistance, isEndOfPath );
	
		if ( isEndOfPath )
		{
			m_isSuccess = true;
		}

		// steer
		diff = followPoint.AsVector2() - position.AsVector2();
		followDistance = diff.Mag();
		
		if ( isEndOfPath )
		{
			m_isSuccess = true;
			if ( diff.IsAlmostZero() )
			{
				m_isCompleted = true;
				return;
			}
			break;
		}
	}
	while( followDistance < referencePointDistance );
	Vector2 velocity = diff / followDistance;
	Float orientation = EulerAngles::YawFromXY( velocity.X, velocity.Y );
	
	// Slow down on path end
	// TODO: Temporary (until final step)
	EMoveType movementType = m_isUsingDoors ? m_inDoorMoveType : GetMoveType();
	Float speedGoal = agent.GetSpeedForMoveType( movementType, m_moveSpeed );
	Float distanceToDestination = ( m_destinationPosition.AsVector2() - position.AsVector2() ).Mag();
	if ( distanceToDestination < 2.f )
	{
		speedGoal *= 0.3f + distanceToDestination * (0.7f / 2.f);
		if ( isEndOfPath && distanceToDestination < m_arrivalDistance )
		{
			m_isCompleted = true;
		}
	}

	
	m_distanceFromStart		= pathComponent->CalculateDistanceFromStart( m_pathEdgeIdx, m_pathEdgeAlpha );

	Float tighteningDir = orientation;			// TODO: it should be curve tangent at point
	goal.SetFlag( CNAME( Tightening ), Vector( followPoint.X, followPoint.Y, tighteningDir, tighteningRatio ) );

	goal.SetHeadingGoal( agent, velocity );
	goal.SetGoalPosition( followPoint );
	goal.SetDistanceToGoal( followDistance );
	goal.MatchOrientationWithHeading();
	goal.SetSpeedGoal( speedGoal );
	goal.SetDestinationPosition( m_destinationPosition );
	goal.SetDistanceToDestination( distanceToDestination );
	goal.SetOrientationGoal( agent, orientation );

	Float speedMult	= 1.0f;
	
	if ( pathComponent->CalculateSpeedMult( m_pathEdgeIdx, m_pathEdgeAlpha, speedMult ) )
	{
		goal.SetFlag( CNAME( SpeedMultOnPath ), speedMult );
	}
}

EMoveType CBehTreeNodePredefinedPathInstance::GetMoveType()const
{
	CPathComponent* pathComponent = m_pathComponent.Get();
	if ( pathComponent && pathComponent->HasSpeedCurve() )
	{
		return MT_Sprint;
	}
	return Super::GetMoveType();
}

Bool CBehTreeNodePredefinedPathInstance::IsFinished() const
{
	return m_isCompleted;
}

Bool CBehTreeNodePredefinedPathInstance::IsThisClosestUser( CDoorComponent* door )
{
	CActor* thisActor = m_owner->GetActor();
	Vector3 thisPos	 = thisActor->GetWorldPosition();
	Vector3 doorsPos = door->GetEntity()->GetWorldPosition();
	Float thisDistanceSqr = ( thisPos - doorsPos ).SquareMag();

	const TDynArray< THandle< CActor > >& doorUsers = door->GetDoorUsers();
	
	for( Int16 i = 0; i<doorUsers.SizeInt(); ++i )
	{
		const CActor* user = doorUsers[ i ].Get();
		if( !user || user == thisActor )
			continue;

		Float distanceSqr = ( doorsPos - user->GetWorldPosition() ).SquareMag();
		if( distanceSqr < thisDistanceSqr )
			return false;
	}

	return true;
}

void CBehTreeNodePredefinedPathInstance::UpdateDoorsMovement()
{
	CDoorComponent* door = m_doorsInUse.Get();
	if( !door )
	{
		m_isUsingDoors	= false;
		m_doorsInUse	= nullptr;
		return;
	}

	//EMoveType moveType = GetMoveType();
	if( door->GetDoorUsers().Size() > 1 )
	{
		Bool isClosest = IsThisClosestUser( door );
		if( isClosest )
		{
			//if( moveType < MT_Sprint )
			{
				m_inDoorMoveType = MT_Sprint;//( EMoveType )( moveType + 1 );
				m_wasForced = true;
			}
		}
		else if( !m_wasForced )//if someone was forced, he wil not be slow down again ( this can happen when forced npc corssed doors )
		{
			//if( moveType > MT_Walk )
			{
				m_inDoorMoveType = MT_Walk;//( EMoveType )( moveType - 1 );
			}
		}
	}
	else
	{
		EMoveType moveType = GetMoveType();
		m_inDoorMoveType = moveType;
	}
}

Bool CBehTreeNodePredefinedPathInstance::OnEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( AI_DoorTriggerEntered ) )
	{//agent/taverm community scenes/piggie
		SGameplayEventParamObject* params = e.m_gameplayEventData.Get< SGameplayEventParamObject >();
		if( params )
		{
			CEntity* doorEntity = Cast< CEntity >( params->m_value );
			CDoorComponent* door = doorEntity->FindComponent< CDoorComponent >();
			m_doorsInUse = door;
			if( door )
			{
				if( door->IsInteractive() )
				{
					door->Open();
				}
				m_isUsingDoors	= true;
				m_wasForced		= false;
				UpdateDoorsMovement();
			}
		}
	}
	else if( e.m_eventName == CNAME( AI_DoorTriggerExit ) )
	{
		m_isUsingDoors	= false;
		m_doorsInUse	= nullptr;
	}

	return Super::OnEvent( e );
}

void CBehTreeNodePredefinedPathInstance::OnGenerateDebugFragments( CRenderFrame* frame ) 
{
	CPathComponent* pathComponent = m_pathComponent.Get();
	if ( pathComponent )
	{
		const Vector& position = m_owner->GetActor()->GetWorldPositionRef();
		Vector positionOnPath	= pathComponent->GetPathPoint( m_pathEdgeIdx, m_pathEdgeAlpha );
		Vector direction( Vector::ZEROS );

		SMultiCurvePosition p;
		p.m_edgeIdx = m_pathEdgeIdx;
		p.m_edgeAlpha = m_pathEdgeAlpha;
		pathComponent->GetCurve().CalculateAbsolute2DTangentFromCurveDirection( p, direction.AsVector2() );

		Vector2 leftPerpendicular = MathUtils::GeometryUtils::PerpendicularL( direction ) * m_tolerance;
		Vector2 rightPerpendicular = MathUtils::GeometryUtils::PerpendicularR( direction ) * m_tolerance;

		frame->AddDebugLine( positionOnPath + leftPerpendicular, positionOnPath + leftPerpendicular + direction, Color::RED, true );
		frame->AddDebugLine( positionOnPath + rightPerpendicular, positionOnPath + rightPerpendicular + direction, Color::RED, true );
		frame->AddDebugLine( positionOnPath, position + direction, Color::BLUE, true );
	}
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicMoveToPredefinedPathDefinition
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeAtomicMoveToPredefinedPathDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicMoveToPredefinedPathInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodeAtomicMoveToPredefinedPathInstance::CBehTreeNodeAtomicMoveToPredefinedPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_pathName( def.m_pathName.GetVal( context ) )
	, m_upThePath( def.m_upThePath.GetVal( context ) )
	, m_startFromBeginning( def.m_startFromBeginning.GetVal( context ) )
	, m_arrivedAtPath( false )
{
	SBehTreeEvenListeningData eventData;
	eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	eventData.m_eventName = CNAME( ForceArriveAtPath );
	context.AddEventListener( eventData, this );
}

void CBehTreeNodeAtomicMoveToPredefinedPathInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( ForceArriveAtPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

void CBehTreeNodeAtomicMoveToPredefinedPathInstance::Update()
{
	// If we don't start from beginning
	// update the target on path because we want to reach the closest point to the actor on the path
	// because this closest point might move as the actor move.
	// Do not do that when we rotate to target because ActionMoveToChangeTarget will reset the state to move.
	if ( m_state == E_MOVE )
	{
		if ( StartFromBeginning() )
		{
			if ( CPathComponent* pathComponent = m_pathComponent.Get() )
			{
				SMultiCurve& curve = pathComponent->GetCurve();
				if ( curve.Size() > 1 )
				{
					SMultiCurvePosition correctedCurvePosition;
					correctedCurvePosition.m_edgeIdx = m_upThePath ? 0 : curve.Size() - 1;
					correctedCurvePosition.m_edgeAlpha = 0.f;
					Vector closestSpot;

					CActor* actor = m_owner->GetActor();
					curve.CorrectCurvePoint( actor->GetWorldPositionRef(), correctedCurvePosition, closestSpot );
					CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
					CPathAgent* pathAgent = mac->GetPathAgent();
					Bool atTheBeginingOfThePath = false;
					if ( m_upThePath )
					{
						atTheBeginingOfThePath = correctedCurvePosition.m_edgeIdx <= 1;
					}
					else
					{
						atTheBeginingOfThePath = correctedCurvePosition.m_edgeIdx + 2u >= curve.Size();
					}

					if ( atTheBeginingOfThePath && pathAgent->TestLine( actor->GetWorldPositionRef(), closestSpot ) && !Vector::Equal3( m_target, closestSpot ) )
					{
						m_target = closestSpot;
						actor->ActionMoveToChangeTarget( m_target, m_moveType, m_moveSpeed, m_maxDistance );
					}
				}
			}
		}
		else
		{
			CActor* actor = m_owner->GetActor();

			if ( actor->IsMoving() )
			{
				ComputeTargetAndHeading();

				actor->ActionMoveToChangeTarget( m_target, m_moveType, m_moveSpeed, m_maxDistance );
			}
		}
	}

	CBehTreeNodeAtomicMoveToInstance::Update();
}


Bool CBehTreeNodeAtomicMoveToPredefinedPathInstance::ComputeTargetAndHeading()
{
	CPathComponent* pathComponent = m_pathComponent.Get();
	if ( !pathComponent )
	{
		CNode* node = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_pathName );
		if ( !node )
		{
			ERR_GAME( TXT("AI: Cannot find path with tag '%s'!"), m_pathName.AsChar() );
			// invalid tag
			return false;
		}
		if ( node->IsA< CPathComponent >() )
		{
			pathComponent = static_cast< CPathComponent* >( node );
		}
		else
		{
			CEntity* entity = Cast< CEntity >( node );
			if ( entity )
			{
				pathComponent = entity->FindComponent< CPathComponent >();
			}
		}

		if ( !pathComponent )
		{
			ERR_GAME( TXT("AI: Entity with tag '%s' has no path component!"), m_pathName.AsChar() );
			// no path component in given node
			return false;
		}
		m_pathComponent = pathComponent;
	}

	SMultiCurve& curve = pathComponent->GetCurve();

	if ( curve.Size() == 0 )
	{
		return false;
	}

	Vector startingPoint;
	Vector2 heading( 0.f, 0.f );

	if ( StartFromBeginning() )
	{
		const Uint32 controlPointIdx = m_upThePath ? 0 : curve.Size() - 1;
		curve.GetAbsoluteControlPointPosition( controlPointIdx, startingPoint );

		if ( curve.Size() > 1 )
		{
			const Uint32 nextControlPointIdx = controlPointIdx + ( m_upThePath ? 1 : -1 );
			Vector worldPoint;
			curve.GetAbsoluteControlPointPosition( nextControlPointIdx, worldPoint );

			heading = worldPoint.AsVector2() - startingPoint.AsVector2();
		}
	}
	else
	{
		CActor* actor = m_owner->GetActor();
		const Vector& actorPos = actor->GetWorldPositionRef();

		Int32 edgeIdx;
		Float edgeAlpha;
		Bool isEndpoint = false;

		Vector closestPoint = pathComponent->GetClosestPointOnPath( actorPos, edgeIdx, edgeAlpha );
		Vector nextPoint = pathComponent->GetNextPointOnPath( edgeIdx, edgeAlpha, m_upThePath ? 0.1f : -0.1f, isEndpoint );
		if ( isEndpoint )
		{
			// move backward if
			Vector prevPoint =  pathComponent->GetNextPointOnPath( edgeIdx, edgeAlpha, m_upThePath ? -0.1f : 0.1f, isEndpoint );
			heading = closestPoint.AsVector2() - prevPoint.AsVector2();
		}
		else
		{
			heading = nextPoint.AsVector2() - closestPoint.AsVector2();
		}

		startingPoint = closestPoint.AsVector3();
	}
	m_target = startingPoint;
	m_heading = EulerAngles::YawFromXY( heading.X, heading.Y );

	return true;
}

void CBehTreeNodeAtomicMoveToPredefinedPathInstance::Complete( eTaskOutcome outcome )
{
	m_arrivedAtPath = true;
	GetOwner()->GetActor()->SignalGameplayEvent( CNAME( ArrivedAtPath ) );
	Super::Complete( outcome );
}

Bool CBehTreeNodeAtomicMoveToPredefinedPathInstance::IsSavingState() const
{
	if ( m_startFromBeginning && m_arrivedAtPath )
	{
		return true;
	}
	return false;
}
void CBehTreeNodeAtomicMoveToPredefinedPathInstance::SaveState( IGameSaver* writer )
{
	writer->WriteValue< Bool >( CNAME( arrived ), m_arrivedAtPath );
}
Bool CBehTreeNodeAtomicMoveToPredefinedPathInstance::LoadState( IGameLoader* reader )
{
	reader->ReadValue< Bool >( CNAME( arrived ), m_arrivedAtPath );
	return true;
}

Bool CBehTreeNodeAtomicMoveToPredefinedPathInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( ForceArriveAtPath ) )
	{
		m_arrivedAtPath = true;
	}

	return Super::OnListenedEvent( e );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathWithCompanionDefinition
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodePredefinedPathWithCompanionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathWithCompanionInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodePredefinedPathWithCompanionInstance::CBehTreeNodePredefinedPathWithCompanionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodePredefinedPathInstance( def, owner, context, parent )
	, m_companionTag( def.m_companionTag.GetVal( context ) )
	, m_progressWhenCompanionIsAhead( def.m_progressWhenCompanionIsAhead.GetVal( context ) )
	, m_progressOnlyWhenCompanionIsAhead( def.m_progressOnlyWhenCompanionIsAhead.GetVal( context ) )
	, m_companionOffset( def.m_companionOffset.GetVal( context ) )
	, m_paused ( false )
	, m_companionIsInFront( false )
	, m_companionPositionOnPath()
	, m_matchCompanionSpeed( def.m_matchCompanionSpeed.GetVal( context ) )
	, m_previousMoveType( MT_Walk )
	, m_keepMovingWhenMaxDistanceReached( def.m_keepMovingWhenMaxDistanceReached.GetVal( context ) )
	, m_moveTypeAfterMaxDistanceReached( def.m_moveTypeAfterMaxDistanceReached.GetVal( context ) )
	, m_isRiddenOff( false )
{
	m_stopMovementDistSq = def.m_maxDistance.GetVal( context );
	m_stopMovementDistSq *= m_stopMovementDistSq;

	m_startMovementDistSq = ( def.m_minDistance.GetVal( context ) );
	m_startMovementDistSq *= m_startMovementDistSq;
}

void CBehTreeNodePredefinedPathWithCompanionInstance::CalcCompanionIsInFront()
{
	CActor* actor					= m_owner->GetActor();
	const Vector& actorPos			= actor->GetWorldPositionRef();

	CEntity* companion				= m_companion.Get();
	if ( companion == nullptr )
	{
		m_companionIsInFront = false;
		return;
	}
	const Vector& companionPos		= companion->GetWorldPositionRef();

	CPathComponent* pathComponent	= m_pathComponent.Get();
	// sanity check
	if ( !pathComponent )
	{
		m_companionIsInFront = true;
		return;
	}

	Int32 companionEdgeIdx;
	Float companionEdgeAlpha;
	m_companionPositionOnPath = pathComponent->GetClosestPointOnPath( companionPos, companionEdgeIdx, companionEdgeAlpha, 0.5f );
	// check if closest point on path is before or after out current position
	if ( m_upThePath )
	{
		if ( m_pathEdgeIdx > companionEdgeIdx ||
			(m_pathEdgeIdx == companionEdgeIdx && m_pathEdgeAlpha > companionEdgeAlpha) )
		{
			m_companionIsInFront = false;
			return;
		}
	}
	else
	{
		if ( m_pathEdgeIdx < companionEdgeIdx ||
			(m_pathEdgeIdx == companionEdgeIdx && m_pathEdgeAlpha < companionEdgeAlpha) )
		{
			m_companionIsInFront = false;
			return;
		}
	}
	m_companionIsInFront = true;
}

void CBehTreeNodePredefinedPathWithCompanionInstance::CalcIsRiddenOff()
{
	if ( const CEntity* companionEntity = m_companion.Get() )
	{
		const Vector& companionPosition		= companionEntity->GetWorldPositionRef();
		CActor* actor						= m_owner->GetActor();
		const Vector& position				= actor->GetWorldPositionRef();

		if ( m_keepMovingWhenMaxDistanceReached && !m_companionIsInFront )
		{
			if ( position.DistanceSquaredTo( companionPosition ) > m_stopMovementDistSq )
			{
				m_isRiddenOff = true;
			}
		}

		if ( m_isRiddenOff && position.DistanceSquaredTo( companionPosition ) < m_startMovementDistSq )
		{
			m_isRiddenOff = false;
		}
	}
}

Bool CBehTreeNodePredefinedPathWithCompanionInstance::IsCompanionAway()
{
	CEntity* companion = m_companion.Get();
	if ( !companion )
	{
		return true;
	}
	CActor* actor = m_owner->GetActor();

	Float distanceLimitSq = m_paused ? m_startMovementDistSq : m_stopMovementDistSq;

	const Vector& companionPos = companion->GetWorldPositionRef();
	const Vector& actorPos = actor->GetWorldPositionRef();
	
	Float distanceSq = actorPos.DistanceSquaredTo( companionPos );
	if ( distanceSq < distanceLimitSq )
	{
		// If companion is ahead on the path AND is within starting distance ( straight line distance )
		if ( m_progressOnlyWhenCompanionIsAhead )
		{
			return m_companionIsInFront == false;
		}
		return false;
	}

	if ( m_progressWhenCompanionIsAhead )
	{
		if ( !m_companionIsInFront && !m_keepMovingWhenMaxDistanceReached )
		{
			return true;
		}
		// Checking if companion is not too far from the path
		const Float distanceSq = m_companionPositionOnPath.DistanceSquaredTo( companionPos );
		if ( distanceSq < distanceLimitSq || m_keepMovingWhenMaxDistanceReached )
		{
			return false;
		}
	}
	return true;
}

CEntity *const CBehTreeNodePredefinedPathWithCompanionInstance::GetCompanion()const
{
	return m_companion.Get();
}

Bool CBehTreeNodePredefinedPathWithCompanionInstance::Activate()
{
	CEntity* companion	= GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_companionTag );
	m_companion			= companion;
	m_previousMoveType	= GetMoveType();
	if ( companion )
	{
		m_paused = true;
		m_companionTestDelay = 0.f;
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodePredefinedPathWithCompanionInstance::Update()
{	
	if( m_isUsingDoors )
	{
		UpdateDoorsMovement();
	}

	if( m_isInExploration )
	{
		UpdateExploration();
		return;
	}
	else
	{
		SwitchToExplorationIfNeeded();
	}

	if( m_timeToSwitchOnCollisionAfterExploration > 0.0f && m_timeToSwitchOnCollisionAfterExploration < m_owner->GetLocalTime() )
	{
		DisableExplorationWalking();		
	}

	m_companion = GetCompanion();

	CActor* ownerActior			= m_owner->GetActor();
	CMovingAgentComponent* mac	= ownerActior->GetMovingAgentComponent();
	const EMoveType moveType	= GetMoveType();
	if ( moveType != m_previousMoveType )
	{
		m_previousMoveType = moveType;
		mac->SetMoveType( moveType );
	}
	// checking if the actor has reached the end :
	if ( m_isCompleted )
	{
		Complete( m_isSuccess ? BTTO_SUCCESS : BTTO_FAILED );
	}
	else
	{
		if ( m_companionTestDelay < m_owner->GetLocalTime() )
		{
			CalcCompanionIsInFront();
			CalcIsRiddenOff();
			m_paused = IsCompanionAway();
			m_companionTestDelay = m_owner->GetLocalTime() + 0.111f;
		}
	}
}


Bool CBehTreeNodePredefinedPathWithCompanionInstance::ShouldMaintainTargetSpeed() const
{
	if ( !m_matchCompanionSpeed )
	{
		return false;
	}

	const CEntity* const companionEntity = m_companion.Get();
	if ( !companionEntity )
	{
		return false;
	}

	if ( m_companionIsInFront )
	{
		const CActor* actor					= m_owner->GetActor();
		const Vector& companionPosition		= companionEntity->GetWorldPositionRef();
		const Vector& position				= actor->GetWorldPositionRef();

		// If actor is lagging behind, then let the normal speed system take over
		return position.DistanceSquaredTo( companionPosition ) <= m_stopMovementDistSq;
	}

	return true;
}

// IMovementTargeter interface
void CBehTreeNodePredefinedPathWithCompanionInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CEntity *const companionEntity = m_companion.Get();

	if ( companionEntity )
	{
		goal.SetGoalTargetNode( companionEntity );
	}

	goal.SetFlag( CNAME( MaintainTargetSpeed ), ShouldMaintainTargetSpeed() );
	goal.SetFlag( CNAME( CompanionOffset ), m_companionOffset );

	if( m_isInExploration )
	{
		goal.SetFulfilled( false );
		return;
	}


	Super::UpdateChannels( agent, goal, timeDelta );
	
	if( m_paused )
	{
		// if Paused the orientation must still be computed as before otherwise the actor might go out of the path  
		// because of his movement inertia
		// Do not override orient

		// Overriding speeds to 0:
		goal.SetDistanceToGoal( 0.0f );
		goal.SetSpeedGoal( 0.f );
		goal.SetFulfilled( false );
	}
}

EMoveType CBehTreeNodePredefinedPathWithCompanionInstance::GetMoveType()const
{
	if ( ShouldMaintainTargetSpeed() )
	{
		return MT_Sprint;
	}
	else if ( m_keepMovingWhenMaxDistanceReached && !m_companionIsInFront )
	{
		if ( const CEntity* companionEntity = m_companion.Get() )
		{
			CActor* actor						= m_owner->GetActor();
			const Vector& companionPosition		= companionEntity->GetWorldPositionRef();
			const Vector& position				= actor->GetWorldPositionRef();

			if ( position.DistanceSquaredTo( companionPosition ) > m_stopMovementDistSq )
			{
				return m_moveTypeAfterMaxDistanceReached;
			}
		}
	} 

	if ( m_isRiddenOff )
	{
		return m_moveTypeAfterMaxDistanceReached;
	}

	return Super::GetMoveType();
}

//#pragma optimize ("", on )
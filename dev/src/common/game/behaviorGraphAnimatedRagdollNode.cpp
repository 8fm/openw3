/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphOutput.h"
#include "../engine/behaviorGraphSocket.h"
#include "behaviorGraphAnimatedRagdollNode.h"
#include "../engine/allocatedBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/behaviorGraphValueNode.h"

#include "../physics/PhysicsRagdollWrapper.h"
#include "../physics/PhysicsWorld.h"

#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/cacheBehaviorGraphOutput.h"

#include "../engine/baseEngine.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/behaviorProfiler.h"
#include "../engine/behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimatedRagdollNode );
IMPLEMENT_ENGINE_CLASS( SBehaviorGraphAnimatedRagdollDirDefinition );
IMPLEMENT_ENGINE_CLASS( SBehaviorGraphAnimatedRagdollDirReplacement );
IMPLEMENT_RTTI_ENUM( EBehaviorGraphAnimatedRagdollNodeState );

///////////////////////////////////////////////////////////////////////////////

//RED_DEFINE_STATIC_NAME( Ragdoll ); <- already defined
RED_DEFINE_STATIC_NAME( Flying );
RED_DEFINE_STATIC_NAME( HitWall );
RED_DEFINE_STATIC_NAME( HitGround );
RED_DEFINE_STATIC_NAME( SwitchAnimatedRagdollToRagdoll );

RED_DEFINE_STATIC_NAME( DirIndex );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphAnimatedRagdollNode::CBehaviorGraphAnimatedRagdollNode()
: m_chanceToGoToRagdoll( 0.0f )//0.25f )
, m_stateBlendTime( 0.15f )
, m_maxFlightTime( 5.0f, 8.0f )
, m_initialVelocityBoostZ( 1.5f, 2.5f ) // tweaked to go up a little bit
, m_gravity( 14.0f ) // and quickly fall down
, m_topVerticalVelocity( 10.0f )
, m_switchAnimatedRagdollToRagdollEvent( CNAME( SwitchAnimatedRagdollToRagdoll ) )
{
	m_poseRotateIK.m_bone = CNAME( pelvis );
}

void CBehaviorGraphAnimatedRagdollNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_velocityWS;
	compiler << i_state;
	compiler << i_prevState;
	compiler << i_stateWeight;
	compiler << i_timeInState;
	compiler << i_flightTime;
	compiler << i_switchToRagdoll;
	compiler << i_poseRotateIK;
	compiler << i_hitGroundNormalMS;
	compiler << i_hitWallNormalMS;
	compiler << i_physicalRadius;
	compiler << i_dirIndex;
	compiler << i_dirIndexApplyAngle;
}

void CBehaviorGraphAnimatedRagdollNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_velocityWS ] = Vector::ZEROS;
	instance[ i_state ] = BGARN_Ragdoll;
	instance[ i_prevState ] = BGARN_Ragdoll;
	instance[ i_stateWeight ] = 1.0f;
	instance[ i_timeInState ] = 0.0f;
	instance[ i_flightTime ] = 0.0f;
	instance[ i_switchToRagdoll ] = false;
	instance[ i_poseRotateIK ].Setup( instance, m_poseRotateIK );
	instance[ i_hitGroundNormalMS ] = Vector::EZ;
	instance[ i_hitWallNormalMS ] = Vector::EZ;
	instance[ i_physicalRadius ] = 0.0f;
	instance[ i_dirIndex ] = 0.0f;
	instance[ i_dirIndexApplyAngle ] = 180.0f;
}

void CBehaviorGraphAnimatedRagdollNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_velocityWS );
	INST_PROP( i_state );
	INST_PROP( i_prevState );
	INST_PROP( i_stateWeight );
	INST_PROP( i_timeInState );
	INST_PROP( i_flightTime );
	INST_PROP( i_switchToRagdoll );
	INST_PROP( i_poseRotateIK );
	INST_PROP( i_hitGroundNormalMS );
	INST_PROP( i_hitWallNormalMS );
	INST_PROP( i_physicalRadius );
	INST_PROP( i_dirIndex );
	INST_PROP( i_dirIndexApplyAngle );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimatedRagdollNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( DirIndex ) ) );		
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Ragdoll ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Flying ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( HitWall ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( HitGround ) ) );
}

#endif

void CBehaviorGraphAnimatedRagdollNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedNodes.Resize( BGARN_MAX );
	m_cachedNodes[ BGARN_Ragdoll ] = CacheBlock( TXT("Ragdoll") );
	m_cachedNodes[ BGARN_Flying ] = CacheBlock( TXT("Flying") );
	m_cachedNodes[ BGARN_HitWall ] = CacheBlock( TXT("HitWall") );
	m_cachedNodes[ BGARN_HitGround ] = CacheBlock( TXT("HitGround") );
}

RED_INLINE static Bool TraceHit( CPhysicsWorld* physicsWorld, Vector const & startWS, Vector const & dir, Float distance, Vector& hitLocWS, Vector& normalWS )
{
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
	CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	Vector endWS = startWS + dir * distance;
	SPhysicsContactInfo contactInfo;
	if( physicsWorld->RayCastWithSingleResult( startWS, endWS, include, exclude, contactInfo ) == TRV_Hit )
	{
		hitLocWS = contactInfo.m_position;
		normalWS = contactInfo.m_normal;
		return true;
	}
	else
	{
		hitLocWS = startWS;
		normalWS = Vector( 0.0f, 0.0f, 1.0f );
		return false;
	}
}

RED_INLINE static Bool GetTerrainNormal( CAnimatedComponent const * ac, Vector const & startWS, Float radius, Vector& normalWS )
{
	Bool result = false;

	if ( CWorld* world = ac->GetWorld() )
	{
		CPhysicsWorld* physicsWorld = nullptr;
		if (  world->GetPhysicsWorld( physicsWorld) )
		{
			radius = Max( 0.5f, radius );
			Vector updatedStartWS = startWS + Vector::EZ * radius;

			Vector offsetWS[] = { Vector(  0.000f,  1.000f, 0.0) * radius,
								  Vector(  0.707f, -0.707f, 0.0) * radius,
								  Vector( -0.707f, -0.707f, 0.0) * radius };
			Vector hitLocWS[3];
			Vector hitNormalWS[3];
			result = true;
			for ( Int32 i = 0; i < 3; ++ i )
			{
				result &= TraceHit( physicsWorld, updatedStartWS + offsetWS[i], -Vector::EZ, 2.0f * radius, hitLocWS[i], hitNormalWS[i] );
			}

			if ( result )
			{
				normalWS = Vector::Cross( hitLocWS[1] - hitLocWS[2], hitLocWS[0] - hitLocWS[2] ).Normalized3();
			}
		}
	}

	if ( ! result )
	{
		if ( CMovingPhysicalAgentComponent const * mpac = Cast< CMovingPhysicalAgentComponent >( ac ) )
		{
			normalWS = mpac->GetTerrainNormal( false );
			result = true;
		}
	}

	return result;
}

void CBehaviorGraphAnimatedRagdollNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimatedRagdoll );
	if ( instance[ i_switchToRagdoll ] )
	{
		instance[ i_switchToRagdoll ] = false;
		ChangeState( instance, BGARN_Ragdoll );
	}

	instance[ i_timeDelta ] = timeDelta;
	EBehaviorGraphAnimatedRagdollNodeState & state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState & prevState = instance[ i_prevState ];
	Float & stateWeight = instance[ i_stateWeight ];
	Float & timeInState = instance[ i_timeInState ];
	Float flightTime = instance[ i_flightTime ];
	Vector & velocityWS = instance[ i_velocityWS ];

	Float prevStateWeight = stateWeight;

	if ( state == BGARN_Ragdoll && stateWeight < 1.0f )
	{
		stateWeight = instance.GetAnimatedComponent()->GetSkeletonPreRagdollWeight() > 0.0f? 0.0f : 1.0f; // use previous state when we're still in pre-ragdoll
	}
	else
	{
		stateWeight = BlendOnOffWithSpeedBasedOnTime( stateWeight, 1.0f, m_stateBlendTime, timeDelta );
	}
	timeInState += timeDelta;
	if ( state == BGARN_Flying )
	{
		velocityWS.Z = Clamp( velocityWS.Z - m_gravity * timeDelta, -m_topVerticalVelocity, m_topVerticalVelocity );
	}

	if ( prevStateWeight < 1.0f && stateWeight >= 1.0f )
	{
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->Deactivate( instance );
		}
	}

	// check if we hit something and change state, or maybe we are flying for too long
	if ( state == BGARN_Flying && timeInState >= flightTime )
	{
		ChangeState( instance, BGARN_Ragdoll );
	}
	if ( state == BGARN_Flying )
	{
		if ( CMovingPhysicalAgentComponent const * mpac = Cast< CMovingPhysicalAgentComponent >( instance.GetAnimatedComponent() ) )
		{
			Float velocityActualVsRequested = Vector::Dot2( mpac->GetVelocity().Normalized2(), velocityWS.Normalized2() );
			if ( velocityActualVsRequested < 0.7f )
			{
				if ( CWorld* world = instance.GetAnimatedComponent()->GetWorld() )
				{
					CPhysicsWorld* physicsWorld = nullptr;
					if ( world->GetPhysicsWorld( physicsWorld ) )
					{
						Vector hitLoc;
						Vector hitNormal;
						if ( TraceHit( physicsWorld, mpac->GetWorldPosition() + Vector::EZ, velocityWS.Normalized2(), 0.7f, hitLoc, hitNormal ) ) // TODO check radius?
						{
							ChangeState( instance, BGARN_HitWall );
							hitNormal = hitNormal.Normalized2();
							Matrix worldToLocal;
							mpac->GetWorldToLocal( worldToLocal );
							instance[ i_hitWallNormalMS ] = worldToLocal.TransformVector( hitNormal );
						}
					}
				}
			}
			if ( state == BGARN_Flying && velocityActualVsRequested < 0.85f )
			{
				// we've hit something, maybe we should just switch to ragdoll
				ChangeState( instance, BGARN_Ragdoll );
			}
			if ( state == BGARN_Flying && mpac->IsCollidingDown() && timeInState > 0.5f )
			{
				Vector hitNormalWS;
				if ( GetTerrainNormal( mpac, mpac->GetWorldPosition(), instance[ i_physicalRadius ], hitNormalWS ) )
				{
					if ( Vector::Dot3( velocityWS, hitNormalWS ) <= 0.0f )
					{
						ChangeState( instance, BGARN_HitGround );
						Matrix worldToLocal;
						mpac->GetWorldToLocal( worldToLocal );
						instance[ i_hitGroundNormalMS ] = worldToLocal.TransformVector( hitNormalWS );
					}
				}
			}
		}
	}

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->Update( context, instance, timeDelta );
	}

	if ( stateWeight < 1.0f )
	{
		ASSERT( prevState != state, TXT("If state weight is not 1.0, prev state should be different from normal state") );
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphAnimatedRagdollNode::ApplyVelocity( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Float const timeDelta = instance[ i_timeDelta ];
	Matrix worldToLocal;
	instance.GetAnimatedComponent()->GetWorldToLocal( worldToLocal );
	Vector const velocityWS = instance[ i_velocityWS ];
	Vector const velocityMS = worldToLocal.TransformVector( velocityWS );
	Vector const deltaMove = velocityMS * timeDelta;
	output.m_deltaReferenceFrameLocal.SetTranslation( VectorToAnimVector( deltaMove ) );
	
	// rotate to match (requested) movement direction
	EulerAngles deltaRotation = EulerAngles::ZEROS;
	if ( velocityWS.SquareMag2() > 1.0f )
	{
		Vector forwardDir = instance.GetAnimatedComponent()->GetWorldForward();
		Float requestedYaw = EulerAngles::NormalizeAngle180( EulerAngles::YawFromXY( velocityWS.X, velocityWS.Y ) - ( EulerAngles::YawFromXY( forwardDir.X, forwardDir.Y ) + instance[ i_dirIndexApplyAngle ] ) );
		Float possibleDeltaYaw = 1080.0f * timeDelta;
		deltaRotation.Yaw = Clamp( requestedYaw, -possibleDeltaYaw, possibleDeltaYaw );
	}
	if ( deltaRotation.Yaw != 0.0f )
	{
		output.m_deltaReferenceFrameLocal.SetRotation( EulerAnglesToAnimQuat( deltaRotation ) );
	}

	// calculate dir perpendicular to velocity, but not make it too harsh
	Vector uprightDir = Vector::Cross( Vector::Cross( velocityMS, Vector::EZ ), velocityMS );
	uprightDir.Normalize3();
	uprightDir.Z = Max( uprightDir.Z, 0.7f );
	uprightDir.Normalize3();

	SApplyRotationIKSolver & poseRotateIK = instance[ i_poseRotateIK ];
	poseRotateIK.StorePreIK( instance, output );
	poseRotateIK.SetAdjustment( AnimVector4::EZ, VectorToAnimVector( uprightDir ) );
	poseRotateIK.UpdatePose( instance, output, m_poseRotateIK, 1.0f );
}

void CBehaviorGraphAnimatedRagdollNode::ApplyAdjustmentsToOutput( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, EBehaviorGraphAnimatedRagdollNodeState state ) const
{
	Float const timeDelta = instance[ i_timeDelta ];
	EBehaviorGraphAnimatedRagdollNodeState actualState = instance[ i_state ];

	if ( state == BGARN_Flying )
	{
		ApplyVelocity( instance, output );
	}

	if ( state == BGARN_HitGround )
	{
		Vector & hitNormalMS = instance[ i_hitGroundNormalMS ];
		if ( hitNormalMS.SquareMag3() > 0.0f )
		{
			// update normal if we're in actual state
			if ( state == actualState )
			{
				Vector newHitNormalMS = hitNormalMS;
				if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
				{
					Vector newHitNormalWS;
					if ( GetTerrainNormal( ac, ac->GetWorldPosition(), instance[ i_physicalRadius ], newHitNormalWS ) )
					{
						Matrix worldToLocal;
						ac->GetWorldToLocal( worldToLocal );
						newHitNormalMS = worldToLocal.TransformVector( newHitNormalWS );
					}
				}
				hitNormalMS = BlendToWithBlendTime( hitNormalMS, newHitNormalMS, 0.06f, timeDelta );
				hitNormalMS.Normalize3();
			}

			// rotate character
			SApplyRotationIKSolver & poseRotateIK = instance[ i_poseRotateIK ];
			poseRotateIK.StorePreIK( instance, output );
			poseRotateIK.SetAdjustment( AnimVector4::EZ, VectorToAnimVector( hitNormalMS ) );
			poseRotateIK.UpdatePose( instance, output, m_poseRotateIK, 1.0f, true, 0.08f ); // offset is hack to move above ground (animation now clips it with ground)

			// move to align to ground
			if ( output.m_numBones > 0 )
			{
				// if we're turning character, we should lower it to be on the ground
				AnimVector4 rootTranslation = output.m_outputPose[0].GetTranslation();
				//
				//			+---------------+-------r-------+			// r - physical radius
				//							|\b		 	a,-'			// a - angle between r and d
				//							|a\		   ,'				// b - angle between x and r, it is 90' - a (x is perpendicular to d)
				//							h  x .	,-'					// then angle between x and h is 90' - b = a
				//							|   \,-d					// sin(a) = x / r    =>    x = sin(a) * r
				//							|b_,'						// cos(a) = x / h    =>    h = x / cos(a)
				//							+'							// there we have how we should lower character
				//
				Float r = instance[ i_physicalRadius ];
				Float cosA = Max( 0.1f, hitNormalMS.Z );
				Float sinA = Sqrt( Clamp( 1.0f - cosA * cosA, 0.0f, 1.0f ) );
				Float x = r * sinA;
				Float h = x / cosA;
				rootTranslation.Z -= Min( r * 1.5f, h );
				output.m_outputPose[0].SetTranslation( rootTranslation );
			}
		}
	}
	if ( state == BGARN_HitWall )
	{
		Vector hitNormalMS = instance[ i_hitWallNormalMS ];
		if ( hitNormalMS.SquareMag3() > 0.0f )
		{
			SApplyRotationIKSolver & poseRotateIK = instance[ i_poseRotateIK ];
			poseRotateIK.StorePreIK( instance, output );
			// assume that we always flight backward
			poseRotateIK.SetAdjustment( AnimVector4( 0.0f, 1.0f, 0.0f ), VectorToAnimVector( hitNormalMS ) );
			poseRotateIK.UpdatePose( instance, output, m_poseRotateIK, 1.0f );
		}
	}
}

void CBehaviorGraphAnimatedRagdollNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState prevState = instance[ i_prevState ];
	Float stateWeight = instance[ i_stateWeight ];

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->Sample( context, instance, output );
	}

	if ( state == BGARN_Ragdoll && stateWeight < 1.0f && instance.GetAnimatedComponentUnsafe() ) // we haven't blended into ragdoll yet, we will be playing previous animation
	{
		instance.GetAnimatedComponentUnsafe()->SetRagdollNodeProvidesValidPose();
	}

	ApplyAdjustmentsToOutput( instance, output, state );

	if ( stateWeight < 1.0f && m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
	{
		CCacheBehaviorGraphOutput cacheTempPose( context );

		SBehaviorGraphOutput* tempPose = cacheTempPose.GetPose();

		m_cachedNodes[ prevState ]->Sample( context, instance, *tempPose );
		ApplyAdjustmentsToOutput( instance, *tempPose, prevState );

		BehaviorUtils::BlendingUtils::BlendPosesNormal( output, output, *tempPose, 1.0f - stateWeight );

		output.MergeEventsAndUsedAnims( output, *tempPose, 1.0f - stateWeight );
	}

	for ( CAnimationEventFired *firedEvent = output.m_eventsFired, *endFiredEvent = &output.m_eventsFired[output.m_numEventsFired]; firedEvent != endFiredEvent; ++ firedEvent )
	{
		if ( firedEvent->GetEventName() == m_switchAnimatedRagdollToRagdollEvent )
		{
			instance[ i_switchToRagdoll ] = true;
		}
	}
}

void CBehaviorGraphAnimatedRagdollNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_switchToRagdoll ] = false;

	CAnimatedComponent* animComponent = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( animComponent );

	if ( movingAgentComponent )
	{
		movingAgentComponent->AccessAnimationProxy().SetInAnimatedRagdoll( true );
	}

	//
	Float initialVelocityBoostZ = GEngine->GetRandomNumberGenerator().Get< Float >( m_initialVelocityBoostZ.X, Max(m_initialVelocityBoostZ.X, m_initialVelocityBoostZ.Y ) );
	Vector & velocityWS = instance[ i_velocityWS ];
	if ( instance.GetAnimatedComponent() )
	{
		velocityWS = movingAgentComponent? movingAgentComponent->GetVelocity() + Vector( 0.0f, 0.0f, initialVelocityBoostZ ) : Vector::ZEROS;
		velocityWS.Z = Max( velocityWS.Z, initialVelocityBoostZ );
		velocityWS.Z = Clamp( velocityWS.Z, -m_topVerticalVelocity, m_topVerticalVelocity );
	}
	else
	{
		velocityWS = Vector( 0.0f, 0.0f, initialVelocityBoostZ );
	}

	//
	EBehaviorGraphAnimatedRagdollNodeState & state = instance[ i_state ];
	state = (animComponent && animComponent->IsRagdolled( true )) || velocityWS.Mag2() < 3.0f ? BGARN_Ragdoll : BGARN_Flying; // TODO parameters for XY speed?

	//
	EBehaviorGraphAnimatedRagdollNodeState & prevState = instance[ i_prevState ];
	prevState = state;

	// increase a little bit
	velocityWS.X *= 1.2f;
	velocityWS.Y *= 1.2f;

	// check direction in which it should go
	Float bestDiff = 360.0f;
	Float dirIndex = 0.0f;
	Float applyAngleToMovement = 180.0f;
	Float relAngle = EulerAngles::YawFromXY( velocityWS.X, velocityWS.Y );
	relAngle = relAngle - animComponent->GetEntity()->GetWorldYaw();
	for( auto iDirIndex = m_dirIndices.Begin(), iDirEnd = m_dirIndices.End(); iDirIndex != iDirEnd; ++ iDirIndex )
	{
		Float diff = Abs( EulerAngles::NormalizeAngle180( relAngle - iDirIndex->m_relativeAngle ) );
		if ( bestDiff > diff )
		{
			bestDiff = diff;
			dirIndex = iDirIndex->m_dirIndexValue;
			applyAngleToMovement = iDirIndex->m_applyAngleToMovement;
			if (! iDirIndex->m_replacements.Empty())
			{
				// choose replacement
				Float probability = GEngine->GetRandomNumberGenerator().Get< Float >( 0.0f, 1.0f );
				for( auto iRepIndex = iDirIndex->m_replacements.Begin(), iRepEnd = iDirIndex->m_replacements.End(); iRepIndex != iRepEnd; ++ iRepIndex )
				{
					probability -= iRepIndex->m_probability;
					if ( probability <= 0.0f )
					{
						ASSERT( iRepIndex->m_index < m_dirIndices.Size() );
						SBehaviorGraphAnimatedRagdollDirDefinition const & repDef = m_dirIndices[ iRepIndex->m_index ];
						dirIndex = repDef.m_dirIndexValue;
						applyAngleToMovement = repDef.m_applyAngleToMovement;
						break;
					}
				}
			}
		}
	}
	instance[ i_dirIndex ] = dirIndex;
	instance[ i_dirIndexApplyAngle ] = applyAngleToMovement;

	// everything set up? perform activation and change state
	TBaseClass::OnActivated( instance );

	ChangeState( instance, state, true );
}

void CBehaviorGraphAnimatedRagdollNode::ChangeState( CBehaviorGraphInstance& instance, EBehaviorGraphAnimatedRagdollNodeState newState, Bool initial ) const
{
	if ( newState != BGARN_Flying &&
		 m_chanceToGoToRagdoll > 0.0f &&
		 GEngine->GetRandomNumberGenerator().Get< Float >( 0.0f, 1.0f ) < m_chanceToGoToRagdoll )
	{
		newState = BGARN_Ragdoll;
	}
	EBehaviorGraphAnimatedRagdollNodeState & state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState & prevState = instance[ i_prevState ];
	Float & stateWeight = instance[ i_stateWeight ];
	Float & timeInState = instance[ i_timeInState ];
	Float & flightTime = instance[ i_flightTime ];

	if ( ! initial && state == newState )
	{
		return;
	}

	if ( ! initial && stateWeight < 1.0f )
	{
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->Deactivate( instance );
		}
	}

	prevState = state;
	state = newState;
	stateWeight = initial? 1.0f : 0.0f;
	timeInState = 0.0f;
	flightTime = GEngine->GetRandomNumberGenerator().Get< Float >( m_maxFlightTime.X, Max(m_maxFlightTime.X, m_maxFlightTime.Y ) );

	if ( ! initial && stateWeight == 1.0f )
	{
		// we fully blended into new one
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->Deactivate( instance );
		}
	}

	if ( state != BGARN_Ragdoll )
	{
		if ( CMovingPhysicalAgentComponent* movingPhysicalAgentComponent = Cast< CMovingPhysicalAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			movingPhysicalAgentComponent->SetAnimatedMovement( true );
			movingPhysicalAgentComponent->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_On, CMovingAgentComponent::LS_AI );
			movingPhysicalAgentComponent->SetGravity( state != BGARN_Flying && state != BGARN_HitWall ); // gravity shouldn't affect us when we're flying or we have hit wall - if we have hit wall, stick to it
		}
	}

	if ( state == BGARN_HitWall )
	{
		instance[ i_hitWallNormalMS ] = Vector::ZEROS;
	}

	if ( state == BGARN_HitGround )
	{
		Vector hitNormalMS = Vector::EZ;
		if ( CMovingPhysicalAgentComponent const * mpac = Cast< CMovingPhysicalAgentComponent >( instance.GetAnimatedComponent() ) )
		{
			Matrix worldToLocal;
			mpac->GetWorldToLocal( worldToLocal );
			hitNormalMS = worldToLocal.TransformVector( mpac->GetTerrainNormal() );
			instance[ i_physicalRadius ] = mpac->GetPhysicalRadius();
		}
		// this is actually fallback based on terrain normal, but as terrain normal may give any results, it should be updated to proper value after ChangeState is called
		instance[ i_hitGroundNormalMS ] = hitNormalMS;
	}

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		CSyncInfo tempSync;
		m_cachedNodes[ state ]->Activate( instance );
		if ( state == BGARN_HitWall )
		{
			// TODO temp - before we change animation
			tempSync.m_currTime = 0.2f;
		}
		if ( state == BGARN_HitGround )
		{
			// TODO temp - before we change animation
			tempSync.m_currTime = 0.1f;
		}
		tempSync.m_prevTime = tempSync.m_currTime;
		m_cachedNodes[ state ]->SynchronizeTo( instance, tempSync );
	}

	if ( CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
	{
		movingAgentComponent->AccessAnimationProxy().SetInAnimatedRagdoll( state != BGARN_Ragdoll );
	}
}

void CBehaviorGraphAnimatedRagdollNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( CAnimatedComponent* ac = instance.GetAnimatedComponentUnsafe() )
	{
		ac->SetRagdollNodeProvidesValidPose( false );
	}

	if ( CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
	{
		movingAgentComponent->AccessAnimationProxy().SetInAnimatedRagdoll( false );
	}

	if ( CMovingPhysicalAgentComponent* movingPhysicalAgentComponent = Cast< CMovingPhysicalAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
	{
		movingPhysicalAgentComponent->SetAnimatedMovement( false );
		movingPhysicalAgentComponent->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_Off, CMovingAgentComponent::LS_AI );
		movingPhysicalAgentComponent->SetGravity( true );
	}

	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState prevState = instance[ i_prevState ];
	Float stateWeight = instance[ i_stateWeight ];

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->Deactivate( instance );
	}

	if ( stateWeight < 1.0f )
	{
		ASSERT( prevState != state, TXT("If state weight is not 1.0, prev state should be different from normal state") );
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->Deactivate( instance );
		}
	}
}

void CBehaviorGraphAnimatedRagdollNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState prevState = instance[ i_prevState ];
	Float stateWeight = instance[ i_stateWeight ];

	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->ProcessActivationAlpha( instance, alpha * stateWeight );
	}

	if ( stateWeight < 1.0f )
	{
		ASSERT( prevState != state, TXT("If state weight is not 1.0, prev state should be different from normal state") );
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->ProcessActivationAlpha( instance, alpha * ( 1.0f - stateWeight ) );
		}
	}
}

void CBehaviorGraphAnimatedRagdollNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimatedRagdollNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState prevState = instance[ i_prevState ];
	Float stateWeight = instance[ i_stateWeight ];

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		m_cachedNodes[ state ]->SynchronizeTo( instance, info );
	}

	if ( stateWeight < 1.0f )
	{
		ASSERT( prevState != state, TXT("If state weight is not 1.0, prev state should be different from normal state") );
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			m_cachedNodes[ prevState ]->SynchronizeTo( instance, info );
		}
	}
}

Bool CBehaviorGraphAnimatedRagdollNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	EBehaviorGraphAnimatedRagdollNodeState state = instance[ i_state ];
	EBehaviorGraphAnimatedRagdollNodeState prevState = instance[ i_prevState ];
	Float stateWeight = instance[ i_stateWeight ];

	Bool result = false;

	if ( m_cachedNodes.Size() > (Uint32)state && m_cachedNodes[ state ] )
	{
		result |= m_cachedNodes[ state ]->ProcessEvent( instance, event );
	}

	if ( stateWeight < 1.0f )
	{
		ASSERT( prevState != state, TXT("If state weight is not 1.0, prev state should be different from normal state") );
		if ( m_cachedNodes.Size() > (Uint32)prevState && m_cachedNodes[ prevState ] )
		{
			result |= m_cachedNodes[ prevState ]->ProcessEvent( instance, event );
		}
	}

	return result;
}

Float CBehaviorGraphAnimatedRagdollNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_dirIndex ];
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif

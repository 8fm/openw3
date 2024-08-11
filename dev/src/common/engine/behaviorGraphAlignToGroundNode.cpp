/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphAlignToGroundNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphUtils.inl"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "../engine/world.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAlignToGroundNode );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( AlignToGround );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphAlignToGroundNode::CBehaviorGraphAlignToGroundNode()
: m_groundNormalBlendTime( 0.5f )
, m_additionalOffset( 0.08f )
, m_eatEvent( false )
{
}

void CBehaviorGraphAlignToGroundNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_groundNormalWS;
	compiler << i_groundNormalTargetWS;
	compiler << i_groundOffsetWS;
	compiler << i_groundOffsetTargetWS;
	compiler << i_alignToGroundWeight;
	compiler << i_lastGroundNormalCheckLocationWS;
}

void CBehaviorGraphAlignToGroundNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_groundNormalWS ] = Vector::ZEROS;
	instance[ i_groundNormalTargetWS ] = Vector::ZEROS;
	instance[ i_groundOffsetWS ] = Vector::ZEROS;
	instance[ i_groundOffsetTargetWS ] = Vector::ZEROS;
	instance[ i_alignToGroundWeight ] = 0.0f;
	instance[ i_lastGroundNormalCheckLocationWS ] = Vector::ZEROS;
}

void CBehaviorGraphAlignToGroundNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_groundNormalWS );
	INST_PROP( i_groundNormalTargetWS );
	INST_PROP( i_groundOffsetWS );
	INST_PROP( i_groundOffsetTargetWS );
	INST_PROP( i_alignToGroundWeight );
	INST_PROP( i_lastGroundNormalCheckLocationWS );
}

void CBehaviorGraphAlignToGroundNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( AlignToGround );

	TBaseClass::OnUpdate( context, instance, timeDelta );
	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorGraphAlignToGroundNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( AlignToGround );

	Vector & groundNormalWS = instance[ i_groundNormalWS ];
	Vector & groundNormalTargetWS = instance[ i_groundNormalTargetWS ];
	Vector & groundOffsetWS = instance[ i_groundOffsetWS ];
	Vector & groundOffsetTargetWS = instance[ i_groundOffsetTargetWS ];
	Float & alignToGroundWeight = instance[ i_alignToGroundWeight ];
	Float targetAlignToGroundWeight = 0.0f;
	Vector & lastGroundNormalCheckLocationWS = instance[ i_lastGroundNormalCheckLocationWS ];

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
		CAnimationEventFired* firedEvent = output.m_eventsFired;
		for ( Int32 idx = output.m_numEventsFired; idx > 0; -- idx, ++ firedEvent )
		{
			if ( firedEvent->GetEventName() == CNAME( AlignToGround) )
			{
				targetAlignToGroundWeight = 1.0f;
				if ( m_eatEvent )
				{
					// remove this event from output - we don't want to mess with global AlignToGround node
					-- output.m_numEventsFired;
					// we move that last event to replace our current event (if it is the same, it is still faster than branching)
					output.m_eventsFired[ idx ] = output.m_eventsFired[ output.m_numEventsFired ];
				}
				break;
			}
		}
	}

	// align with ground if needed
	Vector currentLocationWS = instance.GetAnimatedComponent()->GetWorldPosition();
	if ( targetAlignToGroundWeight > 0.0f && alignToGroundWeight <= 0.0f )
	{
		// for now - do it once
		FindGroundNormal( instance );
		groundNormalWS = groundNormalTargetWS;
		groundOffsetWS = groundOffsetTargetWS;
		lastGroundNormalCheckLocationWS = currentLocationWS;
	}
	else if ( alignToGroundWeight > 0.0f && ( lastGroundNormalCheckLocationWS - currentLocationWS ).SquareMag2() > 0.25f * 0.25f )
	{
		FindGroundNormal( instance );
		lastGroundNormalCheckLocationWS = currentLocationWS;
	}
	alignToGroundWeight = BlendToWithBlendTime( alignToGroundWeight, targetAlignToGroundWeight, m_groundNormalBlendTime, instance[ i_timeDelta ] );
	if ( alignToGroundWeight > 0.0f )
	{
		groundNormalWS = BlendToWithBlendTime( groundNormalWS, groundNormalTargetWS, 0.2f, instance[ i_timeDelta ] );
		groundNormalWS.Normalize3();
		groundOffsetWS = BlendToWithBlendTime( groundOffsetWS, groundOffsetTargetWS, 0.2f, instance[ i_timeDelta ] );
		if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
		{
			// calculate align with ground vector
			Vector useGroundNormalWS = groundNormalWS * alignToGroundWeight + Vector( 0.0f, 0.0f, 1.0f ) * (1.0f - alignToGroundWeight);
			useGroundNormalWS.Normalize3();

			// use advanced character rotation
			EulerAngles characterRotationWS = ac->GetWorldRotation();
			Vector useGroundNormalMS = ( -characterRotationWS ).TransformVector( useGroundNormalWS );

			// calculate quaternion
			RedQuaternion alignToGround;
			alignToGround.SetShortestRotation( VectorToAnimVector( Vector( 0.0f, 0.0f, 1.0f ) ), VectorToAnimVector( useGroundNormalMS ) );

			// calculate rotated root
			RedQuaternion useRotation = RedQuaternion::Mul( output.m_outputPose[0].GetRotation(), alignToGround );
			useRotation.Normalize();
			output.m_outputPose[0].SetRotation( useRotation );

			// apply offset
			Matrix worldToLocal;
			ac->GetWorldToLocal( worldToLocal );
			AnimVector4 useTranslation = VectorToAnimVector( worldToLocal.TransformVector( ( groundOffsetWS + groundNormalWS * m_additionalOffset ) * alignToGroundWeight ) );
			output.m_outputPose[0].SetTranslation( Add( output.m_outputPose[0].GetTranslation(), useTranslation ) );
		}
	}
}

void CBehaviorGraphAlignToGroundNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
}

static Vector TraceAgainstGround( CPhysicsWorld* physicsWorld, CAnimatedComponent const * ac, Vector const & offset )
{
	CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
	CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );

	Vector refWS = ac->GetWorldPosition() + offset;
	Vector startWS = refWS + Vector( 0.0f, 0.0f, 1.5f );
	Vector endWS = refWS - Vector( 0.0f, 0.0f, 1.5f );
	SPhysicsContactInfo contactInfo;
	if( physicsWorld->RayCastWithSingleResult( startWS, endWS, include, exclude, contactInfo ) == TRV_Hit )
	{
		return contactInfo.m_position;
	}
	return refWS;
}

void CBehaviorGraphAlignToGroundNode::FindGroundNormal( CBehaviorGraphInstance& instance ) const
{
	Vector & groundNormalTargetWS = instance[ i_groundNormalTargetWS ];
	Vector & groundOffsetTargetWS = instance[ i_groundOffsetTargetWS ];

	groundNormalTargetWS = Vector( 0.0f, 0.0f, 1.0f );
	groundOffsetTargetWS = Vector::ZEROS;

	if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
	{
		if ( CWorld* world = ac->GetWorld() )
		{
			CPhysicsWorld* physicsWorld = nullptr;
			if ( world->GetPhysicsWorld( physicsWorld ) )
			{
				const Float e = 0.15f;
				const Vector a = TraceAgainstGround( physicsWorld, ac, Vector( 0.0f,	 e, 0.0f ) );
				const Vector b = TraceAgainstGround( physicsWorld, ac, Vector( -e,	-e, 0.0f ) );
				const Vector c = TraceAgainstGround( physicsWorld, ac, Vector(  e,	-e, 0.0f ) );
				const Vector a2b = b - a;
				const Vector a2c = c - a;
				groundNormalTargetWS = Vector::Cross( a2b, a2c );
				groundNormalTargetWS.Normalize3();
				const Float dotA = Vector::Dot3( groundNormalTargetWS, ( a - ac->GetWorldPosition() ) );
				const Float dotB = Vector::Dot3( groundNormalTargetWS, ( a - ac->GetWorldPosition() ) );
				const Float dotC = Vector::Dot3( groundNormalTargetWS, ( a - ac->GetWorldPosition() ) );
				Float offset = Max( dotA, Max( dotB, dotC ) );
				const Float maxOffset = 1.0f;
				groundOffsetTargetWS = groundNormalTargetWS * Min( offset, maxOffset );
			}
		}
	}
}

void CBehaviorGraphAlignToGroundNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	Vector groundNormalWS = instance[ i_groundNormalWS ];
	Vector groundOffsetWS = instance[ i_groundOffsetWS ];
	Float alignToGroundWeight = instance[ i_alignToGroundWeight ];

	if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
	{
		Vector pos = ac->GetWorldPosition();
		Uint8 alpha = (Uint8)( 255.0f * Clamp( alignToGroundWeight, 0.0f, 1.0f ) );
		frame->AddDebugFatLine( pos, pos + groundOffsetWS, Color( 255, 0, 0, alpha ), 0.02f, true );
		frame->AddDebugFatLine( pos + groundOffsetWS, pos + groundOffsetWS + groundNormalWS, Color( 0, 255, 0, alpha ), 0.02f, true );
	}
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif

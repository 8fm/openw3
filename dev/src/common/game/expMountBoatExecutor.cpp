
#include "build.h"
#include "expMountBoatExecutor.h"
#include "expSlideExecutor.h"
#include "expEvents.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "../core/mathUtils.h"
#include "../engine/renderFrame.h"

ExpMountBoatExecutor::ExpMountBoatExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn, Float blendOut, Float earlyEndOffset, Bool swapSide, Bool alignWhenCloseToEnd, Bool blockCloseToEnd, Bool alignTowardsInside )
	: ExpSlideExecutor( e, desc, setup, animName, blendIn, blendOut, earlyEndOffset, swapSide, alignWhenCloseToEnd, blockCloseToEnd, alignTowardsInside )
{
}

ExpMountBoatExecutor::~ExpMountBoatExecutor()
{
}

void ExpMountBoatExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	const Bool finished = UpdateAnimation( context.m_dt, result.m_timeRest, result );

	AnimQsTransform motion( AnimQsTransform::IDENTITY );
	AnimQuaternion remainingExRot( AnimQuaternion::IDENTITY );
	AnimVector4 remainingExTrans;

	Bool allowPostEndTransAdjustment = m_exploration != nullptr;
	if( m_animationEntry )
	{
		const CSkeletalAnimation* anim = m_animationEntry->GetAnimation();
		if( anim && m_timeEndToCollision > 0.0f ) // if time to end collision is greater than 0, it means that there is collision set up
		{
			// allow adjustment until to collision kicks in
			allowPostEndTransAdjustment &= m_animationState.m_currTime < m_timeStartToCollision;
		}
		// allow after end time to adjust to moving object
		// we're not afraid of negative time here, as we are adjusting our current location to where we were a while ago
		if( anim && ( allowPostEndTransAdjustment || m_animationState.m_prevTime < m_timeSyncRot ) )
		{
			if( m_animationState.m_currTime > m_timeStartRot )
			{
				const AnimQsTransform movement = anim->GetMovementBetweenTime( m_timeStartRot > m_animationState.m_prevTime ? m_timeStartRot : m_animationState.m_prevTime, m_timeSyncRot, 0 );
				remainingExRot = movement.GetRotation();
			}
		}
		if( anim && ( allowPostEndTransAdjustment || m_animationState.m_prevTime < m_timeSyncTrans ) )
		{
			if( m_animationState.m_currTime > m_timeStartTrans )
			{
				const AnimQsTransform movement = anim->GetMovementBetweenTime( m_timeStartTrans > m_animationState.m_prevTime ? m_timeStartTrans : m_animationState.m_prevTime, m_timeSyncTrans, 0 );

				remainingExTrans = movement.GetTranslation();
			}
		}
	}

	// update point on edge for moving objects - assume everything may move
	UpdatePointOnEdge( m_pointOnEdge, m_targetYaw, m_edgeMat );

	const ESliderResult slided = m_slider.Update( motion, remainingExRot, remainingExTrans, m_animationState.m_prevTime, m_animationState.m_currTime, m_entity, m_pointOnEdge, m_targetYaw + m_targetYawOffset, m_alignRotToEdgeExceeding, allowPostEndTransAdjustment );

	// Collision Point sliding - this must be moved to another place
	if( m_timeEndToCollision > 0.f && slided == SR_FinishedSliding )
	{
		// Delta is definitely to big to be a pure matrix calculation inaccuracy VERIFY THIS!!!
		// Vector delta = Sub4( m_entity->GetWorldPositionRef(), m_pointOnEdge );

		// We assume that we are slided to the 'm_pointOnEdge' and that from that point the slider will not update the position any more
		// so taking this as a fact we can safely calculate the ending position of animation and make a trace at this position
		Vector n = m_entity->GetLocalToWorld().GetAxisY();

		Matrix mat;
		mat.BuildFromDirectionVector( n );
		AnimQsTransform motionToCollision( AnimQsTransform::IDENTITY );
		if( m_animationEntry )
		{
			const CSkeletalAnimation* anim = m_animationEntry->GetAnimation();
			if( anim )
			{
				motionToCollision = anim->GetMovementBetweenTime( m_animationState.m_prevTime, m_timeEndToCollision, 0 );
			}
		}

		const Vector finalOffset = mat.TransformVector( reinterpret_cast< const Vector& >( motionToCollision.Translation ) );
		m_collisionPoint = m_entity->GetWorldPositionRef() + finalOffset;

		// keep minimal distance beyond edge
		if ( m_minDistBeyondEdge > 0.0f )
		{
			Vector nEdge, p1, p2;
			m_exploration->GetNormal( nEdge );
			m_exploration->GetEdgeWS( p1, p2 );
			Float distanceBeyondEdge = Vector::Dot2( m_collisionPoint - p1, nEdge );
			if ( distanceBeyondEdge < 0.0f )
			{
				distanceBeyondEdge = -distanceBeyondEdge;
				nEdge = -nEdge;
			}
			if ( distanceBeyondEdge < m_minDistBeyondEdge )
			{
				m_collisionPoint += nEdge * ( m_minDistBeyondEdge - distanceBeyondEdge );
			}
		}

		Vector startTrace = m_collisionPoint;
		Vector endTrace = m_collisionPoint;
		startTrace.Z += 1.5f;
		endTrace.Z -= 2.f;

		SPhysicsContactInfo contactInfo;
		CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
		CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
		CPhysicsWorld* physicsWorld = nullptr;
		if( GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) && physicsWorld->RayCastWithSingleResult( startTrace, endTrace, include, exclude, contactInfo ) == TRV_Hit )
		{
			m_collisionPoint.Z = contactInfo.m_position.Z + 0.001f;

			m_pointOnEdge = m_collisionPoint;

			// This is temporary so why not use the same slider :)
			m_slider.Setup( m_timeStartToCollision, m_timeEndToCollision, 0.f, 0.f );
			m_timeStartTrans = m_timeStartToCollision;
			m_timeSyncTrans = m_timeEndToCollision;
			m_timeEndToCollision = 0.f;
		}
		else
		{
			RED_LOG(ExplorationExecution, TXT("Exploration trace failed!"));
			HALT( "Exploration trace failed!!! This is very bad :(" );
		}
	}

	if ( m_slot.IsValid() )
	{
		if ( finished )
		{
			m_slot.ResetMotion();
		}
		else
		{
			// We need to apply only motion extraction from exploration animation not from the animation that we are blending from
			AnimQsTransform exTrans( AnimQsTransform::IDENTITY );
			if( m_animationEntry && m_animationEntry->GetAnimation() )
			{
				exTrans = m_animationEntry->GetAnimation()->GetMovementBetweenTime( m_animationState.m_prevTime, m_animationState.m_currTime, 0 );
			}

			Bool useBlending = m_animationState.m_currTime > m_blendIn;

			if ( slided != SR_NotSliding )
			{
				motion.SetMul( motion, exTrans );

				auto transl = motion.GetTranslation();
				auto ppos = GGame->GetPlayerEntity()->GetWorldPosition();
				transl.Z = m_pointOnEdge.Z - ppos.Z;
				motion.SetTranslation( transl );

				if ( useBlending )
				{
					m_slot.BlendMotion( motion );
				}
				else
				{
					m_slot.SetMotion( motion );
				}
			}
			else
			{
				if ( useBlending )
				{
					m_slot.BlendMotion( exTrans );
				}
				else
				{
					m_slot.SetMotion( exTrans );
				}
			}
		}
	}

	result.m_finished = finished;

	if ( result.m_finished )
	{
		result.AddNotification( ENT_SlideFinished );
	}
}

void ExpMountBoatExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
	ExpSlideExecutor::GenerateDebugFragments( frame );
}

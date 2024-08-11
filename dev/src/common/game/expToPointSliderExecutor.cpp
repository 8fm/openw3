
#include "build.h"
#include "ExpToPointSliderExecutor.h"
#include "expEvents.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "../engine/physicsCharacterWrapper.h"

//////////////////////////////////////////////////////////////////////////

ExpToPointSliderExecutor::ExpToPointSliderExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn, Float blendOut, Float earlyEndOffset )
	: ExpSingleAnimExecutor( setup, animName, blendIn, blendOut, earlyEndOffset )
	, m_exploration( e )
	, m_explorationDesc( desc )
	, m_alignRotToEdge( setup.AlignRotToEdge() )
	, m_alignRotToEdgeExceeding( setup.AlignRotToEdgeExceeding() )
	, m_hasValidEndPoint( false )
	, m_timeStartTrans( 0.f )
	, m_timeEndTrans( 0.5f )
{
	TDynArray< CExpSlideEvent* > slideEvents;

	Float timeStartRot = 0.f;
	Float timeEndRot = 0.f;

	if ( m_animationEntry )
	{
		m_animationEntry->GetEventsOfType( slideEvents );

		Bool transTest = false;
		Bool rotTest = false;
		const Uint32 slideEvtSize = slideEvents.Size();
		for ( Uint32 i=0; i<slideEvtSize; ++i )
		{
			CExpSlideEvent* evt = slideEvents[ i ];

			if ( !transTest && evt->Translation() )
			{
				m_timeStartTrans = evt->GetStartTime();
				m_timeEndTrans = m_timeStartTrans + evt->GetDuration();
				transTest = true;
			}

			if ( !rotTest && evt->Rotation() )
			{
				timeStartRot = evt->GetStartTime();
				timeEndRot = timeStartRot + evt->GetDuration();
				rotTest = true;
			}
		}
	}

	if( timeEndRot > 0.f )
	{
		CalcYaw( m_targetYaw );
	}

    m_slider.Setup( m_timeStartTrans, m_timeEndTrans, timeStartRot, timeEndRot );
}


ExpToPointSliderExecutor::~ExpToPointSliderExecutor()
{

}

void ExpToPointSliderExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	const Bool finished = UpdateAnimation( context.m_dt, result.m_timeRest, result );
	AnimQsTransform motion( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
	const ESliderResult slided = ! m_hasValidEndPoint? SR_NotSliding : m_slider.Update( motion, motion.getRotation(), motion.getTranslation(), m_animationState.m_prevTime, m_animationState.m_currTime, m_entity, m_endPoint, m_targetYaw, m_alignRotToEdgeExceeding, true );
#else
	const ESliderResult slided = ! m_hasValidEndPoint? SR_NotSliding : m_slider.Update( motion, motion.GetRotation(), motion.GetTranslation(), m_animationState.m_prevTime, m_animationState.m_currTime, m_entity, m_endPoint, m_targetYaw, m_alignRotToEdgeExceeding, true );
#endif
	

	if ( m_slot.IsValid() )
	{
		if ( finished )
		{
			m_slot.ResetMotion();

			CloseSlot();
		}
		else if ( slided != SR_NotSliding )
        {
			m_slot.SetMotion( motion );
		}
		else
		{
			m_slot.ResetMotion();
		}
	}

	result.m_finished = finished;

	if ( result.m_finished )
	{
        CAnimatedComponent* animComp = m_entity->GetRootAnimatedComponent();
        CMovingPhysicalAgentComponent* physcomp = SafeCast<CMovingPhysicalAgentComponent>( animComp );

        Vector ep = m_entity->GetWorldPositionRef();

		if ( m_hasValidEndPoint )
		{
			m_endPoint.Z -= 0.05f;

			physcomp->ForceMoveToPosition(m_endPoint, false);
		}

		result.AddNotification( ENT_SlideFinished );
	}
}

void ExpToPointSliderExecutor::CalcYaw( Float& yaw )
{
	if ( m_exploration && m_alignRotToEdge )
	{
		Vector p1, p2, n;
		m_exploration->GetEdgeWS( p1, p2 );
		const Vector p = ( p1 + p2 ) / 2.f;

		const Vector& ep = m_entity->GetWorldPositionRef();
		m_exploration->GetNormal( n );

		const Float dot2 = n.Dot2( ep - p );

		yaw = EulerAngles::YawFromXY( n.X, n.Y );
		yaw = EulerAngles::NormalizeAngle( yaw );

		if ( dot2 > 0.f )
		{
			yaw = EulerAngles::NormalizeAngle( yaw - 180.f );
		}
	}
	else
	{
		yaw = m_entity->GetLocalToWorld().GetYaw();
	}
}

void ExpToPointSliderExecutor::InitializeSlide()
{
    const Vector& ep = m_entity->GetWorldPositionRef();

#ifdef USE_HAVOK_ANIMATION
    hkQsTransform offsetTrans = m_animationEntry->GetAnimation()->GetMovementAtTime(m_timeEndTrans);
    Vector offset = TO_CONST_VECTOR_REF(offsetTrans.getTranslation());
#else
	RedQsTransform offsetTrans = m_animationEntry->GetAnimation()->GetMovementAtTime(m_timeEndTrans);
	Vector offset = reinterpret_cast< const Vector& >( offsetTrans.GetTranslation() );
#endif
	
	Vector entityLoc = m_entity->GetWorldPosition();
    Vector correctOffset = ep + m_entity->GetLocalToWorld().TransformVector(offset);

    SPhysicsContactInfo contactInfo;
    TDynArray< CComponent* > collisionComponents;

    CAnimatedComponent* animComp = m_entity->GetRootAnimatedComponent();

    CMovingPhysicalAgentComponent* physcomp = SafeCast<CMovingPhysicalAgentComponent>( animComp );


	if( physcomp )
    {
		Vector startTrace = correctOffset;
		Vector endTrace = correctOffset;
		if ( offset.Z < 0.0f ) // we're moving down
		{
			// Don't adjust underwater
			const CMRPhysicalCharacter* character = physcomp->GetPhysicalCharacter();
			if ( character )
			{
#ifdef USE_PHYSX
				CPhysicsCharacterWrapper* controller = character->GetCharacterController();
				if ( controller )
				{
#ifdef USE_PHYSX
					if( controller->GetWaterLevel() >= m_entity->GetWorldPosition().Z + 1.5f )
					{
						return ;
					}
#endif
				}
#endif // USE_PHYSX
			}

			startTrace.Z = Max( startTrace.Z, entityLoc.Z ) + 0.5f;
			endTrace.Z -= 1.5f; // TODO this is distance to the bottom below endpoint - move to parameters? 
		}
		else
		{
			startTrace.Z += 1.0f;
			endTrace.Z -= 1.5f; // TODO this is distance to the ground below end point that is on top - move to parameters?
		}

		SPhysicsContactInfo contactInfo;
		STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
		CPhysicsWorld* physicsWorld = nullptr;
		if( GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) && physicsWorld->RayCastWithSingleResult( startTrace, endTrace, include, 0, contactInfo ) == TRV_Hit )
		{
			m_hasValidEndPoint = true;
			m_endPoint.X = correctOffset.X;
			m_endPoint.Y = correctOffset.Y;
			m_endPoint.Z = contactInfo.m_position.Z + 0.001f;
		}
    }
}

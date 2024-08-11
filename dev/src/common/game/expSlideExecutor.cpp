
#include "build.h"
#include "expSlideExecutor.h"
#include "expEvents.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "../core/mathUtils.h"
#include "../engine/renderFrame.h"

//////////////////////////////////////////////////////////////////////////
void ExpSlideExecutor::EditBoneOffset( const AnimQsTransform& motionEx, AnimQsTransform& offset, AnimQsTransform& offsetWithMotion ) const
{
	Bool x, y, z;
	x = y = z = true;

	m_explorationDesc->UseEdgeOffsetAxis( x, y, z );
#ifdef USE_HAVOK_ANIMATION
	if ( !x )
	{
		offset.m_translation.zeroElement( 0 );
	}
	if ( !y )
	{
		offset.m_translation.zeroElement( 1 );
	}
	if ( !z )
	{
		offset.m_translation.zeroElement( 2 );
	}
#else
	if ( !x )
	{
		offset.Translation.X = 0.0f;
	}
	if ( !y )
	{
		offset.Translation.Y = 0.0f;
	}
	if ( !z )
	{
		offset.Translation.Z = 0.0f;
	}
#endif
	const Vector& edgeOffsetUserMS = m_edgeOffsetUserMS;
#ifdef USE_HAVOK_ANIMATION
	offset.m_translation.sub4( TO_CONST_HK_VECTOR_REF( edgeOffsetUserMS ) );
#else
	SetSub( offset.Translation, reinterpret_cast< const RedVector4& >( edgeOffsetUserMS ) );
#endif
	m_explorationDesc->UseMotionOffsetAxis( x, y, z );
	if ( x || y || z )
	{
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform slide = motionEx;

		if ( !x )
		{
			slide.m_translation.zeroElement( 0 );
		}
		if ( !y )
		{
			slide.m_translation.zeroElement( 1 );
		}
		if ( !z )
		{
			slide.m_translation.zeroElement( 2 );
		}

		offsetWithMotion = offset;
		offsetWithMotion.m_translation.add4( slide.m_translation );
#else
		RedQsTransform slide = motionEx;

		if ( !x )
		{
			slide.Translation.X = 0.0f;
		}
		if ( !y )
		{
			slide.Translation.Y = 0.0f;
		}
		if ( !z )
		{
			slide.Translation.Z = 0.0f;
		}

		offsetWithMotion = offset;
		SetAdd( offsetWithMotion.Translation, slide.Translation );
#endif
	}
}

ExpSlideExecutor::ExpSlideExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn, Float blendOut, Float earlyEndOffset, Bool swapSide, Bool alignWhenCloseToEnd, Bool blockCloseToEnd, Bool alignTowardsInside )
	: ExpSingleAnimExecutor( setup, animName, blendIn, blendOut, earlyEndOffset )
	, m_exploration( e )
	, m_explorationDesc( desc )
	, m_edgeOffsetUserMS( setup.GetEdgeOffset() )
	, m_offsetExpOwnerMS( setup.GetOffsetInModelSpace() )
	, m_blockCloseToEnd( blockCloseToEnd )
	, m_minDistBeyondEdge( 0.0f )
	, m_alignTransToEdge( setup.AlignTransToEdge() )
	, m_alignRotToEdge( setup.AlignRotToEdge() )
	, m_alignRotToEdgeExceeding( setup.AlignRotToEdgeExceeding() )
	, m_offset( Vector::ZERO_3D_POINT )
	, m_offsetWithMotion( Vector::ZERO_3D_POINT )
	, m_targetYawOffset( 0.0f )
	, m_timeStartTrans( 0.f )
	, m_timeEndTrans( 0.5f )
	, m_timeStartRot( 0.f )
	, m_timeEndRot( 0.f )
	, m_timeStartToCollision( 0.f )
	, m_timeEndToCollision( 0.f )
	, m_timeSyncTrans( 0.5f )
	, m_timeSyncRot( 0.5f )
{
#ifdef EXPLORATION_DEBUG
	m_isLeftBone = setup.m_useLeftSideForEdgeBone;
#endif

	TDynArray< CExpSyncEvent* > syncEvents;
	TDynArray< CExpSlideEvent* > slideEvents;
	AnimQsTransform motion( AnimQsTransform::IDENTITY );
	AnimQsTransform offset( AnimQsTransform::IDENTITY );
	AnimQsTransform offsetWithMotion( AnimQsTransform::IDENTITY );
	if ( m_animationEntry )
	{
		m_animationEntry->GetEventsOfType( syncEvents );
		m_animationEntry->GetEventsOfType( slideEvents );

		if ( syncEvents.Size() > 0 )
		{
			m_timeSyncTrans = syncEvents[ 0 ]->GetStartTime();
			m_timeSyncRot = syncEvents[ 0 ]->GetStartTime();
			for ( auto iSyncEvent = syncEvents.Begin(); iSyncEvent != syncEvents.End(); ++ iSyncEvent )
			{
				if ( (*iSyncEvent)->m_translation )
				{
					m_timeSyncTrans = (*iSyncEvent)->GetStartTime();
				}
				if ( (*iSyncEvent)->m_rotation )
				{
					m_timeSyncRot = (*iSyncEvent)->GetStartTime();
				}
			}
		}

		Bool transTest = false;
        Bool rotTest = false;
		Bool colTest = false;

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
				m_timeStartRot = evt->GetStartTime();
				m_timeEndRot = m_timeStartRot + evt->GetDuration();
				rotTest = true;
			}

			if( !colTest && evt->ToCollision() )
			{
				m_timeStartToCollision = evt->GetStartTime();
				m_timeEndToCollision = m_timeStartToCollision + evt->GetDuration();
				colTest = true;
			}
		}

		ASSERT(m_explorationDesc == setup.m_desc);
		const CName boneName = setup.GetEdgeBone();

		const CSkeletalAnimation* anim = m_animationEntry->GetAnimation();
		if( anim )
		{
			RED_ASSERT(anim->IsLoaded(), TXT("Those animations should always be loaded"));
			motion = anim->GetMovementBetweenTime( 0.f, m_timeSyncTrans, 0 );
			SkeletonBonesUtils::CalcBoneOffset( m_entity, anim, boneName, m_timeSyncTrans, offset );
			EditBoneOffset( motion, offset, offsetWithMotion );
		}
#ifdef USE_HAVOK_ANIMATION
		m_offset = TO_CONST_VECTOR_REF( offset.m_translation );
		m_offsetWithMotion = TO_CONST_VECTOR_REF( offsetWithMotion.m_translation );
#else
		m_offset = reinterpret_cast< const Vector&>( offset.Translation );
		m_offsetWithMotion = reinterpret_cast< const Vector&>( offsetWithMotion.Translation);
#endif
	}

	// limit to make sure we're move before end
	if ( m_timeEndToCollision > 0.0f )
	{
		const CSkeletalAnimation* anim = m_animationEntry->GetAnimation();
		if( anim )
		{
			m_timeEndToCollision = Min( m_timeEndToCollision, anim->GetDuration() - m_earlyEndOffset );
			m_timeEndToCollision = Max( m_timeStartToCollision, m_timeEndToCollision );
		}
	}

	m_side = CalcSide();
	if( swapSide )
	{
		m_side = !m_side;
	}
	// HACK BOAT & HORSE
	Int32 explorationID = e->GetId();

	// Replace this if content with m_explorationDesc->GetIsDoublesided() == false ?
	if ( explorationID == ET_Boat_B || explorationID == ET_Boat_Passenger_B  )
	{
		m_side = false;
	}

	// we may end up behind edge (this may happen as we now don't depend on edge location but use hardcoded angles to determine which animation to use, but even without that with test angle of 210 we could end on wrong side of edge)
	if ( explorationID >= ET_Horse_LF && explorationID <= ET_Horse_B )
	{
		m_side = true;
	}

	Bool pointOnEdgeCloseToEnd = false;
	CalcPointOnEdge( m_pointOnEdge, m_targetYaw, m_edgeMat, pointOnEdgeCloseToEnd );

	// try going straight forward when close - hardcoded just for sake of making it work properly and not be abused by .ini modifications
	if ( explorationID >= ET_Horse_LF && explorationID <= ET_Horse_B )
	{
		// always align rotation
		m_targetYawOffset = 0.0f;
		m_alignRotToEdgeExceeding = 0.0f;
	}
	else
	{
		if ( pointOnEdgeCloseToEnd && alignWhenCloseToEnd && m_alignRotToEdge && m_exploration )
		{
			Vector entityFwd = m_entity->GetLocalToWorld().GetAxisY(); // forward dir

			Vector p1, p2;
			m_exploration->GetEdgeWS( p1, p2 );
			Vector ph = ( p1 + p2 ) * 0.5f;
			if ( Vector::Dot2( ph - m_pointOnEdge, entityFwd ) < 0.0f )
			{
				// when entity points outside of edge, align rotation
				m_targetYawOffset = 0.0f;
				m_alignRotToEdgeExceeding = 0.0f;
			}
		}
	}

	if ( pointOnEdgeCloseToEnd && alignTowardsInside )
	{
		Vector toPointOnEdge = m_pointOnEdge - m_entity->GetWorldPosition();
		m_targetYawOffset = Clamp( EulerAngles::AngleDistance( m_targetYaw, EulerAngles::YawFromXY( toPointOnEdge.X, toPointOnEdge.Y ) ), -m_alignRotToEdgeExceeding, m_alignRotToEdgeExceeding );
		m_alignRotToEdgeExceeding = 0.0f;
	}

	m_slider.Setup( m_timeStartTrans, m_timeEndTrans, m_timeStartRot, m_timeEndRot );
}

ExpSlideExecutor::~ExpSlideExecutor()
{
	CloseSlot();
}

void ExpSlideExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	const Bool finished = UpdateAnimation( context.m_dt, result.m_timeRest, result );

	AnimQsTransform motion( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
	AnimQuaternion remainingExRot( hkQuaternion::getIdentity() );
#else
	AnimQuaternion remainingExRot( AnimQuaternion::IDENTITY );
#endif
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
#ifdef USE_HAVOK_ANIMATION
				remainingExRot = movement.getRotation();
#else
				remainingExRot = movement.GetRotation();
#endif
			}
		}
		if( anim && ( allowPostEndTransAdjustment || m_animationState.m_prevTime < m_timeSyncTrans ) )
		{
			if( m_animationState.m_currTime > m_timeStartTrans )
			{
				const AnimQsTransform movement = anim->GetMovementBetweenTime( m_timeStartTrans > m_animationState.m_prevTime ? m_timeStartTrans : m_animationState.m_prevTime, m_timeSyncTrans, 0 );
#ifdef USE_HAVOK_ANIMATION
				remainingExTrans = movement.getTranslation();
#else
				remainingExTrans = movement.GetTranslation();
#endif
			}
		}
	}

	// update point on edge for moving objects - assume everything may move
	UpdatePointOnEdge( m_pointOnEdge, m_targetYaw, m_edgeMat );

	const ESliderResult slided = m_slider.Update( motion, remainingExRot, remainingExTrans, m_animationState.m_prevTime, m_animationState.m_currTime, m_entity, m_pointOnEdge, m_targetYaw + m_targetYawOffset, m_alignRotToEdgeExceeding, allowPostEndTransAdjustment );

#ifdef EXPLORATION_DEBUG
	static Bool finishedLastFrame = false;
	if( finishedLastFrame )
	{
		const CAnimatedComponent* comp = m_entity->GetRootAnimatedComponent();
		const CName boneName = m_explorationDesc->GetEdgeBone( m_isLeftBone );
		const Int32 boneIdx = comp->FindBoneByName( boneName );
		m_pointBoneAfterSlide = m_entity->GetRootAnimatedComponent()->GetBoneMatrixWorldSpace( boneIdx ).GetTranslation();
		m_pointEntAfterSlide = m_entity->GetWorldPosition();
	}
	finishedLastFrame = slided == SR_FinishedSliding;
#endif

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
#ifdef USE_HAVOK_ANIMATION
		const Vector finalOffset = mat.TransformVector( TO_CONST_VECTOR_REF( motionToCollision.m_translation ) );
#else
		const Vector finalOffset = mat.TransformVector( reinterpret_cast< const Vector& >( motionToCollision.Translation ) );
#endif
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
#ifdef USE_HAVOK_ANIMATION
				motion.setMul( motion, exTrans );
#else
				motion.SetMul( motion, exTrans );
#endif
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

Bool ExpSlideExecutor::CalcSide() const
{
	Vector p1, p2, n;
	m_exploration->GetEdgeWS( p1, p2 );
	const Vector& ep = m_entity->GetWorldPositionRef();
	if( m_explorationDesc && ! m_explorationDesc->IsHorizontal() )
	{
		Vector nz = p2 - p1;
		nz.Normalize3();

		Float offS, offE;
		m_explorationDesc->GetEdgePointsOffset( offS, offE );
		Vector endPoint = p2 - (nz * offE);

		Vector pe = endPoint - ep;
		pe.Normalize3();
		const Float dot3 = pe.Dot3( nz );
		return dot3 > 0.f;
	}
	else
	{
		const Vector p = ( p1 + p2 ) / 2.f;

		m_exploration->GetNormal( n );

		Float dot2 = n.Dot2( ep - p );
		Bool side = dot2 > 0.f;

		//HACK - sometimes npc miss exploration ledge
		const Int32 eId = m_exploration->GetId();
		if ( side && eId == ET_Ledge )
		{			
			//if npc is close to the top edge...
			if( Abs( p.Z - ep.Z ) < 0.1f )
			{
				//...and is facing proper direction
				if( n.Dot2( m_entity->GetWorldForward() ) )
				{
					side = false;
				}
			}		
		}
		// End Of HACK

		return side;
	}
}

void ExpSlideExecutor::UpdatePointOnEdge( Vector& p, Float& yaw, Matrix& edgeMat ) const
{
	Matrix oldEdgeMat = edgeMat;
	m_exploration->GetMatWS( edgeMat );
	if ( edgeMat != oldEdgeMat )
	{
		// full inversion (we need to do that due to scale) is more expensive (cpu time wise), that's why there's "if" here
		Matrix invOldEdgeMat = oldEdgeMat.FullInverted();
		Vector localP = invOldEdgeMat.TransformPoint( p );
		p = edgeMat.TransformPoint( localP );
		
		// Update also rotation. Warning - rotation is calculated only in Z axis, maybe we should operate on full 3d rotation?
		yaw += edgeMat.GetYaw() - oldEdgeMat.GetYaw();
	}
}

void ExpSlideExecutor::CalcPointOnEdge( Vector& p, Float& yaw, Matrix& edgeMat, Bool& closeToEnd ) const
{
	const Vector& ep = m_entity->GetWorldPositionRef();

	Vector p1, p2, n;
	m_exploration->GetEdgeWS( p1, p2 );
	m_exploration->GetMatWS( edgeMat );
	Matrix edgeOwnerMat;
	m_exploration->GetParentMatWS( edgeOwnerMat );

	Vector epo = ep + m_entity->GetLocalToWorld().TransformVector( m_offsetWithMotion ) + edgeOwnerMat.TransformVector( -m_offsetExpOwnerMS ); // minus as we want to offset requested position

	Float proj = 0.f;

	if ( m_alignTransToEdge )
	{
		proj = MathUtils::GeometryUtils::ProjectVecOnEdge( epo, p1, p2 );
	}
	else
	{
		Vector rayDir = m_entity->GetLocalToWorld().GetAxisY();
		Vector rayOrigin = epo;

		if ( !MathUtils::GeometryUtils::TestIntersectionRayLine2D( rayDir, rayOrigin, p1, p2, 0, 1, proj ) )
		{
			proj = Clamp( proj, 0.f, 1.f );
		}
	}

	Float stepDistance = 0.f;
	if ( m_explorationDesc->UseEdgeGranularity( stepDistance ) )
	{
		static const	int	stepsBeforeEndUp	= 1;

		// Find the closest step
		Float	totalDistance	= ( p2.Z - p1.Z );
		Float	totalSteps		= MFloor( totalDistance / stepDistance ); // Total num steps can't exceed real total length
		Float	distance		= totalDistance * proj;
		Float	exactStep		= MCeil( distance / stepDistance ); // Ceil is better in case the ladder is in the ground MRound

		// Limit the step to max num lader spokes
		exactStep	= Min( exactStep, totalSteps - stepsBeforeEndUp );

		closeToEnd	= exactStep <= 1.f || exactStep >= totalSteps - stepsBeforeEndUp; // We need to account for the steps before the explorer starts leaving the ladder from the upper position

		// Recalc point on ladder
		proj					=  exactStep * stepDistance / totalDistance;

		Clamp( proj, 0.f, 1.f );
	}
	else
	{
		const Float p1p2 = (p1 - p2).Mag3(); // ED We need Mag3 for ladders
		if ( p1p2 != 0.0f )
		{
			const Float fromEndLoc = 0.3f;
			const Float fromEndClose = 0.6f;
			const Float projDistLoc = Min( fromEndLoc / p1p2, 0.5f );
			const Float projDistClose = Min( fromEndClose / p1p2, 0.5f );
			closeToEnd = proj < projDistClose || proj > 1.0f - projDistClose;
			if ( m_blockCloseToEnd )
			{
				proj = Clamp( proj, projDistLoc, 1.0f - projDistLoc );
			}
		}
	}

	MathUtils::GeometryUtils::GetPointFromEdge( proj, p1, p2, p );

#ifdef EXPLORATION_DEBUG
	m_pointBoneOnEdge = p;
	m_pointBoneAfterSlide = Vector::ZERO_3D_POINT;
	m_pointEntAfterSlide = Vector::ZERO_3D_POINT;
#endif

	Matrix mat;

	if ( m_alignRotToEdge )
	{
		m_exploration->GetNormal( n );

		yaw = EulerAngles::YawFromXY( n.X, n.Y );
		yaw = EulerAngles::NormalizeAngle( yaw );

        yaw += m_explorationDesc->GetEdgeYawOffset();

		if ( m_side )
		{
			static Bool FGH = true;
			if ( FGH )
			{
				n *= -1.f;
			}
			
			static Bool FGH2 = false;
			if ( FGH2 )
			{
				yaw = EulerAngles::NormalizeAngle( 180.f - yaw );
			}

			static Bool FGH3 = true;
			if ( FGH3 )
			{
				yaw = EulerAngles::NormalizeAngle( yaw - 180.f );
			}
		}
	}
	else
	{
		n = m_entity->GetLocalToWorld().GetAxisY();

		yaw = m_entity->GetLocalToWorld().GetYaw();
	}

	mat.BuildFromDirectionVector( n );

	const Vector finalOffset = mat.TransformVector( m_offset ) + edgeOwnerMat.TransformVector( -m_offsetExpOwnerMS ); // minus as we want to offset requested position
	p -= finalOffset;
}

void ExpSlideExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
#ifdef EXPLORATION_DEBUG
	frame->AddDebugSphere( m_pointBoneOnEdge, 0.1f, Matrix::IDENTITY, Color::RED );
	frame->AddDebugSphere( m_pointBoneAfterSlide, 0.1f, Matrix::IDENTITY, Color::GREEN );

	frame->AddDebugSphere( m_pointEntAfterSlide, 0.1f, Matrix::IDENTITY, Color::GREEN );

	const String text = String::Printf( TXT("Distance error: Entity - %f; Bone - %f"), (m_pointEntAfterSlide - m_pointOnEdge).Mag3(), (m_pointBoneAfterSlide - m_pointBoneOnEdge).Mag3() );
	frame->AddDebugScreenText( 500, 100, text );
#endif

	frame->AddDebugSphere( m_pointOnEdge, 0.1f, Matrix::IDENTITY, Color::RED );

	const Matrix& ent = m_entity->GetLocalToWorld();
	frame->AddDebugAxis( ent.GetTranslation(), ent, 1.f );

	frame->AddDebugSphere( m_collisionPoint, 0.2f, Matrix::IDENTITY, Color::BLACK );

	frame->AddDebugFatLine( m_collisionPoint + Vector(0.0f, 0.0f, 1.0f), m_collisionPoint, Color(255,0,0), 0.02f, true );

	m_slider.GenerateDebugFragments( frame );
}

#ifdef EXPLORATION_DEBUG
Bool ExpSlideExecutor::UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result )
{
	m_animationState.m_prevTime = m_animationState.m_currTime;
	m_animationState.m_currTime += m_timeMul * dt;

	if( m_animationState.m_currTime > m_timeSyncTrans && m_animationState.m_prevTime < m_timeSyncTrans )
	{
		m_animationState.m_currTime = m_timeSyncTrans;
	}

	if ( m_firstUpdate && m_animationState.m_currTime != 0.f )
	{
		m_firstUpdate = false;

		result.AddNotification( ENT_AnimationStarted, m_animationState.m_animation );
	}

	Float marker1, marker2;
	WrapTime( m_animationState.m_prevTime, marker1 );
	WrapTime( m_animationState.m_currTime, marker2 );

	AlignPreTimeToCurr( m_animationState.m_prevTime, m_animationState.m_currTime );

	if ( m_slot.IsValid() )
	{
		ASSERT( m_animationState.m_prevTime >= 0.f );
		ASSERT( m_animationState.m_currTime >= 0.f );

		m_slot.PlayAnimation( m_animationState, CalcWeight( m_animationState.m_currTime ) );

		if ( m_animationEntry )
		{
			CollectEvents( m_animationState.m_prevTime, m_animationState.m_currTime, result );
		}
	}

	UnwrapTime( m_animationState.m_prevTime, marker1 );
	UnwrapTime( m_animationState.m_currTime, marker2 );

	const Float absTime = MAbs( m_animationState.m_currTime );
	if ( absTime >= m_duration )
	{
		timeRest = absTime - m_duration;

		result.AddNotification( ENT_AnimationFinished, m_animationState.m_animation );

		return true;
	}

	return false;
}
#endif

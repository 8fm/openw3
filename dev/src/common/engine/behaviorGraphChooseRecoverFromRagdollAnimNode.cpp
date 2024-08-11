/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphChooseRecoverFromRagdollAnimNode.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
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

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphChooseRecoverFromRagdollAnimNode );
IMPLEMENT_RTTI_ENUM( EBehaviorGraphChooseRecoverFromRagdollAnimMode );

///////////////////////////////////////////////////////////////////////////////

// RED_DEFINE_STATIC_NAME( pelvis ); <- declared in name registry

RED_DEFINE_STATIC_NAME( torso3 );

RED_DEFINE_STATIC_NAME( AlignToGround );

///////////////////////////////////////////////////////////////////////////////

#define FROM_FRONT	0
#define FROM_BACK	1
#define FROM_LEFT	2
#define FROM_RIGHT	3

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphChooseRecoverFromRagdollAnimNode::CBehaviorGraphChooseRecoverFromRagdollAnimNode()
	: m_mode( RFR_FrontBack )
	, m_additionalOneFrameRotationYaw( 0.0f )
	, m_pelvisBone( CNAME( pelvis ) )
	, m_pelvisBoneFrontAxis( A_X )
	, m_pelvisBoneFrontAxisInverted( true )
	, m_pelvisBoneWeight( 1.0f )
	, m_shoulderBone( CNAME( torso3 ) )
	, m_shoulderBoneFrontAxis( A_Y )
	, m_shoulderBoneFrontAxisInverted( false )
	, m_shoulderBoneWeight( 1.0f )
{
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_pelvisBoneIdx;
	compiler << i_shoulderBoneIdx;
	compiler << i_chosenChildIdx;
	compiler << i_oneFrameRotation;
	compiler << i_groundNormalWS;
	compiler << i_groundNormalWeight;
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_pelvisBoneIdx ] = skeleton->FindBoneByName( m_pelvisBone );
	instance[ i_shoulderBoneIdx ] = skeleton->FindBoneByName( m_shoulderBone );
	instance[ i_chosenChildIdx ] = 0;
	instance[ i_oneFrameRotation ] = EulerAngles::ZEROS;
	instance[ i_groundNormalWS ] = Vector::ZEROS;
	instance[ i_groundNormalWeight ] = 0.0f;
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_pelvisBoneIdx );
	INST_PROP( i_shoulderBoneIdx );
	INST_PROP( i_chosenChildIdx );
	INST_PROP( i_oneFrameRotation );
	INST_PROP( i_groundNormalWS );
	INST_PROP( i_groundNormalWeight );
}

Int32 CBehaviorGraphChooseRecoverFromRagdollAnimNode::GetNumberOfChildren() const
{
	switch ( m_mode )
	{
	case RFR_JustOne:			return 1;	break;
	case RFR_FrontBack:			return 2;	break;
	case RFR_FrontBackSides:	return 4;	break;
	}
	return 0;
}

String CBehaviorGraphChooseRecoverFromRagdollAnimNode::CreateSocketName( Int32 childIdx ) const
{
	switch ( m_mode )
	{
	case RFR_JustOne:		return TXT("one only"); break;
	case RFR_FrontBack:
		switch ( childIdx )
		{
		case FROM_FRONT:	return TXT("from front"); break;
		case FROM_BACK:		return TXT("from back"); break;
		}
		break;
	case RFR_FrontBackSides:
		switch ( childIdx )
		{
		case FROM_FRONT:	return TXT("from front"); break;
		case FROM_BACK:		return TXT("from back"); break;
		case FROM_LEFT:		return TXT("from left"); break;
		case FROM_RIGHT:	return TXT("from right"); break;
		}
		break;
	}
	return TXT("??");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT( "mode" ) )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	

	for( Int32 i = 0; i < GetNumberOfChildren(); ++ i )
	{
		const String socketName = CreateSocketName( i );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CName( socketName ) ) );
	}
}

#endif

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input nodes
	m_cachedInputNodes.Clear();
	const Uint32 numInputs = GetNumberOfChildren();
	for ( Uint32 i=0; i<numInputs; i++ )
	{
		const String socketName = CreateSocketName( i );
		m_cachedInputNodes.PushBack( CacheBlock( socketName ) );
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( ChooseRecoverFromRagdollAnim );
	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		m_cachedInputNodes[ chosenChildIdx ]->Update( context, instance, timeDelta );
	}
	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	Vector const & groundNormalWS = instance[ i_groundNormalWS ];
	Float & groundNormalWeight = instance[ i_groundNormalWeight ];
	EulerAngles& oneFrameRotation = instance[ i_oneFrameRotation ];
	Float targetGroundNormalWeight = 0.0f;

	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		m_cachedInputNodes[ chosenChildIdx ]->Sample( context, instance, output );
		CAnimationEventFired* firedEvent = output.m_eventsFired;
		for ( Int32 idx = output.m_numEventsFired; idx > 0; -- idx, ++ firedEvent )
		{
			if ( firedEvent->GetEventName() == CNAME( AlignToGround) )
			{
				// remove this event from output - we don't want to mess with global AlignToGround node
				-- output.m_numEventsFired;
				// we move that last event to replace our current event (if it is the same, it is still faster than branching)
				output.m_eventsFired[ idx ] = output.m_eventsFired[ output.m_numEventsFired ];
				targetGroundNormalWeight = 1.0f;
				break;
			}
		}
	}

	// align with ground if needed
	groundNormalWeight = BlendToWithBlendTime( groundNormalWeight, targetGroundNormalWeight, 0.5f, instance[ i_timeDelta ] );
	if ( groundNormalWeight > 0.0f )
	{
		if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
		{
			// calculate align with ground vector
			Vector useGroundNormalWS = groundNormalWS * groundNormalWeight + Vector( 0.0f, 0.0f, 1.0f ) * (1.0f - groundNormalWeight);
			useGroundNormalWS.Normalize3();

			// use advanced character rotation
			EulerAngles characterRotationWS = ac->GetWorldRotation() + oneFrameRotation;
			Vector useGroundNormalMS = ( -characterRotationWS ).TransformVector( useGroundNormalWS );

			// calculate quaternion
			RedQuaternion alignToGround;
			alignToGround.SetShortestRotation( VectorToAnimVector( Vector( 0.0f, 0.0f, 1.0f ) ), VectorToAnimVector( useGroundNormalMS ) );

			// calculate rotated root
			RedQuaternion useRotation = RedQuaternion::Mul( output.m_outputPose[0].GetRotation(), alignToGround );
			useRotation.Normalize();
			output.m_outputPose[0].SetRotation( useRotation );
		}
	}

	// send rotation (we should be with weight 100%, so we can do this in one frame)
	if ( oneFrameRotation != EulerAngles::ZEROS )
	{
		Vector oneFrameRotationQuat = oneFrameRotation.ToQuat();
		RedQuaternion oneFrameQuat = RedQuaternion( oneFrameRotationQuat.X, oneFrameRotationQuat.Y, oneFrameRotationQuat.Z, oneFrameRotationQuat.W );
		output.m_deltaReferenceFrameLocal.SetRotation( RedQuaternion::Mul( output.m_deltaReferenceFrameLocal.GetRotation(), oneFrameQuat ) );
		oneFrameRotation = EulerAngles::ZEROS;
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		m_cachedInputNodes[ chosenChildIdx ]->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		m_cachedInputNodes[ chosenChildIdx ]->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphChooseRecoverFromRagdollAnimNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		return m_cachedInputNodes[ chosenChildIdx ]->ProcessEvent( instance, event );
	}
	return false;
}

Bool CBehaviorGraphChooseRecoverFromRagdollAnimNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];
	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		return m_cachedInputNodes[ chosenChildIdx ]->ProcessForceEvent( instance, event );
	}
	return false;
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	ChooseAnimAndRotation( instance );

	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];

	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		return m_cachedInputNodes[ chosenChildIdx ]->Activate( instance );
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];

	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		return m_cachedInputNodes[ chosenChildIdx ]->Deactivate( instance );
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	Int32 const & chosenChildIdx = instance[ i_chosenChildIdx ];

	if ( m_cachedInputNodes[ chosenChildIdx ] )
	{
		return m_cachedInputNodes[ chosenChildIdx ]->ProcessActivationAlpha( instance, alpha );
	}
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

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::ChooseAnimAndRotation( CBehaviorGraphInstance& instance ) const
{
	Int32 & chosenChildIdx = instance[ i_chosenChildIdx ];
	EulerAngles & oneFrameRotation = instance[ i_oneFrameRotation ];
	Vector & groundNormalWS = instance[ i_groundNormalWS ];
	Float & groundNormalWeight = instance[ i_groundNormalWeight ];

	chosenChildIdx = 0;
	oneFrameRotation = EulerAngles::ZEROS;
	groundNormalWS = Vector( 0.0f, 0.0f, 1.0f );
	groundNormalWeight = 1.0f;

	groundNormalWS = Vector( 0.7f, 0.0f, 0.7f );

	if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
	{
		EulerAngles requestedRot = ac->GetWorldRotation();

		// calculate requested rotation basing on spine location
		Int32 const pelvisBoneIdx = instance[ i_pelvisBoneIdx ];
		Int32 const shoulderBoneIdx = instance[ i_shoulderBoneIdx ];
		if ( pelvisBoneIdx >= 0 && shoulderBoneIdx >= 0 )
		{
			Vector const pelvisLocWS = ac->GetBoneMatrixWorldSpace( pelvisBoneIdx ).GetTranslation();
			Vector const shoulderLocWS = ac->GetBoneMatrixWorldSpace( shoulderBoneIdx ).GetTranslation();
			Vector const pelvisToShoulderWS = shoulderLocWS - pelvisLocWS;
			requestedRot = EulerAngles( 0.0f, 0.0f, EulerAngles::YawFromXY( pelvisToShoulderWS.X, pelvisToShoulderWS.Y ) );
		}

		// calculate front vector
		Vector frontVector = Vector::ZEROS;
		Float frontVectorWeight = 0.0f;
		IncreaseFrontVector( ac, instance[ i_pelvisBoneIdx ], m_pelvisBoneFrontAxis, m_pelvisBoneFrontAxisInverted, m_pelvisBoneWeight, frontVector, frontVectorWeight );
		IncreaseFrontVector( ac, instance[ i_shoulderBoneIdx ], m_shoulderBoneFrontAxis, m_shoulderBoneFrontAxisInverted, m_shoulderBoneWeight, frontVector, frontVectorWeight );

		if ( frontVectorWeight != 0.0f )
		{
			frontVector.Normalize3();

			if ( m_mode == RFR_JustOne )
			{
				chosenChildIdx = FROM_FRONT;
			}
			else
			{
				if ( frontVector.Z <= 0.0f )
				{
					chosenChildIdx = FROM_FRONT;
				}
				else
				{
					chosenChildIdx = FROM_BACK;
				}

				if ( m_mode == RFR_FrontBackSides && Abs( frontVector.Z ) < 0.7f )
				{
					// calculate difference between front vector and requested rot (along spine)
					Float frontYaw = EulerAngles::YawFromXY( frontVector.X, frontVector.Y );
					Float diffYaw = EulerAngles::NormalizeAngle180( frontYaw - requestedRot.Yaw );
					chosenChildIdx = diffYaw < 0.0f? FROM_RIGHT : FROM_LEFT; // when laying on right side, front is pointing forward
				}
			}
		}

		if ( CWorld* world = ac->GetWorld() )
		{
			CPhysicsWorld* physicsWorld = nullptr;
			if ( world->GetPhysicsWorld( physicsWorld ) )
			{
				Float const e = 0.15f;
				Vector a = TraceAgainstGround( physicsWorld, ac, Vector( 0.0f,	 e, 0.0f ) );
				Vector b = TraceAgainstGround( physicsWorld, ac, Vector( -e,	-e, 0.0f ) );
				Vector c = TraceAgainstGround( physicsWorld, ac, Vector(  e,	-e, 0.0f ) );
				Vector ab = b - a;
				Vector ac = c - a;
				groundNormalWS = Vector::Cross( ab, ac );
				groundNormalWS.Normalize3();
			}
		}

		// when laying on back, rotate character by 180'
		if ( chosenChildIdx == FROM_BACK )
		{
			requestedRot.Yaw += 180.0f;
		}

		requestedRot.Yaw += m_additionalOneFrameRotationYaw;

		// Rotate, so the pose matches spine
		EulerAngles currentRot = ac->GetWorldRotation();
		oneFrameRotation = EulerAngles( 0.0f, 0.0f, EulerAngles::NormalizeAngle180( ( requestedRot - currentRot ).Yaw ) );
	}
}

void CBehaviorGraphChooseRecoverFromRagdollAnimNode::IncreaseFrontVector( CAnimatedComponent const * ac, Int32 boneIdx, EAxis frontAxis, Bool invertAxis, Float boneWeight, Vector& inOutFrontVector, Float& inOutWeight ) const
{
	if ( boneIdx >= 0 )
	{
		Float const axisWeight = invertAxis? -boneWeight : boneWeight;
		Matrix const boneMat = ac->GetBoneMatrixWorldSpace( boneIdx );
		switch ( frontAxis )
		{
		case A_X:	inOutFrontVector += boneMat.GetAxisX() * axisWeight; inOutWeight += boneWeight;	break;
		case A_Y:	inOutFrontVector += boneMat.GetAxisY() * axisWeight; inOutWeight += boneWeight;	break;
		case A_Z:	inOutFrontVector += boneMat.GetAxisZ() * axisWeight; inOutWeight += boneWeight;	break;
		}
	}
}

CSkeleton* CBehaviorGraphChooseRecoverFromRagdollAnimNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetSkeleton();
}

///////////////////////////////////////////////////////////////////////////////

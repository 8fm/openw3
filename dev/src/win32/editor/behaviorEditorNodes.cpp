/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphValueNode.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/behaviorGraphSocket.h"
#include "../../common/engine/skeletalAnimationContainer.h"

#include "behaviorEditorNodes.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/graphConnectionRebuilder.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationTrajDrawNode );

void CBehaviorGraphAnimationTrajDrawNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( m_generateEditorFragments && IsActive( instance ) )
	{
		{
			const CAnimatedComponent* ac = instance.GetAnimatedComponent();
			const Matrix& localToWorld = ac->GetLocalToWorld();

			Color color( 100, 100, 100 );

			Color color2( 200, 200, 200 );
			TDynArray< Vector >& positions = instance[ i_allPositions ];
			TDynArray< Vector >& rotations = instance[ i_allRotations ];
			TDynArray< Vector >& vels = instance[ i_allVels ];

			const Float minVel = instance[ i_minVel ];
			const Float maxVel = instance[ i_maxVel ];

			const Uint32 size = positions.Size();

			Vector prev;

			for ( Uint32 i=0; i<size; ++i )
			{
				const Vector& pos = positions[ i ];
				const Vector& rot = rotations[ i ];

				AnimQsTransform temp( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
				temp.m_translation = TO_CONST_HK_VECTOR_REF( pos );
				temp.m_rotation.m_vec = TO_CONST_HK_VECTOR_REF( rot );

				Matrix tempMat;
				HavokTransformToMatrix_Renormalize( temp, &tempMat );

				tempMat = tempMat * localToWorld;
#else
				temp.Translation = reinterpret_cast< const RedVector4& >( pos );
				temp.Rotation.Quat = reinterpret_cast< const RedVector4& >( rot );
				
				RedMatrix4x4 conversionMatrix = temp.ConvertToMatrixNormalized();

				Matrix tempMat = reinterpret_cast< const Matrix& >( conversionMatrix );

				tempMat = tempMat * localToWorld;
#endif
				if ( m_applyOffsets )
				{
					const Vector& offsetPosA = instance[ i_offsetPosA ];
					const Vector& offsetPosB = instance[ i_offsetPosB ];
					const Vector& offsetRotA = instance[ i_offsetRotA ];
					const Vector& offsetRotB = instance[ i_offsetRotB ];

					static Float wA = 1.f;

					EulerAngles rotA( offsetRotA.X * wA, offsetRotA.Y * wA, offsetRotA.Z * wA );
					Matrix offsetA = rotA.ToMatrix();
					offsetA.SetTranslation( offsetPosA * wA );

					EulerAngles rotB( offsetRotB.X, offsetRotB.Y, offsetRotB.Z );
					Matrix offsetB = rotB.ToMatrix();
					offsetB.SetTranslation( offsetPosB );

					tempMat = tempMat * offsetA;
				}

				if ( m_showRot )
				{
					frame->AddDebugAxis( tempMat.GetTranslation(), tempMat, 0.1f, color, true );
				}

				if ( i > 0 && m_showVel )
				{
					const Vector& vel = vels[ i ];

					Float val = vel.Mag3();

					Float p = ( val - minVel ) / ( maxVel - minVel );

					color2.R = (Uint8)Lerp< Float >( p*p, 80.f, 255.f );
					color2.G = 80;
					color2.B = 80;
				}

				if ( i > 0 && m_showLine )
				{
					frame->AddDebugLine( prev, tempMat.GetTranslation(), color2, true );
				}

				prev = tempMat.GetTranslation();
			}

			if ( m_showPoints )
			{
				const Matrix& pointA = instance[ i_pointA ];
				const Matrix& pointB = instance[ i_pointB ];

				Matrix tempMat;

				tempMat = pointA * localToWorld;
				frame->AddDebugSphere( tempMat.GetTranslation(), 0.1f, Matrix::IDENTITY, Color::WHITE, true );

				tempMat = pointB * localToWorld;
				frame->AddDebugSphere( tempMat.GetTranslation(), 0.1f, Matrix::IDENTITY, Color::WHITE, true );
			}

			if ( m_showCurrent )
			{
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform temp;
				temp.setIdentity();
				temp.m_translation = TO_CONST_HK_VECTOR_REF( instance[ i_position ] );
				temp.m_rotation.m_vec = TO_CONST_HK_VECTOR_REF( instance[ i_rotation ] );

				Matrix tempMat;
				HavokTransformToMatrix_Renormalize( temp, &tempMat );
#else
				RedQsTransform temp;
				temp.SetIdentity();
				temp.Translation = reinterpret_cast< const RedVector4& >( instance[ i_position ] );
				temp.Rotation.Quat = reinterpret_cast< const RedVector4& >( instance[ i_rotation ] );
				RedMatrix4x4 conversionMatrix = temp.ConvertToMatrixNormalized();
				Matrix tempMat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
				frame->AddDebugAxis( tempMat.GetTranslation(), tempMat, 0.1f, true );
			}
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationTrajDrawNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Value ) ) );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( OffsetPosA ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( OffsetPosB ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( OffsetRotA ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( OffsetRotB ) ) );
}

#endif

void CBehaviorGraphAnimationTrajDrawNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheBlock( TXT("Input") );

	m_cachedOffsetPosA = CacheVectorValueBlock( TXT("OffsetPosA") );
	m_cachedOffsetPosB = CacheVectorValueBlock( TXT("OffsetPosB") );
	m_cachedOffsetRotA = CacheVectorValueBlock( TXT("OffsetRotA") );
	m_cachedOffsetRotB = CacheVectorValueBlock( TXT("OffsetRotB") );
}

void CBehaviorGraphAnimationTrajDrawNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_animation;

	compiler << i_bone;
	compiler << i_time;

	compiler << i_allPositions;
	compiler << i_allRotations;
	compiler << i_allVels;

	compiler << i_position;
	compiler << i_rotation;

	compiler << i_minVel;
	compiler << i_maxVel;

	compiler << i_offsetPosA;
	compiler << i_offsetPosB;
	compiler << i_offsetRotA;
	compiler << i_offsetRotB;

	compiler << i_pointA;
	compiler << i_pointB;
}

void CBehaviorGraphAnimationTrajDrawNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );
	
	instance[ i_animation ] = instance.GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( m_animationName );

	instance[ i_bone ] = FindBoneIndex( m_bone, instance );

	instance[ i_position ] = Vector::ZERO_3D_POINT;
	instance[ i_rotation ] = Vector::ZERO_3D_POINT;

	instance[ i_offsetPosA ] = Vector::ZERO_3D_POINT;
	instance[ i_offsetPosB ] = Vector::ZERO_3D_POINT;
	instance[ i_offsetRotA ] = Vector::ZERO_3D_POINT;
	instance[ i_offsetRotB ] = Vector::ZERO_3D_POINT;

	instance[ i_time ] = 0.f;

	const Int32 boneIdx = instance[ i_bone ];

	// Cache trajectory
	CSkeletalAnimationSetEntry* animEntry = instance[ i_animation ];
	if ( animEntry && boneIdx != -1 )
	{
		CSkeletalAnimation* anim = animEntry->GetAnimation();
		if ( anim )
		{
			const CAnimatedComponent* ac = instance.GetAnimatedComponent();
			const CSkeleton* skeleton = ac->GetSkeleton();

			TDynArray< Vector >& positions = instance[ i_allPositions ];
			TDynArray< Vector >& rotations = instance[ i_allRotations ];
			TDynArray< Vector >& vels = instance[ i_allVels ];

			Uint32 numBones = skeleton->GetBonesNum();
			Uint32 numTracks = skeleton->GetTracksNum();

			SBehaviorGraphOutput pose;
			pose.Init( numBones, numTracks );
#ifdef USE_HAVOK_ANIMATION
			pose.m_deltaReferenceFrameLocal.setIdentity();
#else
			pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
			const Float duration = anim->GetDuration();
			const Float timeStep = 1.f / 60.f;
			Float time = 0.f;

			Vector prevPos = Vector::ZERO_3D_POINT;

			Float& minVel = instance[ i_minVel ];
			Float& maxVel = instance[ i_maxVel ];

			minVel = NumericLimits< Float >::Max();
			maxVel = NumericLimits< Float >::Min();

			while ( time < duration )
			{
				anim->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

				pose.ExtractTrajectory( ac );

				if ( m_applyMotion )
				{
					AnimQsTransform motion = anim->GetMovementAtTime( time );
					AnimQsTransform& root = pose.m_outputPose[ 0 ];
#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else
					root.SetMul( root, motion );
#endif
				}
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				transform.m_rotation.normalize();

				Vector pos = TO_CONST_VECTOR_REF( transform.m_translation );
				Vector rot = TO_CONST_VECTOR_REF( transform.m_rotation );
#else
				RedQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				transform.Rotation.Normalize();

				Vector pos = reinterpret_cast< const Vector& >( transform.Translation );
				Vector rot = reinterpret_cast< const Vector& >( transform.Rotation );
#endif
				positions.PushBack( pos );
				rotations.PushBack( rot );

				Vector vel = ( pos - prevPos ) / timeStep;
				vels.PushBack( vel );

				Float velMag = vel.Mag3();
				if ( velMag > maxVel )
				{
					maxVel = velMag;
				}
				if ( velMag < minVel )
				{
					minVel = velMag;
				}

				time += timeStep;
			}

			if ( MAbs( time - duration ) > 0.01f )
			{
				anim->Sample( duration, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

				pose.ExtractTrajectory( ac );

				if ( m_applyMotion )
				{
					AnimQsTransform motion = anim->GetMovementAtTime( duration );
					AnimQsTransform& root = pose.m_outputPose[ 0 ];
#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else
					root.SetMul( root, motion );
#endif
				}
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				transform.m_rotation.normalize();

				Vector pos = TO_CONST_VECTOR_REF( transform.m_translation );
				Vector rot = TO_CONST_VECTOR_REF( transform.m_rotation );
#else
				RedQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				transform.Rotation.Normalize();

				Vector pos = reinterpret_cast< const Vector& >( transform.Translation );
				Vector rot = reinterpret_cast< const Vector& >( transform.Rotation );
#endif
				Float timeDiff = duration - time;

				Vector vel = ( pos - prevPos ) / timeDiff;
				vels.PushBack( vel );

				positions.PushBack( pos );
				rotations.PushBack( rot );

				Float velMag = vel.Mag3();
				if ( velMag > maxVel )
				{
					maxVel = velMag;
				}
				if ( velMag < minVel )
				{
					minVel = velMag;
				}
			}

			if ( m_timeA < anim->GetDuration() && m_timeA >= 0.f )
			{
				anim->Sample( m_timeA, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

				pose.ExtractTrajectory( ac );

				if ( m_applyMotion )
				{
					AnimQsTransform motion = anim->GetMovementAtTime( m_timeA );
					AnimQsTransform& root = pose.m_outputPose[ 0 ];
#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else
					root.SetMul( root, motion );
#endif
				}
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				Matrix& pointA = instance[ i_pointA ];

				HavokTransformToMatrix_Renormalize( transform, &pointA );
#else
				RedQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				RedMatrix4x4 conversionMatrix = transform.ConvertToMatrixNormalized();
				Matrix& pointA = instance[ i_pointA ];
				pointA = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
			}

			if ( m_timeB < anim->GetDuration() && m_timeB >= 0.f )
			{
				anim->Sample( m_timeB, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

				pose.ExtractTrajectory( ac );

				if ( m_applyMotion )
				{
					AnimQsTransform motion = anim->GetMovementAtTime( m_timeB );
					AnimQsTransform& root = pose.m_outputPose[ 0 ];

#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else
					root.SetMul( root, motion );
#endif
				}
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				Matrix& pointB = instance[ i_pointB ];

				HavokTransformToMatrix_Renormalize( transform, &pointB );
#else
				RedQsTransform transform = pose.GetBoneModelTransform( ac, boneIdx );
				RedMatrix4x4 conversionMatrix = transform.ConvertToMatrixNormalized();
				Matrix& pointB = instance[ i_pointB ];
				pointB = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
			}
		}
	}
}

void CBehaviorGraphAnimationTrajDrawNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	const Int32 boneIdx = instance[ i_bone ];

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );

		CSkeletalAnimationSetEntry* animEntry = instance[ i_animation ];
		if ( animEntry && boneIdx != -1 )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();
			if ( anim )
			{
				CSyncInfo info;
				m_cachedInputNode->GetSyncInfo( instance, info );

				const CAnimatedComponent* ac = instance.GetAnimatedComponent();
				const CSkeleton* skeleton = ac->GetSkeleton();

				Uint32 numBones = skeleton->GetBonesNum();
				Uint32 numTracks = skeleton->GetTracksNum();

				SBehaviorGraphOutput pose;
				pose.Init( numBones, numTracks );

				anim->Sample( info.m_currTime, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
#ifdef USE_HAVOK_ANIMATION
				pose.m_deltaReferenceFrameLocal.setIdentity();
#else
				pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
				pose.ExtractTrajectory( ac );

				if ( m_applyOffsets )
				{
					Vector& offsetPosA = instance[ i_offsetPosA ];
					Vector& offsetPosB = instance[ i_offsetPosB ];
					Vector& offsetRotA = instance[ i_offsetRotA ];
					Vector& offsetRotB = instance[ i_offsetRotB ];

					if ( m_cachedOffsetPosA )
					{
						m_cachedOffsetPosA->Update( context, instance, timeDelta );
						offsetPosA = m_cachedOffsetPosA->GetVectorValue( instance );
					}
					if ( m_cachedOffsetPosB )
					{
						m_cachedOffsetPosB->Update( context, instance, timeDelta );
						offsetPosB = m_cachedOffsetPosB->GetVectorValue( instance );
					}
					if ( m_cachedOffsetRotA )
					{
						m_cachedOffsetRotA->Update( context, instance, timeDelta );
						offsetRotA = m_cachedOffsetRotA->GetVectorValue( instance );
					}
					if ( m_cachedOffsetRotB )
					{
						m_cachedOffsetRotB->Update( context, instance, timeDelta );
						offsetRotB = m_cachedOffsetRotB->GetVectorValue( instance );
					}

					//...
				}

				if ( m_applyMotion )
				{
					AnimQsTransform motion = anim->GetMovementAtTime( info.m_currTime );
					AnimQsTransform& root = pose.m_outputPose[ 0 ];

#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else					
					root.SetMul( root, motion );
#endif
				}
#ifdef USE_HAVOK_ANIMATION
				hkQsTransform transform = pose.GetBoneWorldTransform( ac, boneIdx );
				transform.m_rotation.normalize();

				instance[ i_position ] = TO_CONST_VECTOR_REF( transform.m_translation );
				instance[ i_rotation ] = TO_CONST_VECTOR_REF( transform.m_rotation );
#else
				RedQsTransform transform = pose.GetBoneWorldTransform( ac, boneIdx );
				transform.Rotation.Normalize();

				instance[ i_position ] = reinterpret_cast< const Vector& >( transform.Translation );
				instance[ i_rotation ] = reinterpret_cast< const Vector& >( transform.Rotation );
#endif
			}
		}
	}
}

Vector CBehaviorGraphAnimationTrajDrawNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	return m_generateRot ? instance[ i_rotation ] : instance[ i_position ];
}

void CBehaviorGraphAnimationTrajDrawNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphAnimationTrajDrawNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphAnimationTrajDrawNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationTrajDrawNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphAnimationTrajDrawNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimationTrajDrawNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRootMotionNode );

void CBehaviorGraphRootMotionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_animation;
}

void CBehaviorGraphRootMotionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_animation ] = instance.GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( m_animationName );
}

void CBehaviorGraphRootMotionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_running )
	{
		CSkeletalAnimationSetEntry* animEntry = instance[ i_animation ];
		if ( animEntry && output.m_numBones > 0 )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();
			if ( anim )
			{
				if ( m_cachedInputNode )
				{
					CSyncInfo info;
					m_cachedInputNode->GetSyncInfo( instance, info );

					AnimQsTransform motion = anim->GetMovementAtTime( info.m_currTime );
					AnimQsTransform& root = output.m_outputPose[ 0 ];

#ifdef USE_HAVOK_ANIMATION
					root.setMul( root, motion );
#else
					root.SetMul( root, motion );
#endif
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

namespace
{
	static Float GetLandscapeHeight( int i, int j )
	{
#ifdef USE_HAVOK_ANIMATION
		Float h = ( hkMath::cos( 7.25f * ( (Float)j + i ) ) ) + ( 0.5f * ( hkMath::sin( 4.0f * i ) ) );
#else
		Float h = ( Red::Math::MCos( 7.25f * ( (Float)j + i ) ) ) + ( 0.5f * ( Red::Math::MSin( 4.0f * i ) ) );
#endif
		h = h + 1.1f;
		return h * 0.3f;
	}
};
#ifdef USE_HAVOK_ANIMATION
hkBool FootIkEditorRaycastInterface::castRay( const hkVector4& fromWS, const hkVector4& toWS, hkReal& hitFractionOut, hkVector4& normalWSOut )
{
	static Float offset = 0.f;

	const Float start = fromWS( 2 );
	const Float end = toWS( 2 );

	normalWSOut = hkVector4( 0.f, 0.f, 1.f );

	Float t = ( offset - start ) / ( end - start );
	if ( t >= 0.f && t <= 1.f )
	{
		hitFractionOut = t;

		return true;
	}
	else
	{
		return false;
	}
}

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphIkFootNode );

CBehaviorGraphIkFootNode::CBehaviorGraphIkFootNode()
	: m_kneeAxis( A_Y )
	, m_footEndLS( Vector::ZERO_3D_POINT )
	, m_minAnkleHeightMS( -0.9f )
	, m_maxAnkleHeightMS( -0.1f )
	, m_maxKneeAngle( 180.f )
	, m_minKneeAngle( -180.f )
	, m_footPlantedAnkleHeightMS( -0.8f )
	, m_footRaisedAnkleHeightMS( -0.2f )
	, m_raycastDistanceUp( 1.f )
	, m_raycastDistanceDown( 1.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIkFootNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
}

#endif

void CBehaviorGraphIkFootNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_bones;
	compiler << i_weight;
	compiler << i_originalGroundHeightMS;

	compiler << i_raycast;
	compiler << i_footPlacementSolverRight;
	compiler << i_footPlacementSolverLeft;
}

void CBehaviorGraphIkFootNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	TDynArray< Int32 >& bones = instance[ i_bones ];
	bones.Resize( BoneLast );

	bones[ HipBoneLeft ] = FindBoneIndex( m_hipBoneLeft, ac );
	bones[ KneeBoneLeft ] = FindBoneIndex( m_kneeBoneLeft, ac );
	bones[ AnkleBoneLeft ] = FindBoneIndex( m_ankleBoneLeft, ac );

	bones[ HipBoneRight ] = FindBoneIndex( m_hipBoneRight, ac );
	bones[ KneeBoneRight ] = FindBoneIndex( m_kneeBoneRight, ac );
	bones[ AnkleBoneRight ] = FindBoneIndex( m_ankleBoneRight, ac );

	instance[ i_weight ] = 1.f;
	instance[ i_originalGroundHeightMS ] = -0.82f;

	CreateRaycast( instance );
	CreateAndSetupFootPlacementSolvers( instance );
}

void CBehaviorGraphIkFootNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	DestroyRaycast( instance );
	DestroyFootPlacementSolvers( instance );

	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphIkFootNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
}

void CBehaviorGraphIkFootNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SimpleIkFoot );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedValueNode->GetValue( instance );
	}
	else
	{
		instance[ i_weight ] = 0.f;
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Update( context, instance, timeDelta );

		//instance[ i_target ] = m_cachedTargetNode->GetVectorValue( instance );
	}
	else
	{
		//instance[ i_target ] = Vector::ZERO_3D_POINT;
	}
}

void CBehaviorGraphIkFootNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( SimpleIkFoot );

	TBaseClass::Sample( context, instance, output );

	const TDynArray< Int32 >& bones = instance[ i_bones ];

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CSkeleton* skeleton = ac->GetSkeleton();

	if ( bones.Size() > 0 )
	{
		/*hkaTwoJointsIkSolver::Setup solverSetup;
		SetupSolver( instance, solverSetup );

		const Vector& targetVec = instance[ i_target ];
		solverSetup.m_endTargetMS = TO_CONST_HK_VECTOR_REF( targetVec );

		solverSetup.m_endTargetRotationMS = hkQuaternion::getIdentity();

		const hkaSkeleton *havokSkeleton = skeleton->GetHavokSkeleton();
		hkaPose pose( havokSkeleton );
		SyncPoseFromOutput( pose, output );

		Bool isLimit = hkaTwoJointsIkSolver::solve( solverSetup, pose );

		output.m_outputPose[ firstJointIdx ]	= pose.getBoneLocalSpace( firstJointIdx );
		output.m_outputPose[ secondJointIdx ]	= pose.getBoneLocalSpace( secondJointIdx );
		output.m_outputPose[ endBoneIdx ]		= pose.getBoneLocalSpace( endBoneIdx );*/

		/*
		// FOOT IK
		hkReal errorLeft;
		hkReal errorRight;
		{

			hkaFootPlacementIkSolver::Input footPlacementInput;
			footPlacementInput.m_worldFromModel = m_currentMotion;
			footPlacementInput.m_onOffGain = 0.2f;
			// we set all gains to 1.0 since the landscape is smooth
			footPlacementInput.m_groundAscendingGain = 1.0f;
			footPlacementInput.m_groundDescendingGain = 1.0f;
			footPlacementInput.m_footRaisedGain = 1.0f;
			footPlacementInput.m_footPlantedGain = 1.0f;
			footPlacementInput.m_footPlacementOn = m_ikOn;
			footPlacementInput.m_raycastInterface = m_raycastInterface;


			// LEFT LEG
			{
				footPlacementInput.m_originalAnkleTransformMS = thePose.getBoneModelSpace(m_footPlacementComponent[LEFT_LEG]->m_setup.m_ankleIndex);

				hkaFootPlacementIkSolver::Output footPlacementOutput;
				m_footPlacementComponent[LEFT_LEG]->doFootPlacement(footPlacementInput, footPlacementOutput, thePose);

				errorLeft = footPlacementOutput.m_verticalError;
			}

			// RIGHT LEG
			{
				footPlacementInput.m_originalAnkleTransformMS = thePose.getBoneModelSpace(m_footPlacementComponent[RIGHT_LEG]->m_setup.m_ankleIndex);

				hkaFootPlacementIkSolver::Output footPlacementOutput;
				m_footPlacementComponent[RIGHT_LEG]->doFootPlacement(footPlacementInput, footPlacementOutput, thePose);

				errorRight = footPlacementOutput.m_verticalError;
			}

		}

		// Skin & render
		{
			// Grab the pose in world space
			const hkArray<hkQsTransform>& poseInWorld = thePose.getSyncedPoseModelSpace();

			// Convert accumlated info to graphics matrix
			hkTransform graphicsTransform;
			m_currentMotion.copyToTransform(graphicsTransform);

			// Construct the composite world transform
			hkLocalArray<hkTransform> compositeWorldInverse( boneCount );
			compositeWorldInverse.setSize( boneCount );

			// Skin the meshes
			for (int i=0; i < m_numSkinBindings; i++)
			{
				// assumes either a straight map (null map) or a single one (1 palette)
				hkInt16* usedBones = m_skinBindings[i]->m_mappings? m_skinBindings[i]->m_mappings[0].m_mapping : HK_NULL;
				int numUsedBones = usedBones? m_skinBindings[i]->m_mappings[0].m_numMapping : boneCount;

				// Multiply through by the bind pose inverse world inverse matrices
				for (int p=0; p < numUsedBones; p++)
				{
					int boneIndex = usedBones? usedBones[p] : p;
					compositeWorldInverse[p].setMul( poseInWorld[ boneIndex ], m_skinBindings[i]->m_boneFromSkinMeshTransforms[ boneIndex ] );
				}

				AnimationUtils::skinMesh( *m_skinBindings[i]->m_mesh, graphicsTransform, compositeWorldInverse.begin(), *m_env->m_sceneConverter );
			}
		}

		// Move the character vertically depending on the error
		// We use the lowest/negative errors (going down) since IK can solve positive errors (going up) easier
		{
			m_currentMotion.m_translation(2) += hkMath::min2(errorLeft, errorRight)  * 0.2f;
		}

		*/
	}
}

void CBehaviorGraphIkFootNode::SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const
{
	hkArray<hkQsTransform>& poseLocalTrans = pose.accessUnsyncedPoseLocalSpace();

	Int32 numBones = Min( (Int32)output.m_numBones, poseLocalTrans.getSize() );

	for ( Int32 i=0; i<numBones; i++ )
	{
		poseLocalTrans[i] = output.m_outputPose[i];
	}

	pose.syncModelSpace();
}

void CBehaviorGraphIkFootNode::CreateRaycast( CBehaviorGraphInstance& instance ) const
{
	FootIkEditorRaycastInterface* raycast = new FootIkEditorRaycastInterface();
	instance[ i_raycast ] = reinterpret_cast< TGenericPtr >( raycast );
}

void CBehaviorGraphIkFootNode::DestroyRaycast( CBehaviorGraphInstance& instance ) const
{
	FootIkEditorRaycastInterface* raycast = GetRaycast( instance );
	delete raycast;
	instance[ i_raycast ] = 0;
}

void CBehaviorGraphIkFootNode::CreateAndSetupFootPlacementSolvers( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	hkaFootPlacementIkSolver::Setup setupData;

	// COMMON
	{
		setupData.m_skeleton = ac->GetSkeleton()->GetHavokSkeleton();
		setupData.m_kneeAxisLS = BehaviorUtils::hkVectorFromAxis( m_kneeAxis );
		setupData.m_footEndLS = TO_CONST_HK_VECTOR_REF( m_footEndLS );
		setupData.m_worldUpDirectionWS = BehaviorUtils::hkVectorFromAxis( A_Z );
		setupData.m_modelUpDirectionMS = BehaviorUtils::hkVectorFromAxis( A_Z );
		setupData.m_originalGroundHeightMS = instance[ i_originalGroundHeightMS ];
		setupData.m_minAnkleHeightMS = m_minAnkleHeightMS;
		setupData.m_maxAnkleHeightMS = m_maxAnkleHeightMS;
		setupData.m_cosineMaxKneeAngle = -0.99f;
		setupData.m_cosineMinKneeAngle = 0.99f;
		setupData.m_footPlantedAnkleHeightMS = m_footPlantedAnkleHeightMS;
		setupData.m_footRaisedAnkleHeightMS = m_footRaisedAnkleHeightMS;
		setupData.m_raycastDistanceUp = m_raycastDistanceUp;
		setupData.m_raycastDistanceDown = m_raycastDistanceDown;
	}

	// LEFT LEG
	{
		setupData.m_hipIndex = FindBoneIndex( m_hipBoneLeft, ac );
		setupData.m_kneeIndex = FindBoneIndex( m_kneeBoneLeft, ac );
		setupData.m_ankleIndex = FindBoneIndex( m_ankleBoneLeft, ac );

		hkaFootPlacementIkSolver* solver = new hkaFootPlacementIkSolver( setupData );

		instance[ i_footPlacementSolverLeft ] = reinterpret_cast< TGenericPtr >( solver );
	}

	// RIGHT LEG
	{
		setupData.m_hipIndex = FindBoneIndex( m_hipBoneRight, ac );
		setupData.m_kneeIndex = FindBoneIndex( m_kneeBoneRight, ac );
		setupData.m_ankleIndex = FindBoneIndex( m_ankleBoneRight, ac );

		hkaFootPlacementIkSolver* solver = new hkaFootPlacementIkSolver( setupData );

		instance[ i_footPlacementSolverRight ] = reinterpret_cast< TGenericPtr >( solver );
	}
}

void CBehaviorGraphIkFootNode::DestroyFootPlacementSolvers( CBehaviorGraphInstance& instance ) const
{
	hkaFootPlacementIkSolver* solverL = GetFootPlacementSolver( instance, LeftLeg );
	hkaFootPlacementIkSolver* solverR = GetFootPlacementSolver( instance, RightLeg );

	delete solverL;
	delete solverR;

	instance[ i_footPlacementSolverLeft ] = 0;
	instance[ i_footPlacementSolverRight ] = 0;
}

FootIkEditorRaycastInterface* CBehaviorGraphIkFootNode::GetRaycast( CBehaviorGraphInstance& instance ) const
{
	return reinterpret_cast< FootIkEditorRaycastInterface* >( instance[ i_raycast ] );
}

hkaFootPlacementIkSolver* CBehaviorGraphIkFootNode::GetFootPlacementSolver( CBehaviorGraphInstance& instance, CBehaviorGraphIkFootNode::ELeg leg ) const
{
	if ( leg == LeftLeg )
	{
		return reinterpret_cast< hkaFootPlacementIkSolver* >( instance[ i_footPlacementSolverLeft ] );
	}
	else if ( leg == RightLeg )
	{
		return reinterpret_cast< hkaFootPlacementIkSolver* >( instance[ i_footPlacementSolverRight ] );
	}
	else
	{
		HALT( "Debil" );
		return NULL;
	}
}

void CBehaviorGraphIkFootNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Activate( instance );
	}
}

void CBehaviorGraphIkFootNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Deactivate( instance );
	}
}

void CBehaviorGraphIkFootNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedTargetNode = CacheVectorValueBlock( TXT("Target") );
}

void CBehaviorGraphIkFootNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIkFootNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_generateEditorFragments )
	{
		//...
	}
}

#endif
#endif
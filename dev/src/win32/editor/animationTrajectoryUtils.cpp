
#include "build.h"
#include "animationTrajectoryUtils.h"
#include "../../common/engine/animationTrajectory.h"

Bool AnimationTrajectoryBuilder::CreateTrajectoryFromUncompressedAnimation( const CSkeletalAnimationSetEntry* uncompressedAnimation, const CSkeleton* skeleton, Int32 boneIndex, Int32 syncFrame, Int32 boneToExtract, AnimationTrajectoryData& out )
{
#ifdef USE_HAVOK_ANIMATION
	CSkeletalAnimation* skAnimation = uncompressedAnimation->GetAnimation();
	if ( !skAnimation )
	{
		return false;
	}

	const AnimBuffer* animBuff = skAnimation->GetAnimBuffer();
	if ( !animBuff )
	{
		ASSERT( animBuff );
		return false;
	}

	if ( animBuff->m_animNum != 1 )
	{
		return false;
	}

	const hkaAnimation* hkAnim = animBuff->GetHavokAnimation( 0 );
	if ( !hkAnim )
	{
		ASSERT( hkAnim );
		return false;
	}

	if ( hkAnim->getType() != hkaAnimation::HK_INTERLEAVED_ANIMATION )
	{
		ASSERT( hkAnim->getType() != hkaAnimation::HK_INTERLEAVED_ANIMATION );
		return false;
	}

	const hkaInterleavedUncompressedAnimation* rawAnimation = static_cast< const hkaInterleavedUncompressedAnimation* >( hkAnim );

	const Uint32 numBones = skeleton->GetBonesNum();
	const Uint32 numTracks = skeleton->GetTracksNum();

	if ( numBones != rawAnimation->m_numberOfTransformTracks )
	{
		ASSERT( numBones != rawAnimation->m_numberOfTransformTracks );
		return false;
	}

	Float minVel = NumericLimits< Float >::Max();
	Float maxVel = NumericLimits< Float >::Min();

	// Write frames to output

	const Int32 numberOfPoses = rawAnimation->getNumOriginalFrames();
	const Int32 numberOfTransformTracks = rawAnimation->m_numberOfTransformTracks;

	const hkReal timeStep = rawAnimation->m_duration / ( numberOfPoses - 1 );
	const hkReal invTimeStep = 1.f / timeStep;

	out.m_pointsLS.Resize( numberOfPoses );
	out.m_pointsMS.Resize( numberOfPoses );
	out.m_weights.Resize( numberOfPoses );

	for ( Int32 p=0; p<numberOfPoses; ++p )
	{
		hkQsTransform& buffer = rawAnimation->m_transforms[ p * numberOfTransformTracks ];

		SBehaviorGraphOutput pose;
		pose.Init( numberOfTransformTracks, 0, &buffer, NULL, false );

		pose.m_deltaReferenceFrameLocal.setIdentity();

		if ( boneToExtract != -1 )
		{
			pose.ExtractTrajectory( skeleton, boneToExtract );
		}

		ASSERT( boneIndex != -1 );

		hkQsTransform boneTran = pose.GetBoneModelTransform( boneIndex, skeleton->GetParentIndices() );

		out.m_pointsLS[ p ] = TO_CONST_VECTOR_REF( boneTran.m_translation );

		hkQsTransform boneTranMS;
		hkQsTransform motion = skAnimation->GetMovementAtTime( timeStep * p );
		boneTranMS.setMul( motion, boneTran );

		out.m_pointsMS[ p ] = TO_CONST_VECTOR_REF( boneTranMS.m_translation );

		out.m_weights[ p ] = p == syncFrame ? 1.f : 0.f;
	}

	return true;
#else
	HALT( "Needs to be implemented" );
	return false;
#endif
}

Bool AnimationTrajectoryBuilder::CreateTrajectoryFromCompressedAnimation( const CSkeletalAnimationSetEntry* compressedAnimation, const CSkeleton* skeleton, Int32 boneIndex, Float syncFrame, Int32 boneToExtract, AnimationTrajectoryData& out )
{
#ifdef USE_HAVOK_ANIMATION
	CSkeletalAnimation* skAnimation = compressedAnimation->GetAnimation();
	if ( !skAnimation )
	{
		return false;
	}

	const AnimBuffer* animBuff = skAnimation->GetAnimBuffer();
	if ( !animBuff )
	{
		ASSERT( animBuff );
		return false;
	}

	if ( animBuff->m_animNum != 1 )
	{
		return false;
	}

	const hkaAnimation* hkAnim = animBuff->GetHavokAnimation( 0 );
	if ( !hkAnim )
	{
		ASSERT( hkAnim );
		return false;
	}

	const Uint32 numBones = skeleton->GetBonesNum();
	const Uint32 numTracks = skeleton->GetTracksNum();

	if ( numBones != hkAnim->m_numberOfTransformTracks )
	{
		ASSERT( numBones != hkAnim->m_numberOfTransformTracks );
		return false;
	}

	Float minVel = NumericLimits< Float >::Max();
	Float maxVel = NumericLimits< Float >::Min();

	// Write frames to output

	const hkReal timeStep = 1.f / 30.f;
	const hkReal invTimeStep = 1.f / timeStep;

	const Int32 numberOfPoses = MRound( hkAnim->m_duration / timeStep ) + 1;
	const Int32 numberOfTransformTracks = hkAnim->m_numberOfTransformTracks;

	const Int32 syncFrameNum = syncFrame / timeStep;

	out.m_pointsLS.Resize( numberOfPoses );
	out.m_rotLS.Resize( numberOfPoses );
	out.m_pointsMS.Resize( numberOfPoses );
	out.m_rotMS.Resize( numberOfPoses );
	out.m_weights.Resize( numberOfPoses );
	out.m_pointsLSO.Resize( numberOfPoses );
	out.m_pointsMSO.Resize( numberOfPoses );

	out.m_syncFrame = syncFrameNum;

	SBehaviorGraphOutput pose;
	pose.Init( numberOfTransformTracks, hkAnim->m_numberOfFloatTracks );

	hkReal time = 0.f;

	for ( Int32 p=0; p<numberOfPoses; ++p )
	{
		time = p * timeStep;

		hkAnim->sampleTracks( time, pose.m_outputPose, pose.m_floatTracks, GHavokEngine->GetDefaultChunkCache() );

		pose.m_deltaReferenceFrameLocal.setIdentity();

		if ( boneToExtract != -1 )
		{
			pose.ExtractTrajectory( skeleton, boneToExtract );
		}

		for ( Uint32 i=0; i<pose.m_numBones; ++i )
		{
			pose.m_outputPose[ i ].m_rotation.normalize();
		}

		ASSERT( boneIndex != -1 );

		hkQsTransform boneTran = pose.GetBoneModelTransform( boneIndex, skeleton->GetParentIndices() );

		out.m_pointsLS[ p ] = TO_CONST_VECTOR_REF( boneTran.m_translation );
		out.m_rotLS[ p ] = TO_CONST_VECTOR_REF( boneTran.m_rotation.m_vec );

		hkQsTransform boneTranMS;
		hkQsTransform motion = skAnimation->GetMovementAtTime( timeStep * p );
		boneTranMS.setMul( motion, boneTran );

		out.m_pointsMS[ p ] = TO_CONST_VECTOR_REF( boneTranMS.m_translation );
		out.m_rotMS[ p ] = TO_CONST_VECTOR_REF( boneTranMS.m_rotation.m_vec );

		static Vector offset( 0.f, 0.f, 1.f );
		hkQsTransform offsetLS( hkQsTransform::IDENTITY );
		offsetLS.m_translation = TO_CONST_HK_VECTOR_REF( offset );

		hkQsTransform boneTranO = pose.GetBoneModelTransformWithOffset( boneIndex, skeleton->GetParentIndices(), offsetLS );

		out.m_pointsLSO[ p ] = TO_CONST_VECTOR_REF( boneTranO.m_translation );

		boneTranMS.setMul( motion, boneTranO );

		out.m_pointsMSO[ p ] = TO_CONST_VECTOR_REF( boneTranMS.m_translation );

		out.m_weights[ p ] = p == syncFrame ? 1.f : 0.f;
	}

	ASSERT( MAbs( time - hkAnim->m_duration ) < 0.001f );

	return true;
#else
	HALT(  "Needs to be implemented" );
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////

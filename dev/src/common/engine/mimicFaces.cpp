/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mimicFaces.h"
#include "behaviorGraphOutput.h"


IMPLEMENT_ENGINE_CLASS( CMimicFaces );

CSkeleton* CMimicFaces::GetMimicSkeleton() const
{
	return m_mimicSkeleton.Get();
}

Uint32 CMimicFaces::GetMimicPoseNum() const
{
	return m_mimicPoses.Size();
}

const CMimicFaces::TMimicPose CMimicFaces::GetMimicPose( Uint32 pose ) const
{
	ASSERT( m_mimicPoses.Size() > pose );

	return m_mimicPoses[ pose ];
}

Bool CMimicFaces::GetMimicPose( Uint32 num, SBehaviorGraphOutput& pose ) const
{
	if ( m_mimicPoses.Size() > num && pose.m_eventsFired == NULL && pose.m_numBones == 0 && pose.m_numFloatTracks == 0 )
	{
		const TMimicPose& mimicPose = m_mimicPoses[ num ];

		Uint32 bonesNum = mimicPose.Size();

		EngineQsTransform* eTrans = const_cast< EngineQsTransform* >( mimicPose.TypedData() );

		struct SBehaviorGraphOutputParameter param =
		{
			bonesNum,
			0,
			reinterpret_cast< RedQsTransform* >( eTrans ),
			nullptr,
			nullptr,
			false
		};

		pose.Init( param );
		return true;
	}
	else
	{
		ASSERT( pose.m_numBones == 0 );
		ASSERT( pose.m_numFloatTracks == 0 );
		ASSERT( pose.m_eventsFired == NULL );

		return false;
	}
}

void CMimicFaces::GetNeckAndHead( Int32& neck, Int32& head ) const
{
	neck = m_neckIndex;
	head = m_headIndex;
}

void CMimicFaces::AddPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& additivePose, Float weight ) const
{
	ASSERT( m_mapping.Size() == additivePose.m_numBones );

	const Uint32 size = Min( m_mapping.Size(), additivePose.m_numBones );
#ifdef USE_HAVOK_ANIMATION
	const hkSimdReal simdWeight = weight;
	hkQsTransform temp;

	for ( Uint32 i=0; i<size; ++i )
	{
		const Int32 index = m_mapping[ i ];

		temp.setMul( additivePose.m_outputPose[i], mainPose.m_outputPose[index] );

		mainPose.m_outputPose[index].setInterpolate4( mainPose.m_outputPose[index], temp, simdWeight );
	}
#else
	const Float simdWeight = weight;
	RedQsTransform temp;

	for ( Uint32 i=0; i<size; ++i )
	{
		const Int32 index = m_mapping[ i ];

		temp.SetMul( additivePose.m_outputPose[i], mainPose.m_outputPose[index] );

		mainPose.m_outputPose[index].Lerp( mainPose.m_outputPose[index], temp, simdWeight );
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_RESOURCE_IMPORT

CMimicFaces* CMimicFaces::Create( const FactoryInfo& data )
{
#ifdef USE_HAVOK_ANIMATION
	CMimicFaces* faces = data.CreateResource();
	if ( faces )
	{
		// Skeleton
		{
			CSkeleton::FactoryInfo skeletonInfo;
			skeletonInfo.m_parent = faces;
			skeletonInfo.m_reuse = data.m_reuse ? data.m_reuse->m_mimicSkeleton : NULL;
			skeletonInfo.m_skeleton = data.m_skeleton;

			CSkeleton* skeleton = CSkeleton::Create( skeletonInfo );
			if ( skeleton == NULL )
			{
				return NULL;
			}

			faces->m_mimicSkeleton = skeleton;
		}

		if ( faces->m_mimicSkeleton == NULL || faces->m_mimicSkeleton->GetHavokSkeleton() == NULL )
		{
			return NULL;
		}

		faces->m_mapping = data.m_mapper;

		//dex++
		faces->m_neckIndex = faces->m_mimicSkeleton->FindBoneByName( TXT("neck") );
		faces->m_headIndex = faces->m_mimicSkeleton->FindBoneByName( TXT("head") );
		//dex--

		if (data.m_areas != NULL)
		{
			faces->m_normalBlendAreas.Resize(NUM_NORMALBLEND_AREAS);
			for ( Uint32 i=0; i<NUM_NORMALBLEND_AREAS; ++i )
			{
				faces->m_normalBlendAreas[i] = data.m_areas[i];
			}
		}

		if ( data.m_animation )
		{

			// Create temp animation
			CSkeletalAnimation::FactoryInfo animInfo;
			animInfo.m_animations.PushBack( data.m_animation );
			animInfo.m_name = TXT("Temp");
			animInfo.m_motionExtraction = NULL;

			CSkeletalAnimation* anim = CSkeletalAnimation::Create( animInfo );
			if ( anim == NULL || anim->GetDuration() < 1.f )
			{
				return NULL;
			}

			anim->AddToRootSet();

			// Create mimic pose buffer
			Int32 facePosesNum = BehaviorUtils::ConvertFloatToInt( anim->GetDuration() ) + 1;
			ASSERT(	facePosesNum > 0 );

			SBehaviorGraphOutput* poseBuffer = new SBehaviorGraphOutput[ facePosesNum ];

			// Get mimic faces from mimic pose animation
			const hkaSkeleton* havokSkeleton = faces->m_mimicSkeleton->GetHavokSkeleton();
			Int32 mimicBonesNum = havokSkeleton->m_numBones;
			Int32 mimicFloatTracksNum = havokSkeleton->m_numFloatSlots;

			// Create mimic T-pose
			SBehaviorGraphOutput tPose;
			tPose.Init( mimicBonesNum, mimicFloatTracksNum );
			tPose.Reset( faces->m_mimicSkeleton );

			// Initialize & fill animation buffer
			for ( Int32 i=0; i<=facePosesNum; ++i )
			{
				// Initialize
				SBehaviorGraphOutput& pose = poseBuffer[ i ];
				pose.Init( mimicBonesNum, mimicFloatTracksNum );

				// Get pose
				Float time = Min( (Float)i, anim->GetDuration() );

				anim->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

				// Set additive
				pose.SetMulInv( tPose );
			}

			// Create and copy mimic poses
			faces->m_mimicPoses.Resize( facePosesNum );

			for ( Uint32 i=0; i<faces->m_mimicPoses.Size(); ++i )
			{
				SBehaviorGraphOutput&	pose =		poseBuffer[ i ];
				TMimicPose&				mimicPose = faces->m_mimicPoses[ i ];

				mimicPose.Resize( pose.m_numBones );

				for ( Uint32 j=0; j<pose.m_numBones; ++j )
				{
					HkToEngineQsTransform( pose.m_outputPose[ j ], mimicPose[ j ] );
				}
			}

			// Delete object
			anim->RemoveFromRootSet();
			anim->Discard();
			anim = NULL;

			// Delete pose buffer
			delete [] poseBuffer;
		}
		else
		{
			faces->m_mimicPoses.Resize( data.m_numPoses );

			for ( Uint32 i=0; i<faces->m_mimicPoses.Size(); ++i )
			{
				TMimicPose&				mimicPose = faces->m_mimicPoses[ i ];
				mimicPose.Resize( data.m_numBones );

				for ( Uint32 j=0; j<mimicPose.Size(); ++j )
				{
					HkToEngineQsTransform( data.m_poses[j+(data.m_numBones*i)], mimicPose[ j ] );
				}
			}
			if(data.m_poses){delete [] data.m_poses;}
		}
	}
	
	// Return
	return faces;
#else
	//Not getting here on this path
	return NULL;
#endif
}

#endif

//////////////////////////////////////////////////////////////////////////

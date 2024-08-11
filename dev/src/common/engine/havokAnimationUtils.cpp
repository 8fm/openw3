/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphOutput.h"
#include "havokAnimationUtils.h"
#include "behaviorIncludes.h"

namespace ImportAnimationUtils
{
#ifdef USE_HAVOK_ANIMATION
	const hkClass &GetAnimationClass(const hkaAnimation *anim)
	{
		switch (anim->getType())
		{
		case hkaAnimation::HK_INTERLEAVED_ANIMATION:
			return hkaInterleavedUncompressedAnimationClass;
		case hkaAnimation::HK_DELTA_COMPRESSED_ANIMATION:
			return hkaDeltaCompressedAnimationClass;
		case hkaAnimation::HK_WAVELET_COMPRESSED_ANIMATION:
			return hkaWaveletCompressedAnimationClass;
		case hkaAnimation::HK_SPLINE_COMPRESSED_ANIMATION:
			return hkaSplineCompressedAnimationClass;
		default:
			return hkaAnimationClass;
		}
	}


	const hkaAnimatedReferenceFrame* ExtractMotionFromAnimation( const hkaAnimation* animation )
	{
		class FakeAnimation : public hkaSplineCompressedAnimation
		{
		public:
			FakeAnimation(
				const hkaInterleavedUncompressedAnimation& raw,
				const TrackCompressionParams& trackParams,
				const AnimationCompressionParams& animationParams )
				: hkaSplineCompressedAnimation( raw, trackParams, animationParams )
			{}
			const hkaAnimatedReferenceFrame* GetExtractedMotion() const { return m_extractedMotion; }
		};

		if ( animation && animation->getType() == hkaAnimation::HK_SPLINE_COMPRESSED_ANIMATION )
		{
			const FakeAnimation* fake = static_cast< const FakeAnimation* >( animation );
			return fake->GetExtractedMotion();
		}

		return NULL;
	}

	void ConvertToAdditiveAnimation( const hkQsTransform* tPoseTransform, Int32 tPoseTransformNum, hkaInterleavedUncompressedAnimation* animation )
	{
		Int32 numberOfPoses = animation->getNumOriginalFrames();
		Int32 numberOfTransformTracks = animation->m_numberOfTransformTracks;

		if ( numberOfTransformTracks != tPoseTransformNum )
		{
			return;
		}

		const hkQsTransform* refFrame = tPoseTransform;

		for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
		{
			for ( Int32 t=0; t<numberOfTransformTracks; ++t )
			{
				hkQsTransform& trans = animation->m_transforms[ p * numberOfTransformTracks + t ];
				trans.setMulInverseMul( refFrame[ t ], trans );
			}
		}
	}

	void ConvertToAdditiveAnimation( hkaInterleavedUncompressedAnimation* animation, EAdditiveType type )
	{
		Int32 numberOfPoses = animation->getNumOriginalFrames();
		Int32 numberOfTransformTracks = animation->m_numberOfTransformTracks;

		if ( type == AT_Local )
		{
			ConvertToAdditiveAnimation1( animation->m_transforms, numberOfTransformTracks, numberOfPoses );
		}
		else if ( type == AT_Ref )
		{
			ConvertToAdditiveAnimation2( animation->m_transforms, numberOfTransformTracks, numberOfPoses );
		}
	}

	void ConvertToAdditiveAnimation1( hkQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses )
	{
		hkQsTransform* firstFrame = buffer;

		for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
		{
			for ( Int32 t=0; t<numberOfTransformTracks; ++t )
			{
				hkQsTransform& trans = buffer[ p * numberOfTransformTracks + t ];
				trans.setMulInverseMul( firstFrame[ t ], trans );
			}
		}
	}

	void ConvertToAdditiveAnimation2( hkQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses )
	{
		hkQsTransform* firstFrame = buffer;

		for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
		{
			for ( Int32 t=0; t<numberOfTransformTracks; ++t )
			{
				hkQsTransform& trans = buffer[ p * numberOfTransformTracks + t ];
				trans.setMulMulInverse( trans, firstFrame[ t ] );
			}
		}
	}

	void ConvertToMSAnimation( hkaInterleavedUncompressedAnimation* animation, const CSkeleton* skeleton )
	{
		Int32 numberOfPoses = animation->getNumOriginalFrames();
		Int32 numberOfTransformTracks = animation->m_numberOfTransformTracks;

		for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
		{
			hkQsTransform& buffer = animation->m_transforms[ p * numberOfTransformTracks ];

			SBehaviorGraphOutput pose;
			pose.Init( numberOfTransformTracks, 0, &buffer, NULL, false );

			TDynArray< hkQsTransform > ms;
			ms.Resize( numberOfTransformTracks );

			pose.GetBonesModelSpace( skeleton, ms );

			for ( Int32 i=0; i<numberOfTransformTracks; ++i )
			{
				ms[ i ].m_rotation.normalize();
				animation->m_transforms[ p * numberOfTransformTracks + i ] = ms[ i ];
			}
		}
	}

	void ConvertToAdditiveAnimation( const hkaInterleavedUncompressedAnimation* animationLoop, hkaInterleavedUncompressedAnimation* animationOut )
	{
		Float time = 0.f;
		Int32 bonesNum = animationOut->m_numberOfTransformTracks;
		Int32 floatNum = animationOut->m_numberOfFloatTracks;

		Float dur = animationOut->m_duration;
		Float numFrames = (Float)animationOut->m_numTransforms/(Float)bonesNum;
		Float delta = dur/(numFrames-1);

		Float loopdur = animationLoop->m_duration;

		hkQsTransform* newTrans = new hkQsTransform[ bonesNum ];
		hkReal* floats = new hkReal[ floatNum ];

		for ( Int32 i=0; i<(Int32)numFrames; i++ )
		{
			time = (Float)i*delta;
			Float ntime = time/loopdur;
			Float looptime = (ntime - floorf(ntime))*loopdur;
			animationLoop->sampleTracks( looptime, newTrans, floats, GHavokEngine->GetDefaultChunkCache() );
			for( Int32 j=0; j<(Int32)bonesNum;j++)
			{
				Int32 index = (bonesNum*i)+j;
				hkQsTransform& trans = animationOut->m_transforms[ index ];
				trans.setMulInverseMul( newTrans[j], trans );
			}
		}
		delete [] newTrans;
		delete [] floats;
	}
#else

void ConvertToAdditiveAnimation1( AnimQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses )
{
	AnimQsTransform* firstFrame = buffer;

	for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
	{
		for ( Int32 t=0; t<numberOfTransformTracks; ++t )
		{
			AnimQsTransform& trans = buffer[ p * numberOfTransformTracks + t ];
			trans.SetMulInverseMul( firstFrame[ t ], trans );
		}
	}
}

void ConvertToAdditiveAnimation2( AnimQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses )
{
	AnimQsTransform* firstFrame = buffer;

	for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
	{
		for ( Int32 t=0; t<numberOfTransformTracks; ++t )
		{
			AnimQsTransform& trans = buffer[ p * numberOfTransformTracks + t ];
			trans.SetMulMulInverse( trans, firstFrame[ t ] );
		}
	}
}

void ConvertToAdditiveAnimation( const AnimQsTransform* tPoseTransform, Int32 tPoseTransformNum, IAnimationBuffer::SourceData::Part* animation, Int32 numBones, Int32 numTracks  )
{
	Int32 numberOfPoses = animation->m_numFrames;
	Int32 numberOfTransformTracks = numBones;

	if ( numberOfTransformTracks != tPoseTransformNum )
	{
		return;
	}

	const AnimQsTransform* refFrame = tPoseTransform;

	for ( Int32 p=numberOfPoses-1; p>=0 ; p-- )
	{
		for ( Int32 t=0; t<numberOfTransformTracks; ++t )
		{
			AnimQsTransform& trans = const_cast<AnimQsTransform&>( animation->m_bones[ p * numberOfTransformTracks + t ] );
			trans.SetMulInverseMul( refFrame[ t ], trans );
		}
	}

}

void ConvertToAdditiveAnimation( IAnimationBuffer::SourceData::Part* animation, Int32 numBones, EAdditiveType type )
{
	Int32 numberOfPoses = animation->m_numFrames;
	Int32 numberOfTransformTracks = numBones;

	if ( type == AT_Local )
	{
		ConvertToAdditiveAnimation1( const_cast<AnimQsTransform*>(animation->m_bones), numberOfTransformTracks, numberOfPoses );
	}
	else if ( type == AT_Ref )
	{
		ConvertToAdditiveAnimation2( const_cast<AnimQsTransform*>(animation->m_bones), numberOfTransformTracks, numberOfPoses );
	}
}

void ConvertToAdditiveFrame( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks, EAdditiveType type )
{
	if ( type == AT_Local )
	{
		ConvertToAdditiveFrame1( firstFrame, currFrame, numberOfTransformTracks );
	}
	else if ( type == AT_Ref )
	{
		ConvertToAdditiveFrame2( firstFrame, currFrame, numberOfTransformTracks );
	}
	else
	{
		ASSERT( 0 );
	}
}

void ConvertToAdditiveFrame1( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks )
{
	for ( Int32 t=0; t<numberOfTransformTracks; ++t )
	{
		AnimQsTransform& trans = currFrame[ t ];
		trans.SetMulInverseMul( firstFrame[ t ], trans );
	}
}

void ConvertToAdditiveFrame2( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks )
{
	for ( Int32 t=0; t<numberOfTransformTracks; ++t )
	{
		AnimQsTransform& trans = currFrame[ t ];
		trans.SetMulMulInverse( trans, firstFrame[ t ] );
	}
}

#endif
}

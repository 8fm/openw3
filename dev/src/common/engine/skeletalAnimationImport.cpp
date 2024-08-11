
#include "build.h"
#include "havokAnimationUtils.h"
#include "skeletalAnimation.h"
#include "animationBuffer.h"
#include "animationTrajectoryTrackParam.h"
#include "meshSkinningAttachment.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "motionExtraction.h"
#include "motionExtractionCompression.h"
#include "poseBBoxGenerator.h"

#ifndef NO_RESOURCE_IMPORT

namespace
{
	void ConvertBones( TDynArray< AnimQsTransform >& bonesLS, TDynArray< Matrix >& bonesMS, const CAnimatedComponent* component )
	{
		// Get havok skeleton
		const Uint32 numBones = Min( bonesLS.Size(), bonesMS.Size() );
		for ( Uint32 boneIdx=0; boneIdx<numBones; boneIdx++ )
		{
			// Bone in local space
			AnimQsTransform& bone = bonesLS[ boneIdx ];

			// Get parent index
			Int32 parentIdx = (Int32)component->GetSkeleton()->GetParentBoneIndex(boneIdx);
			ASSERT( parentIdx < (Int32)boneIdx );

			// If bone is not root
			if ( parentIdx != -1 )
			{
				// Get parent bone in model space
				const AnimQsTransform& parentMS = bonesLS[ parentIdx ];

				// Calc bone in model space
				bone.SetMul( parentMS, bone );
			}

			const AnimMatrix44 matrix = bone.ConvertToMatrixNormalized();
			bonesMS[ boneIdx ] = reinterpret_cast<const Matrix&>( matrix );
		}
	}

	void CreateAnimationParams( CSkeletalAnimation* animation, const CSkeletalAnimation::FactoryInfo& info )
	{
		// Trajectory track param
		CSkeletalAnimationSetEntry* entry = info.m_animationEntry;
		if ( entry )
		{
			entry->RemoveAllParamsByClass< CSkeletalAnimationTrajectoryTrackParam >();

			const Uint32 numTraj = info.m_trajectories.Size();
			if ( numTraj > 0 )
			{
				CSkeletalAnimationTrajectoryTrackParam* param = new CSkeletalAnimationTrajectoryTrackParam();
					
				for ( Uint32 i=0; i<numTraj; ++i )
				{
					param->m_names.PushBack( CName( info.m_trajectories[ i ].m_name ) );
					param->m_datas.PushBack( info.m_trajectories[ i ].m_data );
				}

#ifndef NO_EDITOR
				param->SetEditorOnly( true );
#endif

				entry->AddParam( param );
			}
		}
	}
}

Bool CSkeletalAnimation::GenerateBoundingBox( const CAnimatedComponent* component )
{
	ASSERT( IsLoaded() );

	// Clear current bounding box
	m_boundingBox.Clear();
	m_hasBundingBox = false;

	if ( !component || !component->GetSkeleton() )
	{
		return true;
	}

	// Collect bones with mesh skin
	const Uint32 numBones = component->GetSkeleton()->GetBonesNum();
	const Uint32 numTracks = component->GetSkeleton()->GetTracksNum();
	SBehaviorGraphOutput pose;
	pose.Init( numBones, numTracks );

	TDynArray< Matrix > bonesMS;
	bonesMS.Resize( pose.m_numBones );

	const Float timeSample = 1.f / 30.f;
	const Float duration = GetDuration();
	Float t = 0.f;

	// Determine used mesh bones
	TDynArray< Int32 > meshBonesIndices;
	{
		Int32 trajectoryBone = component->GetTrajectoryBone();

		const TList< IAttachment* >& attachments = component->GetChildAttachments();
		for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
		{
			const CMeshSkinningAttachment* meshSkinAtt = Cast< CMeshSkinningAttachment >( *it );
			if ( meshSkinAtt )
			{
				const auto& skinBones = meshSkinAtt->GetCachedMappingTable();
				for ( Uint32 j=0; j<skinBones.Size(); ++j )
				{
					Int32 bone = skinBones[j];
					if ( bone != -1 && bone != 0 && bone != trajectoryBone )
					{
						// Skip IK bones
						const AnsiChar* boneName = component->GetSkeleton()->GetBoneNameAnsi( bone );
						if ( Red::System::StringSearch( boneName, "IK" ) != NULL )
						{
							continue;
						}

						meshBonesIndices.PushBack( bone );
					}
				}
			}
		}
	}

	// Sample animation over time
	while ( t <= duration )
	{
		const Bool ret = Sample( t, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
		if ( !ret )
		{
			return false;
		}

		pose.GetBonesModelSpace( component, bonesMS );

		for ( Uint32 i=0; i<meshBonesIndices.Size(); ++i )
		{
			// Process only mesh bones
			const Uint32 boneIndex = meshBonesIndices[i];
			m_boundingBox.AddPoint( bonesMS[boneIndex].GetTranslation() );
		}

		t += timeSample;
	}

	// Done
	if ( !m_boundingBox.IsEmpty() )
	{
		m_boundingBox.Extrude( 0.25f );
		m_hasBundingBox = true;
	}

	return true;
}

ICompressedPose* CSkeletalAnimation::CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, Float time )
{
	return NULL;
}

void CSkeletalAnimation::CreateBBox( const CSkeleton* skeleton )
{
	const CPoseBBoxGenerator* gen = skeleton->GetPoseBBoxGen();
	if ( gen )
	{
		Box box;
		if ( gen->GenerateBBox( this, box ) )
		{
			m_boundingBox = box;
			m_hasBundingBox = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
#ifndef NO_RESOURCE_IMPORT
void CSkeletalAnimation::UseCompressMotionExtraction( Bool flag )
{
	if ( flag )
	{
		CreateCompressedMotionExtraction();
	}
#ifdef NO_EDITOR
	RED_HALT( "Cannot use uncompressed motion extraction in non-editor builds" );
#else
	else if ( m_uncompressedMotionExtraction )
	{
		if ( m_uncompressedMotionExtraction->GetClass()->IsA< CUncompressedMotionExtraction >() )
		{
			m_motionExtraction = static_cast<CUncompressedMotionExtraction*>( m_uncompressedMotionExtraction->CreateCopy() );
		}
		else
		{
			m_motionExtraction = NULL;
		}
	}
#endif
}

void CSkeletalAnimation::CreateCompressedMotionExtraction()
{
#ifdef NO_EDITOR
	RED_HALT( "Cannot create uncompressed motion extraction in non-editor builds" );
#else
	if ( m_uncompressedMotionExtraction )
	{
		CMotionExtractionLineCompression2* compression = CreateObject< CMotionExtractionLineCompression2 >();
		m_motionExtraction = Cast< CLineMotionExtraction2 >( compression->Compress( m_uncompressedMotionExtraction ) );
	}
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////

CSkeletalAnimation* CSkeletalAnimation::Create( const FactoryInfo& data )
{
	const CSkeletalAnimation::FactoryInfo& info = static_cast< const CSkeletalAnimation::FactoryInfo& >( data );

	// try to reuse animation object, if it does not exist create new one
	CSkeletalAnimation* obj = data.m_animationEntry ? data.m_animationEntry->GetAnimation() : new CSkeletalAnimation();
	if ( !obj )
	{
		return NULL;
	}

	// Copy animation attributes
	obj->m_duration = info.m_duration;
	obj->m_framesPerSecond = info.m_framesPerSecond;

	// Set animation name
	obj->SetName( CName( data.m_name ) );

#ifndef NO_EDITOR
	obj->m_sourceDataCreatedFromAnimBuffer = false;
	obj->m_sourceAnimData.LoadAnimDataFrom( info.m_animationData );
#ifdef USE_REF_IK_MISSING_BONES_HACK
		// read it again to fill some missing data
		obj->m_sourceAnimData.ReadAnimDataTo( const_cast<CSkeletalAnimation::FactoryInfo*>(&info)->m_animationData, obj );
#endif
#endif

	// Create animation data
	obj->m_animBuffer = IAnimationBuffer::CreateFromSourceData( data.m_animationSet, obj, info.m_animationData, obj ? obj->m_animBuffer : NULL, info.m_preferBetterQuality );
	if ( !obj->m_animBuffer )
	{
		WARN_ENGINE(TXT("Failed to import animation data"));
		return NULL;
	}

	// Force the bounding box to be recomputed
	if ( !data.m_animationEntry )
	{
		obj->m_hasBundingBox = false;
	}

	// Reset old motion extraction
#ifdef NO_EDITOR
	RED_ASSERT( info.m_motionFrames.Empty(), TXT( "Cannot import motion extraction in game build" ) );
#else
	obj->m_uncompressedMotionExtraction = NULL;
	obj->m_motionExtraction = NULL;

	// Import new motion extraction
	if ( !info.m_motionFrames.Empty() )
	{
		CUncompressedMotionExtraction* uncompressedMotionEx = new CUncompressedMotionExtraction();
		if (uncompressedMotionEx->Initialize(info))
		{
			obj->m_uncompressedMotionExtraction = uncompressedMotionEx;
			obj->CreateCompressedMotionExtraction();
		}
	}
#endif

	CreateAnimationParams( obj, info );
	return obj;
}

#endif

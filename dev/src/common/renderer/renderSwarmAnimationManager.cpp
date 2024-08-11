/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderSwarmAnimationManager.h"
#include "renderSkinningData.h"

#include "../engine/mesh.h"
#include "../engine/skeleton.h"
#include "../engine/skeletalAnimationEntry.h"
#include "../engine/skeletonUtils.h"
#include "../engine/skeletalAnimationSet.h"

namespace SwarmMassiveAnimation
{
	/**************************************************************************************************************/
	CMassiveAnimation::CMassiveAnimation()
		: m_mesh( nullptr )
		, m_skeleton( nullptr )
		, m_animation( nullptr )
		, m_instanceCount( 0 )
	{

	}

	CMassiveAnimation::CMassiveAnimation( CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSetEntry* animation )
		: m_mesh( mesh )
		, m_skeleton( skeleton )
		, m_animation( animation )
		, m_instanceCount( 0 )
	{

	}

	CMassiveAnimation::~CMassiveAnimation()
	{
		for( Uint8 i=0; i<m_instanceCount; ++i )
		{
			SAFE_RELEASE( m_instances[i].m_skinningData );
		}
	}

	CMassiveAnimation* CMassiveAnimation::Create(CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSetEntry* animation)
	{
		RED_FATAL_ASSERT( mesh != nullptr && animation != nullptr && skeleton != nullptr && skeleton->IsValid(), "CMassiveAnimation creation data is invalid" );

		CMassiveAnimation* resultAnimation = new CMassiveAnimation( mesh, skeleton, animation );
		resultAnimation->Initialize();

		return resultAnimation;
	}

	void CMassiveAnimation::Initialize()
	{
		CreateBoneMapping();
		InitializeInstances();
	}

	void CMassiveAnimation::InitializeInstances()
	{
		const Uint32 NUM_ANIMATION_STEPS = MAX_INSTANCE_COUNT;	// Number instances per animation
		const Float MIN_ANIMATION_STEP = 0.25f;			// make sure each animation instance is spaced at least X secs apart

		Float animStep = Max( m_animation->GetDuration() / NUM_ANIMATION_STEPS, MIN_ANIMATION_STEP );
		Uint32 stepCount = (Uint32)(m_animation->GetDuration() / animStep);

		for( Float t = 0.0f; t<m_animation->GetDuration() && m_instanceCount < MAX_INSTANCE_COUNT; t += animStep )
		{
			m_instances[m_instanceCount] = CMassiveAnimationInstance( t, new CRenderSkinningData( m_boneMapping.Size(), true ) );
			m_instanceCount++;
		}
	}

	void CMassiveAnimation::CreateBoneMapping()
	{
		// Get bones from source skeleton
		TDynArray< ISkeletonDataProvider::BoneInfo > sourceBones;
		m_skeleton->GetBones( sourceBones );

		// Map mesh bones
		Uint32 numMeshBones = m_mesh->GetBoneCount();
		const CName* meshBoneNames = m_mesh->GetBoneNames();
		m_boneMapping.Resize( numMeshBones );

		for ( Uint32 i=0; i<numMeshBones; i++ )
		{
			const CName& meshBoneName = meshBoneNames[i];

			// Find bone in source skeleton
			Int32 mappedBoneIndex = -1;
			for ( Uint32 j=0; j<sourceBones.Size(); j++ )
			{
				const ISkeletonDataProvider::BoneInfo& sourceBone = sourceBones[j];
				if ( sourceBone.m_name == meshBoneName )
				{
					mappedBoneIndex = j;
					break;
				}
			}

			// Remember
			m_boneMapping[ i ] = mappedBoneIndex;
		}
	}

	void CMassiveAnimation::UpdateInstance(Float animTime, Uint32 animationInstanceId, Uint32 frameIndex)
	{
		ASSERT( m_mesh != nullptr && m_animation != nullptr && m_skeleton != nullptr && m_skeleton->IsValid() );

		// Get instance by id
		ASSERT( animationInstanceId < m_instanceCount );
		CMassiveAnimationInstance& animInstance = m_instances[ animationInstanceId ];

		// Skip if animation instance was already updated in this frame
		if( frameIndex == animInstance.m_lastFrameIndex )
		{
			return;
		}	
		animInstance.m_lastFrameIndex = frameIndex;

		// Get animation time for instance
		Float instanceAnimTime = GetAnimationInstanceTime( &animInstance, animTime );

		//LOG_ENGINE( TXT("Anim: %ls    Inst: %i    Time: %f    FrameId: %i"), m_animation->GetName().AsChar(), animationInstanceId, instanceAnimTime, frameIndex );

		// Update skinning data
		static TDynArray<Matrix> skeletonModelSpace;

		// Sample animation
		static TDynArray< AnimQsTransform > animatedBones;
		static TDynArray< AnimFloat > tracks;
		CSkeletalAnimation* anim = m_animation->GetAnimation();
		anim->Sample( instanceAnimTime, animatedBones, tracks );

		// Get bones in model space
		skeletonModelSpace.Resize( m_skeleton->GetBonesNum() );
		GetBonesModelSpace( skeletonModelSpace, animatedBones );

		// Write to skinning data
		const Matrix* rigMatrices = m_mesh->GetBoneRigMatrices();

		Box outBox;

		ISkeletonDataProvider::SBonesData		bonesData( outBox );
		bonesData.m_boneIndices					= m_boneMapping.TypedData();
		bonesData.m_numBones					= m_boneMapping.Size();
		bonesData.m_rigMatrices					= rigMatrices;
		bonesData.m_vertexEpsilons				= m_mesh->GetBoneVertexEpsilons();
		bonesData.m_outMatricesArray			= animInstance.m_skinningData->GetWriteData();
		bonesData.m_outMatricesType				= animInstance.m_skinningData->GetMatrixType();
		SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, skeletonModelSpace );

		animInstance.m_skinningData->AdvanceWrite();
		animInstance.m_skinningData->AdvanceRead();
	}

	Uint32 CMassiveAnimation::GetBeginningInstanceId( Float animTime ) const
	{
		ASSERT( m_instanceCount > 0 );

		// Find animation closest to 0.0
		float bestTime = FLT_MAX;
		Uint32 bestInstance = 0;

		for( Uint32 i=0; i<m_instanceCount; ++i )
		{
			animTime = GetAnimationInstanceTime( &m_instances[i], animTime );
			if( animTime < bestTime )
			{
				bestInstance		= i;
				bestTime			= animTime;
			}
		}

		return bestInstance;
	}

	Float CMassiveAnimation::GetAnimationInstanceTime(const CMassiveAnimationInstance* instance, Float animTime) const
	{
		ASSERT( m_animation != nullptr );

		// Add instance offset
		animTime += instance->m_timeOffset;

		// Make cyclical
		Float duration = m_animation->GetDuration();
		animTime -= (((int)(animTime / duration)) * duration);

		return animTime;
	}

	void CMassiveAnimation::GetBonesModelSpace( TDynArray<Matrix>& bonesMS, TDynArray< AnimQsTransform >& animatedBones )
	{
		// To many bones
		AnimQsTransform bonesMSTransforms[ 512 ];
		ASSERT( bonesMS.Size() < ARRAY_COUNT( bonesMSTransforms ) )

		// Calculate MS transforms
		const Uint32 bonesNum = (Uint32)m_skeleton->GetBonesNum();
		for (Uint32 boneIdx=0; boneIdx<bonesNum; boneIdx++)
		{
			// Bone in local space
			AnimQsTransform& boneTransform = bonesMSTransforms[ boneIdx ];

			// Get parent index
			Int32 parentIdx = m_skeleton->GetParentBoneIndex( boneIdx );

			// If bone is not root
			if ( parentIdx != -1 )
			{
				// Get parent bone in model space
				const AnimQsTransform& parentMS = bonesMSTransforms[parentIdx];

				AnimQsTransform bone = animatedBones[ boneIdx ];

				boneTransform.SetMul( parentMS, bone );
				ASSERT(boneTransform.Rotation.IsOk());
				boneTransform.Rotation.Normalize();
				const RedMatrix4x4 mat = boneTransform.ConvertToMatrix();
				bonesMS[boneIdx] = reinterpret_cast<const Matrix&>(mat);
			}
			else
			{
				boneTransform = animatedBones[ boneIdx ];
				const RedMatrix4x4 mat = boneTransform.ConvertToMatrix();
				bonesMS[boneIdx] = reinterpret_cast<const Matrix&>(mat);
			}
		}
	}

	const CRenderSkinningData* CMassiveAnimation::GetSkinningDataForInstance(Uint32 animationInstanceId) const
	{
		return m_instances[animationInstanceId].m_skinningData;
	}

	/**************************************************************************************************************/

	CMassiveAnimationSet::CMassiveAnimationSet()
		: m_name( CName::NONE )
	{
		/* Intentionally Empty */
	}

	CMassiveAnimationSet::CMassiveAnimationSet( CName name )
		: m_name( name )
	{
		/* Intentionally Empty */
	}

	CMassiveAnimationSet::~CMassiveAnimationSet()
	{
		m_animations.ClearPtr();
	}

	CMassiveAnimation* CMassiveAnimationSet::GetAnimation(CName animation)
	{
		CMassiveAnimation* result = nullptr;
		m_animations.Find( animation, result );
		return result;
	}

	Bool CMassiveAnimationSet::AddMassiveAnimation(CName name, CMassiveAnimation* massiveAnimation)
	{
		return m_animations.Insert( name, massiveAnimation );
	}

	CMassiveAnimation* CMassiveAnimationSet::GetDefaultAnimation()
	{
		ASSERT( m_animations.Size() > 0 );
		return m_animations.Begin().Value();
	}

	/**************************************************************************************************************/

	MassiveAnimationSetSharedPtr CMassiveAnimationRegister::GetMassiveAnimationSet(CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSet* animationSet)
	{
		ASSERT( mesh != nullptr && skeleton != nullptr && animationSet != nullptr && skeleton->IsValid() );

		String hashName = mesh->GetDepotPath() + skeleton->GetDepotPath() + animationSet->GetDepotPath();
		CName animSetName = CName(hashName);

		MassiveAnimationSetSharedPtr result;
		MassiveAnimationSetWeakPtr weakResult;

		if( m_animationSets.Find( animSetName, weakResult ) == true && weakResult.Expired() == false )
		{
			result = weakResult.Lock();
		}
		else
		{
			result = CreateMassiveAnimationSet( animSetName, mesh, skeleton, animationSet );
			if( result != nullptr )
			{
				m_animationSets.Insert( animSetName, result );
			}
		}

		return result;
	}

	MassiveAnimationSetSharedPtr CMassiveAnimationRegister::CreateMassiveAnimationSet(CName name, CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSet* animationSet)
	{
		ASSERT( mesh != nullptr && skeleton != nullptr && animationSet != nullptr && skeleton->IsValid() );

		Bool result = true;

		MassiveAnimationSetSharedPtr resultAnimationSet = MassiveAnimationSetSharedPtr( new CMassiveAnimationSet( name ) );

		// Cache massive animation set
		m_animationSets.Insert( name, resultAnimationSet );

		// Create massive animations
		TDynArray<CSkeletalAnimationSetEntry*>& animations = animationSet->GetAnimations();
		for( CSkeletalAnimationSetEntry* anim : animations )
		{
			RED_ASSERT( anim != nullptr, TXT("Swarm animation data is invalid") );
			if( anim == nullptr )
			{
				return MassiveAnimationSetSharedPtr();
			}
			resultAnimationSet->AddMassiveAnimation( anim->GetAnimation()->GetName(), CMassiveAnimation::Create( mesh, skeleton, anim ) );
		}

		return resultAnimationSet;
	}

}

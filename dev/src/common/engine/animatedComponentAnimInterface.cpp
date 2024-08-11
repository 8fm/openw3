/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animatedComponent.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphContext.h"
#include "../core/taskManager.h"
#include "skeletonUtils.h"
#include "skeletalAnimationContainer.h"
#include "skeletalAnimationSet.h"
#include "skeleton.h"
#include "animatedSkeleton.h"
#include "fxDefinition.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

void CAnimatedComponent::AddAnimationSet( CSkeletalAnimationSet *animSet )
{
	ASSERT( animSet );

	if ( m_animationSets.PushBackUnique( animSet ) )
	{
		if ( m_animations )
		{
			m_animations->AddAnimationSet( animSet );
		}
	}
}

void CAnimatedComponent::RemoveAnimationSet( CSkeletalAnimationSet *animSet )
{
	ASSERT( animSet );

	if ( m_animationSets.Remove( animSet ) )
	{
		if ( m_animations )
		{
			m_animations->RemoveAnimationSet( animSet );
		}
	}
}

Float CAnimatedComponent::GetAnimationDuration( const CName& animationName ) const
{
	// Find animation
	if ( m_animations )
	{
		CSkeletalAnimationSetEntry* entry = m_animations->FindAnimation( animationName );
		if ( entry && entry->GetAnimation() )
		{
			// Return animation duration
			return entry->GetAnimation()->GetDuration();
		}
	}

	// Not found
	return 1.0f;
}

Bool CAnimatedComponent::PlayAnimationOnSkeleton( const CName& animation, Bool repleace /* = true */, Bool looped /* = true */, Float weight /* = 1.f  */)
{
	CSkeletalAnimationSetEntry* animEntry = GetAnimationContainer()->FindAnimation( animation );
	if ( animEntry && m_animatedSkeleton )
	{
		if ( m_animatedSkeleton->PlayAnimation( animEntry, repleace, looped, weight ) )
		{
			if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Deactivate();
			}

			return true;
		}
	}

	return false;
}

Bool CAnimatedComponent::PlayAnimationAtTimeOnSkeleton( const CName& animation, Float time, Bool repleace /* = true */, Bool looped /* = true */, Float weight /* = 1.f  */)
{
	CSkeletalAnimationSetEntry* animEntry = GetAnimationContainer()->FindAnimation( animation );
	if ( animEntry && m_animatedSkeleton )
	{
		CPlayedSkeletalAnimation * anim = m_animatedSkeleton->PlayAnimation( animEntry, repleace, looped, weight );
		if ( anim )
		{
			anim->SetTime( time );
			anim->Pause();
			if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Deactivate();
			}

			return true;
		}
	}

	return false;
}

Bool CAnimatedComponent::PlayAnimationOnSkeletonWithSync( const CName& animation, const CSyncInfo& syncInfo )
{
	//THIS SHOULDN'T HAPPEN ASYNC BUT IT DOES SO I HAVE TO STORE THE INFORMATION AND DO IT SYNCHRONOUSLY
	m_asyncPlayedAnimSyncInfo = syncInfo;
	m_asyncPlayedAnimName = animation;

	//if ( m_animatedSkeleton )
	//{
	//	CPlayedSkeletalAnimation* anim = m_animatedSkeleton->GetPlayedAnimation( animation );
	//	if ( !anim )
	//	{
	//		CSkeletalAnimationSetEntry* animEntry = GetAnimationContainer()->FindAnimation( animation );
	//		anim = m_animatedSkeleton->PlayAnimation( animEntry );
	//	}

	//	if ( anim )
	//	{
	//		anim->SynchronizeTo( syncInfo );
	//		anim->Pause();
	//		if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() )
	//		{
	//			m_behaviorGraphStack->Deactivate();
	//		}

	//		return true;
	//	}
	//}

	return false;
}

void CAnimatedComponent::StopAllAnimationsOnSkeleton()
{
	m_asyncPlayedAnimName = CNAME(StopAllAnimationsOnSkeleton);
}

void CAnimatedComponent::PauseAllAnimationsOnSkeleton( Bool flag )
{
	if ( m_animatedSkeleton )
	{
		m_animatedSkeleton->PauseAllAnimations( flag );
	}
}

void CAnimatedComponent::TogglePauseAllAnimationsOnSkeleton()
{
	if ( m_animatedSkeleton )
	{
		const Bool flag = m_animatedSkeleton->IsAnyAnimationPaused();
		m_animatedSkeleton->PauseAllAnimations( !flag );
	}
}

void CAnimatedComponent::GetBehaviorInstanceSlots( TDynArray< CName >& slotNames ) const
{
	for ( Uint32 i=0; i<m_behaviorInstanceSlots.Size(); ++i )
	{
		slotNames.PushBack( m_behaviorInstanceSlots[i].m_instanceName );
	}
}

CBehaviorGraph* CAnimatedComponent::GetBehaviorInstanceGraph( const CName& slotName ) const
{
	for ( Uint32 i=0; i<m_behaviorInstanceSlots.Size(); ++i )
	{
		if ( m_behaviorInstanceSlots[ i ].m_instanceName == slotName )
		{
			return m_behaviorInstanceSlots[ i ].m_graph.Get();
		}
	}
	return NULL;
}

Int32 CAnimatedComponent::FindBoneByName( const Char* name ) const
{
	// No skeleton
	if ( !m_skeleton )
	{
		return -1;
	}

	//dex++: moved code inside the CSkeleton 
	return m_skeleton->FindBoneByName( name );
	//dex--;
}

Int32 CAnimatedComponent::FindBoneByName( const AnsiChar* name ) const
{
	// No skeleton
	if ( !m_skeleton )
	{
		return -1;
	}

	//dex++: moved code inside the CSkeleton 
	return m_skeleton->FindBoneByName( name );
	//dex--;
}

Int32 CAnimatedComponent::FindBoneByName( const CName& name ) const
{
	// No skeleton
	if ( !m_skeleton )
	{
		return -1;
	}

	return m_skeleton->FindBoneByName( name );
}

Uint32 CAnimatedComponent::GetRuntimeCacheIndex() const
{
	return m_skeleton ? m_skeleton->GetRuntimeIndex() : 0;
}

const struct SSkeletonSkeletonCacheEntryData* CAnimatedComponent::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	if ( m_skeleton )
		return m_skeleton->GetMappingCache().GetMappingEntry( parentSkeleton );

	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

Uint32 CAnimatedComponent::GetBones( TDynArray< BoneInfo >& bones ) const
{
	// No skeleton
	if ( !m_skeleton )
	{
		return 0;
	}

	//dex++: moved inside the CSkeleton class
	return m_skeleton->GetBones( bones );
	//dex--;
}

Uint32 CAnimatedComponent::GetBones( TAllocArray< BoneInfo >& bones ) const
{
	// No skeleton
	if ( !m_skeleton )
	{
		return 0;
	}

	//dex++: moved inside the CSkeleton class
	return m_skeleton->GetBones( bones );
	//dex--;
}

const CRagdoll* CAnimatedComponent::GetRagdoll() const
{
	return m_ragdoll.Get();
}

Bool CAnimatedComponent::HasRagdoll() const
{
	return m_ragdoll.IsValid();
}

Uint32 CAnimatedComponent::GetFloatTrackNum() const
{
	if( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		return m_behaviorGraphSampleContext->GetFloatTrackNum();
	}
	else
	{
		return 0;
	}
}

Float CAnimatedComponent::GetFloatTrack( Uint32 trackIndex ) const
{
	if( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		return m_behaviorGraphSampleContext->GetFloatTrack( trackIndex );
	}
	else
	{
		return 0.f;
	}
}

Matrix CAnimatedComponent::GetBoneMatrixLocalSpace( Uint32 boneIndex ) const
{
	if( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		return m_behaviorGraphSampleContext->GetBoneMatrixLocalSpace( boneIndex );
	}
	else
	{
		return Matrix::IDENTITY;
	}
}

AnimQsTransform CAnimatedComponent::GetBoneTransformLocalSpace( Uint32 boneIndex ) const
{
	AnimQsTransform transform( AnimQsTransform::IDENTITY );

	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		transform = m_behaviorGraphSampleContext->GetBoneTransformLocalSpace( boneIndex );
	}

	return transform;
}

Bool CAnimatedComponent::IsCurrentPoseTPose() const
{
	if( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		return m_behaviorGraphSampleContext->IsMainPoseTPose();
	}
	return true;
}

Bool CAnimatedComponent::HasAnimationEventOccurred( const CName& animEventName ) const
{
	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		return m_behaviorGraphSampleContext->HasAnimationEventOccurred( animEventName );
	}
	return false;
}

Matrix CAnimatedComponent::CalcBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return Matrix::IDENTITY;
}

Matrix CAnimatedComponent::GetBoneMatrixModelSpace( Uint32 boneIndex ) const
{
	return boneIndex < m_skeletonModelSpace.Size() ? m_skeletonModelSpace[ boneIndex ] : Matrix::IDENTITY;
}

Matrix CAnimatedComponent::GetBoneMatrixWorldSpace( Uint32 boneIndex ) const
{
	// TODO: remove this hack
	// we don't have m_skeletonModelSpace on first attach and calculating bbox
	if ( m_skeletonWorldSpace.Empty() )
		return Matrix::IDENTITY;

	return boneIndex < m_skeletonWorldSpace.Size() ? m_skeletonWorldSpace[ boneIndex ] : Matrix::IDENTITY;
}

Bool CAnimatedComponent::GetThisFrameTempBoneTranslationWorldSpace( const Int32 boneIdx, Vector& outTranslationWS ) const
{
	if ( boneIdx != -1 && boneIdx < m_skeletonModelSpace.SizeInt() )
	{
		const Matrix& l2w = GetThisFrameTempLocalToWorld();
		const Matrix& boneMS = GetBoneMatrixModelSpace( (Uint32)boneIdx );

		outTranslationWS = l2w.TransformPoint( boneMS.GetTranslation() );
		return true;
	}

	outTranslationWS = Vector::ZERO_3D_POINT;
	return false;
}

Bool CAnimatedComponent::GetThisFrameTempBoneMatrixWorldSpace( const Int32 boneIdx, Matrix& outMatrixWS ) const
{
	if ( boneIdx != -1 && boneIdx < m_skeletonModelSpace.SizeInt() )
	{
		const Matrix& l2w = GetThisFrameTempLocalToWorld();
		const Matrix& boneMS = GetBoneMatrixModelSpace( (Uint32)boneIdx );

		outMatrixWS = Matrix::Mul( l2w, boneMS );
		return true;
	}

	outMatrixWS = Matrix::IDENTITY;
	return false;
}

void CAnimatedComponent::SetThisFrameTempBoneModelSpace( const Int32 boneIdx, const Matrix& m )
{
	if ( boneIdx != -1 && boneIdx < m_skeletonModelSpace.SizeInt() )
	{
		m_skeletonModelSpace[ boneIdx ] = m;
	}
}

void CAnimatedComponent::GetBoneMatricesAndBoundingBoxModelSpace( const ISkeletonDataProvider::SBonesData& bonesData ) const
{
#ifdef USE_OPT_SKINNING
	static Bool USE_THIS = false;
	if ( USE_THIS )
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpace_opt( bonesData, m_skeletonModelSpace, m_skeletonInvBindSpace );
	}
	else
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, m_skeletonModelSpace );
	}
#else
	if( m_wetnessSupplier )
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpaceWithWetnessData( bonesData, m_skeletonModelSpace, m_wetnessSupplier );
	}
	else
	{
		SkeletonBonesUtils::GetBoneMatricesModelSpace( bonesData, m_skeletonModelSpace );
	}
#endif

	SkeletonBonesUtils::CalcBoundingBoxModelSpace( bonesData, nullptr, 0, m_skeletonModelSpace );
}

Bool CAnimatedComponent::GetEffectParameterValue( CName paramName, CVariant &value /* out */ ) const
{
	if ( m_behaviorGraphStack )
	{
		RED_MESSAGE(  "Can we convert this call stack into either purely CNames or purely strings?" )
		value = CVariant( m_behaviorGraphStack->GetBehaviorFloatVariable( paramName ) );
		return true;
	}

	return false;
}

Bool CAnimatedComponent::SetEffectParameterValue( CName paramName, const CVariant &value )
{
	if ( m_behaviorGraphStack )
	{
		Float paramValue;
		value.AsType( paramValue );

		RED_MESSAGE(  "Can we convert this call stack into either purely CNames or purely strings?" )
		return m_behaviorGraphStack->SetBehaviorVariable( paramName, paramValue );
	}

	return false;
}

void CAnimatedComponent::EnumEffectParameters( CFXParameters &effectParams /* out */ )
{
	if ( m_behaviorGraphStack )
	{
		// Enumerate parameters from behavior graph
		TDynArray< String > behaviorVarNames;
		ASSERT( 0 );
		//graphInstance->EnumVariableNames( behaviorVarNames, true );

		// Convert to CNames...
		for ( Uint32 i=0; i<behaviorVarNames.Size(); i++ )
		{
			CName parameterName( behaviorVarNames[i] );
			effectParams.AddParameter< Float >( parameterName );
		}
	}
}

void CAnimatedComponent::EnumEffectBehaviorEvents( TDynArray< CName > &behaviorEvents )
{
	// Get the event names from behavior graph
	if ( m_behaviorGraphStack )
	{
		ASSERT( 0 );
		//graphInstance->EnumEventNames( behaviorEvents, true );
	}
}

#ifdef DEBUG_AC
#pragma optimize("",on)
#endif

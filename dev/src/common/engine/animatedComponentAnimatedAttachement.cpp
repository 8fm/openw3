
#include "build.h"
#include "animatedComponent.h"
#include "behaviorGraphContext.h"
#include "animatedAttachment.h"
#include "mimicComponent.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

void CAnimatedComponent::CacheAnimatedAttachments()
{
	for ( TList< IAttachment* >::iterator it = m_childAttachments.Begin(); it != m_childAttachments.End(); ++it )
	{
		IAttachment* att = *it;

		ASSERT( att->GetChild(), TXT("Attachment without child?") );

		if ( att->IsA< CAnimatedAttachment >() )
		{
			CAnimatedAttachment* aa = static_cast< CAnimatedAttachment* >( att );

			// This should be already added to list in void CAnimatedComponent::OnChildAttachmentAdded( IAttachment* attachment )
			// No it doesn't work after some changes with streaming....
			//ASSERT( m_cachedAnimatedAttachments.Exist( aa ) );
			
			AddChildAnimatedAttachment( aa );
		}
		else if ( att->GetChild()->IsA< CAnimatedComponent >() )
		{
			m_cachedAnimatedChildComponents.PushBack( static_cast< CAnimatedComponent* >( att->GetChild() ) );
		}
	}

	UpdateAttachedToAnimParent();
}

void CAnimatedComponent::AddChildAnimatedAttachment( CAnimatedAttachment* aa )
{
	if ( !m_cachedAnimatedAttachments.Exist( aa ) )
	{
		m_cachedAnimatedAttachments.PushBack( aa );

		SortAnimatedAttachments();
	}
}

void CAnimatedComponent::RemoveChildAnimatedAttachment( CAnimatedAttachment* aa )
{
	if ( m_cachedAnimatedAttachments.Remove( aa ) )
	{
		SortAnimatedAttachments();
	}
}

void CAnimatedComponent::SortAnimatedAttachments()
{
	//++ DEBUG
	/*
	{
		Int32 counter = 0;
		for ( Uint32 i=0; i<m_cachedAnimatedAttachments.Size(); ++i )
		{
			CAnimatedAttachment* a = m_cachedAnimatedAttachments[ i ];
			if ( a && a->GetChild() && a->GetChild()->IsA< CMimicComponent >() )
			{
				counter++;
			}
		}
		if ( counter > 1 )
		{
			ASSERT( 0 );
		}

		ASSERT( counter <= 1 );
	}
	*/
	//--

	const Uint32 num = m_cachedAnimatedAttachments.Size();
	if ( num == 1 )
	{
		CAnimatedAttachment* a = m_cachedAnimatedAttachments[ 0 ];
		if ( a && a->GetChild() && a->GetChild()->IsA< CMimicComponent >() )
		{
			return;
		}
	}

	for ( Uint32 i=1; i<num; ++i )
	{
		CAnimatedAttachment* a = m_cachedAnimatedAttachments[ i ];
		if ( a && a->GetChild() && a->GetChild()->IsA< CMimicComponent >() )
		{
			m_cachedAnimatedAttachments.Swap( 0, i );
			break;
		}
	}
}

void CAnimatedComponent::UpdateAttachedToAnimParent()
{
	ASSERT( ! GetTransformParent() || GetTransformParent()->GetParent(), TXT("Attachment without parent?") );
	m_attachedToParentWithAnimatedAttachment = GetTransformParent() && GetTransformParent()->IsA< CAnimatedAttachment >();
	m_attachedToAnimatedComponentParent = GetTransformParent() && GetTransformParent()->GetParent()->IsA< CAnimatedComponent >();
}

Bool CAnimatedComponent::ShouldBeUpdatedByAnimatedComponentParent() const
{
	return m_attachedToAnimatedComponentParent && ! m_attachedToParentWithAnimatedAttachment;
}

void CAnimatedComponent::UpdateAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];
		if ( att )
		{
			att->ParentPoseUpdateLS( dt, poseLS );
		}
	}
}

void CAnimatedComponent::UpdateAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];
		if ( att )
		{
			att->ParentPoseUpdateLSAsync( dt, poseLS );
		}
	}
}

void CAnimatedComponent::UpdateAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];
		if ( att )
		{
			att->ParentPoseUpdateLSSync( dt, poseLS );
		}
	}
}

void CAnimatedComponent::UpdateAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS )
{
	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];
		if ( att )
		{
			att->ParentPoseUpdateLSConstrainted( dt, poseLS );
		}
	}
}

void CAnimatedComponent::UpdateAttachedAnimatedObjectsWS()
{
	PC_SCOPE( UpdateAttachedAnimatedObjectsWS );

	SBehaviorGraphOutput* poseLS = m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() ? &(m_behaviorGraphSampleContext->GetSampledPose()) : NULL;

	const Uint32 size = m_cachedAnimatedAttachments.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedAttachment* att = m_cachedAnimatedAttachments[ i ];
		if ( att )
		{
			att->ParentPoseUpdateWS( poseLS, &m_skeletonModelSpace, &m_skeletonWorldSpace );
		}
	}
}

void CAnimatedComponent::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();

		const Uint32 size = bones.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SBoneMapping& m = bones[ i ];

			if ( (Int32)pose.m_numBones > m.m_boneA )
			{
				pose.m_outputPose[ m.m_boneA ] = poseLS->m_outputPose[ m.m_boneB ];
			}
		}

		pose.m_outputPose[ 0 ].SetIdentity();
	}
}

void CAnimatedComponent::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();

		const Uint32 size = bones.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SBoneMapping& m = bones[ i ];

			if ( (Int32)pose.m_numBones > m.m_boneA )
			{
				pose.m_outputPose[ m.m_boneA ] = poseLS->m_outputPose[ m.m_boneB ];
			}
		}

		pose.m_outputPose[ 0 ].SetIdentity();
	}
}

void CAnimatedComponent::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CAnimatedComponent::OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	
}

void CAnimatedComponent::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	/*const Uint32 size = bones.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SBoneMapping& m = bones[ i ];

		if ( m_skeletonModelSpace.SizeInt() > m.m_boneA )
		{
			m_skeletonModelSpace[ m.m_boneA ] = (*poseMS)[ m.m_boneB ];
			m_skeletonWorldSpace[ m.m_boneA ] = (*poseWS)[ m.m_boneB ];
		}
	}*/
}

#ifdef DEBUG_AC
#pragma optimize("",on)
#endif

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animatedAttachment.h"
#include "animatedObjectInterface.h"
#include "animatedComponent.h"
#include "skeletonSkeletonMappingCache.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimatedAttachment );

AnimatedAttachmentSpawnInfo::AnimatedAttachmentSpawnInfo()
	: HardAttachmentSpawnInfo()
{
	m_attachmentClass = ClassID< CAnimatedAttachment >();
}

//////////////////////////////////////////////////////////////////////////

CAnimatedAttachment::CAnimatedAttachment()
{
	m_isAnimatedAttachment = true;
}

void CAnimatedAttachment::ParentPoseUpdateLS( Float dt, SBehaviorGraphOutput* poseLS )
{
	CNode* childComp = GetChild();
	IAnimatedObjectInterface* child = childComp ? childComp->QueryAnimatedObjectInterface() : NULL;
	if ( child )
	{
		const struct SSkeletonSkeletonCacheEntryData* mapping = GetSkeletonMaping(); 
		child->OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, mapping->m_boneMapping );
	}
}

void CAnimatedAttachment::ParentPoseUpdateLSAsync( Float dt, SBehaviorGraphOutput* poseLS )
{
	CNode* childComp = GetChild();
	IAnimatedObjectInterface* child = childComp ? childComp->QueryAnimatedObjectInterface() : NULL;
	if ( child )
	{
		const struct SSkeletonSkeletonCacheEntryData* mapping = GetSkeletonMaping(); 
		child->OnParentUpdatedAttachedAnimatedObjectsLSAsync( dt, poseLS, mapping->m_boneMapping );
	}
}

void CAnimatedAttachment::ParentPoseUpdateLSSync( Float dt, SBehaviorGraphOutput* poseLS )
{
	CNode* childComp = GetChild();
	IAnimatedObjectInterface* child = childComp ? childComp->QueryAnimatedObjectInterface() : NULL;
	if ( child )
	{
		const struct SSkeletonSkeletonCacheEntryData* mapping = GetSkeletonMaping(); 
		child->OnParentUpdatedAttachedAnimatedObjectsLSSync( dt, poseLS, mapping->m_boneMapping );
	}
}

void CAnimatedAttachment::ParentPoseUpdateLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS )
{
	CNode* childComp = GetChild();
	IAnimatedObjectInterface* child = childComp ? childComp->QueryAnimatedObjectInterface() : nullptr;
	if ( child )
	{
		const struct SSkeletonSkeletonCacheEntryData* mapping = GetSkeletonMaping(); 
		child->OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( dt, poseLS, mapping->m_boneMapping );
	}
}

void CAnimatedAttachment::ParentPoseUpdateWS( SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS )
{
	CNode* childComp = GetChild();
	CNode* parentComp = GetParent();

	CAnimatedComponent* parent = parentComp ? Cast< CAnimatedComponent >( parentComp ) : nullptr;
	IAnimatedObjectInterface* child = childComp ? childComp->QueryAnimatedObjectInterface() : nullptr;
	if ( child )
	{
		const struct SSkeletonSkeletonCacheEntryData* mapping = GetSkeletonMaping(); 
		child->OnParentUpdatedAttachedAnimatedObjectsWS( parent, poseLS, poseMS, poseWS, mapping->m_boneMapping );
	}
}

Bool CAnimatedAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// Parent component should be animated component
	if ( !parent->QueryAnimatedObjectInterface() )
	{
		if( !GIsCooker ) { WARN_ENGINE( TXT("Unable to create animated attachment because parent component '%ls' is not a IAnimatedObjectInterface"), parent->GetName().AsChar() ); }
		return false;
	}

	// Child component should be animated component
	if ( !child->QueryAnimatedObjectInterface() )
	{
		if( !GIsCooker ) { WARN_ENGINE( TXT("Unable to create animated attachment because child component '%ls' is not a IAnimatedObjectInterface"), child->GetName().AsChar() ); }
		return false;
	}

	// Initialize base attachment
	return TBaseClass::Init( parent, child, info );
}

const struct SSkeletonSkeletonCacheEntryData* CAnimatedAttachment::GetSkeletonMaping() const
{
	ASSERT( GetChild() != nullptr, TXT("Animated attachment %ls has null child - WTF ?"), GetFriendlyName().AsChar() );
	const ISkeletonDataProvider* child = GetChild() ? GetChild()->QuerySkeletonDataProvider() : nullptr;

	ASSERT( GetParent() != nullptr, TXT("Animated attachment %ls has null parent - WTF ?"), GetFriendlyName().AsChar() );
	const ISkeletonDataProvider* parent = GetParent() ? GetParent()->QuerySkeletonDataProvider() : nullptr;

	if ( child && parent )
	{
		return child->GetSkeletonMaping( parent );
	}

	ERR_ENGINE( TXT("Do not ignore - Unable to determine skeleton mapping in animated attachment between child (%ls) and parent (%ls)"),
		GetChild() ? GetChild()->GetFriendlyName().AsChar() : TXT("NULL"),
		GetParent() ? GetParent()->GetFriendlyName().AsChar() : TXT("NULL") );

	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

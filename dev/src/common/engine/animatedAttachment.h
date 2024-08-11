/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "hardAttachment.h"
#include "skeletonProvider.h"

struct SBehaviorGraphOutput;

struct SBoneMapping
{
	Int16	m_boneA;
	Int16	m_boneB;

	DECLARE_RTTI_STRUCT( SBoneMapping );
};

typedef TDynArray< SBoneMapping, MC_SkinningMapping > BoneMappingContainer;

//////////////////////////////////////////////////////////////////////////

/// Animated attachment spawn info
class AnimatedAttachmentSpawnInfo : public HardAttachmentSpawnInfo
{
public:
	AnimatedAttachmentSpawnInfo();
};

//////////////////////////////////////////////////////////////////////////

/// Special attachments that connect two animated components
class CAnimatedAttachment : public CHardAttachment
{
	DECLARE_ENGINE_CLASS( CAnimatedAttachment, CHardAttachment, 0 );

public:
	CAnimatedAttachment();

	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

public:
	void ParentPoseUpdateLS( Float dt, SBehaviorGraphOutput* poseLS );
	void ParentPoseUpdateLSAsync( Float dt, SBehaviorGraphOutput* poseLS );
	void ParentPoseUpdateLSSync( Float dt, SBehaviorGraphOutput* poseLS );
	void ParentPoseUpdateLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS );
	void ParentPoseUpdateWS( SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS );

	const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping() const;
};

BEGIN_CLASS_RTTI( CAnimatedAttachment );
	PARENT_CLASS( CHardAttachment );
END_CLASS_RTTI();

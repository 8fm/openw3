/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "attachment.h"
#include "hardAttachment.h"
#include "node.h"

class ISkeletonDataProvider;

/// Skinning attachment spawn info
class SkinningAttachmentSpawnInfo : public HardAttachmentSpawnInfo
{
public:
	SkinningAttachmentSpawnInfo();
};

/// Skinning attachment - extracts the skinning data from ISkeletonDataProvider
class CSkinningAttachment : public CHardAttachment
{
	DECLARE_ENGINE_CLASS( CSkinningAttachment, CHardAttachment, 0 )

public:
	//! Get skeleton data provider
	RED_INLINE const ISkeletonDataProvider* GetSkeletonDataProvider() const { return GetParent()->QuerySkeletonDataProvider(); }

public:
	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

	// We cannot create this attachment
	virtual Bool IsManualCreationAllowed() const { return false; }

	// Calculate final localToWorld matrix for attached component
	virtual void CalcAttachedLocalToWorld( Matrix& out ) const;

	//virtual void UpdateTransformAndSkinningData() {} // Do not use fake virtual functions - it's slower
};

BEGIN_CLASS_RTTI( CSkinningAttachment )
	PARENT_CLASS( CHardAttachment );
END_CLASS_RTTI();

	

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "skinningAttachment.h"
#include "renderProxy.h"

class IRenderSkinningData;
struct SMeshSkinningUpdateContext;

/// Skinning attachment spawn info
class MeshSkinningAttachmentSpawnInfo : public SkinningAttachmentSpawnInfo
{
public:
	MeshSkinningAttachmentSpawnInfo();
};

/// Special attachments that skins skinned mesh
class CMeshSkinningAttachment : public CSkinningAttachment
{
	DECLARE_ENGINE_CLASS( CMeshSkinningAttachment, CSkinningAttachment, 0 );

protected:
	IRenderSkinningData*	m_skinningData;				// Render skinning data
	Uint16					m_skinningDataBoneCount;	// Number of bones in the skinning data

public:
	CMeshSkinningAttachment();
	virtual ~CMeshSkinningAttachment();

	virtual void OnFinalize() override;

	RED_FORCE_INLINE const IRenderSkinningData* GetSkinningData() const { return m_skinningData; }

	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

	// We can create this attachment by hand
	virtual Bool IsManualCreationAllowed() const { return true; }

	//! Recreate skinning data
	virtual void RecreateSkinningData();

	//! Force the attachment to release its IRenderSkinningData. It will be recreated in the next update.
	void DiscardSkinningData();

	// Retrieve the cached mapping data - use and forget, do not store pointer
	const struct SMeshSkeletonCacheEntryData* GetCachedData() const;

	// Retrieve the cached bone mapping table - will return EMPTY table for missing mapping
	const TDynArray< Int16, MC_SkinningMapping >& GetCachedMappingTable() const;

	// Update all transform and sinning data in child
	void UpdateTransformAndSkinningData( Box& outBoxMS, SMeshSkinningUpdateContext& skinningContext );
	void UpdateTransformWithoutSkinningData( const Box& boxMS, SMeshSkinningUpdateContext& skinningContext );

	// Check if skinning att is valid or not
	Bool IsSkinningMappingValid() const;

	// Tempshit - entity.cpp was locked and we need this fix...
	const TDynArray< Int32 >& GetBoneMapping() const;

	virtual void Break() override;

protected:
	// Get skinning matrices, returns number of skinning matrices filled
	Uint32 GetSkinningMatricesAndBox( void* skinningMatrices, Box& outBoxMS, const Matrix* rigMatrices, const Float* vertexEpsilons );
};

BEGIN_CLASS_RTTI( CMeshSkinningAttachment );
	PARENT_CLASS( CSkinningAttachment );
END_CLASS_RTTI();

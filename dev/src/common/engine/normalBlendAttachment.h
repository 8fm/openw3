/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "meshTypeResource.h"
#include "attachment.h"


class NormalBlendAttachmentSpawnInfo : public AttachmentSpawnInfo
{
public:
	NormalBlendAttachmentSpawnInfo();
};

class CNormalBlendAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CNormalBlendAttachment, IAttachment, 0 )

public:
	CNormalBlendAttachment();
	virtual ~CNormalBlendAttachment();

	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

	void SetNormalBlendWeights( Uint32 firstWeight, Uint32 numWeights, const Float* weights );
	void SetNormalBlendAreas( Uint32 firstArea, Uint32 numAreas, const Vector* areas );
	void SetNormalBlendMaterial( CMaterialInstance* material, IMaterial* sourceBaseMaterial, ITexture* sourceNormalTexture );
	
	/// Get the materials used by the attached Render Proxies.
	void GetMaterials( CMeshTypeResource::TMaterials& materials );
};


BEGIN_CLASS_RTTI( CNormalBlendAttachment );
PARENT_CLASS( IAttachment );
END_CLASS_RTTI();

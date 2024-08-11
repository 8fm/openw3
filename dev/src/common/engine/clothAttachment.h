/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "meshSkinningAttachment.h"


/// Old attachment for skinning a CClothComponent. It is now deprecated and CMeshSkinningAttachment should be used instead.
/// This inherits from CMeshSkinningAttachment, so that places where it is currently in use will still function properly,
/// but some data errors will be reported.
class CClothAttachment : public CMeshSkinningAttachment
{
	DECLARE_ENGINE_CLASS( CClothAttachment, CMeshSkinningAttachment, 0 );

protected:
	void ReportDeprecated();

public:
	CClothAttachment();
	virtual ~CClothAttachment();

	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );
	virtual void OnSerialize( IFile& file );

};

BEGIN_CLASS_RTTI( CClothAttachment );
	PARENT_CLASS( CMeshSkinningAttachment );
END_CLASS_RTTI();

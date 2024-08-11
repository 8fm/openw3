/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
					 
/// AI action performer attachment
class CAIActionPerformerAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CAIActionPerformerAttachment, IAttachment, 0 );

public:
	CAIActionPerformerAttachment();

	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );
};

BEGIN_CLASS_RTTI( CAIActionPerformerAttachment )
	PARENT_CLASS( IAttachment );
END_CLASS_RTTI();

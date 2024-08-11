/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once


#include "..\..\common\engine\attachment.h"





///
/// This is an attachment used to relate states of entities (eg. walk / run / jump) with specific cameras
/// @author M.Sobiecki
/// @created 2014-01-17
///
class CR6CameraAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CR6CameraAttachment, IAttachment, 0 );

public:
	// init attachment (returns true if we can hook up parent & child)
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info ) override;
};

BEGIN_CLASS_RTTI( CR6CameraAttachment );
	PARENT_CLASS( IAttachment );
END_CLASS_RTTI();

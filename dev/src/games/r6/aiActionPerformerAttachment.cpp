/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "aiActionPerformerAttachment.h"
#include "aiTreeComponent.h"

IMPLEMENT_ENGINE_CLASS( CAIActionPerformerAttachment );

CAIActionPerformerAttachment::CAIActionPerformerAttachment()
{
}

Bool CAIActionPerformerAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// Test if parent/child are valid
	if ( nullptr == parent || nullptr == child || !parent->IsA< CAITreeComponent > () )
	{
		return false;
	}

	// child is required to be a component
	CComponent* childComponent = Cast< CComponent > ( child );
	if ( nullptr == childComponent )
	{
		return false;
	}

	// get the r6 ai system
	CR6AISystem* system = GCommonGame->GetSystem< CR6AISystem > ();
	if ( nullptr == system )
	{
		return false;
	}

	// initialize base attachment
	return TBaseClass::Init( parent, child, info );
}

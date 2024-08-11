/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodePCReleaseControl.h"

#include "../../common/game/behTreeInstance.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodePCReleaseControlDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodePCReleaseControlInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodePCReleaseControlInstance::Activate()
{
	m_owner->GetActor()->SetIsAIControlled( false );

	return Super::Activate();
}
void CBehTreeNodePCReleaseControlInstance::Deactivate()
{
	m_owner->GetActor()->SetIsAIControlled( true );

	Super::Deactivate();
}
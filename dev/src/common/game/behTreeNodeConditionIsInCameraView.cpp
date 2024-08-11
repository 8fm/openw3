/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionIsInCameraView.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsInCameraViewDefinition )


Bool CBehTreeNodeConditionIsInCameraViewInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	return actor->WasVisibleLastFrame();
}
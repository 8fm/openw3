/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/game/behTreeRttiImplementation.h"

#include "behTreeNodeDebugLog.h"
#include "behTreeNodeAIAction.h"
#include "behTreeNodeInputDecorator.h"
#include "behTreeNodeSelectTargetByTagR6.h"
#include "behTreeNodeSequenceR6.h"
#include "behTreeNodePlayerStateSelector.h"
#include "behTreeConditionIsPlayer.h"
#include "behTreeNodePlayerState.h"
#include "behTreeNodePlayerStateIdle.h"
#include "behTreeNodePlayerStateWalk.h"
#include "behTreeNodePlayerStateJump.h"
#include "behTreeNodePlayerChooseInteraction.h"

IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDebugLogDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAIActionDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeInputDecoratorDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDecoratorR6Definition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSequenceCheckAvailabilityR6Definition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayerStateSelectorDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsPlayerDefinition )
IMPLEMENT_BEHTREE_NODE( IBehTreeNodePlayerStateDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayerStateIdleDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayerStateWalkDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayerStateJumpDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayerChooseInteractionDefinition )

void TouchR6BehTreeRTTIFile_CompilerHack()
{
	Int32 foo = 1;
}
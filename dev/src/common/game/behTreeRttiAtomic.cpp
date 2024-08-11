/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeRttiImplementation.h"

#include "behTreeNodeAtomicAction.h"
#include "behTreeNodeAtomicDamageReaction.h"
#include "behTreeNodeAtomicIdle.h"
#include "behTreeNodeAtomicPlayAnimation.h"
#include "behTreeNodeAtomicPlayAnimationEvent.h"
#include "behTreeNodeAtomicTeleport.h"
#include "behTreeNodeAttachToCurve.h"
#include "behTreeNodeDespawn.h"
#include "behTreeNodeFinishAnimations.h"


IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicActionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicDamageReactionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicIdleDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCompleteImmediatelyDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationEventDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicTeleportDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeTeleportToActionTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeTeleportToActionTargetCheckPositionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAttachToCurveDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFlyOnCurveDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDespawnDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFinishAnimationsDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBreakAnimationsDefinition );

/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeRttiImplementation.h"

#include "behTreeNodeCasualMovementDecorator.h"
#include "behTreeNodeConditionIsAtWork.h"
#include "behTreeNodeSelectPatrolingTarget.h"
#include "behTreeNodeSelectWanderingTarget.h"
#include "behTreeNodePerformWork.h"
#include "behTreeNodeSelectActionPoint.h"
#include "behTreeNodeWorkAtomic.h"
#include "behTreeWorkInitializer.h"
#include "behTreeNodeIdleRoot.h"
#include "behTreeNodeWorkDecorator.h"
#include "behTreeNodePartyWorkSynchronizer.h"

IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCasualMovementDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectPatrolingTargetDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectWanderingTargetDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRandomWanderingTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeHistoryWanderingTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDynamicWanderingTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePerformWorkDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeWorkDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCustomWorkDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectActionPointDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE(CBehTreeNodePartyWorkSynchronizerDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicTeleportToActionPointDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToActionPointDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionTeleportToWorkDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAlreadyAtWorkDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeMetanodeWorkInitializer );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBaseIdleDynamicRootDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeIdleDynamicRootDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeWorkRelatedConditionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeIsAtWorkConditionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCanUseChatSceneConditionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeIsSittingInInteriorConditionDefinition );

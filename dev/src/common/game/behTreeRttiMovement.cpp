/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeRttiImplementation.h"

#include "behTreeAtomicSteerWithTarget.h"
#include "behTreeDecoratorSteeringGraph.h"
#include "behTreeDecoratorSteeringTargeter.h"
#include "behTreeNodeAtomicGuardRetreat.h"
#include "behTreeNodeAtomicMove.h"
#include "behTreeNodeAtomicMoveToWanderpoint.h"
#include "behTreeNodeAtomicRotate.h"
#include "behTreeNodeAtomicPursue.h"
#include "behTreeNodeCustomSteering.h"
#include "behTreeNodeFleeReaction.h"
#include "behTreeNodeFollowFormation.h"
#include "behTreeNodeKeepDistance.h"
#include "behTreeNodePassMetaobstacles.h"
#include "behTreeNodePredefinedPath.h"
#include "behTreeNodeExplorationQueue.h"
#include "behTreeNodeSnapToNavigation.h"
#include "behTreeNodeSetupCustomMoveData.h"
#include "behTreeNodeUseExplorationAction.h"

IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorSteeringGraphDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeAtomicSteerWithTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeAtomicSteerWithCustomTargetDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDecoratorSteeringTargeterDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorGuardRetreatDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToActionTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToWanderpointDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToCombatTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToCustomPointDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBaseRotateToTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicRotateToTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBaseAtomicPursueTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicPursueTargetDefinition);
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFleeReactionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMatchActionTargetRotationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCustomSteeringDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFollowFormationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatFollowFormationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeKeepDistanceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePredefinedPathDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorPassMetaobstaclesDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePredefinedPathWithCompanionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicMoveToPredefinedPathDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeExplorationQueueDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeExplorationQueueRegisterDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeExplorationQueueUseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSnapToNavigationDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeSetupCustomMoveDataDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSetupCustomMoveTargetToPositionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeNotifyDoorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeUseExplorationActionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeTeleportToMetalinkDestinationDefinition );



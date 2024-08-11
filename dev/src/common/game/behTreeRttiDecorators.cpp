/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeRttiImplementation.h"

#include "behTreeDecorator.h"
#include "behTreeDecoratorActivePriority.h"
#include "behTreeDecoratorAsyncQuery.h"
#include "behTreeDecoratorCompleteInProximity.h"
#include "behTreeDecoratorDelay.h"
#include "behTreeDecoratorDuration.h"
#include "behTreeDecoratorLeadFormation.h"
#include "behTreeDecoratorSelectFleeDestination.h"
#include "behTreeDecoratorSetupFormation.h"
#include "behTreeDecoratorUninterruptable.h"
#include "behTreeDecoratorPoke.h"
#include "behTreeDecoratorSemaphore.h"
#include "behTreeDecoratorSemaphorePriority.h"
#include "behTreeDecoratorWalkablePositionQuery.h"
#include "behTreeNodeLoop.h"
#include "behTreeNodeSelectPartyMember.h"
#include "behTreeNodeSelectTargetByTag.h"
#include "behTreeNodeSelectTargetByTagInArea.h"
#include "behTreeNodeSelectFormationLeader.h"
#include "behTreeNodeCondition.h"
#include "behTreeNodeComplexCondition.h"
#include "behTreeNodeConditionChance.h"
#include "behTreeNodeConditionReactionEvent.h"
#include "behTreeNodeConditionDistanceToTarget.h"
#include "behTreeNodeConditionExternalToggle.h"
#include "behTreeNodeConditionLineofSight.h"
#include "behTreeNodeConditionCheckRotationToTarget.h"
#include "behTreeNodeConditionIsInAttackRange.h"
#include "behTreeNodeConditionIsInCameraView.h"
#include "behTreeNodeConditionIsInGuardArea.h"
#include "behTreeNodeConditionClearLineToTarget.h"
#include "behTreeNodeConditionHasTarget.h"
#include "behTreeNodeConditionCounter.h"
#include "behTreeNodeConditionPartyMembersCount.h"
#include "behTreeNodeConditionRecentEvent.h"
#include "behTreeNodeConditionSpeedEngineVal.h"
#include "behTreeNodeConditionTrue.h"
#include "behTreeNodeSelectRandomPositionInAreaDecorator.h"
#include "sceneDefinitionNode.h"
#include "reactionSceneAssignActorNode.h"
#include "reactionSceneFlowController.h"
#include "reactionSceneFlowSynchronizationDecorator.h"
#include "reactionSceneAssignments.h"
#include "reactionSceneEndNode.h"
#include "isConsciousAtWork.h"
#include "behTreeNodeConditionNoticedObject.h"
#include "behTreeDecoratorAfraidDefinition.h"
#include "behTreeConditionRespondToMusicDefinition.h"

IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeLoopDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeLoopWithTimelimitDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorSelectFleeDestinationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSetupFormationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorLeadFormationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorUninterruptableDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectPartyMemberDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectFormationLeaderDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition )
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorActivePriorityDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDecoratorAsyncQueryDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDecoratorAsyncResultDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeDecoratorWalkableSpotQueryDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotResultDefintion );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorCompleteInProximityDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeActivationDelayDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDelayActivationDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDurationDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDurationRangeDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePokeDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorSemaphoreDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeComplexConditionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionChanceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionReactionEventDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDistanceToActionTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDistanceToNamedTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionExternalToggleDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionLineofSightDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionTargetNoticedDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCombatTargetNoticedDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionLineofSightToNamedTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDistanceToTaggedDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDistanceToCombatTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeConditionRespondToMusicDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionDistanceToCustomTargetDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionCheckRotationDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToCombatTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsConsciousAtWorkDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsWorkingDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToActionTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToNamedTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsInAttackRangeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsInCameraViewDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionIsInGuardAreaDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsThisActorInGuardAreaDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionClearLineToTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionHasCombatTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCounterDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionCounterNewDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionPartyMembersCountDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionWasEventFiredRecentlyDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionSpeedEngineValDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionTrueDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorAfraidDefinition );


//reaction scenes
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSceneDefinitionDecorator );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSceneAssignActorNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSceneFlowControllerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSFlowSynchroDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSceneAssignmentsDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReactionSceneEndNodeDefinition );
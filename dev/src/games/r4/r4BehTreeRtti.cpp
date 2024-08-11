#include "build.h"
#include "../../common/game/behTreeRttiImplementation.h"

#include "behTreeNodeVitalSpotActive.h"
#include "behTreeConditionAttackersCount.h"
#include "behTreeConditionIsDead.h"
#include "behTreeConditionIsEnemyAround.h"
#include "behTreeDecoratorChangeBehaviorGraph.h"
#include "behTreeDecoratorSetBehaviourVariable.h"
#include "behTreeNodeConditionSpeech.h"
#include "behTreeNodeConditionHasAbility.h"
#include "behTreeNodeDecoratorFindLandingSpot.h"
#include "behTreeNodeDisableTalkInteraction.h"
#include "behTreeNodeDynamicFlightIdle.h"
#include "behTreeNodeSpeech.h"
#include "behTreeNodeTicketDecoratorBase.h"
#include "behTreeNodeTicketManager.h"
#include "behTreeNodeTicketGet.h"
#include "behTreeNodeTicketHas.h"
#include "behTreeNodeTicketRequest.h"
#include "behTreeNodeTicketRelease.h"
#include "behTreeNodeTicketLock.h"
#include "behTreeNodeClosestNonFriendlyTargetSelection.h"
#include "behTreeNodeCombatTargetSelection.h"
#include "behTreeNodeCombatTargetSelectionBase.h"
#include "behTreeNodeDecoratorBruxaDeath.h"
#include "behTreeNodeDecoratorHLCombatWildHorse.h"
#include "behTreeNodeHLCombatLock.h"
#include "behTreeNodeStrafing.h"
#include "behTreeDecoratorHLCombat.h"
#include "behTreeAtomicForgetCombatTarget.h"
#include "behTreeNodePCLockControl.h"
#include "behTreeNodePCReleaseControl.h"
#include "behTreeNodeRequestItems.h"
#include "behTreeRiderSpecific.h"
#include "behTreeRidingManager.h"
#include "behTreeNodeRiderIdleRoot.h"
#include "behTreeNodeRiderForced.h"
#include "behTreeDecoratorRiderWaitHorseScriptedAction.h"
#include "behTreeDecoratorCluePath.h"
#include "behTreeNodePredefinedPathRubberBand.h"
#include "behTreeNodeAtomicFlight.h"
#include "behTreeNodeAtomicFlyAround.h"
#include "behTreeNodeAtomicFlyTo.h"
#include "behTreeNodeDecoratorFlight.h"
#include "behTreeNodeDecoratorGlide.h"
#include "behTreeDecoratorHorseSpeedManager.h"
#include "behTreeNodeFlee.h"
#include "behTreeDecoratorRiderHorseReachability.h"
#include "behTreeNodeBroadcastReactionScene.h"
#include "behTreeSailorFollowPath.h"
#include "behTreeSailorMoveTo.h" 
#include "behTreeNodeConditionIsInInterior.h"
#include "behTreeNodeConditionIsTargetThePlayer.h"
#include "behTreeNodeConditionShouldPursueCombatTarget.h"
#include "behTreeNodeDynamicCombatStyle.h"
#include "behTreeMetanodeOnCombatstyle.h"
#include "behTreeMetanodeSetupReachability.h"
#include "behTreeFindDestinyItemStoreDecorator.h"
#include "behTreeFindSourceItemStoreDecorator.h"
#include "behTreeCarryingItemBaseNode.h"
#include "behTreeCarryingItemPick.h"
#include "behTreeCarryingItemDrop.h"
#include "behTreeCarryingItemBaseDecorator.h"
#include "behTreeNodeSelectTargetOrMountByTag.h"
#include "r4BehTreeNodePredefinedPath.h"
#include "behTreeNodeGreetingReactionScene.h"

IMPLEMENT_BEHTREE_NODE( CBehTreeNodeVitalSpotActiveDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeStrafingDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionAttackersCountDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionIsInInteriorBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionAmIInInteriorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsPlayerInInteriorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionAmIOrAPInInteriorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsTargetThePlayerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsDeadDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsEnemyAroundDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorChangeBehaviorGraphDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorSetBehaviorVariableDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeLandingDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeTakeOffDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFindLandingSpotDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorDisableTalkInteractionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorOverrideBehaviorVariableDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeCombatTicketDecoratorBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketManagerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketManagedGetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketHasDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketRequestDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketReleaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTicketLockDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeCombatTargetSelectionBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCombatTargetSelectionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeHLOffenceBaseDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeHLCombatBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeHLCombatDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeHLAnimalCombatDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorBruxaDeathDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeDecoratorBaseHLDangerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorHLDangerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorHLDangerTamableDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorHLWildHorseDangerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeProlongHLCombatDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeAtomicForgetCombatTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePCLockControlDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePCReleaseControlDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicSailorFollowPathDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicSailorMoveToPathDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicSailorMoveToDefinition );
IMPLEMENT_BEHTREE_NODE( CR4BehTreeNodePredefinedPathWithCompanionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRiderPursueHorseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRiderRotateToHorseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorRidingManagerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorRidingCheckDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRiderIdleDynamicRootDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRiderForcedBehaviorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorDismountCheckDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRequestItemsDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeRiderWaitHorseScriptedActionDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorRiderPairingLogicDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorRiderPairWithHorseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorHorsePairWithRiderDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorCluePathDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePredefinedPathRuberBandDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeAtomicFlightDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeAtomicFlyAroundBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicFlyAroundDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicFlyAroundPositionDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeAtomicFlyToBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicFlyToDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicFlyToPositionDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeFlightDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorGlideDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorHorseSpeedManagerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBroadcastReactionSceneDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionShouldPursueCombatTargetDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDynamicCombatStyleDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSendScaredEventDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeReceiveScaredEventDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionSpeechDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionHasAbilityDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsInChatSceneDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsSpeakingDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionHasVoicesetDefintion );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeSpeechDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayVoicesetDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorRiderHorseReachabilityDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeMetanodeSetupCombatReachability );
IMPLEMENT_BEHTREE_NODE( CBehTreeMetanodeDecorateOnCombatstyle );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorCarryingItemManagerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorCarryingItemsBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionIsCarryingItemDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorFindSourceItemStoreDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDecoratorFindDestinationItemStoreDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeCarryingItemBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePickItemDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDropItemDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeGreetingReactionSceneDecoratorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeFlightIdleDynamicRootDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeNotifyCombatActivationDecoratorDefinition );

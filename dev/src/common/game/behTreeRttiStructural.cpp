/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeRttiImplementation.h"

#include "behTreeDynamicNode.h"
#include "behTreeMetanode.h"
#include "behTreeMetanodeOnSpawn.h"
#include "behTreeNodeForced.h"
#include "behTreeNodeSpecial.h"
#include "behTreeNodeTemplate.h"
#include "behTreeNodeVoid.h"
#include "behTreeNodeComment.h"
#include "behTreeMetaNodeNpcTypeCondition.h"



IMPLEMENT_BEHTREE_NODE( IBehTreeDynamicNodeBaseDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeDynamicNodeDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeSpecialDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalTreeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBaseConditionalTreeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalTreeNPCTypeDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeNodeConditionalBaseNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalNameNodeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalFlagTreeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeConditionalChooseBranchDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSubtreeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSubtreeListDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeTemplateDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeMetanodeDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeVoidDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeCommentDefinition );
IMPLEMENT_BEHTREE_NODE( IBehTreeMetanodeOnSpawnDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeMetanodeScriptOnSpawnDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeBaseForcedBehaviorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeForcedBehaviorDefinition );

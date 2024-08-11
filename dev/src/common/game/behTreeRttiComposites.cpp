/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeRttiImplementation.h"

#include "behTreeNodeComposite.h"
#include "behTreeNodeSequence.h"
#include "behTreeNodeRandom.h"
#include "behTreeNodeSelector.h"
#include "behTreeNodeParallel.h"

IMPLEMENT_BEHTREE_NODE( IBehTreeNodeCompositeDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeParallelDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeProbabilitySelectorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeChoiceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeEvaluatingChoiceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSelectorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeEvaluatingSelectorDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSequenceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePersistantSequenceDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSequenceCheckAvailabilityDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeSequenceFowardAndBackDefinition );


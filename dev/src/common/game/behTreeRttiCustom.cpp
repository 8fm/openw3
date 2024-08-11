/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeRttiImplementation.h"

#include "behTreeMetanodeGuardSetup.h"
#include "behTreeNodeLookat.h"
#include "behTreeNodePlayScene.h"
#include "behTreeNodeQuestActions.h"
#include "behTreeNodeAtomicPlayAnimationManualMotionExtraction.h"

IMPLEMENT_BEHTREE_NODE( CBehTreeMetanodeSetupGuardDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeDecoratorLookAtDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicLookAtDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlaySceneDefinition );
//IMPLEMENT_BEHTREE_NODE( CBehTreeNodePlaySceneIntroDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeExternalListenerDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeScriptedActionsListReaderDefinition );
IMPLEMENT_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition );

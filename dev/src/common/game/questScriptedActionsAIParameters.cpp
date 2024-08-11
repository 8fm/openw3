#include "build.h"
#include "questScriptedActionsAIParameters.h"
#include "aiActionParameters.h"
#include "questScriptedActionsBlock.h"

IMPLEMENT_ENGINE_CLASS( CAIQuestScriptedActionsTree );

void CAIQuestScriptedActionsTree::InitializeData( IAITree* ai )
{
	TBaseClass::InitializeData();
	m_actionTree = ai;
}

CName CAIQuestScriptedActionsTree::ListenerParamName()
{
	return CNAME( listener );
}
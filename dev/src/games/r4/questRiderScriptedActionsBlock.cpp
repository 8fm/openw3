#include "build.h"
#include "questRiderScriptedActionsBlock.h"
#include "../../common/game/questScriptedActionsAIParameters.h"


///////////////////////////////////////////////////////////////////////////////
// CQuestRiderScriptedActionsBlock
IMPLEMENT_ENGINE_CLASS( CQuestRiderScriptedActionsBlock );

CQuestRiderScriptedActionsBlock::CQuestRiderScriptedActionsBlock()
	: CBaseQuestScriptedActionsBlock() 
{
	m_name = TXT("Rider Scripted actions");
}



CAIQuestScriptedActionsTree* CQuestRiderScriptedActionsBlock::ComputeAIActions() const
{
	if ( !m_forcedAction )
	{
		if ( m_ai )
		{
			CAIQuestScriptedActionsTree* tree = CAIQuestScriptedActionsTree::GetStaticClass()->CreateObject< CAIQuestScriptedActionsTree >();
			tree->InitializeTree();
			tree->InitializeData( m_ai );
			m_forcedAction = tree;

			tree->InitializeAIParameters();

			return tree;
		}
		else
		{
			return nullptr;
		}
	}
	return m_forcedAction.Get();
}
#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CQuestRiderScriptedActionsBlock::GetClientColor() const
{
	return Color( 122, 30, 27 );
}

#endif
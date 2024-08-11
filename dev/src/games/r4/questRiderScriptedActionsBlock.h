#pragma once

#include "../../common/game/questScriptedActionsBlock.h"
#include "r4AiParameters.h"

///////////////////////////////////////////////////////////////////////////
// CQuestRiderScriptedActionsBlock
class CQuestRiderScriptedActionsBlock : public CBaseQuestScriptedActionsBlock
{
	DECLARE_ENGINE_CLASS( CQuestRiderScriptedActionsBlock, CBaseQuestScriptedActionsBlock, 0 );
	typedef CBaseQuestScriptedActionsBlock Super;
	
	THandle< IRiderActionTree >								m_ai;
public:
	CQuestRiderScriptedActionsBlock();
private:
	CAIQuestScriptedActionsTree*		ComputeAIActions() const override;
	CName								GetCancelEventName() const override	{ return CNAME( AI_Rider_Forced_Cancel ); }
	CName								GetEventName() const override		{ return CNAME( AI_Rider_Load_Forced ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	Color GetClientColor() const override;
#endif
};

BEGIN_CLASS_RTTI( CQuestRiderScriptedActionsBlock );
	PARENT_CLASS( CBaseQuestScriptedActionsBlock );
	PROPERTY_INLINED( m_ai, TXT( "Ai tree representing scripted behavior" ) );
END_CLASS_RTTI();

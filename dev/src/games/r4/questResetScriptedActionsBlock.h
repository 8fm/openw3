#pragma once

#include "../../common/game/questGraphBlock.h"


///////////////////////////////////////////////////////////////////////////////
// CQuestResetScriptedActionsBlock
///////////////////////////////////////////////////////////////////////////////
class CQuestResetScriptedActionsBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestResetScriptedActionsBlock, CQuestGraphBlock, 0 );
	typedef CQuestGraphBlock Super;

protected:
	CName											m_npcTag;
	Bool											m_onlyOneActor;

#ifndef NO_EDITOR_GRAPH_SUPPORT

private:
	//! CGraphBlock interface
	virtual void				OnRebuildSockets() override;
	virtual EGraphBlockShape	GetBlockShape() const override;
	virtual Color				GetClientColor() const override;
	virtual String				GetBlockCategory() const override;
	virtual Bool				CanBeAddedToGraph( const CQuestGraph* graph ) const override;

#endif // NO_EDITOR_GRAPH_SUPPORT

public:
	CQuestResetScriptedActionsBlock();

	virtual Bool OnProcessActivation( InstanceBuffer& data ) const override;

private:

};

BEGIN_CLASS_RTTI( CQuestResetScriptedActionsBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_EDIT( m_npcTag, TXT( "Who should reset actions?" ) )
	PROPERTY_EDIT( m_onlyOneActor, TXT("Set to false if you want all actors under this tag to be affected") );
END_CLASS_RTTI();

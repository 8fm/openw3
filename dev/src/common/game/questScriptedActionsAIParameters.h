#pragma once

#include "aiParameters.h"
#include "behTreeNodeQuestActions.h"

class CAIQuestScriptedActionsTree : public IAITree
{
	typedef IAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIQuestScriptedActionsTree );

protected:
	THandle< IAITree >						m_actionTree;
	SBehTreeExternalListenerPtr				m_listener;

public:
	CAIQuestScriptedActionsTree()																		{}

	void									InitializeData( IAITree* ai );
	void									SetListenerParam( CBehTreeExternalListener* listener )		{ m_listener.m_ptr = listener; }
	void									ClearListenerParam()										{ m_listener.m_ptr = nullptr; }

	static CName							ListenerParamName();
};

BEGIN_CLASS_RTTI( CAIQuestScriptedActionsTree )
	PARENT_CLASS( IAITree )
	PROPERTY_EDIT( m_actionTree, TXT("Internal") )
	PROPERTY_EDIT( m_listener, TXT("Internal") )
END_CLASS_RTTI();



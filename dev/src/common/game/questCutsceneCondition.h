#pragma once

#include "questCondition.h"

class CQuestCutsceneCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestCutsceneCondition, IQuestCondition, 0 )

private:
	String	m_cutsceneName;
	CName	m_event;

	Bool	m_isFulfilled;

public:
	void OnEvent( const String& csName, const CName& csEvent );

protected:
	// ------------------------------------------------------------------------
	// IQuestCondition implementation
	// ------------------------------------------------------------------------
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestCutsceneCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_cutsceneName, TXT("Name of the cutscene that should trigger the event."))
	PROPERTY_EDIT( m_event, TXT("Name of the event we're waiting for.") )
END_CLASS_RTTI()

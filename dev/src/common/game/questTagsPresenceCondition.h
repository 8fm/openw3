#pragma once

#include "questCondition.h"
#include "../../common/engine/globalEventsManager.h"

class CQuestTagsPresenceCondition : public IQuestCondition, public IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestTagsPresenceCondition, IQuestCondition, 0 )

private:
	TagList			m_tags;
	Bool			m_all;
	Uint32			m_howMany;
	Bool			m_includeStubs;

	Bool			m_isFulfilled;
	Bool			m_wasRegistered;

public:
	CQuestTagsPresenceCondition();
	virtual ~CQuestTagsPresenceCondition();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Tags: %s, All: %s, How Many: %d" ),
			m_tags.ToString().AsChar(), ( m_all ) ? TXT( "true" ) : TXT( "false" ), m_howMany ); 
	}
#endif

protected:

	// ------------------------------------------------------------------------
	// IQuestCondition implementation
	// ------------------------------------------------------------------------
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	Bool CheckCondition();
	Bool RegisterCallback( Bool reg );

	// IGlobalEventsListener
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;
};

BEGIN_CLASS_RTTI( CQuestTagsPresenceCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_tags,	TXT( "Tag we're looking for." ), TXT( "TagListEditor" ) )
	PROPERTY_EDIT( m_all,			TXT( "Should all tags be preset, or will any one of them do?" ) )
	PROPERTY_EDIT( m_howMany,		TXT( "How many tags should be present?" ) )
	PROPERTY_EDIT( m_includeStubs,	TXT( "Include stubbed npcs in results" ) )
END_CLASS_RTTI()

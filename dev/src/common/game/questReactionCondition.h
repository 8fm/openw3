#pragma once

#include "questCondition.h"

class CQuestReactionCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestReactionCondition, IQuestCondition, 0 )

private:
	CName				m_fieldName;		//!< The name of interaction
	TagList			m_actorsTags;

	Bool				m_isFulfilled;

public:
	void OnReaction( CNewNPC* npc, CInterestPointInstance* interestPoint );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestReactionCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_fieldName, TXT("The name of potential field that initaited the reaction."), TXT( "ReactionFieldEditor" ) )
	PROPERTY_CUSTOM_EDIT( m_actorsTags, TXT( "Tags of the reacting actors" ), TXT( "TagListEditor" ) );
END_CLASS_RTTI()

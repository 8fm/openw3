#pragma once

#include "questCondition.h"

class CQuestInteractionCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestInteractionCondition, IQuestCondition, 0 )

private:
	String			m_interactionName;		//!< The name of interaction
	TagList			m_ownerTags;

	Bool			m_isFulfilled;

public:
	void OnInteraction( const String& eventName, CEntity* owner );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestInteractionCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_interactionName, TXT("The name of interaction.") )
	PROPERTY_CUSTOM_EDIT( m_ownerTags, TXT( "Entities tags" ), TXT( "TagListEditor" ) );
END_CLASS_RTTI()

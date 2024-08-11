#include "build.h"
#include "questInteractionCondition.h"
#include "questsSystem.h"

IMPLEMENT_ENGINE_CLASS( CQuestInteractionCondition )

void CQuestInteractionCondition::OnInteraction( const String& eventName, CEntity* owner )
{
	if ( m_isFulfilled || !owner)
	{
		return;
	}

	m_isFulfilled = ( m_interactionName == eventName ) &&
		( TagList::MatchAny( m_ownerTags, owner->GetTags() ) );
}

void CQuestInteractionCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_isFulfilled = false;
	GCommonGame->GetSystem< CQuestsSystem >()->AttachInteractionListener( *this );
}

void CQuestInteractionCondition::OnDeactivate()
{
	TBaseClass::OnDeactivate();

	GCommonGame->GetSystem< CQuestsSystem >()->DetachInteractionListener( *this );
}

Bool CQuestInteractionCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}

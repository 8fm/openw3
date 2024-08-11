#include "build.h"
#include "questReactionCondition.h"
#include "questsSystem.h"
#include "../game/interestPointComponent.h"

IMPLEMENT_ENGINE_CLASS( CQuestReactionCondition )

void CQuestReactionCondition::OnReaction( CNewNPC* npc, CInterestPointInstance* interestPoint )
{
	if ( m_isFulfilled || !npc || !interestPoint )
	{
		return;
	}

	Bool doTagsMatch = TagList::MatchAny( m_actorsTags, npc->GetTags() );
	m_isFulfilled = ( m_fieldName == interestPoint->GetName() ) && doTagsMatch;
}

void CQuestReactionCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_isFulfilled = false;
	GCommonGame->GetSystem< CQuestsSystem >()->AttachReactionListener( *this );
}

void CQuestReactionCondition::OnDeactivate()
{
	TBaseClass::OnDeactivate();

	GCommonGame->GetSystem< CQuestsSystem >()->DetachReactionListener( *this );
}

Bool CQuestReactionCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}

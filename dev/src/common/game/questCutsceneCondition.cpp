#include "build.h"
#include "questCutsceneCondition.h"
#include "questsSystem.h"

IMPLEMENT_ENGINE_CLASS( CQuestCutsceneCondition )

void CQuestCutsceneCondition::OnEvent( const String& csName, const CName& csEvent )
{
	m_isFulfilled = ( csName == m_cutsceneName ) && ( csEvent == m_event );
}

void CQuestCutsceneCondition::OnActivate()
{
	m_isFulfilled = false;
	if ( GCommonGame && GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->AttachCutsceneListener( *this );
	}
}

void CQuestCutsceneCondition::OnDeactivate()
{
	if ( GCommonGame && GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->DetachCutsceneListener( *this );
	}
}

Bool CQuestCutsceneCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}

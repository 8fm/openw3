#include "build.h"
#include "questLoadingSCreenCondition.h"

IMPLEMENT_ENGINE_CLASS( CQuestLoadingScreenCondition )

CQuestLoadingScreenCondition::CQuestLoadingScreenCondition()
	: m_isShown( false )
{
}

CQuestLoadingScreenCondition::~CQuestLoadingScreenCondition()
{
}

Bool CQuestLoadingScreenCondition::OnIsFulfilled()
{
	return m_isShown == GCommonGame->IsLoadingScreenShown();
}

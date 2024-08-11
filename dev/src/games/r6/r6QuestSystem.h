/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questsSystem.h"

class CR6QuestSystem : public CQuestsSystem
{
	DECLARE_ENGINE_CLASS( CR6QuestSystem, CQuestsSystem, 0 );

public:
	CR6QuestSystem();
	~CR6QuestSystem();

	ASSIGN_GAME_SYSTEM_ID( GS_QuestsSystem )

	Bool ActivateQuest( const CQuestGraphR6QuestBlock& r6QuestBlock );
	void StartQuest( const CQuestGraphR6QuestBlock& r6QuestBlock, InstanceBuffer& instanceData );
	void DeactivateQuest( const CQuestGraphR6QuestBlock& r6QuestBlock, InstanceBuffer& instanceData );

private:
	TDynArray< const CQuestGraphR6QuestBlock* > m_activeR6Quests;
	Bool								m_isMainQuestActive;
};

BEGIN_CLASS_RTTI( CR6QuestSystem );
	PARENT_CLASS( CQuestsSystem );
END_CLASS_RTTI();
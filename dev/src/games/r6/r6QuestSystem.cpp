/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r6QuestSystem.h"
#include "questGraphR6QuestBlock.h"

IMPLEMENT_ENGINE_CLASS( CR6QuestSystem );

CR6QuestSystem::CR6QuestSystem()
	: CQuestsSystem()
	, m_isMainQuestActive( false )
{
}

CR6QuestSystem::~CR6QuestSystem()
{
}

Bool CR6QuestSystem::ActivateQuest( const CQuestGraphR6QuestBlock& r6QuestBlock )
{
	m_activeR6Quests.PushBack( &r6QuestBlock );

	if ( r6QuestBlock.GetType() == QT_Main && m_isMainQuestActive )
	{
		return false;
	}
	return true;
}

void CR6QuestSystem::DeactivateQuest( const CQuestGraphR6QuestBlock& r6QuestBlock, InstanceBuffer& instanceData )
{
	VERIFY(m_activeR6Quests.Remove( &r6QuestBlock ));

	if ( r6QuestBlock.GetType() == QT_Main )
	{
		RED_ASSERT( m_isMainQuestActive );
		m_isMainQuestActive = false;

		// Unblock all main quests
		for ( auto it = m_activeR6Quests.Begin(); it != m_activeR6Quests.End(); ++it )
		{
			const CQuestGraphR6QuestBlock* itBlock = *it;
			if ( itBlock->GetType () == QT_Main )
			{
				itBlock->Unblock( instanceData );
			}
		}
	}
}

void CR6QuestSystem::StartQuest( const CQuestGraphR6QuestBlock& r6QuestBlock, InstanceBuffer& instanceData )
{
	if ( r6QuestBlock.GetType() == QT_Main )
	{
		RED_ASSERT( !m_isMainQuestActive );
		m_isMainQuestActive = true;

		// Block all others main quests
		for ( auto it = m_activeR6Quests.Begin(); it != m_activeR6Quests.End(); ++it )
		{
			const CQuestGraphR6QuestBlock* itBlock = *it;
			if ( itBlock->GetType () == QT_Main )
			{
				if ( &r6QuestBlock != itBlock )
				{
					itBlock->Block( instanceData );
				}
			}
		}
	}
}

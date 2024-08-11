/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4QuestDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4QuestDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4QuestDLCMounter();

	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameStarted() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

protected:
	void LoadVoicetagTable();
	void UnloadVoicetagTable();

	// quest definition to start
	THandle< CQuest >		m_quest;

	// "taint" fact - if this fact is in the FactDB we consider our savegame tainted with the quest
	CName					m_taintFact;

	// path to csv file with voice tags
	String					m_sceneVoiceTagsTableFilePath;
	Bool					m_sceneVoiceTagsTableLoaded;

	// path to csv file with quest levels
	String					m_questLevelsFilePath;

};

BEGIN_CLASS_RTTI( CR4QuestDLCMounter );
	PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT( m_quest, TXT("Quest to include in the DLC") );
	PROPERTY_EDIT( m_taintFact, TXT("If this fact is in the FactDB we consider our savegame tainted with the quest") );
	PROPERTY_EDIT( m_sceneVoiceTagsTableFilePath, TXT("Path to csv file with voice tags (e.g. dlc\\dlc3\\data\\gameplay\\globals\\scene_voice_tags.csv)") );
	PROPERTY_EDIT( m_questLevelsFilePath, TXT("Path to csv file with quest levels (e.g. dlc\\dlc3\\data\\gameplay\\globals\\quest_levels.csv)") );
END_CLASS_RTTI();

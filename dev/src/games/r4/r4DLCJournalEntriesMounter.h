/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/game/journalResource.h"

class CR4JournalEntriesDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4JournalEntriesDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4JournalEntriesDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameStarted() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void Activate();
	void Deactivate();

	TSoftHandle< CJournalInitialEntriesResource > m_journalEntriesFilePath;
};

BEGIN_CLASS_RTTI( CR4JournalEntriesDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_journalEntriesFilePath, TXT("Path to resource for DLC(e.g. \"dlc\\ep1\\data\\gameplay\\journal\\start_ep1.w2je\")") );
END_CLASS_RTTI();

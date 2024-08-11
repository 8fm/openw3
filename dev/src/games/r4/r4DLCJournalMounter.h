/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4JournalDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4JournalDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4JournalDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameStarted() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual void OnSerialize( class IFile& file ) override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void NormalizeJournalDirectoryPath();

private:
	void Activate();
	void Deactivate();

	String m_journalDirectoryPath;
};

BEGIN_CLASS_RTTI( CR4JournalDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_journalDirectoryPath, TXT("Path to DLC journal directry (in editor mode this mounter is optional, e.g. dlc\\dlc3\\journal).") );
END_CLASS_RTTI();

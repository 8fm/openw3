/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4VideoDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4VideoDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4VideoDLCMounter() {}

	//! IGameplayDLCMounter
public:
	// Do nothing. Mounter just to gather USM files.
	virtual bool OnCheckContentUsage() override { return false; }
	virtual void OnGameStarting() override {} 
	virtual void OnGameEnding() override {}

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	String  m_videoDirectoryPath;
};

BEGIN_CLASS_RTTI( CR4VideoDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT( m_videoDirectoryPath, TXT("Path to directory with video USM resources (e.g. \"dlc\\dlc1\\data\\movies\\\")") );
END_CLASS_RTTI();

#ifndef NO_EDITOR

RED_INLINE void CR4VideoDLCMounter::OnEditorStarted()
{}

RED_INLINE void CR4VideoDLCMounter::OnEditorStopped()
{}

#endif // !NO_EDITOR

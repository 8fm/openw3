/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4ScaleformContentDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4ScaleformContentDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4ScaleformContentDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void Activate();
	void Deactivate();

	String  m_scaleformDirectoryPath;
};

BEGIN_CLASS_RTTI( CR4ScaleformContentDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_scaleformDirectoryPath, TXT("Path to directory with Scaleform resources like *.png/*.dds/*.redswf (e.g. \"dlc\\dlc1\\data\\gameplay\\gui_new\\\")") );
END_CLASS_RTTI();

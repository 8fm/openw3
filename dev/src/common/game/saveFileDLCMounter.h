/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dlcMounter.h"

#ifndef NO_EDITOR
#	include "../core/analyzer.h"
#endif

/// DLC mounter for save files - for starting DLCs in standalone mode
class CSaveFileDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CSaveFileDLCMounter, IGameplayDLCMounter, 0 );

	String m_starterSaveFilePath;

public:
	CSaveFileDLCMounter();

	Bool IsValid() const;
	IFile* CreateStarterFileReader() const;

	// IGameplayDLCMounter interface
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;
	virtual bool OnCheckContentUsage() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR
};

BEGIN_CLASS_RTTI( CSaveFileDLCMounter );
	PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT( m_starterSaveFilePath, TXT("Path to .sav for starting this DLC in standalone mode (e.g. \"dlc\\ep1\\data\\gamepaly\\standalone\\starter.sav\")") );	
END_CLASS_RTTI();
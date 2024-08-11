/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4DefinitionsEntitieTemplatesDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4DefinitionsEntitieTemplatesDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4DefinitionsEntitieTemplatesDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void Activate();
	void Deactivate();

	String				m_entitieTemplatesDirectoryPath;
	Bool				m_directoryLoaded;
};

BEGIN_CLASS_RTTI( CR4DefinitionsEntitieTemplatesDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_entitieTemplatesDirectoryPath, TXT("Path to directory with entitie templates used in definition files for DLC (e.g. \"dlc\\dlc1\\data\\player\\armor\\armor__temerian\")") );
END_CLASS_RTTI();

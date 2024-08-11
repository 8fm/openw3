/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4ResourceDefinitionsDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4ResourceDefinitionsDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4ResourceDefinitionsDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	//! ISerializable
	virtual void OnPostLoad();

	//! CObject
	virtual void OnFinalize();

	//! IGameplayDLCMounter
	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void Activate();
	void Deactivate();

	String			   m_resourceDefinitionXmlFilePath;

	Red::System::GUID  m_creatorTag;
	Bool			   m_xmlFileLoaded;
};

BEGIN_CLASS_RTTI( CR4ResourceDefinitionsDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_resourceDefinitionXmlFilePath, TXT("Path to XMLs with resource definitions for DLC(e.g. \"dlc\\ep1\\data\\gamepaly\\globals\\resources\\gamepaly.xml\")") );
END_CLASS_RTTI();
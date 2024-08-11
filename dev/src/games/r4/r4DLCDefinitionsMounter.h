/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/game/definitionsManagerListener.h"

class CR4DefinitionsDLCMounter : public IGameplayDLCMounter, public CDefinitionsManagerListener
{
	DECLARE_ENGINE_CLASS( CR4DefinitionsDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4DefinitionsDLCMounter();

protected:
	String				m_definitionXmlFilePath;

private:
	Red::System::GUID	m_creatorTag;
	Bool				m_xmlFileLoaded;

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	//! ISerializable
	virtual void OnSerialize( class IFile& file ) override;

	//! IGameplayDLCMounter
	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

	//! CDefinitionsManagerListener
	virtual void OnDefinitionsReloaded() override;

protected:
	void LoadDefinitions();
	void UnloadDefinitions();

	virtual void Activate();
	virtual void Deactivate();

	virtual Bool ShouldLoad();
};

BEGIN_CLASS_RTTI( CR4DefinitionsDLCMounter );
	PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT( m_definitionXmlFilePath, TXT("Path to XML file or directory with definitions for DLC (e.g. \"dlc\\dlc1\\data\\gameplay\\itemsDLC1_items.xml\" or\"dlc\\dlc1\\data\\gameplay\\items\\\").") );
END_CLASS_RTTI();

/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "EntityMappinsResource.h"
#include "QuestMappinsResource.h"

class CR4MappinsDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4MappinsDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4MappinsDLCMounter();

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

	TSoftHandle< CWorld >					m_worldFilePath;
	TSoftHandle< CEntityMapPinsResource >	m_mappinsFilePath;
	TSoftHandle< CQuestMapPinsResource >	m_questMappinsFilePath;
};

BEGIN_CLASS_RTTI( CR4MappinsDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_worldFilePath, TXT( "World filepath" ) );
PROPERTY_EDIT( m_mappinsFilePath, TXT("Path to resource for DLC (in editor mode this mounter is optional, e.g. dlc\\ep1\\data\\gameplay\\journal\\start_ep1.w2je).") );
PROPERTY_EDIT( m_questMappinsFilePath, TXT("Path to resource for DLC (in editor mode this mounter is optional, e.g. dlc\\ep1\\data\\gameplay\\journal\\start_ep1.w2je).") );
END_CLASS_RTTI();

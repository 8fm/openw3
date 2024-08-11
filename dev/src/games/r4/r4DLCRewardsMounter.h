/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4RewardsDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4RewardsDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4RewardsDLCMounter();

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

	String	m_rewordsXmlFilePath;

	Bool				m_xmlFileLoaded;
	TDynArray< CName >	m_loadedRewardNames;
};

BEGIN_CLASS_RTTI( CR4RewardsDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_rewordsXmlFilePath, TXT("Path to XMLs with rewards for DLC(e.g. \"dlc\\dlc1\\data\\gameplay\\rewards\\rewards.xml\")") );
END_CLASS_RTTI();

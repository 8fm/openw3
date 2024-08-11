/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4AttitudesDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4AttitudesDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4AttitudesDLCMounter();

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

	String					m_attitudeGroupsTableFilePath;
	Bool					m_attitudeGroupsTableLoaded;

	String					m_attitudesXMLFilePath;
	Bool					m_attitudesXMLLoaded;
};

BEGIN_CLASS_RTTI( CR4AttitudesDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_attitudeGroupsTableFilePath, TXT("Path to csv file with attitude groups (e.g. dlc\\dlc3\\data\\gameplay\\globals\\attitude_groups.csv)") );
PROPERTY_EDIT( m_attitudesXMLFilePath, TXT("Path to XML file with attitude (e.g. dlc\\dlc3\\data\\gameplay\\globals\\attitudes.xml)") );
END_CLASS_RTTI();

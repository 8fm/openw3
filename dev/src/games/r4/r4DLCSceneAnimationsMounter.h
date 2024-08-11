/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4SceneAnimationsDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4SceneAnimationsDLCMounter, IGameplayDLCMounter, 0 );
public:
	CR4SceneAnimationsDLCMounter();

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

	String					m_sceneAnimationsBodyFilePath;
	String					m_sceneAnimationsMimicsFilePath;
	String					m_sceneAnimationsMimicsEmoStatesFilePath;
	unsigned char			m_sceneAnimationsTableLoaded;
};

BEGIN_CLASS_RTTI( CR4SceneAnimationsDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_sceneAnimationsBodyFilePath, TXT("Path to csv file with scene body animations (e.g. dlc\\dlc3\\data\\gameplay\\globals\\scene_body_animations.csv)") );
PROPERTY_EDIT( m_sceneAnimationsMimicsFilePath, TXT("Path to csv file with scene mimics animations (e.g. dlc\\dlc3\\data\\gameplay\\globals\\scene_mimics_animations.csv)") );
PROPERTY_EDIT( m_sceneAnimationsMimicsEmoStatesFilePath, TXT("Path to csv file with scene mimics emotional states (e.g. dlc\\dlc3\\data\\gameplay\\globals\\scene_mimics_emotional_states.csv)") );
END_CLASS_RTTI();

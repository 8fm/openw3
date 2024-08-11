/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4AnimationsCategoriesDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4AnimationsCategoriesDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4AnimationsCategoriesDLCMounter();

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

	String					m_animationsCategoriesTableFilePath;
	Bool					m_animationsCategoriesTableLoaded;
};

BEGIN_CLASS_RTTI( CR4AnimationsCategoriesDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_animationsCategoriesTableFilePath, TXT("Path to csv file with animations categories (e.g. dlc\\dlc3\\data\\gameplay\\globals\\animations.csv)") );
END_CLASS_RTTI();

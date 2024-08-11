/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4ActionPointCategoriesDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4ActionPointCategoriesDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4ActionPointCategoriesDLCMounter();

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

	String					m_actionPointCategoriesTableFilePath;
	Bool					m_actionPointCategoriesTableLoaded;
};

BEGIN_CLASS_RTTI( CR4ActionPointCategoriesDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_actionPointCategoriesTableFilePath, TXT("Path to csv file with action point categories (e.g. dlc\\dlc3\\data\\gameplay\\globals\\action_categories.csv)") );
END_CLASS_RTTI();
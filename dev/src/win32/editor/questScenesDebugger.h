/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questScenePlayer.h"
#include "questEdTool.h"


class CQuestScenesDebugger : public wxPanel,
	public IQuestEdTool,
	public IQuestScenesManagerListener
{

private:
	enum EMsgType
	{
		QSD_INFO,
		QSD_WARNING,
		QSD_ERROR
	};

private:
	wxListCtrl*				m_activeScenes;
	wxListCtrl*				m_scheduledScenes;
	wxListCtrl*				m_completedScenes;
	wxListBox*				m_lockedTags;

	CQuestScenesManager*	m_scenesMgr;
	CEdQuestEditor*		m_host;

public:
	CQuestScenesDebugger();
	~CQuestScenesDebugger();

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  ) {}
	virtual void OnGraphSet( CQuestGraph& graph ) {}
	virtual wxPanel* GetPanel();
	virtual String GetToolName() const { return TXT( "Scenes" ); }

	virtual void Notify( CQuestScenesManager& mgr );
	virtual void NotifySceneCompleted( CQuestScenePlayer& player, SceneCompletionReason reason );

protected:
	void OnClose( wxCloseEvent& event );
};

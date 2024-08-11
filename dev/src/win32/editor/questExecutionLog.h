/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questsSystem.h"
#include "../../common/game/questThread.h"
#include "questEdTool.h"


class CQuestExecutionLog :	public wxPanel,
									public IQuestEdTool,
									public IQuestSystemListener
{
private:
	enum Type
	{
		QEL_INFO,
		QEL_WARNING,
		QEL_ERROR
	};

private:
	CQuestsSystem*			m_system;
	
	wxListCtrl*				m_log;
	wxBitmapButton*			m_findButton;
	wxTextCtrl*				m_searchPhrase;

	CEdQuestEditor*			m_host;

public:
	CQuestExecutionLog();
	~CQuestExecutionLog();

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  ) {}
	virtual void OnGraphSet( CQuestGraph& graph ) {}
	virtual wxPanel* GetPanel() { return this; }
	virtual String GetToolName() const { return TXT( "Log" ); }

	// ------------------------------------------------------------------------
	// IQuestSystemListener impl
	// ------------------------------------------------------------------------
	void OnQuestStarted( CQuestThread* thread, CQuest& quest );
	void OnQuestStopped( CQuestThread* thread );
	void OnSystemPaused( bool paused );

	// ------------------------------------------------------------------------
	// IQuestThreadListener impl
	// ------------------------------------------------------------------------
	void OnThreadPaused( CQuestThread* thread, bool paused );
	void OnAddThread( CQuestThread* parentThread, CQuestThread* thread );
	void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread );
	void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block );
	void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block );
	void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block );

protected:
	void OnClose( wxCloseEvent& event );
	void OnFind( wxCommandEvent& event );

private:
	void AddEntry( Type type, const String& desc, const CQuestGraphBlock* block );
	String GetThreadDesc( CQuestThread* parentThread, CQuestThread* thread ) const;
	String GetBlockDesc( CQuestThread* thread, const CQuestGraphBlock* block ) const;
};

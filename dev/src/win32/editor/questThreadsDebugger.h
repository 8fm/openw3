/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/questsSystem.h"
#include "../../common/game/questThread.h"
#include "questEdTool.h"


class CQuestThreadsDebugger :	public wxPanel,
										public IQuestEdTool,
										public IQuestSystemListener
{
private:
	struct ItemData
	{
		CQuestThread&					questThread;
		const CQuestScriptBlock&		block;

		ItemData( CQuestThread&	_questThread,  const CQuestScriptBlock&	_block )
			: questThread( _questThread )
			, block( _block )
		{}

		const CScriptThread* const GetThread() const;
	};

private:
	CQuestsSystem*				m_system;
	wxListCtrl*					m_threadsInfo;
	CEdTimer*					m_updateTimer;

public:
	CQuestThreadsDebugger();
	~CQuestThreadsDebugger();

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  ) {}
	virtual void OnGraphSet( CQuestGraph& graph ) {}
	virtual wxPanel* GetPanel() { return this; }
	virtual String GetToolName() const { return TXT( "Threads" ); }

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
	void OnTimer( wxTimerEvent& event );

private:
	Int32 Find( const CQuestScriptBlock* scriptBlock ) const;
};

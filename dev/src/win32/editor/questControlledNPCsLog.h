/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../games/r4/questControlledNPC.h"
#include "questEdTool.h"


class CR4QuestSystem;

class CQuestControlledNPCsLog :	public wxPanel,
											public IQuestEdTool,
											public IQuestNPCsManagerListener
{
private:
	CR4QuestSystem*			m_system;
	wxListCtrl*				m_log;
	CEdQuestEditor*		m_host;

public:
	CQuestControlledNPCsLog();
	~CQuestControlledNPCsLog();

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  ) {}
	virtual void OnGraphSet( CQuestGraph& graph ) {}
	virtual wxPanel* GetPanel() { return this; }
	virtual String GetToolName() const { return TXT( "Behaviors" ); }

	// ------------------------------------------------------------------------
	// IQuestNPCsManagerListener impl
	// ------------------------------------------------------------------------
	void NotifyError( const String& errMsg ) const;
	void NotifySuccess( const String& msg ) const;

protected:
	void OnClose( wxCloseEvent& event );
};

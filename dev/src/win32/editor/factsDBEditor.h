/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questEdTool.h"
#include "../../common/game/factsDB.h"


class CEdGameFact;

/// Facts database editor
class CEdFactsDB: public wxPanel, 
						public IQuestEdTool, 
						public IEdEventListener
{
	DECLARE_EVENT_TABLE();

private:
	CFactsDB* m_factsDB;

	wxListBox* m_idsListBox;
	wxListBox* m_valuesListBox;
	wxStaticText* m_queryResult;
	wxButton* m_filterIdsButton;
	wxTextCtrl* m_idFilterValue;
	CEdGameFact* m_factEditor;

	String m_idFilter;

	TDynArray< String > m_idsList;
	Int32 m_selectedID;

	TDynArray< const CFactsDB::Fact* > m_factsList;

public:
	CEdFactsDB();
	~CEdFactsDB();

	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  );
	virtual void OnGraphSet( CQuestGraph& graph ) {}
	virtual wxPanel* GetPanel() { return this; }
	virtual String GetToolName() const { return TXT( "Facts DB" ); }

	// ------------------------------------------------------------------------
	// CFactsDB::IListener implementation
	// ------------------------------------------------------------------------
	void OnFactsDBChanged( const String& id );
	void OnFactsDBDestroyed();

protected:
	// ------------------------------------------------------------------------
	// window events handlers
	// ------------------------------------------------------------------------
	void OnClose( wxCloseEvent& event );
	void OnIDSelected( wxCommandEvent& event );
	void ViewFact( wxCommandEvent& event );
	void OnAddID( wxCommandEvent& event );
	void OnAddFact( wxCommandEvent& event );
	void OnQuerySum( wxCommandEvent& event );
	void OnQuerySumSince( wxCommandEvent& event );
	void OnQueryLatestValue( wxCommandEvent& event );
	void OnFilterIDs(wxCommandEvent& event );
	void OnIDsContextMenu( wxMouseEvent& event );
	void OnValuesContextMenu( wxMouseEvent& event );
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void ShowContextMenu( wxMouseEvent& event, wxListBox* owner);
	void RefreshIDsList();
	void RefreshFactsList();
	void SelectID( const String& id );
	String FactAsString( const CFactsDB::Fact* fact ) const;
	void AttachToFactsDB();

	String AskForText();
	Float AskForTime();
};


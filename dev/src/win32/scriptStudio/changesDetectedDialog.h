/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SS_CHANGES_DETECTED_DIALOG_H__
#define __SS_CHANGES_DETECTED_DIALOG_H__

#include "widgets/checkedList.h"

class CSSDocument;
class CAlteredFilesListStateChangeEvent;

class CDetectOfflineChanges
{
public:
	CDetectOfflineChanges( vector< CSSDocument* >& openDocuments );
	~CDetectOfflineChanges();

	RED_INLINE bool HasOfflineChanges() const { return ( m_offlineChanges.size() + m_offlineDeleted.size() + m_sourceControlDeleted.size() ) > 0; }

	void FilterDocuments( vector< CSSDocument* >& documents );

	void DisplayDialog( wxWindow* parent );

private:
	void OnOfflineChangeStateChange( CAlteredFilesListStateChangeEvent& event );
	void OnOfflineDeleteStateChange( CAlteredFilesListStateChangeEvent& event );
	void OnDialogClose( wxCommandEvent& event );

	void PerformFileActions();

private:
	enum EAction
	{
		A_Reload = 0,
		A_Close,
		A_Ignore
	};

	vector< CSSDocument* > m_offlineChanges;
	vector< CSSDocument* > m_offlineDeleted;
	vector< CSSDocument* > m_sourceControlDeleted;
	map< CSSDocument*, EAction > m_actionMap;
};

//////////////////////////////////////////////////////////////////////////
class CAlteredFilesListStateChangeEvent : public wxEvent
{
public:
	CAlteredFilesListStateChangeEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CAlteredFilesListStateChangeEvent( CSSDocument* document, CSSCheckListCtrl::EChecked state );

	virtual ~CAlteredFilesListStateChangeEvent();

	RED_INLINE CSSDocument* GetDocument() const { return m_document; }
	RED_INLINE CSSCheckListCtrl::EChecked GetState() const { return m_state; }

private:
	virtual wxEvent* Clone() const override final;

private:
	CSSDocument* m_document;
	CSSCheckListCtrl::EChecked m_state;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CAlteredFilesListStateChangeEvent );
};

wxDECLARE_EVENT( ssEVT_ALTERED_FILES_STATE_CHANGE_EVENT, CAlteredFilesListStateChangeEvent );

//////////////////////////////////////////////////////////////////////////
class CSSAlteredFilesList : public CSSCheckListCtrl
{
	wxDECLARE_CLASS( CSSAlteredFilesList );

public:
	CSSAlteredFilesList( wxWindow* parent );
	virtual ~CSSAlteredFilesList();

	void Add( CSSDocument* document, EChecked state );

	enum EColumn
	{
		Col_Checkbox = 0,
		Col_Path
	};

private:
	virtual void OnStateChange( int itemIndex, EChecked state ) override final;
};

//////////////////////////////////////////////////////////////////////////
class CSSChangesDetectedDialog : public wxDialog
{
	wxDECLARE_CLASS( CSSChangesDetectedDialog );

	typedef void (*StateChangeListener)(CAlteredFilesListStateChangeEvent&);

public:
	CSSChangesDetectedDialog( wxWindow* parent );
	virtual ~CSSChangesDetectedDialog();

	wxEvtHandler* CreateSection( vector< CSSDocument* >& documents, wxString title, wxString description, CSSCheckListCtrl::EChecked defaultState );

private:
	wxSizer* m_sectionSizer;

	vector< wxStaticText* > m_labels;
	wxArrayString m_descriptions;
};

#endif // __SS_CHANGES_DETECTED_DIALOG_H__

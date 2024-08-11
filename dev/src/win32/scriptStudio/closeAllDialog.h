/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Open Solution File dialog
class CSSCloseAllDialog : public wxDialog 
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CSSCloseAllDialog );

protected:
	wxCheckListBox*				m_checkList;
	vector< SolutionFilePtr >	m_modifiedFiles;

public:
	enum EResult
	{
		END_CANCEL = 0,
		END_CLOSE,
		END_SAVE
	};

public:
	CSSCloseAllDialog( wxWindow* parent );
	virtual ~CSSCloseAllDialog();

	void Init( const vector< SolutionFilePtr >& modifiedFiles );
	void GetChecked( vector< SolutionFilePtr >& checkedFiles );

protected:
	void OnBtnSelectAll( wxCommandEvent& event );
	void OnBtnDeselectAll( wxCommandEvent& event );
	void OnBtnSave( wxCommandEvent& event );
	void OnBtnClose( wxCommandEvent& event );
	void OnBtnCancel( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
};

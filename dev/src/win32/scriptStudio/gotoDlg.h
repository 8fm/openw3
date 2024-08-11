/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __GOTO_DIALOG_H__
#define __GOTO_DIALOG_H__

/// Goto line number dialog
class CSSGotoDialog : public wxDialog 
{
	wxDECLARE_CLASS( CSSGotoDialog );
	wxDECLARE_EVENT_TABLE();

private:
	wxStaticText*	m_lblLineNumber;
	wxTextCtrl*		m_txtLineNumber;
	wxString		m_file;

	int				m_maxLines;

public:
	CSSGotoDialog( wxWindow* parent, const wxString& file );
	virtual ~CSSGotoDialog();

	void SetMaxLines( int maxLines );

private:
	void Go();

private:
	void OnOK( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnShow( wxShowEvent& event );
};

#endif // __GOTO_DIALOG_H__

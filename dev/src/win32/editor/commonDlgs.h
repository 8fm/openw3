/**
* Copyright © 2007-2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdChooseFromListDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	String		m_value;

public:
	CEdChooseFromListDlg( wxWindow* parent, TDynArray< String > &values, const String &defaultValue, wxString caption, wxString question = wxEmptyString, wxString labelOk = TXT("OK"), wxString labelCancel = TXT("Cancel") );
	~CEdChooseFromListDlg();

	Int32 DoModal();

	String GetValue(){ return m_value; }
protected:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CDetailsDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CDetailsDlg( wxWindow* parent, wxString caption, wxString information, wxString details, wxString captionOk=TXT("Ok"), wxString captionCancel=TXT("Cancel") );
	~CDetailsDlg();

	Int32 DoModal();

protected:
	void OnCancel( wxCommandEvent& event );
	void OnOk( wxCommandEvent& event );
	void OnDetails( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdMultiChoiceDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdMultiChoiceDlg( wxWindow* parent, wxString caption, wxString message, const TDynArray<String>& options );
	~CEdMultiChoiceDlg();

protected:
	void OnButton( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdMsgDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdMsgDlg( String caption, String message );
	~CEdMsgDlg();

protected:
	void OnButton( wxCommandEvent& event );
};


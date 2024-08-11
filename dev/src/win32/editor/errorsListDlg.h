/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORERRORSLIST_H
#define EDITORERRORSLIST_H


class CEdErrorsListDlg : private wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdErrorsListDlg( wxWindow* parent, Bool modal = true, Bool cancelBtnVisible = false );

	void SetHeader( const String& label );
	void SetFooter( const String& label );
	void SetTitle( const String& title ) { wxDialog::SetTitle( title.AsChar() ); }
	long GetWindowStyle() { return wxDialog::GetWindowStyle(); }
	void SetWindowStyle( long style ) { wxDialog::SetWindowStyle( style ); }
	void ActivateConfigParamCheckBox( const String& label, const String& category, const String& section, const String& param );

	Bool Execute( const String& htmlString );
	Bool Execute( const TDynArray< String >& lines );
	Bool Execute( const TDynArray< String >& lines, const TDynArray< String >& descriptions );

	using wxDialog::Close;
	using wxDialog::Destroy;

private:
	// Events
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnCheckBoxClicked( wxCommandEvent& event );

	class wxWebView*		m_errorsDisplay;
	wxStaticText*			m_headerLabel;
	wxTextCtrl*				m_footerLabel;
	wxCheckBox*				m_cBox;

	String					m_paramCategory;
	String					m_paramSection;
	String					m_paramName;
	Bool					m_modal;
};

#endif // EDITORERRORSLIST_H

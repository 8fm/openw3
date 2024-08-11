
#pragma once

//!
class CEdPropertyDialog : private wxSmartLayoutDialog
{
public:
	typedef std::function< void( Bool result ) > OnCloseHandler;

	//!
	CEdPropertyDialog( wxWindow* parent, const wxString& title, PropertiesPageSettings settings = PropertiesPageSettings() );

	//!
	~CEdPropertyDialog();

	//!
	void SetOkCancelLabels( const wxString& okLabel, const wxString& cancelLabel );

	//! Shows the window modally. The method won't return until the window is closed.
	Bool Execute( CObject* objToEdit );

	//! Shows the window non-modally. The method returns immediately.
	void Show( CObject* objToEdit, OnCloseHandler onCloseHandler );

	//! Hides the non-modal window
	void Hide();

	//! See the cpp for an example how to use the dialog
	static void ShowExampleModal();

	static void ShowExampleNonModal();

private:
	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;
	void OnButton( wxCommandEvent& event );

	CEdPropertiesBrowserWithStatusbar* m_properties;
	OnCloseHandler m_onClose;
};



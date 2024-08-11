/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORWHATSNEW_H
#define EDITORWHATSNEW_H

class CEdWhatsNewDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdWhatsNewDlg( wxWindow* parent, Bool forceShow );
	
	//void AddNewInformation( Uint32 fromChangeList );
	void AddNewInformationToWebView();
	// Get and Set show again value
	Bool GetDontShowAgainValue( Bool& enabled );
	Bool SetDontShowAgainValue( Bool enabled );
	// Get and Set Changelist number
	Bool GetChangeListNumber( UINT32& CLNumber );
	Bool SetChangeListNumber( UINT32 CLNumber );

	String BuildWebString( String prefix, String description, String user, String workspace );

	// Events
	void OnClose( wxCommandEvent& event );
	void OnMore( wxCommandEvent& event );
	void OnClosed( wxCloseEvent& event );

	Uint32					m_fromChangeList;
	Uint32					m_updatedToChangeList;
	Bool					m_showText;
private:
	class wxWebView*		m_whatsNewDisplay;
	Uint32					m_amountOfChangeListToShow;
};

#endif // EDITORWHATSNEW_H

/**
* Copyright © 2007-2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "commonDlgs.h"
#include "callbackData.h"

BEGIN_EVENT_TABLE( CEdChooseFromListDlg, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdChooseFromListDlg::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdChooseFromListDlg::OnCancel )
END_EVENT_TABLE()

CEdChooseFromListDlg::CEdChooseFromListDlg( wxWindow* parent, TDynArray< String > &values, const String &defaultValue, wxString caption, wxString question, wxString labelOk, wxString labelCancel )
: m_value( defaultValue )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("ChooseFromListDialog") );

	wxButton *btnOk = XRCCTRL( *this, "OK", wxButton );
	wxButton *btnCancel = XRCCTRL( *this, "Cancel", wxButton );

	btnOk->SetLabel( labelOk );
	btnCancel->SetLabel( labelCancel );

	SetLabel( caption );
	wxStaticText *label = XRCCTRL( *this, "label", wxStaticText );
	if( question.IsEmpty() )
	{
		label->Hide();
	}
	else
	{
		label->SetLabel( question );
	}

	wxListBox *valuesList = XRCCTRL( *this, "list", wxListBox );

	valuesList->Freeze();
	for( TDynArray< String >::iterator it=values.Begin(); it!=values.End(); it++ )
	{
		Int32 idx = valuesList->Append( it->AsChar() );
		if( m_value == *it )
		{
			valuesList->SetSelection( idx );
		}
	}
	valuesList->Thaw();
}

CEdChooseFromListDlg::~CEdChooseFromListDlg()
{

}

Int32 CEdChooseFromListDlg::DoModal()
{
	return wxDialog::ShowModal();
}

void CEdChooseFromListDlg::OnOK( wxCommandEvent& event )
{
	wxListBox *valuesList = XRCCTRL( *this, "list", wxListBox );
	m_value = valuesList->GetStringSelection();

	EndDialog( wxID_OK );
}

void CEdChooseFromListDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CDetailsDlg, wxDialog )
	EVT_BUTTON( XRCID("DetailsBtn"), CDetailsDlg::OnDetails )
	EVT_BUTTON( XRCID("CancelBtn"), CDetailsDlg::OnCancel )
	EVT_BUTTON( XRCID("OkBtn"), CDetailsDlg::OnOk )
END_EVENT_TABLE()

CDetailsDlg::CDetailsDlg( wxWindow* parent, wxString caption, wxString information, wxString details, wxString captionOk, wxString captionCancel )
{
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("DetailsDialog") );
	SetLabel( caption );
	wxStaticText* info = XRCCTRL( *this, "label", wxStaticText );
	info->SetLabel( information );
	wxTextCtrl* detailsCtrl = XRCCTRL( *this, "details", wxTextCtrl );
	detailsCtrl->SetValue( details );

	wxButton *btnOk = XRCCTRL( *this, "OkBtn", wxButton );
	btnOk->SetLabel( captionOk );
	wxButton *btnCancel = XRCCTRL( *this, "CancelBtn", wxButton );
	if( captionCancel.IsEmpty() )
	{
		btnCancel->Hide();
	}
	else
	{
		btnCancel->SetLabel( captionCancel );
	}
}

CDetailsDlg::~CDetailsDlg()
{

}

Int32 CDetailsDlg::DoModal()
{
	return wxDialog::ShowModal();
}

void CDetailsDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
}

void CDetailsDlg::OnOk( wxCommandEvent& event )
{
	EndDialog( wxID_OK );
}

void CDetailsDlg::OnDetails( wxCommandEvent& event )
{
	wxTextCtrl* detailsCtrl = XRCCTRL( *this, "details", wxTextCtrl );
	detailsCtrl->Show();
	SetSize( -1, 250 );
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdMultiChoiceDlg, wxDialog )
END_EVENT_TABLE()

CEdMultiChoiceDlg::CEdMultiChoiceDlg(wxWindow* parent, wxString caption, wxString message, const TDynArray<String>& options )
{
	ASSERT(options.Size() > 0);

	wxXmlResource::Get()->LoadDialog( this, parent, TXT("MultiChoiceDialog") );
	SetLabel( caption );

	wxPanel* panel = XRCCTRL( *this, "buttPanel", wxPanel );

	wxStaticText* info = XRCCTRL( *this, "message", wxStaticText );
	info->SetLabel( message );

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	for (Uint32 i=0; i<options.Size(); i++)
	{
		wxString option = options[i].AsChar();
		wxButton* butt = new wxButton(panel, -1 , option, wxDefaultPosition, wxSize(100,-1));
		butt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMultiChoiceDlg::OnButton ), new TCallbackData< Int32 >( i ), this );
		sizer->Add(butt, 0, wxALL);
	}

	panel->SetSizer(sizer);
	panel->FitInside();
	panel->Layout();

	SetSize(options.Size()*100+20,-1);

	FitInside();
	Layout();
}

CEdMultiChoiceDlg::~CEdMultiChoiceDlg()
{
}

void CEdMultiChoiceDlg::OnButton( wxCommandEvent& event )
{
	TCallbackData< Int32 >* callData = (TCallbackData< Int32 >*)event.m_callbackUserData;
	Int32 buttNum = callData->GetData();
	EndDialog( buttNum );
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdMsgDlg, wxDialog )
END_EVENT_TABLE()

CEdMsgDlg::CEdMsgDlg( String caption, String message )
	: wxDialog( NULL, wxID_ANY, caption.AsChar(), wxDefaultPosition )
{
	SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* text = new wxStaticText( this, wxID_ANY, message.AsChar(), wxDefaultPosition, wxDefaultSize, 0 );
	text->Wrap( -1 );
	mainSizer->Add( text, 0, wxALL, 10 );

	wxBoxSizer* buttonSizer;
	buttonSizer = new wxBoxSizer( wxVERTICAL );

	wxButton * button = new wxButton( this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( button, 0, wxALIGN_CENTER|wxALL, 5 );

	mainSizer->Add( buttonSizer, 1, wxEXPAND, 5 );

	SetSizer( mainSizer );
	Layout();

	mainSizer->Fit( this );

	Centre( wxBOTH );

	button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMsgDlg::OnButton ), NULL, this );
}

CEdMsgDlg::~CEdMsgDlg()
{
}

void CEdMsgDlg::OnButton( wxCommandEvent& event )
{
	EndDialog( 1 );
}

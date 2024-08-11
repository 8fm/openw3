/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/webview.h>
#include "errorsListDlg.h"

#ifdef _DEBUG
#pragma comment(lib, "wxmsw29ud_webview")
#else
#pragma comment(lib, "wxmsw29u_webview")
#endif

namespace
{
	#define DIALOG_WIDTH	600
}

BEGIN_EVENT_TABLE( CEdErrorsListDlg, wxDialog )
	EVT_BUTTON( XRCID("m_okButton"), CEdErrorsListDlg::OnOK )
	EVT_BUTTON( XRCID("m_cancelButton"), CEdErrorsListDlg::OnCancel )
END_EVENT_TABLE()


CEdErrorsListDlg::CEdErrorsListDlg( wxWindow* parent, Bool modal /*= true*/, Bool cancelBtnVisible /*= false*/ )
	: m_modal( modal )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ErrorsListDlg") );

	m_headerLabel = XRCCTRL( *this, "m_headerLabel", wxStaticText );
	m_footerLabel = XRCCTRL( *this, "m_footerLabel", wxTextCtrl );

	wxPanel* webViewContainer = XRCCTRL( *this, "WebViewContainer", wxPanel );
	webViewContainer->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_errorsDisplay = wxWebView::New( webViewContainer, wxID_ANY );
	webViewContainer->GetSizer()->Add( m_errorsDisplay, 1, wxEXPAND );

	m_cBox = XRCCTRL( *this, "m_cBox", wxCheckBox );
	m_cBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdErrorsListDlg::OnCheckBoxClicked ), NULL, this );
	m_cBox->Hide();

	if ( !cancelBtnVisible )
	{
		wxButton* cancelBtn = XRCCTRL( *this, "m_cancelButton", wxButton );
		cancelBtn->Hide();
	}

	this->SetSize( DIALOG_WIDTH, 800 );
	CenterOnParent();
}

void CEdErrorsListDlg::SetHeader( const String& label )			
{ 
	if ( m_headerLabel )	
	{ 
		m_headerLabel->SetLabel( label.AsChar() ); 
		m_headerLabel->Wrap( DIALOG_WIDTH - 100 );
	} 
} 

void CEdErrorsListDlg::SetFooter( const String& label )			
{ 
	if ( m_footerLabel )	
	{ 
		m_footerLabel->SetLabel( label.AsChar() ); 
	} 
} 

void CEdErrorsListDlg::ActivateConfigParamCheckBox( const String& label, const String& category, const String& section, const String& param )
{
	m_paramCategory = category;
	m_paramSection = section;
	m_paramName = param;

	if ( m_cBox )
	{
		Bool showOnStart = false;
		SUserConfigurationManager::GetInstance().ReadParam( m_paramCategory.AsChar(), m_paramSection.AsChar(), m_paramName.AsChar(), showOnStart );

		m_cBox->SetLabel( label.AsChar() );
		m_cBox->SetValue( showOnStart );
		m_cBox->Show();
	}
}

Bool CEdErrorsListDlg::Execute( const String& htmlString )
{
	wxString newWebString( htmlString.AsChar(), wxConvUTF8 );
	m_errorsDisplay->SetPage( newWebString, wxEmptyString );

	if ( m_modal )
	{
		return ShowModal() == wxID_OK;
	}
	
	
	Show();
	return true;
}

Bool CEdErrorsListDlg::Execute( const TDynArray< String >& lines )
{
	String webString = 
		TXT("<!DOCTYPE html><html><head><title></title><style>html,body { font-family: helvetica; font-size: 13px; padding: 0px; margin: 0px; } .entry { border-bottom: 1px solid gray; padding: 5px } .entry:hover { background: #F8F8F8 } </style></head><body><br/>");

	String baseStringStart = TXT( "<div class=\"entry\"><font color='#505050'><ul>" );
	String baseStringEnd = TXT( "</ul></font></div><br/><br/>" );

	webString += baseStringStart;
	for ( Uint32 i = 0; i < lines.Size(); ++i )
	{
		webString += String::Printf( TXT( "<li>%ls</li>" ), lines[i].AsChar() );
	}
	webString += baseStringEnd;

	webString += TXT("</body></html>");

	Int32 height = 200 + m_headerLabel->GetSize().y + Min( lines.Empty() ? 10 : (Int32)lines.Size(), 20 ) * 30;
	this->SetSize( m_headerLabel->GetSize().GetWidth() + 100, height );
	CenterOnParent();

	return Execute( webString );
}

Bool CEdErrorsListDlg::Execute( const TDynArray< String >& errors, const TDynArray< String >& descriptions )
{
	if ( descriptions.Size() != errors.Size() )
	{
		return Execute( errors );
	}

	String webString = 
		TXT("<!DOCTYPE html><html><head><title></title><style>html,body { font-family: helvetica; font-size: 13px; padding: 0px; margin: 0px; } .entry { border-bottom: 1px solid gray; padding: 5px } .entry:hover { background: #F8F8F8 } </style></head><body><br/>");

	String baseStringStart = TXT( "<div class=\"entry\"><font color='#505050'><ul>" );
	String baseStringEnd = TXT( "</ul></font></div><br/><br/>" );

	webString += baseStringStart;
	for ( Uint32 i = 0; i < errors.Size(); ++i )
	{
		webString += String::Printf( TXT( "<li><font color='#C80000'>%ls</font> <br/> %ls</li>" ), errors[i].AsChar(), descriptions[i].AsChar() );
	}
	webString += baseStringEnd;

	webString += TXT("</body></html>");

	Int32 height = 200 + m_headerLabel->GetSize().GetHeight() + Min( errors.Empty() ? 10 : (Int32)errors.Size(), 12 ) * 50;
	this->SetSize( m_headerLabel->GetSize().GetWidth() + 100, height );
	CenterOnParent();

	return Execute( webString );
}

void CEdErrorsListDlg::OnOK( wxCommandEvent &event )
{
	if ( m_modal )
	{
		EndModal( wxID_OK );
	}
	else
	{
		Close();
	}
}

void CEdErrorsListDlg::OnCancel( wxCommandEvent& event )
{
	if ( m_modal )
	{
		EndModal( wxID_CANCEL );
	}
	else
	{
		Close();
	}
}

void CEdErrorsListDlg::OnCheckBoxClicked( wxCommandEvent& event )
{
	if ( m_cBox && m_cBox->IsShown() )
	{
		Bool showOnStart = event.IsChecked();
		SUserConfigurationManager::GetInstance().WriteParam( m_paramCategory.AsChar(), m_paramSection.AsChar(), m_paramName.AsChar(), showOnStart );
	}
}

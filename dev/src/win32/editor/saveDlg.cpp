#include "build.h"

#include "../../common/core/versionControl.h"

BEGIN_EVENT_TABLE( CEdSaveDialog, wxDialog )
	EVT_BUTTON( XRCID("SaveAs"), CEdSaveDialog::OnSaveAs )
	EVT_BUTTON( XRCID("Cancel"), CEdSaveDialog::OnCancel )
	EVT_BUTTON( XRCID("CheckOut"), CEdSaveDialog::OnCheckOut )
	EVT_BUTTON( XRCID("Overwrite"), CEdSaveDialog::OnOverwrite )
END_EVENT_TABLE()

CEdSaveDialog::CEdSaveDialog( wxWindow *parent, const String &name )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("SaveErrorDialog") );
	m_path = name;
	wxStaticText *info = XRCCTRL( *this, "Info", wxStaticText );
	String msg = name + TXT(" cannot be saved because it was not checked out. What do you wish to do?");
	info->SetLabel( msg.AsChar() );
}

void CEdSaveDialog::OnSaveAs( wxCommandEvent& event )
{
	wxFileDialog openFileDialog(this);
	openFileDialog.SetPath( wxString( m_path.AsChar() ) );
	if (openFileDialog.ShowModal() == wxID_OK)
	{
		m_path = openFileDialog.GetPath().wc_str();
	}
	EndDialog( SC_SAVE_AS );
}

void CEdSaveDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog( SC_CANCEL ); 
}

void CEdSaveDialog::OnCheckOut( wxCommandEvent &event )
{
	EndDialog( SC_CHECK_OUT );
}

void CEdSaveDialog::OnOverwrite( wxCommandEvent &event )
{
	EndDialog( SC_OVERWRITE );
}

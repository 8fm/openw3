#include "build.h"
#include "submitDialog.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/core/configFileManager.h"

BEGIN_EVENT_TABLE( CEdSubmitDialog, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdSubmitDialog::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdSubmitDialog::OnCancel )
END_EVENT_TABLE()

CEdSubmitDialog::CEdSubmitDialog( wxWindow* parent, String &description,
								  const TDynArray< CDiskFile * > &files, TSet< CDiskFile * > &chosen )
	: m_description(description)
	, m_files(files)
	, m_chosen(chosen)
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("SubmitDialog") );
	m_list = XRCCTRL( *this, "Changelist", wxCheckListBox );
	if ( description.Empty() )
	{
		String newDesc = TXT("[") + GGameConfig::GetInstance().GetName() + TXT("]");
		String deptags;
		SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("SourceControl"), TXT("DepartmentTags"), deptags );
		if ( deptags.Empty() )
		{
			wxMessageBox( wxT("Please fill your department tags (f.e. [Env], [Art][Models], etc) using File -> Department tags..."), wxT("Missing department tags"), wxICON_WARNING|wxOK|wxCENTER );
		}
		else
		{
			newDesc += deptags;
		}
		XRCCTRL( *this, "Description", wxTextCtrl )->SetValue( newDesc.AsChar() );
	}
	wxString *list = new wxString[files.Size()];
	for (Uint32 i = 0; i < files.Size(); i++)
	{
		list[i] = files[i]->GetDepotPath().AsChar();
	}
	m_list->InsertItems(files.Size(), list, 0);
	for (Uint32 i = 0; i < files.Size(); i++)
	{
		if ( m_chosen.Find( m_files[i] ) != m_chosen.End() )
		{
			m_list->Check(i);
		}
	}
	m_chosen.Clear();
}

void CEdSubmitDialog::OnOK( wxCommandEvent& event )
{
	m_description = XRCCTRL( *this, "Description", wxTextCtrl )->GetValue().wc_str();
	if ( m_description == TXT( "" ) )
		return;

	for (Uint32 i = 0; i < m_files.Size(); i++)
	{
		if ( m_list->IsChecked( i ) )
		{
			m_chosen.Insert( m_files[i] );
		}
	}
	EndDialog( wxID_OK );
}

void CEdSubmitDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
};
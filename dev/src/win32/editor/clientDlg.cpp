#include "build.h"
#include "clientDlg.h"

#include "../../common/core/versionControl.h"

#define MAX_EXTRA_WORKSPACES 16

BEGIN_EVENT_TABLE( CEdClientDialog, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdClientDialog::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdClientDialog::OnCancel )
END_EVENT_TABLE()

CEdClientDialog::CEdClientDialog( wxWindow *parent )
{
	// Open dialog
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ClientDialog") );

	// Open config
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl64") );
#else
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl") );
#endif

	// Load settings	
	String setting;
	if ( config.Read( TXT("User"), &setting ) )
	{
		XRCCTRL( *this, "User", wxTextCtrl )->SetValue( setting.AsChar() );
	}
	
	// Password for the user
	if ( config.Read( TXT("Password"), &setting ) )
	{
		XRCCTRL( *this, "Password", wxTextCtrl )->SetValue( setting.AsChar() );
	}
	
	// Workspace
	TDynArray< String >	workspaces;
	GetWorkspaces( workspaces );
	if ( !workspaces.Empty() )
	{
		for ( auto item : workspaces )
		{
			XRCCTRL( *this, "Workspace", wxComboBox )->Append( item.AsChar() );
		}
		XRCCTRL( *this, "Workspace", wxComboBox )->SetValue( workspaces[0].AsChar() );
	}

	// Host ( local computer name )
	if ( config.Read( TXT("Host"), &setting ) )
	{
		XRCCTRL( *this, "Host", wxTextCtrl )->SetValue( setting.AsChar() );
	}

	// Server address
	if ( config.Read( TXT("Port"), &setting ) )
	{
		XRCCTRL( *this, "Port", wxTextCtrl )->SetValue( setting.AsChar() );
	}

	// Automatic changelists
	XRCCTRL( *this, "AutomaticChangelists", wxCheckBox )->SetValue( config.Read( TXT("AutomaticChangelists"), 1 ) != 0 );
}

CEdClientDialog::~CEdClientDialog()
{
}

void CEdClientDialog::OnOK( wxCommandEvent &event )
{
	// Get new source control settings
	SSourceControlSettings settings;
	settings.m_user = XRCCTRL( *this, "User", wxTextCtrl )->GetValue().wc_str();
	settings.m_password = XRCCTRL( *this, "Password", wxTextCtrl )->GetValue().wc_str();
	settings.m_client = XRCCTRL( *this, "Workspace", wxComboBox )->GetValue().wc_str();
	settings.m_host = XRCCTRL( *this, "Host", wxTextCtrl )->GetValue().wc_str();
	settings.m_port = XRCCTRL( *this, "Port", wxTextCtrl )->GetValue().wc_str();
	settings.m_automaticChangelists = XRCCTRL( *this, "AutomaticChangelists", wxCheckBox )->GetValue();

	// Apply settings, save in config
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
		CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl64") );
#else
		CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl") );
#endif
		config.Write( TXT("User"), settings.m_user );
		config.Write( TXT("Password"), settings.m_password );
		config.Write( TXT("Workspace"), settings.m_client );
		config.Write( TXT("Host"), settings.m_host );
		config.Write( TXT("Port"), settings.m_port );
		config.Write( TXT("AutomaticChangelists"), settings.m_automaticChangelists ? 1 : 0 );

		// Save extra workspaces (note: we begin from 1 because 0 is the current workspace)
		wxComboBox* cbWorkspaces = XRCCTRL( *this, "Workspace", wxComboBox );
		for ( unsigned int i=0; i < MAX_EXTRA_WORKSPACES; ++i )
		{
			String entry = i < cbWorkspaces->GetCount() ? cbWorkspaces->GetString( i ).wc_str() : String::EMPTY;
			if ( entry != settings.m_client )
			{
				config.Write( String::Printf( TXT("Workspace%d"), i ), entry );
			}
		}
	}

	// Reinitialize source control
	SInitializeVersionControl();

	// Update editor title bar
	wxTheFrame->SetFrameTitle();

	// Changed
	EndDialog( wxID_OK );
}

void CEdClientDialog::OnCancel( wxCommandEvent &event )
{
	// Not changed
	EndDialog( wxID_CANCEL );
}

void CEdClientDialog::GetWorkspaces( TDynArray< String >& workspaces )
{
	workspaces.Clear();

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl64") );
#else
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl") );
#endif
	
	String setting;
	if ( config.Read( TXT("Workspace"), &setting ) )
	{
		workspaces.PushBack( setting );
	}
	for ( Uint32 i=0; i < MAX_EXTRA_WORKSPACES; ++i )
	{
		if ( config.Read( String::Printf( TXT("Workspace%d"), i ), &setting ) )
		{
			if ( !setting.Empty() )
			{
				workspaces.PushBack( setting );
			}
		}
	}
}

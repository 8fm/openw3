#include "build.h"
#include "scriptCompilationErrorsDialog.h"
#include "../../common/core/scriptingSystem.h"
enum EColumn
{
	Col_Type = 0,
	Col_Line,
	Col_Text,
	Col_File,

	Col_Max
};

CEdScriptCompilationErrorsDialog::CEdScriptCompilationErrorsDialog( wxWindow* parent, const CScriptCompilationMessages& errorCollector )
{
	// Load window
	bool loadOk = wxXmlResource::Get()->LoadDialog( this, parent, wxT( "ScriptCompilationErrorsDialog" ) );
	ASSERT( loadOk );

	m_errorCtrl = XRCCTRL( *this, "ErrorList", wxListCtrl );
	ASSERT( m_errorCtrl != NULL );

	m_errorCtrl->ClearAll();

	m_errorCtrl->InsertColumn( Col_Type, wxT( "Type" ) );
	m_errorCtrl->InsertColumn( Col_Line, wxT( "Line" ) );
	m_errorCtrl->InsertColumn( Col_Text, wxT( "Message" ) );
	m_errorCtrl->InsertColumn( Col_File, wxT( "File" ) );

	Fill( errorCollector.m_errors, wxT( "Error" ), wxColour( 255, 128, 128 ) );
	Fill( errorCollector.m_warnings, wxT( "Warning"), wxColour( 255, 255, 128 ) );

	m_errorCtrl->SetColumnWidth( Col_Type, wxLIST_AUTOSIZE_USEHEADER );
	m_errorCtrl->SetColumnWidth( Col_Line, wxLIST_AUTOSIZE_USEHEADER );
	m_errorCtrl->SetColumnWidth( Col_Text, wxLIST_AUTOSIZE );
	m_errorCtrl->SetColumnWidth( Col_File, wxLIST_AUTOSIZE );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdScriptCompilationErrorsDialog::OnRecompile, this, XRCID( "Recompile" ) );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdScriptCompilationErrorsDialog::OnSkipCompile, this, XRCID( "SkipCompile" ) );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdScriptCompilationErrorsDialog::OnCancel, this, XRCID( "Close" ) );
	Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED , &CEdScriptCompilationErrorsDialog::OnItemSelected, this, XRCID( "ErrorList" ) );

	// Start at a value above 0 since it seems to come out slightly under
	int calcWidth = 20;
	for( int i = 0; i < Col_Max; ++i )
	{
		calcWidth += m_errorCtrl->GetColumnWidth( i );
	}

	SetClientSize( calcWidth, 500 );

	// Actually... let's not allow skipping the compilation, it can break stuff
	XRCCTRL( *this, "SkipCompile", wxButton )->Show( false );

	Show();
}

CEdScriptCompilationErrorsDialog::~CEdScriptCompilationErrorsDialog()
{
}

void CEdScriptCompilationErrorsDialog::Fill( const TDynArray< CScriptCompilationMessages::SContext >& contexts, const wxString& typeStr, const wxColour& colour )
{
	for( Uint32 i = 0; i < contexts.Size(); ++i )
	{
		long rowId = m_errorCtrl->InsertItem( i, typeStr );

		wxString line;
		line.Printf( wxT("%i"), contexts[ i ].line );

		m_errorCtrl->SetItem( rowId, Col_Line, line );
		m_errorCtrl->SetItem( rowId, Col_Text, contexts[ i ].text.AsChar() );
		m_errorCtrl->SetItem( rowId, Col_File, contexts[ i ].file.AsChar() );
		m_errorCtrl->SetItemData( rowId, contexts[ i ].line );

		m_errorCtrl->SetItemBackgroundColour( rowId, colour );
	}
}

void CEdScriptCompilationErrorsDialog::OnItemSelected( wxListEvent& event )
{
	// Account for the fact that script studio and human readable English starts counting lines from 1
	int line = event.GetItem().GetData();

	wxString filename = m_errorCtrl->GetItemText( event.GetIndex(), Col_File );

#ifdef RED_NETWORK_ENABLED
	Red::Network::Manager* network = Red::Network::Manager::GetInstance();
	if( network && network->GetNumberOfConnectionsToChannel( RED_NET_CHANNEL_SCRIPT_COMPILER ) > 0 )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_COMPILER );

		RED_VERIFY( packet.WriteString( "Goto" ) );
		RED_VERIFY( packet.WriteString( filename.wx_str() ) );
		RED_VERIFY( packet.Write( line ) );

		network->Send( RED_NET_CHANNEL_SCRIPT_COMPILER, packet );
	}
	else
#endif
	{
		DWORD processID = FindAssociatedProcessWithName( TXT( "scriptstudio" ) );

		if( processID != -1 )
		{
			wxMessageBox( wxT( "Script studio is not currently connected to the editor" ) );
		}
		else
		{
			// No instance of script studio was found, so open a new one and use the command line
			// to instruct it to open it at the file with the selected error
			wxString commandLine;
			String scriptpath = GFileManager->GetDataDirectory() + TXT("scripts");
			commandLine.Printf( wxT( "scriptstudio.exe /scriptpath %s /openfile %s /goto %i" ), scriptpath.AsChar(), filename.wc_str(), line + 1 );
			wxExecute( commandLine );
		}
	}
}

void CEdScriptCompilationErrorsDialog::OnRecompile(wxCommandEvent& event)
{
	m_userChoice = CSRV_Recompile;

	Hide();
}

void CEdScriptCompilationErrorsDialog::OnSkipCompile(wxCommandEvent& event)
{
	m_userChoice = CSRV_Skip;

	Hide();
}

void CEdScriptCompilationErrorsDialog::OnCancel(wxCommandEvent& event)
{
	m_userChoice = CSRV_Quit;

	Hide();
}

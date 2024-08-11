
#include "build.h"
#include "autoScriptWindow.h"
#include "../../common/engine/scriptInvoker.h"

wxIMPLEMENT_CLASS( CEdAutoScriptWindow, wxSmartLayoutPanel );

BEGIN_EVENT_TABLE( CEdAutoScriptWindow, wxSmartLayoutPanel )
END_EVENT_TABLE()

CEdAutoScriptWindow::CEdAutoScriptWindow( wxWindow* parent )
	: wxSmartLayoutPanel( parent, wxT("Auto scripts"), false, nullptr )
	, m_timer( 0.f )
	, m_running( false )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_textCtrl = new wxTextCtrl( this, wxID_ANY, wxT("Delay: 0.0\nGod()"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER );
	bSizer1->Add( m_textCtrl, 1, wxALL|wxEXPAND, 0 );

	SetSizer( bSizer1 );
	Layout();
	Centre( wxBOTH );

	SEvents::GetInstance().RegisterListener( CNAME( GameStarting ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnding ), this );

	ISavableToConfig::RestoreSession();
}

CEdAutoScriptWindow::~CEdAutoScriptWindow()
{
	ISavableToConfig::SaveSession();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdAutoScriptWindow::Tick( Float dt )
{
	if ( !m_running )
	{
		return;
	}

	m_timer += dt;

	const Uint32 num = m_commands.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const Float time = m_commands[ i ].m_first;
		if ( !m_processed[ i ] && m_timer > time )
		{
			const TDynArray< String >& lines = m_commands[ i ].m_second;

			const Uint32 numLines = lines.Size();
			for ( Uint32 j=0; j<numLines; ++j )
			{
				const String& scriptLine = lines[ j ];

				if ( CScriptInvoker::Parse( scriptLine ) )
				{
					RED_LOG( AutoScriptWindow, TXT("%s"), scriptLine.AsChar() );
				}
				else
				{
					RED_LOG( AutoScriptWindow, TXT("Invalid command - %s"), scriptLine.AsChar() );
				}
			}

			m_processed[ i ] = true;
		}
	}
}

void CEdAutoScriptWindow::OnGameStarted()
{
	ParseScriptText();

	m_timer = 0.f;
	m_running = true;
}

void CEdAutoScriptWindow::OnGameEnded()
{
	m_running = false;
}

void CEdAutoScriptWindow::RestoreSession( CConfigurationManager &config )
{
	LoadLayout( TXT("/Frames/CEdAutoScriptWindow") );

	String txt = config.Read( TXT("CEdAutoScriptWindow/txt"), TXT("Delay:0.0\nGod()") );
	m_textCtrl->SetValue( txt.AsChar() );
}

void CEdAutoScriptWindow::SaveSession( CConfigurationManager &config )
{
	SaveLayout( TXT("/Frames/CEdAutoScriptWindow") );

	String txt = m_textCtrl->GetValue().c_str();
	config.Write( TXT("CEdAutoScriptWindow/txt"), txt );
}

void CEdAutoScriptWindow::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorTick ) )
	{
		const Float dt = GetEventData< Float >( data );
		Tick( dt );
	}
	else if ( name == CNAME( GameStarting ) )
	{
		OnGameStarted();
	}
	else if ( name == CNAME( GameEnding ) )
	{
		OnGameEnded();
	}
}

void CEdAutoScriptWindow::ParseScriptText()
{
	static const String DELAY_KEYWORD( TXT("Delay:") );

	m_commands.ClearFast();

	const String code = m_textCtrl->GetValue().c_str();
	const String sep( TXT("\n") );

	const TDynArray< String > lines = code.Split( sep );
	
	const Uint32 numLines = lines.Size();
	for ( Uint32 i=0; i<numLines; ++i )
	{
		const String& line = lines[ i ];
		if ( line.BeginsWith( DELAY_KEYWORD ) )
		{
			String timeStr = line.StringAfter( DELAY_KEYWORD );
			timeStr.Trim();

			Float time = 0.f;
			if ( FromString( timeStr, time ) )
			{
				m_commands.Grow( 1 );
				m_commands.Back().m_first = time;
			}
			else
			{
				RED_LOG( AutoScriptWindow, TXT("Couldn't parse line - %s. Please use 'Delay: 0.2' pattern."), line.AsChar() );
			}
		}
		else if ( !line.Empty() )
		{
			if ( m_commands.Empty() )
			{
				m_commands.Grow( 1 );
				m_commands.Back().m_first = 0.f;
			}

			m_commands.Back().m_second.PushBack( line );
		}
	}

	m_processed.Resize( m_commands.Size() );
	for ( Uint32 i=0; i<m_processed.Size(); ++i )
	{
		m_processed[ i ] = false;
	}
}

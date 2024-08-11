#include "build.h"
#include "steeringEditor.h"
#include "steeringGraphEditor.h"
#include "shortcutsEditor.h"
#include "assetBrowser.h"
#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/actor.h"


#define DEBUG_SAMPLE_FADE_RATE		10


// Event table
BEGIN_EVENT_TABLE( CEdSteeringEditor, wxFrame )
EVT_MENU( XRCID( "editDelete" ), CEdSteeringEditor::OnEditDelete )
EVT_MENU( XRCID( "templateSave" ), CEdSteeringEditor::OnSave )
EVT_MENU( XRCID( "viewZoomAll" ), CEdSteeringEditor::OnZoomAll )
END_EVENT_TABLE()

TDynArray< CEdSteeringEditor * > CEdSteeringEditor::s_editors;

CEdSteeringEditor::CEdSteeringEditor( wxWindow* parent, const THandle< CMovingAgentComponent >& mac )
	: m_template( NULL )
	, m_graphEditor( NULL )
	, m_mac( mac )
{
	CMovingAgentComponent* pMac = m_mac.Get();
	if ( pMac )
	{
		pMac->SetSteeringBehaviorListener( this );
		m_template = pMac->GetCurrentSteeringBehavior();
	}

	wxXmlResource::Get()->LoadFrame( this, parent, wxT("SteeringEditor") );

	Initialize();
}

CEdSteeringEditor::CEdSteeringEditor( wxWindow* parent, CMoveSteeringBehavior* steering )
	: m_template( steering )
	, m_graphEditor( NULL )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("SteeringEditor") );

	Initialize();
}

void CEdSteeringEditor::Initialize()
{
	// Load icons
	m_btIcon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_CAR") );
	m_enableRuntimeTrackingBmp = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONNECT") );
	m_disableRuntimeTrackingBmp = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DISCONNECT") );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( m_btIcon );
	SetIcon( iconSmall );

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdSteeringEditor::OnPropertiesChanged ), NULL, this );
		m_properties->Get().Connect( wxEVT_COMMAND_REFRESH_PROPERTIES, wxCommandEventHandler( CEdSteeringEditor::OnPropertiesRefresh ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Graph editor
	{
		wxPanel* rp = XRCCTRL( *this, "GraphPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_graphEditor = new CEdSteeringGraphEditor( rp, this );
		sizer1->Add( m_graphEditor , 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
		m_graphEditor->SetEditor( this );
	}

	// toolbar
	m_toolBar = XRCCTRL( *this, "toolBar", wxToolBar );
	m_toolBar->Connect( XRCID( "learn" ), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdSteeringEditor::OnRuntimeData ), NULL, this );
	m_toolBar->EnableTool( XRCID( "learn" ), m_mac.Get() != NULL );
	m_toolBar->SetToolNormalBitmap( XRCID( "learn" ), m_enableRuntimeTrackingBmp );
	m_toolBar->SetToolShortHelp( XRCID( "learn" ), wxT( "Start runtime tracking" ) );


	// Update and finalize layout
	Layout();
	LoadOptionsFromConfig();
	Show();

	// Set graph
	m_template->AddToRootSet();
	m_graphEditor->SetSteering( m_template );

	// Set title for newly created window
	String newTitle = m_template->GetFile()->GetFileName()
		+ TXT(" - ") + m_template->GetFile()->GetDepotPath()
		+ TXT(" - Steering Editor");
	SetTitle( wxString(newTitle.AsChar()) );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	OnGraphSelectionChanged();

	s_editors.PushBackUnique( this );
}

CEdSteeringEditor::~CEdSteeringEditor()
{
	CMovingAgentComponent* pMac = m_mac.Get();
	if ( pMac )
	{
		pMac->SetSteeringBehaviorListener( NULL );
	}

	s_editors.Remove( this );

	SaveOptionsToConfig();

	// Unregister listener
	SEvents::GetInstance().UnregisterListener( this );
}

wxString CEdSteeringEditor::GetShortTitle()
{
	return m_template->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Steering Editor"));
}

void CEdSteeringEditor::OnGraphSelectionChanged()
{
	// Get selected blocks
	const TDynArray< IScriptable* > & selection = m_graphEditor->GetSelectedObjects();

	// if nothing is selected, show properties of the whole graph
	if ( selection.Empty() )
	{
		TDynArray< IScriptable* > selection;
		selection.PushBack( m_template );
		m_properties->Get().SetObjects( selection );
	}
	else
	{
		m_properties->Get().SetObjects( selection );
	}
}

void CEdSteeringEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	m_graphEditor->ForceLayoutUpdate();
	m_graphEditor->Repaint();
}

void CEdSteeringEditor::OnPropertiesRefresh( wxCommandEvent& event )
{
	OnGraphSelectionChanged();
}

void CEdSteeringEditor::OnEditDelete( wxCommandEvent& event )
{
	m_graphEditor->DeleteSelection();
}

void CEdSteeringEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_template )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		if ( reloadInfo.m_newResource->IsA< CMoveSteeringBehavior >() )
		{
			CMoveSteeringBehavior* oldTemplate = SafeCast< CMoveSteeringBehavior >( reloadInfo.m_oldResource );
			CMoveSteeringBehavior* newTemplate = SafeCast< CMoveSteeringBehavior >( reloadInfo.m_newResource );
			if ( oldTemplate == m_template )
			{
				m_template = newTemplate;
				m_template->AddToRootSet();

				m_graphEditor->SetSteering( m_template );

				OnGraphSelectionChanged();

				wxTheFrame->GetAssetBrowser()->OnEditorReload( newTemplate, this );
			}
		}
	}
}

void CEdSteeringEditor::OnSave( wxCommandEvent& event )
{
	m_template->Save();
}

void CEdSteeringEditor::OnZoomAll( wxCommandEvent& event )
{
	m_graphEditor->ZoomExtents();
}

void CEdSteeringEditor::OnRuntimeData( wxCommandEvent& event )
{
	Bool enableRuntimeTracking = false;
	m_toolBar->SetToolNormalBitmap( XRCID( "learn" ), enableRuntimeTracking ? m_disableRuntimeTrackingBmp : m_enableRuntimeTrackingBmp );
	m_toolBar->SetToolShortHelp( XRCID( "learn" ), enableRuntimeTracking ? wxT( "Stop runtime tracking" ) : wxT( "Start runtime tracking" ) );
}

void CEdSteeringEditor::OnFrameStart( const SMoveLocomotionGoal& goal )
{
	m_activeGoal = goal;

	m_graphEditor->ForceLayoutUpdate();
	m_graphEditor->Repaint();

	for ( THashMap< const IMoveSteeringNode*, Int32 >::iterator it = m_activationTime.Begin(); it != m_activationTime.End(); ++it )
	{
		Int32 val = it.Value() - DEBUG_SAMPLE_FADE_RATE;
		it.Value() = ::Max< Int32 >( 0, val );
	}

	m_frames.Clear();
}

void CEdSteeringEditor::OnNodeActivation( const IMoveSteeringNode& node )
{
	THashMap< const IMoveSteeringNode*, Int32 >::iterator it = m_activationTime.Find( &node );
	if ( it != m_activationTime.End() )
	{
		it.Value() = 255;
	}
	else
	{
		m_activationTime.Insert( &node, 255 );
	}
}

Uint32 CEdSteeringEditor::AddFrame( const Char* title )
{
	Uint32 frameIdx = m_frames.Size();

	m_frames.PushBack( TDynArray< String >() );
	m_frames.Back().PushBack( title );

	return frameIdx;
}

void CEdSteeringEditor::AddText( Uint32 frameIdx, const Char* format, ... )
{
	Char buf[ 4096 ];
	va_list argptr;
	va_start( argptr, format );
	Red::System::VSNPrintF( buf, ARRAY_COUNT( buf ), format, argptr );
	va_end( argptr ); 

	m_frames[ frameIdx ].PushBack( buf );
}

wxColour CEdSteeringEditor::GetActivationColour( const IMoveSteeringNode& node ) const
{
	THashMap< const IMoveSteeringNode*, Int32 >::const_iterator it = m_activationTime.Find( &node );
	if ( it != m_activationTime.End() )
	{
		Uint8 val = it.Value();
		return wxColour( val, val, val );
	}
	else
	{
		return wxColour( 0, 0, 0 );
	}
}

void CEdSteeringEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* mainSplitter = XRCCTRL( *this, "MainSplitter", wxSplitterWindow );

	config.Write( TXT("/Frames/QuestEditor/MainSplitterPosition"), mainSplitter->GetSashPosition() );
}

void CEdSteeringEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* mainSplitter = XRCCTRL( *this, "MainSplitter", wxSplitterWindow );

	int pos = config.Read( TXT("/Frames/QuestEditor/MainSplitterPosition"), mainSplitter->GetSashPosition() );
	mainSplitter->SetSashPosition( pos );
}

///////////////////////////////////////////////////////////////////////////////

void StartSteeringTreeDebug( CActor* actor )
{
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}

	THandle< CMovingAgentComponent > macHandle( mac );
	CEdSteeringEditor* editor = new CEdSteeringEditor( wxTheFrame, macHandle );
	editor->Center();
	editor->Show();
	editor->SetFocus();
}
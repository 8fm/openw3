#include "build.h"

#include "behaviorEditorGhostsPanel.h"
#include "behaviorEditor.h"
#include "behaviorPreviewPanel.h"
#include "../../common/engine/dynamicLayer.h"

wxBEGIN_EVENT_TABLE( CEdBehaviorEditorGhostsPanel, CEdBehaviorEditorSimplePanel )
	EVT_TOOL( XRCID( "Disable" ), CEdBehaviorEditorGhostsPanel::OnDisable )
	EVT_TOOL( XRCID( "EnablePreview" ), CEdBehaviorEditorGhostsPanel::OnDisplayInPreview )
	EVT_TOOL( XRCID( "EnableGame" ), CEdBehaviorEditorGhostsPanel::OnDisplayInGame )
wxEND_EVENT_TABLE()

#define DEFAULT_FRAME_DELTA ( 1.0f / 30.0f )
#define MAX_FRAME_DELTA ( 1.0f )

CEdBehaviorEditorGhostsPanel::CEdBehaviorEditorGhostsPanel( CEdBehaviorEditor* editor )
:	CEdBehaviorEditorSimplePanel( editor ),
	m_type( PreviewGhostContainer::GhostType_Entity ),
	m_numberOfInstances( 10 ),
	m_entity( NULL )
{
//	VERIFY( wxXmlResource::Get()->LoadPanel( this, editor, wxT( "BehaviourEditorGhostsPanel" ) ) );

	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT( "BehaviourEditorGhostsPanel" ) );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );	
	Layout();

	// Extract widgets
	wxChoice* ghostTypeWidget = XRCCTRL( *innerPanel, "GhostType", wxChoice );
	ASSERT( ghostTypeWidget != NULL, TXT( "GhostType not defined in AnimationGhostConfig in editor_beh XRC" ) );

	wxSpinCtrl* ghostNumberWidget = XRCCTRL( *innerPanel, "NumberOfGhosts", wxSpinCtrl );
	ASSERT( ghostNumberWidget != NULL, TXT( "NumberOfGhosts not defined in AnimationGhostConfig in editor_beh XRC" ) );

	ghostTypeWidget->SetSelection( m_type );
	ghostNumberWidget->SetValue( m_numberOfInstances );

	ghostTypeWidget->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdBehaviorEditorGhostsPanel::OnTypeSelected, this );
	ghostNumberWidget->Bind( wxEVT_COMMAND_SPINCTRL_UPDATED, &CEdBehaviorEditorGhostsPanel::OnNumberOfInstancesChanged, this );

	m_frameDeltaCtrl = XRCCTRL( *innerPanel, "FrameDeltaCtrl", wxSlider );
	ASSERT( m_frameDeltaCtrl != NULL, TXT( "FrameDeltaCtrl not defined in AnimationGhostConfig in editor_beh XRC" ) );

	m_frameDeltaDisplay = XRCCTRL( *innerPanel, "FrameDeltaDisplay", wxStaticText );
	ASSERT( m_frameDeltaDisplay != NULL, TXT( "FrameDeltaDisplay not defined in AnimationGhostConfig in editor_beh XRC" ) );

	m_frameDeltaCtrl->Bind( wxEVT_COMMAND_SLIDER_UPDATED, &CEdBehaviorEditorGhostsPanel::OnFrameDeltaChange, this );

	ChangeFrameDelta( DEFAULT_FRAME_DELTA );
	m_frameDeltaCtrl->SetValue( static_cast< Int32 >( DEFAULT_FRAME_DELTA / MAX_FRAME_DELTA ) * m_frameDeltaCtrl->GetMax() );
}

CEdBehaviorEditorGhostsPanel::~CEdBehaviorEditorGhostsPanel()
{

}

void CEdBehaviorEditorGhostsPanel::OnNumberOfInstancesChanged( wxSpinEvent& event )
{
	m_numberOfInstances = static_cast< Uint32 >( event.GetValue() );

	if( m_ghostContainer.HasGhosts() )
	{
		m_ghostContainer.InitGhosts( m_numberOfInstances, m_type );
	}
	else if( GetEditor()->GetPreviewPanel()->HasGhosts() )
	{
		GetEditor()->GetPreviewPanel()->ShowGhosts( m_numberOfInstances, m_type );
	}
}

void CEdBehaviorEditorGhostsPanel::OnTypeSelected( wxCommandEvent& event )
{
	m_type = static_cast< PreviewGhostContainer::EGhostType >( event.GetInt() );

	if( m_ghostContainer.HasGhosts() )
	{
		m_ghostContainer.InitGhosts( m_numberOfInstances, m_type );
	}
	else if( GetEditor()->GetPreviewPanel()->HasGhosts() )
	{
		GetEditor()->GetPreviewPanel()->ShowGhosts( m_numberOfInstances, m_type );
	}
}

void CEdBehaviorEditorGhostsPanel::OnDisable( wxCommandEvent& event )
{
	m_entity = NULL;
	m_ghostContainer.DestroyGhosts();

	GetEditor()->GetPreviewPanel()->HideGhosts();
}

void CEdBehaviorEditorGhostsPanel::OnDisplayInPreview( wxCommandEvent& event )
{
	m_entity = NULL;
	m_ghostContainer.DestroyGhosts();

	GetEditor()->GetPreviewPanel()->ShowGhosts( m_numberOfInstances, m_type );
}

void CEdBehaviorEditorGhostsPanel::OnDisplayInGame( wxCommandEvent& event )
{
	GetEditor()->GetPreviewPanel()->HideGhosts();

	m_entity = GetEntity();
	m_ghostContainer.InitGhosts( m_numberOfInstances, m_type );
}

void CEdBehaviorEditorGhostsPanel::OnTick( Float dt )
{
	if( m_entity )
	{
		CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();

		m_ghostContainer.UpdateGhosts( dt, ac );
	}
}

void CEdBehaviorEditorGhostsPanel::OnFrameDeltaChange( wxCommandEvent& event )
{
	Float dt = ( static_cast< Float >( event.GetInt() ) / static_cast< Float >( m_frameDeltaCtrl->GetMax() ) ) * MAX_FRAME_DELTA;
	ChangeFrameDelta( dt );
}

void CEdBehaviorEditorGhostsPanel::ChangeFrameDelta( Float dt )
{
	m_ghostContainer.SetTimeStep( dt );

	wxString display;
	display.Printf( wxT( "% .3f (%i fps)" ), dt, static_cast< Int32 >( 1.0f / dt ) );

	m_frameDeltaDisplay->SetLabel( display );
}

//////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE( CEdBehaviorEditorScriptPanel, CEdBehaviorEditorSimplePanel )
	EVT_TOOL( XRCID( "buttConnect" ), CEdBehaviorEditorScriptPanel::OnConnect )
wxEND_EVENT_TABLE()

CEdBehaviorEditorScriptPanel::CEdBehaviorEditorScriptPanel( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_component( NULL )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT( "BehaviorEditorGraphScriptPanel" ) );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );	
	Layout();
}

CEdBehaviorEditorScriptPanel::~CEdBehaviorEditorScriptPanel()
{

}

void CEdBehaviorEditorScriptPanel::OnConnect( wxCommandEvent& event )
{
	Bool ret = event.IsChecked();
	if ( ret )
	{
		SpawnEntity();

		SendEvent( TXT("OnEditorEnabled"), true );
	}
	else
	{
		SendEvent( TXT("OnEditorDisabled") );

		DespawnEntity();
	}
}

void CEdBehaviorEditorScriptPanel::OnTick( Float dt )
{
	SendEvent( TXT("OnTick") );
}

void CEdBehaviorEditorScriptPanel::OnReset()
{
	InternalReload();
}

void CEdBehaviorEditorScriptPanel::OnLoadEntity()
{
	InternalReload();
}

void CEdBehaviorEditorScriptPanel::OnInstanceReload()
{
	InternalReload();
}

void CEdBehaviorEditorScriptPanel::OnGraphModified()
{
	InternalReload();
}

void CEdBehaviorEditorScriptPanel::InternalReload()
{
	SendEvent( TXT("OnEditorDisabled") );
	SendEvent( TXT("OnEditorEnabled"), true );
}

void CEdBehaviorEditorScriptPanel::SpawnEntity()
{
	if ( GetEditor()->GetEntity() )
	{
		CLayer* layer = GetEditor()->GetEntity()->GetLayer()->GetWorld()->GetDynamicLayer();

		EntitySpawnInfo info;
		info.m_name = String::Printf( TXT( "CEdBehaviorEditorScriptPanel_%d" ), ( Int32 ) this );
		CEntity* entity = layer->CreateEntitySync( info );
		if ( entity )
		{
			CClass* cclass = SRTTI::GetInstance().FindClass( CName( TXT("W3DebugScriptBehaviorToolComponent") ) );
			m_component = entity->CreateComponent( cclass, SComponentSpawnInfo() );
		}
	}
}

void CEdBehaviorEditorScriptPanel::DespawnEntity()
{
	if ( m_component )
	{
		m_component->GetEntity()->Destroy();
		m_component = NULL;
	}
}

void CEdBehaviorEditorScriptPanel::SendEvent( const String& str, Bool withEnt )
{
	if ( m_component )
	{
		CName evtName( str );

		if ( withEnt )
		{
			if ( GetEntity() )
			{
				THandle< CEntity > entH( GetEntity() );
				m_component->CallEvent( evtName, entH );
			}
		}
		else
		{
			m_component->CallEvent( evtName );
		}
	}
}

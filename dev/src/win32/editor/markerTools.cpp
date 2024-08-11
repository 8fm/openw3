/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "markerTools.h"
#include "poiEditor.h"
#include "reviewEditor.h"
#include "stickersEditor.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/markersSystem.h"
#include "../../common/core/configFileManager.h"

IMPLEMENT_ENGINE_CLASS( CEdMarkersEditor );

Bool CEdMarkersEditor::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	if( GIsEditorGame == true || GIsGame == true )
	{
		GFeedback->ShowWarn( TXT("If game is running you should use marker tools from debug windows.") );
		return false;
	}

	// No world
	if ( !world )
	{
		return false;
	}

	// Create tool panel
	m_markersPanel = new CEdGeneralMarkersPanel( panel );

	// Create panel for custom window
	panelSizer->Add( m_markersPanel, 1, wxEXPAND, 5 );

	GEngine->GetMarkerSystems()->TurnOnSystems();
	GEngine->GetMarkerSystems()->SendRequest( MSRT_UpdateData );

	// Start tool
	return true;
}

void CEdMarkersEditor::End()
{
	GEngine->GetMarkerSystems()->TurnOffSystems();
}

void CEdMarkersEditor::OpenPOIsEditor( const String& name )
{ 
	if ( m_markersPanel )
	{
		m_markersPanel->OpenPOIsEditor( name ); 
	}
}

CEdGeneralMarkersPanel::CEdGeneralMarkersPanel(wxWindow* parent) 
	: m_reviewFlagTool(NULL)
	, m_poiTool(NULL)
	, m_stickerTool(NULL)
{
	// Load layouts from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("GeneralPanel") );

	m_mainNotebook = XRCCTRL( *this, "mainNotebook", wxNotebook );
	ASSERT(m_mainNotebook != NULL);
	m_mainNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler( CEdGeneralMarkersPanel::OnSelectTab ), NULL, this );

	// Add marker tools
	m_reviewFlagTool = new CEdReviewPanel(m_mainNotebook);
	m_mainNotebook->AddPage(m_reviewFlagTool, "Review flags", false);

	m_poiTool = new CEdPOIPanel(m_mainNotebook);
	m_mainNotebook->AddPage(m_poiTool, "POI", false);

	m_stickerTool = new CEdStickersPanel(m_mainNotebook);
	m_mainNotebook->AddPage(m_stickerTool, "Stickers", false);

	Int32 activeTab = m_mainNotebook->GetPageCount()-1;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("MarkersEditor"), TXT("ActiveTab"), activeTab );
	m_mainNotebook->SetSelection(activeTab);

	m_detachablePanel.Initialize( this, TXT( "Markers" ) );

	SEvents::GetInstance().RegisterListener( CNAME( NodeSelected ), this );
}

CEdGeneralMarkersPanel::~CEdGeneralMarkersPanel()
{
	SEvents::GetInstance().UnregisterListener( CNAME( NodeSelected ), this );
}

void CEdGeneralMarkersPanel::OnSelectTab( wxCommandEvent& event )
{
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("MarkersEditor"), TXT("ActiveTab"), m_mainNotebook->GetSelection());
}

void CEdGeneralMarkersPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( NodeSelected ) )
	{
		CEntity* entity = Cast< CEntity >( GetEventData< CNode* >( data ) );
		if( entity != nullptr )
		{
			if( entity->GetTags().HasTag( CNAME(ReviewFlagObject) ) == true )
			{
				m_mainNotebook->SetSelection(0);
			}
			else if( entity->GetTags().HasTag( CNAME(PointObject) ) == true )
			{
				m_mainNotebook->SetSelection(1);
			}
			else if( entity->GetTags().HasTag( CNAME(StickerObject) ) == true )
			{
				m_mainNotebook->SetSelection(2);
			}
		}
	}
}

void CEdGeneralMarkersPanel::OpenPOIsEditor( const String& name )
{
	if ( m_mainNotebook )
	{
		m_mainNotebook->SetSelection( 1 );
		m_poiTool->SelectPOI( name );
	}
}



#endif	// NO_MARKER_SYSTEMS

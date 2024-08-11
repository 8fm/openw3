/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "encounterEditTool.h"
#include "cascadePropertyEditor.h"
#include "..\..\common\game\encounter.h"
#include "toolsPanel.h"
#include "frame.h"
#include "..\..\common\engine\hitProxyObject.h"

IMPLEMENT_ENGINE_CLASS( CEdEncounterEditTool );

CEdEncounterEditTool::CEdEncounterEditTool()
	: m_editWindow( NULL )
{

}

String CEdEncounterEditTool::GetCaption() const
{
	return TXT( "Encounter edit" );
}

Bool CEdEncounterEditTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_editWindow = new CEdEncounterEditWindow( panel );
	

	if ( selection.Empty() == false )
	{
		CComponent* component = selection[ 0 ];
		if ( component != NULL )
		{
			CEncounter* encounter = component->FindParent< CEncounter >();
			if ( encounter != NULL )
			{
				m_editWindow->SetEncounter( encounter );
			}
		}
	}


	m_editWindow->Show();

	return true;
}

void CEdEncounterEditTool::End()
{
	if ( m_editWindow != NULL )
	{
		m_editWindow->Close( true );
		delete m_editWindow;
		m_editWindow = NULL;
	}
}

Bool CEdEncounterEditTool::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	if ( objects.Empty() == false && objects[ 0 ] != NULL )
	{
		CObject* hitObject = objects[ 0 ]->GetHitObject();
		if ( hitObject != NULL )
		{
			CEncounter* encounter = hitObject->FindParent< CEncounter >();
			if ( encounter != NULL )
			{
				m_editWindow->SetEncounter( encounter );
			}
		}
	}

	return false;
}

Bool CEdEncounterEditTool::UsableInActiveWorldOnly() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////

CEdEncounterEditWindow::CEdEncounterEditWindow( wxWindow* parent )
{
	Create( parent, wxID_ANY, "Encounter edit", wxDefaultPosition, wxSize( 800, 600 ), wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	m_propertyEditor = new CEdCascadePropertyEditor( this, true, true );

	sizer->Add( m_propertyEditor, 1, wxEXPAND );

	LoadOptionsFromConfig();

	Bind( wxEVT_CLOSE_WINDOW, &CEdEncounterEditWindow::OnClose, this );

	SetSizer( sizer );
	Layout();
}

CEdEncounterEditWindow::~CEdEncounterEditWindow()
{
	SaveOptionsToConfig();
	if ( m_propertyEditor != NULL )
	{
		delete m_propertyEditor;
		m_propertyEditor = NULL;
	}
}

void CEdEncounterEditWindow::SetEncounter( CEncounter* encounter )
{
	if ( m_propertyEditor != NULL )
	{
		m_propertyEditor->SetObject( encounter );
	}
	
}

void CEdEncounterEditWindow::SaveSession( CConfigurationManager &config )
{
	config.Write( TXT( "Tools/EncounterEdit/PosX" ), GetPosition().x );
	config.Write( TXT( "Tools/EncounterEdit/PosY" ), GetPosition().y );
	config.Write( TXT( "Tools/EncounterEdit/SizeX" ), GetSize().x );
	config.Write( TXT( "Tools/EncounterEdit/SizeY" ), GetSize().y );
}

void CEdEncounterEditWindow::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CEdEncounterEditWindow::RestoreSession( CConfigurationManager &config )
{
	wxSize windowSize;
	wxPoint windowPosition;

	windowPosition.x = config.Read( TXT( "Tools/EncounterEdit/PosX" ), 100 );
	windowPosition.y = config.Read( TXT( "Tools/EncounterEdit/PosY" ), 100 );
	windowSize.x = config.Read( TXT( "Tools/EncounterEdit/SizeX" ), 900 );
	windowSize.y = config.Read( TXT( "Tools/EncounterEdit/SizeY" ), 600 );

	SetSize( windowSize );
	SetPosition( windowPosition );
}

void CEdEncounterEditWindow::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

void CEdEncounterEditWindow::OnClose( wxCloseEvent& event )
{
	RunLaterOnce( [](){ wxTheFrame->GetToolsPanel()->CancelTool(); } );
	event.Veto();
}

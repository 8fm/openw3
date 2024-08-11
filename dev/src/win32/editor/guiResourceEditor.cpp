/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "guiResourceEditor.h"
#include "guiResourceGraphEditor.h"
#include "../../common/game/guiResource.h"
#include "../../common/game/guiObject.h"
#include "../../common/core/diskFile.h"

BEGIN_EVENT_TABLE( CEdGuiResourceEditor, wxFrame )
	EVT_MENU( XRCID( "menuItemSave" ), CEdGuiResourceEditor::OnSave )
END_EVENT_TABLE()

CEdGuiResourceEditor::CEdGuiResourceEditor( wxWindow* parent, IGuiResource* guiResource /*=nullptr*/ )
	: m_guiResource( guiResource )
{
	//tmp!
	if ( ! m_guiResource )
	{
		return;
	}

	//
	m_guiResource->AddToRootSet();

	// Load window
	wxXmlResource::Get()->LoadFrame( this, parent, wxT( "GUIEditor" ) );

	SetSize( 800, 600 );

	// Set title
	SetTitle( wxString::Format( wxT("%s - GUI Desc Editor [%s]"), 
		m_guiResource->GetFile()->GetFileName().AsChar(), m_guiResource->GetDepotPath().AsChar() ) );

	// Node properties panel
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		ASSERT( rp, TXT("Properties panel placeholder is missing") );
		PropertiesPageSettings settings;
		settings.m_autoExpandGroups = true;
		m_nodeProperties = new CEdPropertiesPage( rp, settings, nullptr );
		m_nodeProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdGuiResourceEditor::OnPropertiesChanged ), NULL, this );
		m_nodeProperties->SetObject( m_guiResource );
		rp->GetSizer()->Add( m_nodeProperties, 1, wxEXPAND | wxALL, 0 );
		rp->Layout();
	}

	// Graph editor
	{
		wxPanel* rp = XRCCTRL( *this, "GraphPanel", wxPanel );
		ASSERT( rp, TXT("Graph panel placeholder is missing") );
		m_graphEditor = new CEdGuiResourceGraphEditor( rp, this );
		rp->GetSizer()->Add( m_graphEditor, 1, wxEXPAND | wxALL );
		rp->Layout();
	}

	m_graphEditor->SetHook( this );

	m_graphEditor->SetGraph( m_guiResource );

	// Splitter
	{
		wxSplitterWindow* splitter = XRCCTRL( *this, "m_splitter1", wxSplitterWindow );
		ASSERT( splitter, TXT("Splitter is missing") );
		splitter->SetSashPosition( splitter->GetSize().GetWidth() - 320 );
	}

	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
}

//////////////////////////////////////////////////////////////////////////

CEdGuiResourceEditor::~CEdGuiResourceEditor()
{
	if ( m_guiResource )
	{
		m_guiResource->RemoveFromRootSet();
	}
	SEvents::GetInstance().UnregisterListener( this );
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceEditor::OnGraphStructureModified( IGraphContainer* graph )
{
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceEditor::OnGraphSelectionChanged()
{
	TDynArray< CGraphBlock* > selectedBlocks;
	m_graphEditor->GetSelectedBlocks( selectedBlocks );
	if ( ! selectedBlocks.Empty() )
	{
		m_nodeProperties->SetObject( selectedBlocks[ selectedBlocks.Size() - 1 ] );
	}
	else
	{
		//CHANGEME: Make a separate panel for this instead of doing it this way...
		m_nodeProperties->SetObject( m_guiResource );
		//m_nodeProperties->SetNoObject();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceEditor::OnPropertiesChanged( wxCommandEvent& event )
{
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceEditor::OnSave( wxCommandEvent& event )
{
	if ( ! m_guiResource->Save() )
	{
		ERR_EDITOR(TXT("Failed to save file: %s"), m_guiResource->GetDepotPath().AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdGuiResourceEditor::SetPropertyGraphBaseItem( ISerializable* item )
{
// 	if ( ! item || ! item->IsA< CGuiGraphBlock >() )
// 	{
// 		m_nodeProperties->SetNoObject();
// 	}
// 	else
// 	{
// 		//TODO: The read-only index isn't updated until we lazily walk the graph.
// 		// Any way to just hide it from the property inspector?
// 		CGuiGraphBlock* guiGraphBlock = SafeCast< CGuiGraphBlock >( item );
// 		m_nodeProperties->SetObject( guiGraphBlock );
// 	}
}

void CEdGuiResourceEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	typedef CEdPropertiesPage::SPropertyEventData SEventData;
	const SEventData& eventData = GetEventData< SEventData >( data );
	if ( name == CNAME( EditorPropertyPreChange ) )
	{
	//	if ( eventData.m_object && eventData.m_object->GetClass()->IsA< IGuiResource >()
	//		&& eventData.m_property && eventData.m_property->GetName() == TXT("guiModuleGroupTemplate") )
	//	{
		//	m_guiResource->GraphGetBlocks().Clear();
	//	}
	}
}

//////////////////////////////////////////////////////////////////////////

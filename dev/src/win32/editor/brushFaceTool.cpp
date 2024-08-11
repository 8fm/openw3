/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "brushFaceTool.h"
#include "../../common/engine/brushFace.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/hitProxyMap.h"

IMPLEMENT_ENGINE_CLASS( CEdBrushFaceEdit );

wxIMPLEMENT_CLASS( CBrushFaceToolPanel, CEdDraggablePanel );

// A little helper
Bool GBrushFaceEditMode = false;

CEdBrushFaceEdit::CEdBrushFaceEdit()
	: m_world( NULL )
	, m_viewport( NULL )
	, m_panel( NULL )
{
}

String CEdBrushFaceEdit::GetCaption() const
{
	return TXT("Brush faces");
}

Bool CEdBrushFaceEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;

	// Create tool panel
	m_panel = new CBrushFaceToolPanel( panel, this );

	// Create panel for custom window
	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();

	/*// Clear selection flag on all brush faces
	for ( CObjectIterator< CBrushFace > i; i; ++i )
	{
		i->SetSelected( false );
	}*/

	// Clear list of selected faces
	m_selectedFaces.Clear();

	// Select faces from brush components
/*	for ( CObjectIterator< CBrushFace > i; i; ++i )
	{
		if ( selection.Exist( i->GetBrush() ) )
		{
			SelectFace( *i );
		}
	}
*/
	// Enable face edit mode
	GBrushFaceEditMode = true;

	// Update properties
	m_panel->UpdateFaceProperties();

	// Created
	return true;
}

void CEdBrushFaceEdit::End()
{
	// Disable face edit mode
	GBrushFaceEditMode = false;
}

Bool CEdBrushFaceEdit::OnDelete()
{
	// No deleting of anything
	return true;
}

Bool CEdBrushFaceEdit::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	return false;
}

Bool CEdBrushFaceEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	return false;
}

Bool CEdBrushFaceEdit::OnViewportMouseMove( const CMousePacket& packet )
{
	return false;
}

Bool CEdBrushFaceEdit::OnViewportTrack( const CMousePacket& packet )
{
	return false;
}

Bool CEdBrushFaceEdit::OnViewportTick( IViewport* view, Float timeDelta )
{
	return false;
}

Bool CEdBrushFaceEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		DeselectAllFaces();
	}

	// Select faces only
	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CBrushFace* tc = Cast< CBrushFace >( objects[i]->GetHitObject() );
		if ( tc )
		{
			if ( tc->IsSelected() )
			{
				DeselectFace( tc );
			}
			else
			{
				SelectFace( tc );
			}
		}
	}

	// Update properties
	m_panel->UpdateFaceProperties();

	// Handled
	return true;
}

Bool CEdBrushFaceEdit::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	// Check if we have clicked brush face
	CHitProxyMap map;
	CHitProxyObject* object = m_viewport->GetHitProxyAtPoint( map, x, y );
	
	// Get clicked brush  face
	CBrushFace* clickedFace = object ? Cast< CBrushFace >( object->GetHitObject() ) : NULL;
	if ( clickedFace )
	{
		// Copy settings ( mapping and material )from current selection
		if ( m_selectedFaces.Size() )
		{
			clickedFace->SetMapping( m_selectedFaces[0]->GetMapping() );
			clickedFace->SetMaterial( m_selectedFaces[0]->GetMaterial() );
		}
	}

	// Handled
	return true;
}

void CEdBrushFaceEdit::DeselectAllFaces()
{
	// Deselect all faces
	for ( Uint32 i=0; i<m_selectedFaces.Size(); i++ )
	{
		ASSERT( m_selectedFaces[i]->IsSelected() );
		m_selectedFaces[i]->SetSelection( false );
	}

	// Clear list
	m_selectedFaces.Clear();
}

void CEdBrushFaceEdit::SelectFace( CBrushFace* face )
{
	// Select face
	ASSERT( face );
	ASSERT( !face->IsSelected() );
	face->SetSelection( true );
	ASSERT( !m_selectedFaces.Exist( face ) );
	m_selectedFaces.PushBack( face );
}

void CEdBrushFaceEdit::DeselectFace( CBrushFace* face )
{
	ASSERT( face );
	ASSERT( face->IsSelected() );
	face->SetSelection( false );
	ASSERT( m_selectedFaces.Exist( face ) );
	m_selectedFaces.Remove( face );
}

Bool CEdBrushFaceEdit::HandleActionClick( Int32 x, Int32 y )
{
	return false;
}

CBrushFaceToolPanel::CBrushFaceToolPanel( wxWindow* parent, CEdBrushFaceEdit* tool )
	: m_tool( tool )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("BrushTool") );

	// Create properties browser
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CBrushFaceToolPanel::OnPropertiesUpdated ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_detachablePanel.Initialize( this, TXT( "Brush Faces" ) );
}

void CBrushFaceToolPanel::UpdateFaceProperties()
{
	// Update properties
	m_properties->Get().SetObjects( ( const TDynArray< CObject* >& ) m_tool->m_selectedFaces );
}

void CBrushFaceToolPanel::OnPropertiesUpdated( wxCommandEvent& event )
{
	for ( Uint32 i=0; i<m_tool->m_selectedFaces.Size(); i++ )
	{
		// Update face mapping and material
		CBrushFace* face = m_tool->m_selectedFaces[i];
		face->SetMapping( face->GetMapping() );
		face->SetMaterial( face->GetMaterial() );
	}
}

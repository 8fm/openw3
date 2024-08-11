/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "environmentPointEdit.h"
#include "../../common/core/depot.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/environmentDefinition.h"

wxIMPLEMENT_CLASS( CEdEnvironmentPointEditPanel, wxPanel );

IMPLEMENT_ENGINE_CLASS( CEdEnvironmentPointEdit );

// CEdEnvironmentPointEditPanel
CEdEnvironmentPointEditPanel::CEdEnvironmentPointEditPanel( wxWindow* parent, class CEdEnvironmentPointEdit* editor )
{
	m_editor = editor;

	// Load resource for this panel
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("EnvironmentPointEditorPanel") );

	// Get widgets
	m_pointsList = XRCCTRL( *this, "m_pointsList", wxChoice );
	m_pointType = XRCCTRL( *this, "m_pointType", wxChoice );
	m_pointBlend = XRCCTRL( *this, "m_pointBlend", wxChoice );
	m_blendScale = XRCCTRL( *this, "m_blendScale", wxSlider );
	m_dirAngle1 = XRCCTRL( *this, "m_dirAngle1", wxSlider );
	m_dirAngle2 = XRCCTRL( *this, "m_dirAngle2", wxSlider );
	m_outerRadius = XRCCTRL( *this, "m_outerRadius", wxSlider );
	m_innerRadius = XRCCTRL( *this, "m_innerRadius", wxSlider );
	m_scaleX = XRCCTRL( *this, "m_scaleX", wxSpinCtrl );
	m_scaleY = XRCCTRL( *this, "m_scaleY", wxSpinCtrl );
	m_scaleZ = XRCCTRL( *this, "m_scaleZ", wxSpinCtrl );
	m_envPath = XRCCTRL( *this, "m_envPath", wxTextCtrl );

	// Install events
	m_pointsList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnPointListSelected ), NULL, this );
	XRCCTRL( *this, "m_addPoint", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnAddPointClicked ), NULL, this );
	XRCCTRL( *this, "m_clonePoint", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnClonePointClicked ), NULL, this );
	XRCCTRL( *this, "m_deletePoint", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnRemovePointClicked ), NULL, this );
	m_pointType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnPointTypeSelected ), NULL, this );
	m_pointBlend->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnPointBlendSelected ), NULL, this );
	m_blendScale->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnBlendScaleChanged ), NULL, this );
	m_dirAngle1->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnDirAngle1Scroll ), NULL, this );
	m_dirAngle2->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnDirAngle2Scroll ), NULL, this );
	m_outerRadius->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnOuterRadiusScroll ), NULL, this );
	XRCCTRL( *this, "m_outerToInner", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnOuterToInnerClicked ), NULL, this );
	m_innerRadius->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnInnerRadiusScroll ), NULL, this );
	m_scaleX->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnScaleXChanged ), NULL, this );
	m_scaleY->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnScaleYChanged ), NULL, this );
	m_scaleZ->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnScaleZChanged ), NULL, this );
	XRCCTRL( *this, "m_useEnvironment", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnUseEnvironmentClicked ), NULL, this );
	XRCCTRL( *this, "m_advanced", wxToggleButton )->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnAdvancedClicked ), NULL, this );

	// Create properties page
	wxPanel* advancedPropertiesPanel = XRCCTRL( *this, "m_advancedPropertiesPanel", wxPanel );
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	advancedPropertiesPanel->SetSizer( sizer );
	m_properties = new CEdPropertiesPage( advancedPropertiesPanel, PropertiesPageSettings(), nullptr );
	m_properties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEnvironmentPointEditPanel::OnPropertiesModified ), NULL, this );
	sizer->Add( m_properties, 1, wxEXPAND );

	// Update UI
	LoadValues();
	UpdateEnabledStates();
}

void CEdEnvironmentPointEditPanel::UpdateEnabledStates()
{
	Bool anglePoint = m_pointBlend->GetSelection() == AEPB_CameraAngleAndDistance;
	Bool subEnv = m_pointType->GetSelection() == AEPT_SubEnvironment;

	m_dirAngle1->Enable( anglePoint );
	m_dirAngle2->Enable( anglePoint );
	m_envPath->Enable( subEnv );
	XRCCTRL( *this, "m_useEnvironment", wxBitmapButton )->Enable( subEnv );
}

void CEdEnvironmentPointEditPanel::LoadValues()
{
	SAreaEnvironmentPoint& point = GetSelectedPoint();
	String positionAsString = ToString( point.m_position );

	m_pointType->SetSelection( point.m_type );
	m_pointBlend->SetSelection( point.m_blend );
	m_blendScale->SetValue( (int)( point.m_blendScale*1000.0f ) );
	m_dirAngle1->SetValue( point.m_direction.Yaw*10.0f );
	m_dirAngle2->SetValue( point.m_direction.Pitch*10.0f );
	m_outerRadius->SetValue( (int)( point.m_outerRadius*100.0f ) );
	m_innerRadius->SetValue( (int)( point.m_innerRadius*100.0f ) );
	m_scaleX->SetValue( (int)( point.m_scaleX*100.0f ) );
	m_scaleY->SetValue( (int)( point.m_scaleY*100.0f ) );
	m_scaleZ->SetValue( (int)( point.m_scaleZ*100.0f ) );
	m_envPath->SetValue( point.m_environmentDefinition ? point.m_environmentDefinition->GetDepotPath().AsChar() : wxEmptyString );

	m_properties->SetNoObject();
	m_properties->SetObject( &point );

	UpdateSelectedVertex();
}

void CEdEnvironmentPointEditPanel::UpdateEnvironmentArea()
{
	m_editor->m_areaComponent->NotifyPropertiesImplicitChange();
}

void CEdEnvironmentPointEditPanel::CreatePointChoices()
{
	// Save previous selection
	int itemIndex = m_pointsList->GetSelection();

	// Fill new points
	m_pointsList->Clear();
	for ( Uint32 i=0; i < m_editor->m_areaComponent->GetPoints().Size(); ++i )
	{
		m_pointsList->AppendString( wxString::Format( wxT("Point #%d"), (int)( i + 1 ) ) );
	}

	// Reselect a point
	if ( itemIndex == wxNOT_FOUND && m_pointsList->GetCount() >= 0 )
	{
		m_pointsList->SetSelection( 0 );
	}
	else
	{
		m_pointsList->SetSelection( itemIndex );
	}

	// Load the values for the point
	LoadValues();
	UpdateEnabledStates();
}

void CEdEnvironmentPointEditPanel::UpdateSelectedVertex()
{
	if ( m_pointsList->GetSelection() != wxNOT_FOUND )
	{
		int selection = m_pointsList->GetSelection();
		if ( selection < m_editor->m_vertices.SizeInt() )
		{
			GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
			GGame->GetActiveWorld()->GetSelectionManager()->Select( m_editor->m_vertices[m_pointsList->GetSelection()] );
		}
	}
}

void CEdEnvironmentPointEditPanel::OpenAdvancedPropertiesPanel()
{
	wxPanel* panel = XRCCTRL( *this, "m_advancedPropertiesPanel", wxPanel );
	XRCCTRL( *this, "m_advanced", wxToggleButton )->SetValue( true );
	panel->Show( true );
	Layout();
}

SAreaEnvironmentPoint& CEdEnvironmentPointEditPanel::GetSelectedPoint()
{
	static SAreaEnvironmentPoint notFound;
	int selection = m_pointsList->GetSelection();
	if ( selection >= 0 && selection < (int)m_editor->m_areaComponent->GetPoints().Size() )
	{
		return *const_cast< SAreaEnvironmentPoint* >( m_editor->m_areaComponent->GetPoints().TypedData() + selection );
	}
	return notFound;
}

void CEdEnvironmentPointEditPanel::OnPointListSelected( wxCommandEvent& event )
{
	LoadValues();
	UpdateSelectedVertex();
}

void CEdEnvironmentPointEditPanel::OnAddPointClicked( wxCommandEvent& event )
{
	m_editor->DestroyVertices();
	m_editor->m_areaComponent->AddPoint( SAreaEnvironmentPoint() );
	CreatePointChoices();
	m_editor->CreateVertices();
	m_pointsList->SetSelection( m_editor->m_areaComponent->GetPoints().SizeInt() - 1 );
	LoadValues();
	UpdateSelectedVertex();
}

void CEdEnvironmentPointEditPanel::OnClonePointClicked( wxCommandEvent& event )
{
	m_editor->DestroyVertices();
	m_editor->m_areaComponent->AddPoint( GetSelectedPoint() );
	CreatePointChoices();
	m_editor->CreateVertices();
	m_pointsList->SetSelection( m_editor->m_areaComponent->GetPoints().SizeInt() - 1 );
	LoadValues();
	UpdateSelectedVertex();
}

void CEdEnvironmentPointEditPanel::OnRemovePointClicked( wxCommandEvent& event )
{
	if ( m_pointsList->GetSelection() != wxNOT_FOUND )
	{
		m_editor->DestroyVertices();
		m_editor->m_areaComponent->RemovePoint( m_pointsList->GetSelection() );
		CreatePointChoices();
		m_editor->CreateVertices();
		UpdateSelectedVertex();
	}
}

void CEdEnvironmentPointEditPanel::OnPointTypeSelected( wxCommandEvent& event )
{
	GetSelectedPoint().m_type = (EAreaEnvironmentPointType)m_pointType->GetSelection();
	UpdateEnvironmentArea();
	UpdateEnabledStates();
}

void CEdEnvironmentPointEditPanel::OnPointBlendSelected( wxCommandEvent& event )
{
	GetSelectedPoint().m_blend = (EAreaEnvironmentPointBlend)m_pointBlend->GetSelection();
	UpdateEnvironmentArea();
	UpdateEnabledStates();
}

void CEdEnvironmentPointEditPanel::OnBlendScaleChanged( wxCommandEvent& event )
{
	GetSelectedPoint().m_blendScale = ((Float)m_blendScale->GetValue())/1000.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnDirAngle1Scroll( wxCommandEvent& event )
{
	GetSelectedPoint().m_direction.Yaw = ((Float)m_dirAngle1->GetValue())/10.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnDirAngle2Scroll( wxCommandEvent& event )
{
	GetSelectedPoint().m_direction.Pitch = ((Float)m_dirAngle2->GetValue())/10.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnOuterRadiusScroll( wxCommandEvent& event )
{
	if ( m_innerRadius->GetValue() > m_outerRadius->GetValue() )
	{
		m_innerRadius->SetValue( m_outerRadius->GetValue() );
		GetSelectedPoint().m_innerRadius = ((Float)m_innerRadius->GetValue())/100.0f;
	}
	GetSelectedPoint().m_outerRadius = ((Float)m_outerRadius->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnInnerRadiusScroll( wxCommandEvent& event )
{
	if ( m_innerRadius->GetValue() > m_outerRadius->GetValue() )
	{
		m_outerRadius->SetValue( m_innerRadius->GetValue() );
		GetSelectedPoint().m_outerRadius = ((Float)m_outerRadius->GetValue())/100.0f;
	}
	GetSelectedPoint().m_innerRadius = ((Float)m_innerRadius->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnScaleXChanged( wxCommandEvent& event )
{
	GetSelectedPoint().m_scaleX = ((Float)m_scaleX->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnScaleYChanged( wxCommandEvent& event )
{
	GetSelectedPoint().m_scaleY = ((Float)m_scaleY->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnScaleZChanged( wxCommandEvent& event )
{
	GetSelectedPoint().m_scaleZ = ((Float)m_scaleZ->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnUseEnvironmentClicked( wxCommandEvent& event )
{
	// Grab active resource
	String resPath;
	if ( !GetActiveResource( resPath, ClassID< CEnvironmentDefinition >() ) )
	{
		return;
	}

	// Attempt to load it as an environment definition
	CEnvironmentDefinition* envDef = Cast< CEnvironmentDefinition >( GDepot->LoadResource( resPath, ResourceLoadingContext() ) );
	if ( !envDef )
	{
		return;
	}

	// Update selected point's environment definition and put the path in the box
	GetSelectedPoint().m_environmentDefinition = envDef;
	m_envPath->SetValue( resPath.AsChar() );
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnAdvancedClicked( wxCommandEvent& event )
{
	wxPanel* panel = XRCCTRL( *this, "m_advancedPropertiesPanel", wxPanel );
	Bool visible = XRCCTRL( *this, "m_advanced", wxToggleButton )->GetValue();

	if ( visible )
	{
		OpenAdvancedPropertiesPanel();
	}
	else
	{
		panel->Hide();
	}

	m_editor->m_advancedOpen = visible;
}

void CEdEnvironmentPointEditPanel::OnOuterToInnerClicked( wxCommandEvent& event )
{
	m_innerRadius->SetValue( m_outerRadius->GetValue() );
	GetSelectedPoint().m_innerRadius = GetSelectedPoint().m_outerRadius = ((Float)m_outerRadius->GetValue())/100.0f;
	UpdateEnvironmentArea();
}

void CEdEnvironmentPointEditPanel::OnPropertiesModified( wxCommandEvent& event )
{
	LoadValues();
}

// CEdEnvironmentPointEditNotifier
void CEdEnvironmentPointEditNotifier::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	m_editor->OnEditorNodeMoved( vertexIndex, oldPosition, wishedPosition, allowedPosition );
}

// CEdEnvironmentPointEdit
Bool CEdEnvironmentPointEdit::m_advancedOpen = false;

CEdEnvironmentPointEdit::CEdEnvironmentPointEdit()
	: m_areaComponent( NULL )
	, m_panel( NULL )
	, m_notifier( NULL )
{
}

void CEdEnvironmentPointEdit::CreateVertices()
{
	// Check if we have an area here
	if ( !m_areaComponent )
	{
		return;
	}

	// Get points
	const TDynArray< SAreaEnvironmentPoint >& points = m_areaComponent->GetPoints();

	// Create notifier component
	m_notifier = new CEdEnvironmentPointEditNotifier();
	m_notifier->m_editor = this;

	// Create a vertex for each point
	for ( Uint32 i=0; i < points.Size(); ++i )
	{
		const SAreaEnvironmentPoint& point = points[i];
		EntitySpawnInfo info;
		info.m_entityClass = ClassID< CVertexEditorEntity >();
		info.m_spawnPosition = m_areaComponent->GetWorldPositionRef() + point.m_position;
		CVertexEditorEntity* vertex = Cast< CVertexEditorEntity >( GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( info ) );
		vertex->m_owner = m_notifier;
		vertex->m_index = i;

		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("Vertex");
		vertex->CreateComponent( ClassID< CVertexComponent >(), spawnInfo );

		m_vertices.PushBack( vertex );
	}
}

void CEdEnvironmentPointEdit::DestroyVertices()
{
	for ( Uint32 i=0; i < m_vertices.Size(); ++i )
	{
		m_vertices[i]->Destroy();
	}
	m_vertices.Clear();

	if ( m_notifier )
	{
		delete m_notifier;
		m_notifier = NULL;
	}
}

void CEdEnvironmentPointEdit::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	// Make sure we have an area and the vertex index is valid
	if ( m_areaComponent == NULL || !( vertexIndex >= 0 && vertexIndex < m_vertices.SizeInt() ) )
	{
		return;
	}

	// We do not have any limitations on the allowed positions
	allowedPosition = wishedPosition;

	// Update point position
	SAreaEnvironmentPoint point = m_areaComponent->GetPoint( vertexIndex );
	point.m_position = allowedPosition - m_areaComponent->GetWorldPositionRef();
	m_areaComponent->UpdatePoint( vertexIndex, point );
}

String CEdEnvironmentPointEdit::GetCaption() const
{
	return TXT("Env Point Editor");
}

Bool CEdEnvironmentPointEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Get environment area component
	m_areaComponent = selection.Empty() ? NULL : Cast< CAreaEnvironmentComponent >( selection[0] );

	// Make sure we have one
	if ( !m_areaComponent )
	{
		WARN_EDITOR( TXT("No environment area is selected") );
		return false;
	}

	// Create vertices
	CreateVertices();

	// Create panel
	m_panel = new CEdEnvironmentPointEditPanel( panel, this );
	m_panelSizer->Add( m_panel, 1, wxEXPAND );

	// Create points choice
	m_panel->CreatePointChoices();
	if ( m_advancedOpen )
	{
		m_panel->OpenAdvancedPropertiesPanel();
	}

	return true;
}

void CEdEnvironmentPointEdit::End()
{
	DestroyVertices();
}

Bool CEdEnvironmentPointEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager* selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction( *selectionManager );

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Ctrl ) )
	{
		selectionManager->DeselectAll();
	}

	// Select only edited vertices
	TDynArray< SAreaEnvironmentPoint* > selectedPoints;
	for ( Uint32 i = 0; i < objects.Size(); i++ )
	{
		CVertexEditorEntity * vertex = Cast< CVertexEditorEntity >( objects[i]->GetHitObject()->GetParent() );
		if ( vertex == NULL || vertex->IsSelected() || !m_vertices.Exist( vertex ) )
		{
			continue;
		}

		selectionManager->Select( vertex );
		selectedPoints.PushBack( const_cast< SAreaEnvironmentPoint* >( m_areaComponent->GetPoints().TypedData() + vertex->m_index ) );
	}

	// Update the panel
	if ( !selectedPoints.Empty() )
	{
		int index = (int)m_areaComponent->GetPoints().GetIndex( selectedPoints[0] );
		if ( index != m_panel->m_pointsList->GetSelection() )
		{
			m_panel->m_pointsList->SetSelection( index );
			m_panel->LoadValues();
		}
	}

	// Handled
	return true;
}

Bool CEdEnvironmentPointEdit::HandleActionClick( Int32 x, Int32 y )
{
	return false;
}

Bool CEdEnvironmentPointEdit::OnDelete()
{
	return true;
}

Bool CEdEnvironmentPointEdit::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( action == IACT_Press && RIM_IS_KEY_DOWN( IK_Ctrl ) )
	{
		switch ( key )
		{
		case IK_D:
			m_panel->OnClonePointClicked( wxCommandEvent() );
			return true;
		case IK_A:
			m_panel->OnAddPointClicked( wxCommandEvent() );
			return true;
		case IK_R:
			m_panel->OnRemovePointClicked( wxCommandEvent() );
			return true;
		}
	}

	return false;
};

Bool CEdEnvironmentPointEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		return HandleActionClick( x, y );
	}
	return false;
}

Bool CEdEnvironmentPointEdit::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	return false;
}

Bool CEdEnvironmentPointEdit::OnViewportTrack( const CMousePacket& packet )
{
	return false;
}

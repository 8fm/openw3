/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushPanel.h"
#include "sceneExplorer.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/brushComponent.h"
#include "../../common/engine/brushBuilder.h"
#include "../../common/engine/brushCompiledData.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/material.h"
#include "../../common/engine/worldIterators.h"

BEGIN_EVENT_TABLE( CEdBrushPanel, CEdDraggablePanel )
	EVT_TOOL( XRCID("BuildCube"), CEdBrushPanel::OnBuildCube )
	EVT_TOOL( XRCID("BuildCylinder"), CEdBrushPanel::OnBuildCylinder )
	EVT_TOOL( XRCID("BuildCone"), CEdBrushPanel::OnBuildCone )
	EVT_TOOL( XRCID("BuildSphere"), CEdBrushPanel::OnBuildSphere )
	EVT_TOOL( XRCID("BuildLinearStairs"), CEdBrushPanel::OnBuildLinearStairs )
	EVT_TOOL( XRCID("BuildCurvedStairs"), CEdBrushPanel::OnBuildCurvedStairs )
	EVT_TOOL( XRCID("BuildSpiralStairs"), CEdBrushPanel::OnBuildSpiralStairs )
	EVT_TOOL( XRCID("CSGAdd"), CEdBrushPanel::OnCSGAdd )
	EVT_TOOL( XRCID("CSGSubtract"), CEdBrushPanel::OnCSGSubtract )
	EVT_TOOL( XRCID("CSGIntersect"), CEdBrushPanel::OnCSGIntersect )
	EVT_TOOL( XRCID("CSGDeintersect"), CEdBrushPanel::OnCSGDeintersect )
	EVT_BUTTON( XRCID("RebuildGeometry"), CEdBrushPanel::OnRebuildGeometry )
	EVT_BUTTON( XRCID("SelectBuildBrush"), CEdBrushPanel::OnSelectBuilderBrush )
END_EVENT_TABLE()

CEdBrushPanel::CEdBrushPanel( wxWindow* parent )
	: m_builderProperties( NULL )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("BrushToolPanel") );

	// Create properties browser
	{
		wxPanel* rp = XRCCTRL( *this, "BrushBuilderProperties", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_builderProperties = new CEdPropertiesPage( rp, settings, nullptr );
		sizer1->Add( m_builderProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Update list of layers
	UpdateLayersList();

	// Register to events
	SEvents::GetInstance().RegisterListener( CNAME( ActiveLayerChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanged ), this );
	//SEvents::GetInstance().RegisterListener( CNAME( LayerLoaded ), this );
	//SEvents::GetInstance().RegisterListener( CNAME( LayerUnloaded ), this );
}

CEdBrushPanel::~CEdBrushPanel()
{
	// Unregister from all events
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdBrushPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Update the list of layers
	UpdateLayersList();
}

void CEdBrushPanel::UpdateLayersList()
{
	// Get the active layer
	CLayer* activeLayer = wxTheFrame->GetSceneExplorer()->GetActiveLayer();

	// Get all visible layers
	if ( GGame->GetActiveWorld() )
	{
		// Filter the layers to get the layers with brushes
		TDynArray< CLayer* > layersWithBrushes;
		for ( WorldAttachedLayerIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			CLayer* layer = *it;
			if ( layer->GetLayerInfo() )
			{
				if ( layer->GetBrushCompiledData() )
				{
					layersWithBrushes.PushBack( layer );
				}
			}
		}

		// Get the combo
		wxChoice* combo = XRCCTRL( *this, "BrushLayerList", wxChoice );
		if ( combo )
		{
			// Clear current shit
			combo->Freeze();
			combo->Clear();

			// Add list of layers with brushes
			for ( Uint32 i=0; i<layersWithBrushes.Size(); i++ )
			{
				CLayer* layer = layersWithBrushes[i];

				// Format name
				String name = layer->GetLayerInfo()->GetShortName();
				name += TXT(" in ");
				name += layer->GetLayerInfo()->GetLayerGroup()->GetGroupPathName( NULL );

				// Add to combo
				Int32 index = combo->Append( name.AsChar(), new ClientDataObjectWrapper( layer ) );
				if ( layer == activeLayer )
				{
					combo->SetSelection( index );
				}
			}

			// Empty combo
			if ( combo->GetCount() == 0 )
			{
				combo->AppendString( TXT("(No layers with brushes)") );
				combo->SetSelection( 0 );
				combo->Enable( false );
			}
			else
			{
				combo->Enable( true );
			}

			// Enable
			combo->Thaw();
			combo->Refresh();
		}
	}

	// Update other shit
	UpdateLayerDependentControls();
}

class wxMyChoice : public wxChoice
{
public:
	wxClientData* GetItemData(unsigned int n) const
	{
		return reinterpret_cast< wxClientData* >( DoGetItemClientData(n) );
	}
};

CLayer* CEdBrushPanel::GetSelectedLayer()
{
	// Get the combo
	wxMyChoice* combo = XRCCTRL( *this, "BrushLayerList", wxMyChoice );
	if ( combo )
	{
		// Get layer from combo box
		Int32 selected = combo->GetSelection();
		if ( selected >= 0 )
		{
			ClientDataObjectWrapper* wrapper = static_cast< ClientDataObjectWrapper* >( combo->GetItemData( selected ) );
			if ( wrapper && wrapper->m_objectClass )
			{
				return SafeCast< CLayer >( wrapper->m_objectClass );
			}
		}
	}

	// Use active layer
	return wxTheFrame->GetSceneExplorer()->GetActiveLayer();
}

void CEdBrushPanel::UpdateLayerDependentControls()
{
	// Get the layer that is selected by tool
	CLayer* activeLayer = GetSelectedLayer();

	// Update button
	wxButton* rebuildButton = XRCCTRL( *this, "RebuildGeometry", wxButton );
	rebuildButton->Enable( activeLayer && activeLayer->GetBrushCompiledData() );
}

CBrushComponent* CEdBrushPanel::GetBuilderBrush( Bool allowToCreate /*=false*/ )
{
	// No active world, no building brush
	if ( !GGame->GetActiveWorld() )
	{
		wxMessageBox( wxT("No world loaded. Unable to access building brush."), wxT("Build brush"), wxOK | wxICON_WARNING );
		return NULL;
	}

	// Get dynamic layer
	CLayer* dynamicLayer = GGame->GetActiveWorld()->GetDynamicLayer();
	if ( !dynamicLayer )
	{
		wxMessageBox( wxT("Loaded world has no dynamic layer. Unable to access building brush."), wxT("Build brush"), wxOK | wxICON_WARNING );
		return NULL;
	}

	// Find the dynamic entity named "buildingbrush"
	CEntity* entity = dynamicLayer->FindEntity( TXT("BuildingBrush") );
	if ( entity )
	{
		CBrushComponent* bc = Cast< CBrushComponent >( entity->FindComponent( TXT("Brush") ) );
		if ( bc )
		{
			return bc;
		}
	}

	// Create if we can
	if ( allowToCreate )
	{
		// Create the entity
		if ( !entity )
		{
			EntitySpawnInfo einfo;
			einfo.m_name = TXT("BuildingBrush");

			// Create entity on the dynamic layer
			entity = dynamicLayer->CreateEntitySync( einfo );
			if ( !entity )
			{
				wxMessageBox( wxT("Unable to create bulding brush entity."), wxT("Build brush"), wxOK | wxICON_WARNING );
				return NULL;
			}
		}

		// Create the brush component
		CBrushComponent* bc = new CBrushComponent();
		bc->SetName( TXT("Brush") );
		bc->SetCSGType( CSG_Edit );
		entity->AddComponent( bc );

		// Return created component
		return bc;
	}

	// Not found and not created
	return NULL;
}

void CEdBrushPanel::BuildBrush()
{
	if ( m_brushBuilder )
	{
		// Create the edit brush
		CBrushComponent* editBrush = GetBuilderBrush( true );
		if ( editBrush )
		{
			// Update the brush
			m_brushBuilder->Build( editBrush );
			editBrush->ScheduleUpdateTransformNode();
		}
	}
}

void CEdBrushPanel::InitializeBuildBrush( CClass* builderClass )
{
	// Save current builder
	CBrushBuilder* curBuilder = m_brushBuilder;
	if ( curBuilder ) 
	{
		curBuilder->SaveObjectConfig( TXT("User") );
		curBuilder->RemoveFromRootSet();
		curBuilder->Discard();
		curBuilder = NULL;
	}

	// Create new builder
	CBrushBuilder* newBuilder = NULL;
	if ( builderClass )
	{
		newBuilder = ::CreateObject< CBrushBuilder >( builderClass );
		newBuilder->LoadObjectConfig( TXT("User") );
		newBuilder->AddToRootSet();
		ASSERT( newBuilder );
	}

	// Change the edited object
	m_brushBuilder = newBuilder;
	m_builderProperties->SetObject( newBuilder );

	// Update builder brush
	BuildBrush();
}

CGatheredResource defBrushMaterial( TXT("engine\\materials\\brush\\brush.w2mi"), 0);

void CEdBrushPanel::CreateBrush( EBrushCSGType brushType )
{
	// We cannot do this crap when there is some editor tool active
	if ( wxTheFrame->GetWorldEditPanel()->GetTool() )
	{
		wxMessageBox( wxT("There is some other editor tool active, exit that tool first"), wxT("Select building brush"), wxOK | wxICON_WARNING );
		return;
	}

	// Get selected layer
	CLayer* selectedLayer = GetSelectedLayer();
	if ( !selectedLayer )
	{
		wxMessageBox( wxT("Select some layer first."), wxT("Create new brush"), wxOK | wxICON_WARNING );
		return;
	}

	// Get builder brush
	CBrushComponent* builderBrush = GetBuilderBrush( false );
	if ( !builderBrush )
	{
		wxMessageBox( wxT("No builder brush. Create it first."), wxT("Create new brush"), wxOK | wxICON_WARNING );
		return;
	}

	// Create brush data on that layer
	CBrushCompiledData* data = selectedLayer->CreateBrushCompiledData();
	if ( !data )
	{
		wxMessageBox( wxT("Unable to create brush data in selected layer."), wxT("Create new brush"), wxOK | wxICON_WARNING );
		return;
	}
	
	// Load default material
	IMaterial* defaultMaterial = defBrushMaterial.LoadAndGet< IMaterial >();
	if ( !defaultMaterial )
	{
		wxMessageBox( wxT("Unable to load default brush material."), wxT("Create new brush"), wxOK | wxICON_WARNING );
		return;
	}
	
	// Update layer list
	UpdateLayersList();

	// Setup brush info
	EntitySpawnInfo einfo;
	einfo.m_name = selectedLayer->GenerateUniqueEntityName( TXT("Brush") );

	// Create new brush entity
	CEntity* brushEntity = selectedLayer->CreateEntitySync( einfo );
	if ( !brushEntity )
	{
		wxMessageBox( wxT("Unable to create brush entity."), wxT("Create new brush"), wxOK | wxICON_WARNING );
		return;
	}

	// Create brush component
	CBrushComponent* bc = new CBrushComponent();
	bc->CopyData( builderBrush, brushType != CSG_Subtractive, defaultMaterial, brushType );

	// Register
	brushEntity->AddComponent( bc );
	data->AddBrush( bc );

	// Mark layer as modified
	selectedLayer->MarkModified();

	// Select new brush
	GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
	GGame->GetActiveWorld()->GetSelectionManager()->Select( brushEntity );

	// Rebuild geometry
	wxCommandEvent fakeEvent;
	OnRebuildGeometry( fakeEvent );
}

void CEdBrushPanel::OnBuildCube( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderCube") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildCylinder( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderCylinder") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildCone( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderCone") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildSphere( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderSphere") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildLinearStairs( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderLinearStairs") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildCurvedStairs( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderCurvedStairs") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnBuildSpiralStairs( wxCommandEvent& event )
{
	static CName className( TXT("CBrushBuilderSpiralStairs") );
	CClass* builderClass = SRTTI::GetInstance().FindClass( className );
	InitializeBuildBrush( builderClass );
}

void CEdBrushPanel::OnCSGAdd( wxCommandEvent& event )
{
	CreateBrush( CSG_Addtive );
}

void CEdBrushPanel::OnCSGSubtract( wxCommandEvent& event )
{
	CreateBrush( CSG_Subtractive );
}

void CEdBrushPanel::OnCSGIntersect( wxCommandEvent& event )
{
	wxMessageBox( wxT("Not implemented yet"), wxT("CSG Intersect"), wxOK | wxICON_INFORMATION );
}

void CEdBrushPanel::OnCSGDeintersect( wxCommandEvent& event )
{
	wxMessageBox( wxT("Not implemented yet"), wxT("CSG Deintersect"), wxOK | wxICON_INFORMATION );
}

void CEdBrushPanel::OnSelectBuilderBrush( wxCommandEvent& event )
{
	// We cannot do this crap when there is some editor tool active
	if ( wxTheFrame->GetWorldEditPanel()->GetTool() )
	{
		wxMessageBox( wxT("There is some other editor tool active, exit that tool first"), wxT("Select building brush"), wxOK | wxICON_WARNING );
		return;
	}

	// Make sure building brush will be visible
	wxTheFrame->GetWorldEditPanel()->GetViewport()->SetRenderingMask( SHOW_BuildingBrush );

	// Create the edit brush
	CBrushComponent* editBrush = GetBuilderBrush( false );
	if ( editBrush )
	{
		CEntity* entity = editBrush->GetEntity();
		if ( entity )
		{
			if ( !entity->IsSelected() )
			{
				GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
				GGame->GetActiveWorld()->GetSelectionManager()->Select( entity );
			}
			else
			{
				wxTheFrame->GetWorldEditPanel()->LookAtNode( entity );
			}
		}
	}
}

void CEdBrushPanel::OnRebuildGeometry( wxCommandEvent& event )
{
	// Get selected layer
	CLayer* selectedLayer = GetSelectedLayer();
	if ( !selectedLayer )
	{
		wxMessageBox( wxT("Select some layer first."), wxT("Rebuild geometry"), wxOK | wxICON_WARNING );
		return;
	}

	// Create brush data on that layer
	CBrushCompiledData* data = selectedLayer->GetBrushCompiledData();
	if ( !data )
	{
		wxMessageBox( wxT("There's no brush data in selected layer."), wxT("Rebuild geometry"), wxOK | wxICON_WARNING );
		return;
	}

	// Rebuild geometry
	data->Compile();
	data->MarkModified();
}

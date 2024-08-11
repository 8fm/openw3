/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialPreviewPanel.h"
#include "editorExternalResources.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/utils.h"

BEGIN_EVENT_TABLE( CEdMaterialPreviewPanel, CEdPreviewPanel )
	EVT_MENU_RANGE( 0, SHAPE_Max, CEdMaterialPreviewPanel::OnMeshSelected )
END_EVENT_TABLE();


CEdMaterialPreviewPanel::CEdMaterialPreviewPanel( wxWindow* parent )
	: CEdPreviewPanel( parent, true )
{
	// Render with lighting
	GetViewport()->SetRenderingMode( RM_Shaded );
	SetCameraMode( RPCM_DefaultOrbiting );
	SetCameraPosition( Vector::ZEROS );

	// Create entity
	EntitySpawnInfo einfo;
	CEntity* entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

	// Add mesh component
	m_component = SafeCast< CMeshComponent >( entity->CreateComponent( ClassID< CMeshComponent >(), SComponentSpawnInfo() ) );
	SetShape( SHAPE_Sphere );

	// Mesh starts out hidden, until we have a material to show.
	m_component->SetVisible( false );

	m_previewWorld->DelayedActions();
}


void CEdMaterialPreviewPanel::SetMaterial( IMaterial* material )
{
	CEntity* entity = m_component->GetEntity();

	// Clear out previous material replacement
	if ( entity->HasMaterialReplacement() )
	{
		entity->DisableMaterialReplacement();
	}

	if ( material )
	{
		entity->SetMaterialReplacement( material );
	}
}


IMaterial* CEdMaterialPreviewPanel::GetMaterial() const
{
	CEntity* entity = m_component->GetEntity();
	const SMaterialReplacementInfo* info = entity->GetMaterialReplacementInfo();
	return info != nullptr ? info->material : nullptr;
}

void CEdMaterialPreviewPanel::SetShape( EPreviewShape shape )
{
	if ( m_component )
	{
		String resourceName;
		switch ( shape )
		{
		case SHAPE_Box:			resourceName = MESH_BOX;		break;
		case SHAPE_Sphere:		resourceName = MESH_SPHERE;		break;
		case SHAPE_Cylinder:	resourceName = MESH_CYLINDER;	break;
		case SHAPE_Plane:		resourceName = MESH_PLANE;		break;
		default:
			HALT( "Invalid shape: %d", (Int32)shape );
			return;
		}

		CMesh* mesh = LoadResource< CMesh >( resourceName );
		if ( mesh )
		{
			m_component->SetResource( mesh );
			//Refresh();
		}
		else
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( m_component ), TXT("Engine Resources"), TXT("Missing engine mesh '%s'. Material preview may not show correctly"), resourceName.AsChar() );
		}
	}
}

void CEdMaterialPreviewPanel::RefreshPreviewVisibility( Bool visible )
{
	m_component->SetVisible( visible );
}

void CEdMaterialPreviewPanel::HandleContextMenu( Int32 x, Int32 y )
{
	// We can use the SHAPE_* value as the menu item ID, for easy handling
	wxMenu menu;
	menu.Append( SHAPE_Box,			TXT("Box") );
	menu.Append( SHAPE_Cylinder,	TXT("Cylinder") );
	menu.Append( SHAPE_Sphere,		TXT("Sphere") );
	menu.Append( SHAPE_Plane,		TXT("Plane") );
	PopupMenu( &menu, x, y );
}

void CEdMaterialPreviewPanel::OnMeshSelected( wxCommandEvent& event )
{
	SetShape( static_cast< EPreviewShape >( event.GetId() ) );
}

void CEdMaterialPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// I switch this off for now, it only added useless debug info that we never want to see in the material editor
	//if ( m_component )
	//{
	//	// Generate normal fragments
	//	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );
	//}
}

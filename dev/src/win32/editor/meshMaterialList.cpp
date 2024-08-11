/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "materialListManager.h"
#include "meshMaterialList.h"
#include "meshEditor.h"
#include "meshMaterialRemapper.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/materialInstance.h"

CEdMeshMaterialList::CEdMeshMaterialList( wxWindow* parent, CEdMeshEditor* editor, CMeshTypeResource* mesh, CEdUndoManager* undoManager )
	: CEdMaterialListManager( parent, mesh, TXT( "engine\\materials\\graphs\\" ), undoManager )
	, m_mesh( mesh )
	, m_editor( editor )
{
}

Int32 CEdMeshMaterialList::GetNumMaterials() const
{
	return m_mesh->GetMaterials().Size();
}

IMaterial* CEdMeshMaterialList::GetMaterial( Int32 index ) const
{
	return m_mesh->GetMaterials()[ index ].Get();
}

String CEdMeshMaterialList::GetMaterialName( Int32 index ) const
{
	if ( index < m_mesh->GetMaterialNames().SizeInt() )
	{
		return m_mesh->GetMaterialNames()[ index ];
	}
	else
	{
		return String::EMPTY;
	}
}

void CEdMeshMaterialList::SetMaterial( Int32 index, IMaterial* material )
{
    m_mesh->SetMaterial( index, material );

    CMaterialInstance *materialInstance = Cast<CMaterialInstance>( material );
    if ( materialInstance )
	{
        materialInstance->AutoAssignTextures();
	}

	m_editor->RefreshPreviewRenderingProxy();
}

Int32 CEdMeshMaterialList::AddMaterial( IMaterial* material, const String& name )
{
	if ( CMesh* mesh = SafeCast< CMesh >( m_mesh ) )
	{
		CMesh::TMaterials&   materials     = mesh->GetMaterials();
		TDynArray< String >& materialNames = mesh->GetMaterialNames();

		materials.PushBack( material );
		materialNames.PushBack( name );

		return materials.Size() - 1;
	}
	else
	{
		return -1;
	}
}

Bool CEdMeshMaterialList::RemoveUnusedMaterials()
{
	if ( CMesh* mesh = SafeCast< CMesh >( m_mesh ) )
	{
		mesh->RemoveUnusedMaterials();
		return true;
	}	
	else
	{
		return false;
	}
}

Bool CEdMeshMaterialList::RemapMaterials()
{
	if ( Cast< CMesh >( m_mesh ) != nullptr || Cast< CApexResource >( m_mesh ) != nullptr )
	{
		CEdMeshMaterialRemapper remapper( m_editor, m_undoManager );
		if ( remapper.Execute( m_mesh ) )
		{
			m_editor->RefreshPreviewRenderingProxy();
			return true;
		}
	}

	return false;
}

void CEdMeshMaterialList::HighlightMaterial( Int32 index, Bool state )
{
	if ( m_mesh->IsA< CMesh >() )
	{
		// Update
		if ( state )
		{
			( new CRenderCommand_ToggleMeshMaterialHighlight( SafeCast< CMesh >( m_mesh ), index ) )->Commit();
		}
		else
		{
			( new CRenderCommand_ToggleMeshMaterialHighlight( nullptr, -1 ) )->Commit();
		}
	}
}

Bool CEdMeshMaterialList::RenameMaterial( Int32 index, const String& newName )
{
	if ( index < m_mesh->GetMaterialNames().SizeInt() )
	{
		m_mesh->GetMaterialNames()[ index ] = newName;
		m_editor->UpdateLODList( true );
		m_editor->UpdateMeshStats();
		return true;
	}

	return false;
}

void CEdMeshMaterialList::MaterialPropertyChanged( CName propertyName, Bool finished )
{
	if ( finished )
	{
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
	else
	{
		m_editor->RefreshPreviewRenderingProxy();
	}
}

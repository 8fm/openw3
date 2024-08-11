
#include "build.h"
#include "undoMeshEditor.h"
#include "materialListManager.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/drawableComponent.h"

IMPLEMENT_ENGINE_CLASS( CUndoMeshMaterialInstanceExistance )

CUndoMeshMaterialInstanceExistance::CUndoMeshMaterialInstanceExistance( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index, Bool creating )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_index( index )
	, m_creating( creating )
{
	m_instance = SafeCast< CMaterialInstance >( editor->GetMaterial( index ) );
	m_originalParent = m_instance->GetParent();
}

/*static*/ 
void CUndoMeshMaterialInstanceExistance::CreateCreationStep( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index )
{
	CUndoMeshMaterialInstanceExistance* step = new CUndoMeshMaterialInstanceExistance( undoManager, editor, index, true );
	step->PushStep();
}

/*static*/ 
void CUndoMeshMaterialInstanceExistance::CreateDeletionStep( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index )
{
	CUndoMeshMaterialInstanceExistance* step = new CUndoMeshMaterialInstanceExistance( undoManager, editor, index, false );
	step->m_instance->SetParent( step );
	step->PushStep();
}

void CUndoMeshMaterialInstanceExistance::DoStep( Bool create )
{
	if ( create )
	{
		m_instance->SetParent( m_originalParent );
		m_editor->SetMaterial( m_index, m_instance );
	}
	else
	{
		m_instance->SetParent( this );
 		m_editor->SetMaterial( m_index, m_instance->GetBaseMaterial() );
	}

	m_editor->UpdateMaterialList();
}

/*virtual*/ 
void CUndoMeshMaterialInstanceExistance::DoUndo()
{
	DoStep( !m_creating );
}

/*virtual*/ 
void CUndoMeshMaterialInstanceExistance::DoRedo()
{
	DoStep( m_creating );
}

/*virtual*/ 
String CUndoMeshMaterialInstanceExistance::GetName()
{
	return String( m_creating ? TXT("creating") : TXT("deleting") ) + TXT(" material instance");
}

/*virtual*/ 
void CUndoMeshMaterialInstanceExistance::OnSerialize( class IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		file << m_instance;
	}
}


IMPLEMENT_ENGINE_CLASS( CUndoMeshChunksChanged )

CUndoMeshChunksChanged::CUndoMeshChunksChanged( CEdUndoManager& undoManager, CMesh* mesh, const String& stepName )
	: IUndoStep( undoManager )
	, m_mesh( mesh )
	, m_stepName( stepName )
{
	m_data = new CMeshData( m_mesh );
}

/*static*/ 
void CUndoMeshChunksChanged::CreateStep( CEdUndoManager& undoManager, CMesh* mesh, const String& stepName )
{
	CUndoMeshChunksChanged* step = new CUndoMeshChunksChanged( undoManager, mesh, stepName );

	step->PushStep();
}

void CUndoMeshChunksChanged::DoStep()
{
	CMeshData* data = new CMeshData( m_mesh );

	m_data->FlushChanges();
	delete m_data;

	m_data = data;
}

/*virtual*/ 
void CUndoMeshChunksChanged::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoMeshChunksChanged::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoMeshChunksChanged::GetName()
{
	return m_stepName;
}


IMPLEMENT_ENGINE_CLASS( CUndoMeshMaterialsChanged )

CUndoMeshMaterialsChanged::CUndoMeshMaterialsChanged( CEdUndoManager& undoManager, CMeshTypeResource* mesh, const String& stepName )
	: IUndoStep( undoManager )
	, m_mesh( mesh )
	, m_stepName( stepName )
{
	StoreData();
}

/*static*/ 
void CUndoMeshMaterialsChanged::CreateStep( CEdUndoManager& undoManager, CMeshTypeResource* mesh, const String& stepName )
{
	CUndoMeshMaterialsChanged* step = new CUndoMeshMaterialsChanged( undoManager, mesh, stepName );

	step->PushStep();
}

void CUndoMeshMaterialsChanged::StoreData()
{
	m_materials = m_mesh->GetMaterials();
	m_materialNames = m_mesh->GetMaterialNames();

	m_chunkMaterialIds.Clear();

	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		const auto& chunks = mesh->GetChunks();
		for ( const auto& chunk : chunks )
		{
			m_chunkMaterialIds.PushBack( chunk.m_materialID );
		}
	}
}


void CUndoMeshMaterialsChanged::DoStep()
{
	CMesh::TMaterials   materialsToSet        = m_materials;
	TDynArray< String > materialNamesToSet    = m_materialNames;
	TDynArray< Int32 >  chunkMaterialIdsToSet = m_chunkMaterialIds;

	StoreData();

	m_mesh->GetMaterials() = materialsToSet;
	m_mesh->GetMaterialNames() = materialNamesToSet;
	
	if ( CMesh* mesh = Cast< CMesh >( m_mesh ) )
	{
		const auto& chunks = mesh->GetChunks();
		ASSERT( chunks.Size() == m_chunkMaterialIds.Size() );

		for ( Uint32 i=0; i<chunks.Size(); ++i )
		{
			const_cast< SMeshChunkPacked& >( chunks[i] ).m_materialID = chunkMaterialIdsToSet[i];
		}
	}

	m_mesh->CreateRenderResource();
}

/*virtual*/ 
void CUndoMeshMaterialsChanged::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoMeshMaterialsChanged::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoMeshMaterialsChanged::GetName()
{
	return m_stepName;
}

/*virtual*/ 
void CUndoMeshMaterialsChanged::OnSerialize( class IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		for ( THandle< IMaterial >& m : m_materials )
		{
			file << m;
		}
	}
}

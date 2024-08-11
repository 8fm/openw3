#include "build.h"

#include "../../common/game/actionAreaVertex.h"
#include "undoVertex.h"
#include "undoManager.h"
#include "areaVertexEdit.h"
#include "../../common/engine/dynamicLayer.h"

IMPLEMENT_ENGINE_CLASS( CUndoVertexCreateDestroy );

void CUndoVertexCreateDestroy::CreateStep( CEdUndoManager* undoManager, CEdRenderingPanel* viewport, CVertexEditorEntity * entity, Bool undoCreation )
{
	CUndoVertexCreateDestroy *stepToAdd = Cast<CUndoVertexCreateDestroy>( undoManager->GetStepToAdd() );
	if ( !stepToAdd || stepToAdd->m_undoCreation != undoCreation || stepToAdd->m_viewport != viewport )
	{
		undoManager->SetStepToAdd( stepToAdd = new CUndoVertexCreateDestroy( *undoManager, viewport, undoCreation ) );
	}

	stepToAdd->m_vertices.PushBack( entity );

	if ( ! stepToAdd->m_undoCreation )
	{
		entity->EditorPreDeletion();
		undoManager->GetWorld()->GetDynamicLayer()->RemoveEntity( entity );
		undoManager->GetWorld()->DelayedActions();
		entity->SetParent( stepToAdd );
	}
}

void CUndoVertexCreateDestroy::FinishStep( CEdUndoManager* undoManager )
{
	ASSERT( undoManager );
	CUndoVertexCreateDestroy *stepToAdd = Cast<CUndoVertexCreateDestroy>( undoManager->GetStepToAdd() );
	ASSERT( stepToAdd );
	if ( stepToAdd )
	{
		stepToAdd->PushStep();
	}
}

void CUndoVertexCreateDestroy::OnObjectRemoved( CObject *object )
{
	for ( Uint32 i = 0; i < m_vertices.Size(); ++i )
	{
		if ( m_vertices[i] == object || static_cast< CComponent* >( m_vertices[i]->m_owner ) == object )
		{
			PopStep();
			return;
		}
	}
}

void CUndoVertexCreateDestroy::DoUndo()
{
	CLayer * layer = m_undoManager->GetWorld()->GetDynamicLayer();

	CEdVertexEdit * tool = Cast< CEdVertexEdit >( m_viewport->GetTool() );
	ASSERT( tool );

	if ( m_undoCreation )
	{
		for ( Uint32 i = 0; i < m_vertices.Size(); ++i )
		{
			CComponent * owner = static_cast< CComponent* >( m_vertices[i]->m_owner );
			owner->OnEditorVertexDestroy( m_vertices[i]->m_index );

			m_vertices[ i ]->EditorPreDeletion();
			layer->RemoveEntity( m_vertices[i] );
		}

		m_undoManager->GetWorld()->DelayedActions();

		for ( Uint32 i = 0; i < m_vertices.Size(); ++i )
		{
			m_vertices[i]->SetParent( this );
			tool->OnVertexRemoved( m_vertices[i] );
		}
	}
	else
	{
		for ( Uint32 i = 0; i < m_vertices.Size(); ++i )
		{
			layer->AddEntity( m_vertices[i] );

			CComponent * owner = static_cast< CComponent* >( m_vertices[i]->m_owner );
			Vector wishedPosition = m_vertices[i]->GetWorldPosition();
			Vector allowedPosition;
			Int32 insertPos;
			owner->OnEditorVertexInsert( m_vertices[i]->m_index-1, wishedPosition, allowedPosition, insertPos );
			ASSERT( wishedPosition == allowedPosition );

			tool->OnVertexAdded( m_vertices[i] );
		}
	}
}

void CUndoVertexCreateDestroy::DoRedo()
{
	m_undoCreation = ! m_undoCreation;
	DoUndo();
	m_undoCreation = ! m_undoCreation;
}

void CUndoVertexCreateDestroy::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( Uint32 i = 0; i < m_vertices.Size(); ++i )
		{
			file << m_vertices[i];
		}
	}
}

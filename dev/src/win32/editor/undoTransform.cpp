
#include "build.h"

#include "undoTransform.h"
#include "../../common/engine/node.h"
#include "../../common/game/actionAreaVertex.h"

IMPLEMENT_ENGINE_CLASS( CUndoTransform );

CUndoTransform::CUndoTransform( CEdUndoManager& undoManager )
	: IUndoStep( undoManager )
{
}

void CUndoTransform::CreateStep( CEdUndoManager& undoManager, CNode* node )
{
	TDynArray< CNode* > nodes;
	nodes.PushBack( node );
	CreateStep( undoManager, nodes );
}

void CUndoTransform::CreateStep( CEdUndoManager& undoManager, TDynArray< CNode* > nodes )
{
	CUndoTransform *step = undoManager.SafeGetStepToAdd< CUndoTransform >();

	if ( !step )
	{
		step = new CUndoTransform( undoManager );
		undoManager.SetStepToAdd( step );
	}

	for ( CNode* node : nodes )
	{
// 		if ( CVertexEditorEntity* vert = Cast< CVertexEditorEntity >( node ) )
// 		{
// 			// TODO:
// 		}
// 		else
		{
			step->m_infos.PushBack( Info( node ) );
		}
	}

	step->PushStep();
}

void CUndoTransform::DoStep()
{
	for ( Info& info : m_infos )
	{
		if ( info.m_node )
		{
			EngineTransform prevTransform = info.m_node->GetTransform();
			info.m_node->SetScale( info.m_transform.GetScale() );
			info.m_node->SetRotation( info.m_transform.GetRotation() );
			info.m_node->SetPosition( info.m_transform.GetPosition() );
			info.m_transform = prevTransform;
			info.m_node->EditorOnTransformChanged();
		}
	}
}


void CUndoTransform::DoUndo()
{
	DoStep();
}

void CUndoTransform::DoRedo()
{
	DoStep();
}

String CUndoTransform::GetName()
{
	return TXT( "node transform changed" );
}

String CUndoTransform::GetTarget()
{
	return String::EMPTY;
}

void CUndoTransform::OnObjectRemoved( CObject *object )
{
	for ( const Info& info : m_infos )
	{
		if ( info.m_node == object )
		{
			PopStep();
			return;
		}
	}
}

void CUndoTransform::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( Info& info : m_infos )
		{
			file << info.m_node;
		}
	}
}
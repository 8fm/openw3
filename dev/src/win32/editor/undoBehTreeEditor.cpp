
#include "build.h"
#include "undoBehTreeEditor.h"
#include "../../common/game/behTreeNode.h"

IMPLEMENT_ENGINE_CLASS( CUndoBehTreeBlockExistance );

CUndoBehTreeBlockExistance::CUndoBehTreeBlockExistance( CEdUndoManager& undoManager )
	: CUndoTreeBlockExistance( undoManager ) // <- here the UndoManager becomes the parent of the step
{
}

/*static*/
CUndoBehTreeBlockExistance* CUndoBehTreeBlockExistance::PrepareStep( CEdUndoManager& undoManager )
{
	CUndoBehTreeBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoBehTreeBlockExistance >();

	if ( !step )
	{
		step = new CUndoBehTreeBlockExistance( undoManager );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

/*static*/
void CUndoBehTreeBlockExistance::PrepareCreationStep( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object )
{
	PrepareStep( undoManager )->DoPrepareCreationStep( object );
}

/*static*/ 
void CUndoBehTreeBlockExistance::PrepareCreationStepWithOffset( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object, Int32 dx, Int32 dy )
{
	CUndoBehTreeBlockExistance* step = PrepareStep( undoManager );
	step->DoPrepareCreationStep( object );
	step->m_offsets.Insert( object, OffsetInfo( dx, dy ) );
}

/*static*/
void CUndoBehTreeBlockExistance::PrepareDeletionStep( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object )
{
	PrepareStep( undoManager )->DoPrepareDeletionStep( object );
}

/*static*/
void CUndoBehTreeBlockExistance::FinalizeStep( CEdUndoManager& undoManager, const String& nameOverride )
{
	if ( CUndoBehTreeBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoBehTreeBlockExistance >() )
	{
		step->SetNameOverride( nameOverride );
		step->PushStep();
	}
}

/*virtual*/ 
void CUndoBehTreeBlockExistance::SetupCustomRelationship( CObject* object, Bool created ) /*override*/
{
	IBehTreeNodeDefinition* node = SafeCast< IBehTreeNodeDefinition >( object );
	
	if ( IBehTreeNodeDefinition* parent = node->GetParentNode() )
	{
		if ( created )
		{
			parent->AddChild( node );
		}
		else
		{
			parent->RemoveChild( node );
		}
	}
}

/*virtual*/ 
String CUndoBehTreeBlockExistance::GetFriendlyBlockName( CObject* object ) /*override*/
{
	IBehTreeNodeDefinition* node = SafeCast< IBehTreeNodeDefinition >( object );
	return node->GetNodeName().AsString();
}

void CUndoBehTreeBlockExistance::ApplyOffsets()
{
	for ( auto offsetIt = m_offsets.Begin(); offsetIt != m_offsets.End(); ++offsetIt )
	{
		offsetIt->m_second.m_dx = -offsetIt->m_second.m_dx;
		offsetIt->m_second.m_dy = -offsetIt->m_second.m_dy;
		offsetIt->m_first->OffsetNodesPosition( offsetIt->m_second.m_dx, offsetIt->m_second.m_dy );
	}
}

/*virtual*/ 
void CUndoBehTreeBlockExistance::DoUndo() /*override*/
{
	CUndoTreeBlockExistance::DoUndo();
	ApplyOffsets();
}

/*virtual*/ 
void CUndoBehTreeBlockExistance::DoRedo() /*override*/
{
	CUndoTreeBlockExistance::DoRedo();
	ApplyOffsets();
}

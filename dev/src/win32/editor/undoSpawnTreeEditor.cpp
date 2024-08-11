
#include "build.h"
#include "undoSpawnTreeEditor.h"

#include "../../common/game/edSpawnTreeNode.h"

IMPLEMENT_ENGINE_CLASS( CUndoSpawnTreeBlockExistance );

namespace
{
	void DoSetupRelationShip( CObject* object, Bool created )
	{
		IEdSpawnTreeNode* node = dynamic_cast< IEdSpawnTreeNode* >( object );
		ASSERT( node );	

		if ( IEdSpawnTreeNode* parent = node->GetParentNode() )
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
}


CUndoSpawnTreeBlockExistance::CUndoSpawnTreeBlockExistance( CEdUndoManager& undoManager )
	: CUndoTreeBlockExistance( undoManager ) // <- here the UndoManager becomes the parent of the step
{
}

/*static*/
CUndoSpawnTreeBlockExistance* CUndoSpawnTreeBlockExistance::PrepareStep( CEdUndoManager& undoManager )
{
	CUndoSpawnTreeBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoSpawnTreeBlockExistance >();

	if ( !step )
	{
		step = new CUndoSpawnTreeBlockExistance( undoManager );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

/*static*/
void CUndoSpawnTreeBlockExistance::PrepareCreationStep( CEdUndoManager& undoManager, CObject* object )
{
	PrepareStep( undoManager )->DoPrepareCreationStep( object );
}

/*static*/
void CUndoSpawnTreeBlockExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CObject* object )
{
	PrepareStep( undoManager )->DoPrepareDeletionStep( object );
}

/*static*/
void CUndoSpawnTreeBlockExistance::FinalizeStep( CEdUndoManager& undoManager, const String& nameOverride )
{
	if ( CUndoSpawnTreeBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoSpawnTreeBlockExistance >() )
	{
		step->SetNameOverride( nameOverride );
		step->PushStep();
	}
}

/*virtual*/ 
void CUndoSpawnTreeBlockExistance::SetupCustomRelationship( CObject* object, Bool created ) /*override*/
{
	DoSetupRelationShip( object, created );
}

/*virtual*/ 
String CUndoSpawnTreeBlockExistance::GetFriendlyBlockName( CObject* object ) /*override*/
{
	IEdSpawnTreeNode* node = dynamic_cast< IEdSpawnTreeNode* >( object );
	ASSERT( node );	

	return node->GetEditorFriendlyName();
}


// ---------------------------

IMPLEMENT_ENGINE_CLASS( CUndoSpawnTreeBlockDecorate );

CUndoSpawnTreeBlockDecorate::CUndoSpawnTreeBlockDecorate( CEdUndoManager& undoManager, CObject* object, CObject* decorated )
	: CUndoTreeBlockDecorate( undoManager, object, decorated )
{
}

/*static*/ 
void CUndoSpawnTreeBlockDecorate::CreateStep( CEdUndoManager& undoManager, CObject* object, CObject* decorated )
{
	( new CUndoSpawnTreeBlockDecorate( undoManager, object, decorated ) )->PushStep();
}

/*virtual*/ 
void CUndoSpawnTreeBlockDecorate::SetupCustomRelationship( CObject* object, Bool created ) /*override*/
{
	DoSetupRelationShip( object, created );
}

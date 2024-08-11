
#pragma once

#include "undoTreeEditor.h"
#include "spawnTreeEditor.h"

class CUndoSpawnTreeBlockExistance : public CUndoTreeBlockExistance
{
	DECLARE_ENGINE_CLASS( CUndoSpawnTreeBlockExistance, CUndoTreeBlockExistance, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CObject* object );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CObject* object );

	static void FinalizeStep( CEdUndoManager& undoManager, const String& nameOverride = String::EMPTY );

private:
	CUndoSpawnTreeBlockExistance() {}
	CUndoSpawnTreeBlockExistance( CEdUndoManager& undoManager );

	static CUndoSpawnTreeBlockExistance* PrepareStep( CEdUndoManager& undoManager );
	virtual void SetupCustomRelationship( CObject* object, Bool created ) override;
	virtual String GetFriendlyBlockName( CObject* object ) override;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoSpawnTreeBlockExistance, CUndoTreeBlockExistance );

// ------------------------------

class CUndoSpawnTreeBlockDecorate : public CUndoTreeBlockDecorate
{
	DECLARE_ENGINE_CLASS( CUndoSpawnTreeBlockDecorate, CUndoTreeBlockDecorate, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CObject* object, CObject* decorated );

private:
	CUndoSpawnTreeBlockDecorate() {}
	CUndoSpawnTreeBlockDecorate( CEdUndoManager& undoManager, CObject* object, CObject* decorated );

	virtual void SetupCustomRelationship( CObject* object, Bool created ) override;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoSpawnTreeBlockDecorate, CUndoTreeBlockDecorate );

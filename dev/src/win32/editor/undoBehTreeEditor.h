
#pragma once

#include "undoTreeEditor.h"
#include "behTreeGraphEditor.h"

class CUndoBehTreeBlockExistance : public CUndoTreeBlockExistance
{
    DECLARE_ENGINE_CLASS( CUndoBehTreeBlockExistance, CUndoTreeBlockExistance, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object );

	static void PrepareCreationStepWithOffset( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object, Int32 dx, Int32 dy );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, IBehTreeNodeDefinition* object );

	static void FinalizeStep( CEdUndoManager& undoManager, const String& nameOverride = String::EMPTY );

private:
	CUndoBehTreeBlockExistance() {}
	CUndoBehTreeBlockExistance( CEdUndoManager& undoManager );

	static CUndoBehTreeBlockExistance* PrepareStep( CEdUndoManager& undoManager );
	virtual void SetupCustomRelationship( CObject* object, Bool created ) override;
	virtual String GetFriendlyBlockName( CObject* object ) override;

	void ApplyOffsets();

    virtual void DoUndo() override;
    virtual void DoRedo() override;

	struct OffsetInfo
	{
		OffsetInfo() {}
		OffsetInfo( Int32 dx, Int32 dy ) : m_dx( dx ), m_dy( dy ) {}
		Int32 m_dx;
		Int32 m_dy;
	};

	THashMap< IBehTreeNodeDefinition*, OffsetInfo > m_offsets;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehTreeBlockExistance, CUndoTreeBlockExistance );

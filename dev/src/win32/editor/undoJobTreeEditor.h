
#pragma once

#include "undoManager.h"

class CJobActionBase;

class CUndoJobEditorAnimationActivated : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoJobEditorAnimationActivated, IUndoStep, 0 )

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobActionBase* action );

private:
	CUndoJobEditorAnimationActivated() {}
	CUndoJobEditorAnimationActivated( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobActionBase* action );

	void DoStep();

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdJobTreeEditor* m_editor;
	THandle< CJobActionBase > m_action;
	CName m_name;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoJobEditorAnimationActivated, IUndoStep )

//-------------------------------------------------------

class CUndoJobEditorNodeExistance : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoJobEditorNodeExistance, IUndoStep, 0 )

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node );

private:
	CUndoJobEditorNodeExistance() {}
	CUndoJobEditorNodeExistance( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool creating );

	void DoStep( Bool creating );
	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdJobTreeEditor* m_editor;
	THandle< CJobTreeNode > m_parent;
	CJobTreeNode* m_node;
	Uint32 m_index;
	Bool m_creating;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoJobEditorNodeExistance, IUndoStep )

//-------------------------------------------------------

class CUndoJobActionExistance : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoJobActionExistance, IUndoStep, 0 )

public:
	enum Type
	{
		Enter, Leave, FastLeave
	};

	static void CreateCreationStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type );

private:
	CUndoJobActionExistance() {}
	CUndoJobActionExistance( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* node, Type type, Bool creating );

	void StoreAnim();
	void RestoreAnim();

	void DoStep( Bool creating );
	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdJobTreeEditor* m_editor;
	CJobActionBase* m_action;
	THandle< CJobTreeNode > m_node;
	Type m_type;
	Bool m_creating;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoJobActionExistance, IUndoStep )
	
//-------------------------------------------------------

class CUndoJobEditorNodeMove : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoJobEditorNodeMove, IUndoStep, 0 )

	static void CreateStep( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool up );

public:
	CUndoJobEditorNodeMove() {}
	CUndoJobEditorNodeMove( CEdUndoManager& undoManager, CEdJobTreeEditor* editor, CJobTreeNode* parent, CJobTreeNode* node, Bool up );

	void DoStep( Bool up );
	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

private:
	CEdJobTreeEditor* m_editor; 
	THandle< CJobTreeNode > m_parent;
	THandle< CJobTreeNode > m_node;
	Bool m_up;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoJobEditorNodeMove, IUndoStep )


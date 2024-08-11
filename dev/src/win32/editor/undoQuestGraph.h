/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "questGraphEditor.h"
#include "undoGraph.h"
#include "undoStep.h"

class CUndoQuestGraphBlockIO : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoQuestGraphBlockIO, CUndoGraphSocketSnaphot, 0 );

public:
	enum Type
	{
		TERMINATION_INPUT, PATCH_OUTPUT
	};

	static void CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type );

private:
	CUndoQuestGraphBlockIO() {}
	CUndoQuestGraphBlockIO( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestGraphBlock* block, Type type, Bool creating );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	Type m_type;
	Bool m_adding;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoQuestGraphBlockIO, CUndoGraphSocketSnaphot );

// ----------------------------

class CUndoQuestGraphRandomBlockOutput : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoQuestGraphRandomBlockOutput, CUndoGraphSocketSnaphot, 0 );

public:
	static void CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block );

private:
	CUndoQuestGraphRandomBlockOutput() {}
	CUndoQuestGraphRandomBlockOutput( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestRandomBlock* block, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep();

	Bool m_adding;
	TDynArray< CName > m_randomOutputs;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoQuestGraphRandomBlockOutput, CUndoGraphSocketSnaphot );

// ----------------------------

class CUndoQuestGraphVariedInputBlock : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoQuestGraphVariedInputBlock, CUndoGraphSocketSnaphot, 0 );

public:
	static void CreateAddingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block );

private:
	CUndoQuestGraphVariedInputBlock() {}
	CUndoQuestGraphVariedInputBlock( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, CQuestVariedInputsBlock* block, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	Bool m_adding;
	Uint32 m_inputsCount;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoQuestGraphVariedInputBlock, CUndoGraphSocketSnaphot );

// ----------------------------

class CUndoQuestGraphPushPop : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoQuestGraphPushPop , CUndoGraphStep, 0 );

public:
	// Call after pushing
	static void CreatePushingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack );

	// Call before popping
	static void CreatePoppingStep( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack );

private:
	CUndoQuestGraphPushPop() {}
	CUndoQuestGraphPushPop( CEdUndoManager& undoManager, CEdQuestGraphEditor* graphEditor, TDynArray< CQuestGraph* >* stack, Bool pushing );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool pushing );

	Bool m_pushing;
	TDynArray< CQuestGraph* >* m_stack;
	CQuestGraph* m_graph;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoQuestGraphPushPop, CUndoGraphStep );

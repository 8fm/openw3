/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "behaviorGraphEditor.h"
#include "undoGraph.h"
#include "undoStep.h"

class CUndoBehaviorGraphSetRoot : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphSetRoot , CUndoGraphStep, 0 );

public:
	// Call after pushing
	static void CreateStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor );

private:
	CUndoBehaviorGraphSetRoot() {}
	CUndoBehaviorGraphSetRoot( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep();

	CBehaviorGraphContainerNode* m_node;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphSetRoot, CUndoGraphStep );

// ----------------------------

class CUndoBehaviorGraphContainerNodeInput : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphContainerNodeInput, CUndoGraphSocketSnaphot, 0 );

public:
	enum Type
	{
		ANIM_INPUT, VALUE_INPUT, VECTOR_VALUE_INPUT, MIMIC_INPUT
	};

	static void CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name );

private:
	CUndoBehaviorGraphContainerNodeInput() {}
	CUndoBehaviorGraphContainerNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphContainerNode* block, Type type, const CName& name, Bool creating );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	CName m_name;
	Type  m_type;
	Bool  m_adding;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphContainerNodeInput, CUndoGraphSocketSnaphot );

// -------------------------------

class CUndoBehaviorGraphBlendNodeInput : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphBlendNodeInput, CUndoGraphSocketSnaphot, 0 );

public:
	static void CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block );

private:
	CUndoBehaviorGraphBlendNodeInput() {}
	CUndoBehaviorGraphBlendNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphBlendMultipleNode* block, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	Bool m_adding;
	TDynArray< Float > m_inputValues;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphBlendNodeInput, CUndoGraphSocketSnaphot );

// -------------------------------

class CUndoBehaviorGraphRandomNodeInput : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphRandomNodeInput, CUndoGraphSocketSnaphot, 0 );

public:
	static void CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block );

private:
	CUndoBehaviorGraphRandomNodeInput() {}
	CUndoBehaviorGraphRandomNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphRandomNode* block, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	Bool m_adding;
	TDynArray< Float > m_weights;
	TDynArray< Float > m_cooldowns;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphRandomNodeInput, CUndoGraphSocketSnaphot );

// -------------------------------

class CUndoBehaviorGraphSwitchNodeInput : public CUndoGraphSocketSnaphot
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphSwitchNodeInput, CUndoGraphSocketSnaphot, 0 );

public:
	static void CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block );

private:
	CUndoBehaviorGraphSwitchNodeInput() {}
	CUndoBehaviorGraphSwitchNodeInput( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CBehaviorGraphAnimationSwitchNode* block, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	Bool m_adding;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphSwitchNodeInput, CUndoGraphSocketSnaphot );

// -------------------------------

class CEdBehaviorVariableEditor;

class CUndoBehaviorGraphVariableExistance : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoBehaviorGraphVariableExistance, CUndoGraphStep, 0 );

public:
	enum Type
	{
		SCALAR, VECTOR, EVENT, INTERNALSCALAR, INTERNALVECTOR
	};

	static void CreateAddingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type );

	static void CreateRemovingStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type );

private:
	CUndoBehaviorGraphVariableExistance() {}
	CUndoBehaviorGraphVariableExistance( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Type type, Bool adding );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep( Bool adding );

	CEdBehaviorVariableEditor* m_editor;
	CName m_name;
	Bool   m_adding;
	Type   m_type;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviorGraphVariableExistance, CUndoGraphStep );

// -------------------------------

class CUndoBehaviourGraphVariableChange : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoBehaviourGraphVariableChange, CUndoGraphStep, 0 );

public:
	static void PrepareScalarStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Float value );
	static void PrepareVectorStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value );
	static void PrepareInternalScalarStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, Float value );
	static void PrepareInternalVectorStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	enum Type
	{
		SCALAR, VECTOR, INTERNALSCALAR, INTERNALVECTOR
	};

	CUndoBehaviourGraphVariableChange() {}
	CUndoBehaviourGraphVariableChange( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value, Type type );
	static CUndoBehaviourGraphVariableChange* PrepareStep( CEdUndoManager& undoManager, CEdBehaviorGraphEditor* graphEditor, CEdBehaviorVariableEditor* editor, const CName& name, const Vector& value, Type type );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoStep();

	CEdBehaviorVariableEditor* m_editor;
	CName m_name;
	Type   m_type;
	Vector m_prevVal;
	Vector m_newVal;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoBehaviourGraphVariableChange, CUndoGraphStep );

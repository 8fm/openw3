/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IUndoStep : public CObject
{
	friend class CEdUndoManager;
	DECLARE_ENGINE_ABSTRACT_CLASS( IUndoStep, CObject );

public:
	virtual String GetName() = 0;
	virtual String GetTarget() { return String::EMPTY; }

	IUndoStep *GetPrevStep() { return m_prevStep; }
	IUndoStep *GetNextStep() { return m_nextStep; }

protected:
	CEdUndoManager *m_undoManager;

	IUndoStep();
	IUndoStep( CEdUndoManager& undoManager );

	virtual void DoUndo() {}
	virtual void DoRedo() {}
	virtual void OnObjectRemoved( CObject *object ) {}

	// Remember to call PushStep() after the step creation!
	void PushStep();
	// Remove step that is no longer valid
	void PopStep();

private:
	IUndoStep *m_prevStep;
	IUndoStep *m_nextStep;

	size_t m_flushedOffset;
	size_t m_flushedSize;
	Bool   m_flushedToDisk;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IUndoStep, CObject );

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

class CUndoProperty : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoProperty, IUndoStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, const STypedObject& typedObject, CName propertyName );

	// Collect Post change values
	static void FinalizeStep( CEdUndoManager& undoManager );

	// Remove history entries that refers to invalidated properties
	void RemoveInvalidProperties();

private:
	struct SHistoryEntry
	{
		STypedObject        m_typedObject;
		CName               m_propertyName;
		CPropertyDataBuffer m_valuePre;
		CPropertyDataBuffer m_valuePost;

		SHistoryEntry( STypedObject typedObject, CProperty* property )
			: m_typedObject( typedObject ), m_propertyName( property->GetName() )
			, m_valuePre( property ), m_valuePost( property )
		{}
	};

	void DoStep( Bool undo );
	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;
	virtual String GetTarget() override;
	virtual void OnObjectRemoved( CObject *object ) override;

	CUndoProperty() {}
	CUndoProperty( CEdUndoManager& undoManager );

	TDynArray< SHistoryEntry > m_history;
	Bool                       m_initFinished;
	String                     m_targetString;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoProperty, IUndoStep );

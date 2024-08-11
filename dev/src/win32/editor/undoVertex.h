/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

class CVertexEditorEntity;
class CEdRenderingPanel;

class CUndoVertexCreateDestroy : public IUndoStep
{
	CUndoVertexCreateDestroy() {}
	DECLARE_ENGINE_CLASS( CUndoVertexCreateDestroy, IUndoStep, 0 );

private:
	Bool				m_undoCreation;
	CEdRenderingPanel*	m_viewport;

	TDynArray< CVertexEditorEntity* >	m_vertices;

protected:
	virtual void DoUndo();
	virtual void DoRedo();

	CUndoVertexCreateDestroy( CEdUndoManager& undoManager, CEdRenderingPanel* viewport, Bool undoCreation )
		: IUndoStep ( undoManager )
		, m_undoCreation( undoCreation )
		, m_viewport( viewport )
	{}

	// Object serialization interface
	virtual void OnSerialize( IFile& file );

public:
	virtual String GetName()
	{
		return m_undoCreation ? TXT("creation") : TXT("destruction");
	}

	static void CreateStep( CEdUndoManager* undoManager, CEdRenderingPanel* viewport, CVertexEditorEntity * entity, Bool undoCreation );
	static void FinishStep( CEdUndoManager* undoManager );

	virtual void OnObjectRemoved( CObject *object );
};

BEGIN_CLASS_RTTI( CUndoVertexCreateDestroy )
	PARENT_CLASS( IUndoStep );
END_CLASS_RTTI();

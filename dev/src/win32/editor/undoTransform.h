
#pragma once

#include "undoManager.h"
#include "undoStep.h"
#include "../../common/core/engineTransform.h"

class CNode;

class CUndoTransform : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoTransform, IUndoStep, 0 );

public:

	static void CreateStep( CEdUndoManager& undoManager, CNode* node );
	static void CreateStep( CEdUndoManager& undoManager, TDynArray< CNode* > nodes );

private:

	struct Info
	{
		explicit Info( CNode* node )
			: m_node( node ), m_transform( node->GetTransform() )
			{}

		THandle< CNode > m_node;
		EngineTransform m_transform;
	};

	TDynArray< Info > m_infos;

	CUndoTransform() {}
	CUndoTransform( CEdUndoManager& undoManager );

	void DoStep();

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;
	virtual String GetTarget() override;
	virtual void OnObjectRemoved( CObject *object ) override;


	virtual void OnSerialize( IFile& file ) override;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTransform, IUndoStep );

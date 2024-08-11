/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

class CSelectionManager;

class CUndoSelection : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoSelection, IUndoStep, 0 );

public:
    static void CreateStep( CEdUndoManager& undoManager, CSelectionManager &selManager, Uint32 transactionId );

private:
    CUndoSelection() : m_selectionManager(NULL), m_selectedLayer(NULL), m_transactionId(0) {}
    CUndoSelection( CEdUndoManager& undoManager, CSelectionManager &selManager, const TDynArray< CNode* >& selectedNodes, THandle< ISerializable > selectedLayer, Uint32 transactionId );

    CSelectionManager*		m_selectionManager;
    TDynArray<CNode*>		m_selectedNodes;
    THandle<ISerializable>	m_selectedLayer;
    Uint32					m_transactionId;

    virtual void DoUndo() override;
    virtual void DoRedo() override;
    virtual String GetName() override { return TXT("selection changed"); };
    virtual void OnObjectRemoved( CObject *object ) override;

	static void StoreSelection( const CSelectionManager& selManager, TDynArray< CNode* >& selectedNodes, THandle< ISerializable >& selectedLayer );
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoSelection, IUndoStep );

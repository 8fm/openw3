/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "undoStep.h"
#include "undoManager.h"

class CUndoGraphStep : public IUndoStep
{
public:
	CUndoGraphStep() {}

	CUndoGraphStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
		: IUndoStep( undoManager )
		, m_graphEditor( graphEditor )
		{}

protected:
	CEdGraphEditor* GetEditor() const
		{ return m_graphEditor; }

	IGraphContainer* GetGraph() const
	{ 
		ASSERT( m_graphEditor );
		return m_graphEditor->GetGraph(); 
	}

	void GraphStructureModified()
	{
		ASSERT( m_graphEditor );
		m_graphEditor->GraphStructureModified(); 
	}

	CEdGraphEditor* m_graphEditor;
};

// ------------------------

class CUndoGraphConnectionInfo
{
public:
	CUndoGraphConnectionInfo() {}
	CUndoGraphConnectionInfo( CGraphConnection* con );

	void DoStep( Bool create ) const;

private:
	THandle< CGraphSocket >		m_source;
	THandle< CGraphSocket >		m_dest;
	Bool						m_active;
};

// ------------------------

class CUndoGraphSocketInfo
{
public:
	CUndoGraphSocketInfo() {}
	CUndoGraphSocketInfo( CGraphSocket* socket );

	void RestoreConnections() const;
	CGraphSocket* GetSocket() const { return m_socket.Get(); }

private:
	THandle< CGraphSocket >					m_socket;
	TDynArray< CUndoGraphConnectionInfo >	m_connections;
};

// ----------------------

class CUndoGraphBlockExistance : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoGraphBlockExistance, CUndoGraphStep, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block );

	static void FinalizeStep( CEdUndoManager& undoManager );

protected:
	CUndoGraphBlockExistance() {}
	CUndoGraphBlockExistance( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );

	void DoPrepareCreationStep( CGraphBlock* block );
	void DoPrepareDeletionStep( CGraphBlock* block );
    virtual void DoUndo() override;
    virtual void DoRedo() override;

	struct Info
	{
		Info( CGraphBlock* block, CObject* parent )
			: m_block( block )
			, m_parent( parent )
			{ }

		CGraphBlock* m_block;
		CObject*     m_parent;
		TDynArray< TDynArray< CUndoGraphConnectionInfo > > m_socketConnections;
	};

	TDynArray< Info > m_createdBlocks;
	TDynArray< Info > m_deletedBlocks;

private:
	static CUndoGraphBlockExistance* PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );

	virtual String GetName() override;
	virtual void OnSerialize( IFile& file );

	void DoCreationOn( TDynArray< Info >& infos );
	void DoDeletionOn( TDynArray< Info >& infos );
	void StoreConnections( Info & info );
	void RestoreConnections( Info & info );
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphBlockExistance, CUndoGraphStep );

// ------------------------

class CUndoGraphSocketSnaphot : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoGraphSocketSnaphot, CUndoGraphStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block );

protected:
	CUndoGraphSocketSnaphot() {}
	CUndoGraphSocketSnaphot( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block );

    virtual void DoUndo() override;
    virtual void DoRedo() override;

	CGraphBlock* GetBlock() const { return m_block; }

private:
	void StoreSocketsFromBlock();
	void DoStep();
	virtual String GetName() override;
	virtual void OnSerialize( IFile& file );

	TDynArray< CUndoGraphSocketInfo > m_sockets;
	CGraphBlock* m_block;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphSocketSnaphot, CUndoGraphStep );

// ------------------------

class CUndoGraphConnectionExistance : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoGraphConnectionExistance, IUndoStep, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoGraphConnectionExistance() {}
	CUndoGraphConnectionExistance( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );
	static CUndoGraphConnectionExistance* PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );

    virtual void DoUndo();
    virtual void DoRedo();
	virtual String GetName();
	virtual void OnSerialize( IFile& file );

	void DoCreationOn( TDynArray< CUndoGraphConnectionInfo > & infos );
	void DoDeletionOn( TDynArray< CUndoGraphConnectionInfo > & infos );

	TDynArray< CUndoGraphConnectionInfo > m_created;
	TDynArray< CUndoGraphConnectionInfo > m_deleted;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphConnectionExistance, CUndoGraphStep );

// ------------------------

class CUndoGraphBlockLayout : public CUndoGraphStep
{
    DECLARE_ENGINE_CLASS( CUndoGraphBlockLayout, CUndoGraphStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block, CEdGraphEditor::BlockLayoutInfo* layout );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoGraphBlockLayout() {}
	CUndoGraphBlockLayout( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;
	void DoStep();

	struct Info
	{
		CEdGraphEditor::BlockLayoutInfo* m_layout;
		Vector m_size;
	};

	THashMap< CGraphBlock*, Info > m_layoutInfos;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphBlockLayout, CUndoGraphStep );

// --------------------------

class CUndoGraphBlockMove : public CUndoGraphStep
{
	CUndoGraphBlockMove() {}
    DECLARE_ENGINE_CLASS( CUndoGraphBlockMove, CUndoGraphStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoGraphBlockMove( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;
	void DoStep();

	THashMap< CGraphBlock*, Vector > m_positions;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphBlockMove, CUndoGraphStep );

// --------------------------

class CUndoGraphConnectionActivity : public CUndoGraphStep
{
	CUndoGraphConnectionActivity() {}
    DECLARE_ENGINE_CLASS( CUndoGraphConnectionActivity, CUndoGraphStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection, Bool state );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoGraphConnectionActivity( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, Bool state );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;
	void DoStep( Bool enable );

	Bool m_state;
	TDynArray< THandle< CGraphConnection > > m_connections;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphConnectionActivity, CUndoGraphStep );

// --------------------------

class CUndoGraphSocketVisibility : public CUndoGraphStep
{
	CUndoGraphSocketVisibility() {}
    DECLARE_ENGINE_CLASS( CUndoGraphSocketVisibility, CUndoGraphStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphSocket* socket, Bool state );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoGraphSocketVisibility( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, Bool state );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;
	void DoStep( bool show );

	Bool m_state;
	TDynArray< CUndoGraphSocketInfo > m_sockets;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGraphSocketVisibility, CUndoGraphStep );


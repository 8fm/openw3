/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "undoStep.h"
#include "undoManager.h"

class CGridEditor;
class CGridPropertyWrapper;
struct SGridExpandableBlock;


struct SGridElementSnapshot
{
	SGridElementSnapshot() 
		: m_type( NULL ), m_data( NULL ), m_index( NULL )
		{}

	SGridElementSnapshot( const CRTTIArrayType* type, void* data, Int32 index )
		: m_type( type ), m_data( data ), m_index( index )
		{}

	SGridElementSnapshot( const IRTTIType* type, void* data )
		: m_type( type ), m_data( data ), m_index( 0 )
		{}

	void StoreData();

	void RestoreData();

	// Restores old, stores new
	void SwapData();

	const IRTTIType* m_type;
	void* m_data;
	Int32 m_index;
	TDynArray< Uint8 > m_snapshot;
};

// ----------------------------------------------------

class CGridEditorUndoStep : public IUndoStep
{
protected:
	CGridEditorUndoStep()
		{}

	CGridEditorUndoStep( CEdUndoManager& undoManager, CGridEditor* grid )
		: IUndoStep( undoManager )
		, m_grid( grid )
		{}

	CGridEditor* GetGrid() const { return m_grid; }

	CEdUndoManager* GetManager() const { return m_undoManager; }

	void UpdateBlocks() const;

private:
	CGridEditor* m_grid;
};

// ----------------------------------------------------

class CUndoGridValueChange : public CGridEditorUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGridValueChange, CGridEditorUndoStep, 0 );	   

public:
	static void CreateStep( CEdUndoManager& undoManager, CGridEditor* grid, CGridPropertyWrapper* wrapper, Int32 row, Int32 col, const wxString& value );

private:
	CUndoGridValueChange() {}
	CUndoGridValueChange( CEdUndoManager& undoManager, CGridEditor* grid, CGridPropertyWrapper* wrapper, Int32 row, Int32 col, const wxString& value );

	void DoStep();

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName()  override;

	CGridPropertyWrapper* m_wrapper;
	Int32 m_row;
	Int32 m_col;
	wxString m_value;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGridValueChange, CGridEditorUndoStep );

// ----------------------------------------------------

class CUndoGridExpandCell : public CGridEditorUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGridExpandCell, CGridEditorUndoStep, 0 );	   

public:
	static void CreateStep( CEdUndoManager& undoManager, CGridEditor* grid, SGridExpandableBlock* block );

private:
	CUndoGridExpandCell() {}
	CUndoGridExpandCell( CEdUndoManager& undoManager, CGridEditor* grid, SGridExpandableBlock* block );

	void DoStep();

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName()  override;

	SGridExpandableBlock* m_block;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGridExpandCell, CGridEditorUndoStep );

// ----------------------------------------------------

class CUndoGridElementExistance : public CGridEditorUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGridElementExistance, CGridEditorUndoStep, 0 );	   

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index );

private:
	CUndoGridElementExistance() {}
	CUndoGridElementExistance( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index, Bool creating );

	void DoStep( Bool creating );

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName()  override;

	SGridElementSnapshot m_info;
	Bool m_creating;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGridElementExistance, CGridEditorUndoStep );

// ----------------------------------------------------

class CUndoGridElementMove : public CGridEditorUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGridElementMove, CGridEditorUndoStep, 0 );	   

public:
	static void CreateUpStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index );

	static void CreateDownStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index );

private:
	CUndoGridElementMove() {}
	CUndoGridElementMove( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index, Bool up );

	void DoStep( Bool up );

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName()  override;

	SGridElementSnapshot m_info;
	Bool m_up;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGridElementMove, CGridEditorUndoStep );

// ----------------------------------------------------

class CUndoGridElementSnapshot : public CGridEditorUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGridElementSnapshot, CGridEditorUndoStep, 0 );	   

public:
	static void PrepareStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index );

	static void PrepareStep( CEdUndoManager& undoManager, CGridEditor* grid, const IRTTIType* type, void *data );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	static CUndoGridElementSnapshot* DoPrepareStep( CEdUndoManager& undoManager, CGridEditor* grid );

	CUndoGridElementSnapshot() {}
	CUndoGridElementSnapshot( CEdUndoManager& undoManager, CGridEditor* grid );

	void DoStep();

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName()  override;

	TDynArray< SGridElementSnapshot > m_snapshots;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGridElementSnapshot, CGridEditorUndoStep );

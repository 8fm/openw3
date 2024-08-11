/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"
#include "gridEditor.h"
#include "gridPropertyWrapper.h"
#include "undoGridEditor.h"

void SGridElementSnapshot::StoreData()
{
	ASSERT( m_snapshot.Empty() );

	if ( m_type->GetType() == RT_Array )
	{
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( m_type );
		m_snapshot.Resize( arrayType->GetInnerType()->GetSize() );
		void* elemData = arrayType->GetArrayElement( m_data, m_index );
		arrayType->GetInnerType()->Copy( m_snapshot.Data(), elemData );
	}
	else
	{
		m_snapshot.Resize( m_type->GetSize() );
		m_type->Copy( m_snapshot.Data(), m_data );	
	}
}

void SGridElementSnapshot::RestoreData()
{
	if ( m_type->GetType() == RT_Array )
	{
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( m_type );
		void* elemData = arrayType->GetArrayElement( m_data, m_index );
		arrayType->GetInnerType()->Copy( elemData, m_snapshot.Data() );
	}
	else
	{
		m_snapshot.Resize( m_type->GetSize() );
		m_type->Copy( m_data, m_snapshot.Data() );
	}

	m_snapshot.Clear();
}

void SGridElementSnapshot::SwapData()
{
	SGridElementSnapshot tmp;
	tmp.m_type  = m_type;
	tmp.m_data  = m_data;
	tmp.m_index = m_index;
	tmp.StoreData();
	RestoreData();
	m_snapshot = tmp.m_snapshot;
}

// ---------------------------------------------------

void CGridEditorUndoStep::UpdateBlocks() const 
{ 
	GetGrid()->UpdateBlocks(); 
}

// ---------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGridValueChange );

CUndoGridValueChange::CUndoGridValueChange( CEdUndoManager& undoManager, CGridEditor* grid , CGridPropertyWrapper* wrapper, Int32 row, Int32 col, const wxString& value )
	: CGridEditorUndoStep( undoManager, grid )
	, m_wrapper( wrapper )
	, m_row( row )
	, m_col( col )
	, m_value( value )
{
}

/*static*/ 
void CUndoGridValueChange::CreateStep( CEdUndoManager& undoManager, CGridEditor* grid , CGridPropertyWrapper* wrapper, Int32 row, Int32 col, const wxString& value )
{
	if ( CUndoGridValueChange* curStep = Cast< CUndoGridValueChange >( undoManager.GetCurrentStep() ) )
	{
		if ( curStep->GetGrid() == grid && curStep->m_wrapper == wrapper && curStep->m_row == row && curStep->m_col == col && curStep->m_value == value )
		{
			// skip repetitive calls on the same property
			return;
		}
	}

	CUndoGridValueChange* step = new CUndoGridValueChange( undoManager, grid, wrapper, row, col, value );
	step->PushStep();
}

void CUndoGridValueChange::DoStep()
{
	const IRTTIType* type;
	wxString prevVal = m_wrapper->GetValue( m_row, m_col, type );
	m_wrapper->SetValue( m_row, m_col, m_value );
	m_value = prevVal;
	UpdateBlocks();
}

/*virtual*/ 
void CUndoGridValueChange::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoGridValueChange::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoGridValueChange::GetName()
{
	return TXT("grid value change");
}

// ---------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGridExpandCell );

CUndoGridExpandCell::CUndoGridExpandCell( CEdUndoManager& undoManager, CGridEditor* grid, SGridExpandableBlock* block )
	: CGridEditorUndoStep( undoManager, grid )
	, m_block( block )
{
}

/*static*/ 
void CUndoGridExpandCell::CreateStep( CEdUndoManager& undoManager, CGridEditor* grid, SGridExpandableBlock* block )
{
	CUndoGridExpandCell* step = new CUndoGridExpandCell( undoManager, grid, block );
	step->PushStep();
}

void CUndoGridExpandCell::DoStep()
{
	m_block->m_expanded = !m_block->m_expanded;
	UpdateBlocks();
}

/*virtual*/ 
void CUndoGridExpandCell::DoUndo()
{
	DoStep();
}

/*virtual*/
void CUndoGridExpandCell::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoGridExpandCell::GetName()
{
	return TXT("expanding grid block");
}

// ---------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGridElementExistance );

CUndoGridElementExistance::CUndoGridElementExistance( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index, Bool creating )
	: CGridEditorUndoStep( undoManager, grid )
	, m_info( type, data, index )
	, m_creating( creating )
{
}

/*static*/ 
void CUndoGridElementExistance::CreateCreationStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void* data, Int32 index )
{
	CUndoGridElementExistance* step = new CUndoGridElementExistance( undoManager, grid, type, data, index, true );
	step->PushStep();
}

/*static*/ 
void CUndoGridElementExistance::CreateDeletionStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void* data, Int32 index )
{
	CUndoGridElementExistance* step = new CUndoGridElementExistance( undoManager, grid, type, data, index, false );
	step->m_info.StoreData();
	step->PushStep();
}

void CUndoGridElementExistance::DoStep( bool creating )
{
	const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( m_info.m_type );

	if ( creating )
	{
		arrayType->InsertArrayElementAt( m_info.m_data, m_info.m_index );
		m_info.RestoreData();
	}
	else
	{
		m_info.StoreData();
		arrayType->DeleteArrayElement( m_info.m_data, m_info.m_index );
	}

	UpdateBlocks();
}

/*virtual*/ 
void CUndoGridElementExistance::DoUndo()
{
	DoStep( !m_creating );
}

/*virtual*/ 
void CUndoGridElementExistance::DoRedo()
{
	DoStep( m_creating );
}

/*virtual*/ 
String CUndoGridElementExistance::GetName()
{
	return m_creating ? TXT("creating grid element") : TXT("removing grid element");
}

// ------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGridElementMove );

CUndoGridElementMove::CUndoGridElementMove( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index, Bool up )
	: CGridEditorUndoStep( undoManager, grid )
	, m_info( type, data, index )
	, m_up( up )
{
}

/*static*/ 
void CUndoGridElementMove::CreateUpStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index )
{
	CUndoGridElementMove* step = new CUndoGridElementMove( undoManager, grid, type, data, index, true );
	--step->m_info.m_index;
	step->PushStep();
}

/*static*/ 
void CUndoGridElementMove::CreateDownStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index )
{
	CUndoGridElementMove* step = new CUndoGridElementMove( undoManager, grid, type, data, index, false );
	++step->m_info.m_index;
	step->PushStep();
}

void CUndoGridElementMove::DoStep( Bool up )
{
	m_info.StoreData();

	const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( m_info.m_type );

	if ( up )
	{
		arrayType->InsertArrayElementAt( m_info.m_data, m_info.m_index - 1 );
		arrayType->DeleteArrayElement( m_info.m_data, m_info.m_index + 1 );
		--m_info.m_index;
		m_info.RestoreData();
	}
	else
	{
		arrayType->InsertArrayElementAt( m_info.m_data, m_info.m_index + 2 );
		arrayType->DeleteArrayElement( m_info.m_data, m_info.m_index );
		++m_info.m_index;
		m_info.RestoreData();
	}

	UpdateBlocks();
}

/*virtual*/ 
void CUndoGridElementMove::DoUndo()
{
	DoStep( !m_up );
}

/*virtual*/ 
void CUndoGridElementMove::DoRedo()
{
	DoStep( m_up );
}

/*virtual*/ 
String CUndoGridElementMove::GetName()
{
	return String( TXT("grid element move ") ) + ( m_up ? TXT("up") : TXT("down") );
}

// ------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGridElementSnapshot );

CUndoGridElementSnapshot::CUndoGridElementSnapshot( CEdUndoManager& undoManager, CGridEditor* grid )
	: CGridEditorUndoStep( undoManager, grid )
{
}

/*static*/ 
CUndoGridElementSnapshot* CUndoGridElementSnapshot::DoPrepareStep( CEdUndoManager& undoManager, CGridEditor* grid )
{
	CUndoGridElementSnapshot* step = undoManager.SafeGetStepToAdd< CUndoGridElementSnapshot >();

	if ( !step )
	{
		step = new CUndoGridElementSnapshot( undoManager, grid );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

/*static*/ 
void CUndoGridElementSnapshot::PrepareStep( CEdUndoManager& undoManager, CGridEditor* grid, const CRTTIArrayType* type, void *data, Int32 index )
{
	CUndoGridElementSnapshot* step = DoPrepareStep( undoManager, grid );
	step->m_snapshots.PushBack( SGridElementSnapshot( type, data, index ) );
	step->m_snapshots.Back().StoreData();
}

/*static*/ 
void CUndoGridElementSnapshot::PrepareStep( CEdUndoManager& undoManager, CGridEditor* grid, const IRTTIType* type, void *data )
{
	CUndoGridElementSnapshot* step = DoPrepareStep( undoManager, grid );
	step->m_snapshots.PushBack( SGridElementSnapshot( type, data ) );
	step->m_snapshots.Back().StoreData();
}

/*static*/ 
void CUndoGridElementSnapshot::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGridElementSnapshot* step = undoManager.SafeGetStepToAdd< CUndoGridElementSnapshot >() )
	{
		step->PushStep();
	}
}

void CUndoGridElementSnapshot::DoStep( )
{
	for ( Uint32 i=0; i<m_snapshots.Size(); ++i )
	{
		m_snapshots[i].SwapData();
	}

	UpdateBlocks();
}

/*virtual*/ 
void CUndoGridElementSnapshot::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoGridElementSnapshot::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoGridElementSnapshot::GetName()
{
	return TXT("");
}

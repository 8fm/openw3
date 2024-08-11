#include "build.h"

#include "undoStep.h"
#include "undoManager.h"

IMPLEMENT_ENGINE_CLASS( IUndoStep );

IUndoStep::IUndoStep() 
	: m_undoManager( nullptr )
	, m_prevStep( nullptr )
	, m_nextStep( nullptr )
	, m_flushedOffset( -1 )
	, m_flushedSize( 0 )
	, m_flushedToDisk( false ) 
{
}

IUndoStep::IUndoStep( CEdUndoManager& undoManager )
	: m_undoManager( &undoManager )
	, m_prevStep( nullptr )
	, m_nextStep( nullptr )
	, m_flushedOffset( -1 )
	, m_flushedSize( 0 )
	, m_flushedToDisk( false )
{
	SetParent( &undoManager );
}

void IUndoStep::PushStep()
{
	m_undoManager->PushStep( this );
}

void IUndoStep::PopStep()
{
	m_undoManager->PopStep( this );
}

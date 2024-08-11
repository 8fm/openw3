/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "autoSizeListCtrl.h"
#include "undoManager.h"
#include "undoHistoryFrame.h"

CEdUndoHistoryFrame::CEdUndoHistoryFrame( wxWindow* parent )
	: m_undoManager( nullptr )
	, m_historyChangedWhileHidden( false )
	, m_currentStepIndex( 0 )
{
	// Load resource
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("UndoHistory") );

	// Get controls
    m_stepsList = XRCCTRL( *this, "m_stepsList", CEdAutosizeListCtrl );
    m_clearButton = XRCCTRL( *this, "m_clearButton", wxButton );
    m_undoButton = XRCCTRL( *this, "m_undoButton", wxButton );
    m_redoButton = XRCCTRL( *this, "m_redoButton", wxButton );

	// Connect events
	m_stepsList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxCommandEventHandler( CEdUndoHistoryFrame::OnStepsListSelected ), NULL, this );
	m_stepsList->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxCommandEventHandler( CEdUndoHistoryFrame::OnStepsListActivated ), NULL, this );
    m_clearButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUndoHistoryFrame::OnClearButtonClicked ), NULL, this );
    m_undoButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUndoHistoryFrame::OnUndoButtonClicked ), NULL, this );
    m_redoButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUndoHistoryFrame::OnRedoButtonClicked ), NULL, this );
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdUndoHistoryFrame::OnClose ), NULL, this );
	Connect( wxEVT_SHOW, wxShowEventHandler( CEdUndoHistoryFrame::OnShow ), NULL, this );

	// Setup the list
	m_stepsList->InsertColumn( 0, wxT("#"), 0, 30 );
	m_stepsList->InsertColumn( 1, wxT("Step"), 0, 200 );
	m_stepsList->InsertColumn( 2, wxT("Target"), 0, -1 );

	SetClientSize( 464, 550 );
}

CEdUndoHistoryFrame::~CEdUndoHistoryFrame()
{
	// Disconnect undo manager if it is connected
	SetUndoManager( nullptr );
}

void CEdUndoHistoryFrame::OnStepsListSelected( wxCommandEvent& event )
{
	UpdateButtonStates();
}

void CEdUndoHistoryFrame::OnStepsListActivated( wxCommandEvent& event )
{
	// Forward action to another event handler (undo/redo) depending
	// on the currently selected step index
	if ( GetSelectedStepIndex() > m_currentStepIndex )
	{
		OnRedoButtonClicked( wxCommandEvent() );
	}
	else if ( GetSelectedStepIndex() < m_currentStepIndex )
	{
		OnUndoButtonClicked( wxCommandEvent() );
	}	
}

void CEdUndoHistoryFrame::OnClearButtonClicked( wxCommandEvent& event )
{
	ASSERT( m_undoManager, TXT("No undo manager associated with the undo history window, this will crash!") );

	// Confirm with the user
	if ( wxMessageBox( wxT("Are you sure that you want to clear the undo history?"), wxT("Clear Undo History"), wxCENTER|wxYES_NO|wxICON_QUESTION, this ) != wxYES )
	{
		return;
	}

	// Clear the history
	m_undoManager->ClearHistory();
}

void CEdUndoHistoryFrame::OnUndoButtonClicked( wxCommandEvent& event )
{
	ASSERT( m_undoManager, TXT("No undo manager associated with the undo history window, this will crash!") );
	
	// Perform as many undo steps as necessary
	for ( Int32 steps = m_currentStepIndex - GetSelectedStepIndex(); steps > 0; --steps )
	{
		m_undoManager->Undo();
	}

	RaiseAndFocus();
}

void CEdUndoHistoryFrame::OnRedoButtonClicked( wxCommandEvent& event )
{
	ASSERT( m_undoManager, TXT("No undo manager associated with the undo history window, this will crash!") );

	// Perform as many redo steps as necessary
	for ( Int32 steps = GetSelectedStepIndex() - m_currentStepIndex; steps > 0; --steps )
	{
		m_undoManager->Redo();
	}

	RaiseAndFocus();
}

void CEdUndoHistoryFrame::OnClose( wxCloseEvent& event )
{
	// Hide the window if a close request comes
	event.Veto();
	Show( false );
}

void CEdUndoHistoryFrame::OnShow( wxShowEvent& event )
{
	if ( event.IsShown() && m_historyChangedWhileHidden )
	{
		RefreshHistory( true );
		m_historyChangedWhileHidden = false;
	}
}

Int32 CEdUndoHistoryFrame::GetSelectedStepIndex() const
{
	long itemIndex = -1;
	itemIndex = m_stepsList->GetNextItem( itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	return (Int32)itemIndex;
}

void CEdUndoHistoryFrame::UpdateButtonStates()
{
	Int32 selectedIndex = GetSelectedStepIndex();
	m_clearButton->Enable( m_undoManager != nullptr );
	m_undoButton->Enable( m_undoManager != nullptr && selectedIndex != -1 && m_currentStepIndex != -1 && selectedIndex < m_currentStepIndex );
	m_redoButton->Enable( m_undoManager != nullptr && selectedIndex != -1 && m_currentStepIndex != -1 && selectedIndex > m_currentStepIndex );
}

void CEdUndoHistoryFrame::RefreshHistoryNow()
{
	if ( !m_undoManager )
	{
		return;
	}

	const TUndoHistory& history = m_undoManager->GetHistory();

	m_stepsList->Freeze();
	m_stepsList->DeleteAllItems();

	// Update item count
// 	Uint32 currentCount = m_stepsList->GetItemCount();
// 	if ( currentCount < history.Size() )
// 	{
// 		for ( Uint32 i=currentCount; i < history.Size(); ++i )
// 		{
// 			m_stepsList->InsertItem( 0, wxEmptyString );
// 		}
// 	}
// 	else while ( currentCount > history.Size() )
// 	{
// 		m_stepsList->DeleteItem( 0 );
// 		--currentCount;
// 	}

	m_currentStepIndex = -1;

	// Create items
	for ( Int32 i = 0; i < history.SizeInt(); ++i )
	{
		const SUndoHistoryEntry& hist = history[i];
		if ( hist.m_current )
		{
			m_currentStepIndex = i;
		}

		m_stepsList->InsertItem( i, wxString::Format( wxT("%d"), i ) );
		m_stepsList->SetItem( i, 1, hist.m_name.AsChar() );
		m_stepsList->SetItem( i, 2, hist.m_target.AsChar() );
	}

	// Update items once we have m_currentStepIndex properly set
	for ( Int32 i = 0; i < history.SizeInt(); ++i )
	{
		const SUndoHistoryEntry& hist = history[i];
		m_stepsList->SetItemTextColour( i, i > m_currentStepIndex ? wxSystemSettings::GetColour( wxSYS_COLOUR_BTNSHADOW ) : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
		m_stepsList->SetItemBackgroundColour( i, hist.m_current ? wxColor( 194, 221, 224 ) : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
		
	}

	// Select the current step
	Int32 selectedStepIndex = GetSelectedStepIndex();

	// Deselect previously selected step (if any)
	if ( selectedStepIndex != -1 )
	{
		m_stepsList->Select( selectedStepIndex, false );
	}

	m_stepsList->Thaw();

	// Select current item
	if ( m_currentStepIndex != -1 )
	{
		m_stepsList->Select( m_currentStepIndex, true );
		m_stepsList->EnsureVisible( m_currentStepIndex );
	}

	// Update buttons
	UpdateButtonStates();
}

void CEdUndoHistoryFrame::OnUndoHistoryChanged()
{
	RefreshHistory();
}

void CEdUndoHistoryFrame::SetUndoManager( CEdUndoManager* undoManager )
{
	// Ignore requests to set the undo manager to the same object
	if ( undoManager == m_undoManager )
	{
		return;
	}

	// Detatch from old manager
	if ( m_undoManager )
	{
		m_undoManager->SetUndoListener( nullptr );
	}

	m_undoManager = undoManager;

	// Attach to new manager
	if ( m_undoManager )
	{
		m_undoManager->SetUndoListener( this );
	}

	RefreshHistory();
}

void CEdUndoHistoryFrame::RaiseAndFocus()
{
	Iconize( false );
	Raise();
	Show();
	SetFocus();
	m_stepsList->SetFocus();
}

void CEdUndoHistoryFrame::RefreshHistory( Bool ignoreVisibility /* = false */ )
{
	// If the frame is not visible, do not refresh the history now
	// to avoid unnecessary updates - just set a flag to update
	// the history when the frame is shown next time
	if ( !ignoreVisibility && !IsShown() )
	{
		m_historyChangedWhileHidden = true;
		return;
	}

	// Setup a task to refresh the history later when the editor
	// is idle, in order to avoid refreshes while undo-able operations
	// are performed
	RunLaterOnce( [ this ](){ RefreshHistoryNow(); } );
}

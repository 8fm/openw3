#include "build.h"
#include "dialogEditorElement.h"
#include "dialogEditorActions.h"
#include "undoDialogEditor.h"

#include "../../common/game/actor.h"
#include "../../common/game/storySceneElement.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdStorySceneElementPanel, wxPanel )
	EVT_CHAR( CEdStorySceneElementPanel::OnCharPressed )
	EVT_NAVIGATION_KEY( CEdStorySceneElementPanel::OnNavigationRequest )
	EVT_CONTEXT_MENU( CEdStorySceneElementPanel::OnElementContextMenu )
	EVT_LEFT_UP( CEdStorySceneElementPanel::OnElementLeftClick )
	EVT_LEFT_DOWN( CEdStorySceneElementPanel::OnLeftDownInElementChild )
	EVT_MENU( wxID_STORYSCENEEDITOR_COPYLINE, CEdStorySceneElementPanel::OnCopy )
	EVT_MENU( wxID_STORYSCENEEDITOR_CUTLINE, CEdStorySceneElementPanel::OnCut )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTELINE, CEdStorySceneElementPanel::OnPaste )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDLINE, CEdStorySceneElementPanel::OnAddDialogLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_INSERTLINE, CEdStorySceneElementPanel::OnInsertDialogLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDCOMMENT, CEdStorySceneElementPanel::OnAddComment )
	EVT_MENU( wxID_STORYSCENEEDITOR_INSERTCOMMENT, CEdStorySceneElementPanel::OnInsertComment )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDSCRIPTLINE, CEdStorySceneElementPanel::OnAddScriptLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_INSERTSCRIPTLINE, CEdStorySceneElementPanel::OnInsertScriptLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_DELETELINE, CEdStorySceneElementPanel::OnDeleteLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_CYCLEELEMENTTYPE, CEdStorySceneElementPanel::OnCycleElementType )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTEELEMENTS, CEdStorySceneElementPanel::OnPasteElements )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHANGETOSCENELINE, CEdStorySceneElementPanel::OnChangeElementToLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHANGETOCOMMENT, CEdStorySceneElementPanel::OnChangeElementToComment )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHANGETOSCRIPTLINE, CEdStorySceneElementPanel::OnChangeElementToScriptLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHANGETOCHOICE, CEdStorySceneElementPanel::OnChangeElementToChoice )
	EVT_MENU( wxID_STORYSCENEEDITOR_CHANGETOQUESTCHOICELINE, CEdStorySceneElementPanel::OnChangeElementToQuestChoiceLine )
	EVT_CHILD_FOCUS( CEdStorySceneElementPanel::OnElementChildFocus )
END_EVENT_TABLE()

wxIMPLEMENT_CLASS( CEdStorySceneElementPanel, wxPanel );

void CEdStorySceneElementPanel::OnPanelSelected( wxMouseEvent& event )
{		
	if ( m_sectionPanel->IsInElementSelectionMode() == false )
	{
		m_sectionPanel->EnterElementSelectionMode( this );
	}
	else
	{
		m_sectionPanel->HandleElementSelection( this, true );
	}
}

CEdStorySceneElementPanel::CEdStorySceneElementPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, EStorySceneElementType type )
	: m_sectionPanel( sectionPanel ), m_undoManager( undoManager ), m_elementType( type ), m_updatingGui( false )
{
	ASSERT( m_undoManager != NULL );
	Bind( wxEVT_LEFT_DOWN, &CEdStorySceneElementPanel::OnPanelSelected, this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPreUndoStep ), this );
}

CEdStorySceneElementPanel::~CEdStorySceneElementPanel(void)
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdStorySceneElementPanel::CommitChanges()
{
	ImplCommitChanges();
}

void CEdStorySceneElementPanel::EnableShortcuts( wxWindow* window, Bool addLineOnEnter /*= true */ )
{
	window->Bind( wxEVT_CHAR, &CEdStorySceneElementPanel::OnCharPressed, this );
	window->Bind( wxEVT_CONTEXT_MENU, &CEdStorySceneElementPanel::OnElementContextMenu, this );
	window->Bind( wxEVT_LEFT_DOWN, &CEdStorySceneElementPanel::OnLeftDownInElementChild, this );
	if ( addLineOnEnter == true )
	{
		window->Bind( wxEVT_KEY_DOWN, &CEdStorySceneElementPanel::OnEnterPressed, this );
	}
}

EStorySceneElementType CEdStorySceneElementPanel::NextElementType()
{
	return m_elementType;
}

void CEdStorySceneElementPanel::FillContextMenu( wxMenu& contextMenu )
{
	// Insert element submenu (before current element)
	wxMenu* subMenu = new wxMenu();
	if ( m_sectionPanel->CanAddDialogLine( m_sectionPanel->FindDialogElementIndex( this ), false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_INSERTLINE, wxT( "Dialog line" ) );
	}
	if ( m_sectionPanel->CanAddComment( m_sectionPanel->FindDialogElementIndex( this ), false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_INSERTCOMMENT, wxT( "Comment" ) );
	}
	if ( m_sectionPanel->CanAddScriptLine( m_sectionPanel->FindDialogElementIndex( this ), false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_INSERTSCRIPTLINE, wxT( "Script line" ) );
	}
	contextMenu.AppendSubMenu( subMenu, wxT("Insert element") );

	// Add element submenu (after current element)
	subMenu = new wxMenu();
	if ( m_sectionPanel->CanAddDialogLine( m_sectionPanel->FindDialogElementIndex( this ), true ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDLINE, wxT( "Dialog line" ) );
	}
	if ( m_sectionPanel->CanAddComment( m_sectionPanel->FindDialogElementIndex( this ), true ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDCOMMENT, wxT( "Comment" ) );
	}
	if ( m_sectionPanel->CanAddScriptLine( m_sectionPanel->FindDialogElementIndex( this ), true ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDSCRIPTLINE, wxT( "Script line" ) );
	}
	if ( m_sectionPanel->CanAddChoice() )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDCHOICE, wxT( "Choice" ) );
	}	
	contextMenu.AppendSubMenu( subMenu, wxT("Add element") );

	// Change current element type
	Int32 index = GetParent()->GetChildren().IndexOf( this );
	Int32 numberOfSizerItems = GetParent()->GetChildren().GetCount();
	Bool canChangeToChoice = ( m_sectionPanel->CanAddChoice() == true && index == numberOfSizerItems - 1 );

	subMenu = new wxMenu();
	if ( m_elementType != SSET_Line )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_CHANGETOSCENELINE, wxT( "to Dialog line" ) );
	}
	if ( m_elementType != SSET_Comment )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_CHANGETOCOMMENT, wxT( "to Comment" ) );
	}
	if ( m_elementType != SSET_Choice && canChangeToChoice == true )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_CHANGETOCHOICE, wxT( "to Choice" ) );
	}
	if ( m_elementType != SSET_ScriptLine )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_CHANGETOSCRIPTLINE, wxT( "to Script line" ) );
	}
	if ( m_elementType != SSET_QuestChoiceLine )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_CHANGETOQUESTCHOICELINE, wxT( "to Quest choice line" ) );
	}
	contextMenu.AppendSubMenu( subMenu, wxT( "Change element type" ) );

	// delete element
	if ( m_sectionPanel->CanRemoveDialogElement( m_sectionPanel->FindDialogElementIndex( this ) ) )
	{
		contextMenu.Append( wxID_STORYSCENEEDITOR_DELETELINE, wxT( "Delete element" ) );
	}

	contextMenu.AppendSeparator();

	contextMenu.Append( wxID_STORYSCENEEDITOR_COPYLINE, wxT( "Copy" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_CUTLINE, wxT( "Cut" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_PASTELINE, wxT( "Paste" ) );
	
	contextMenu.AppendSeparator();

	subMenu = new wxMenu();
	subMenu->Append( wxID_STORYSCENEEDITOR_ADDSECTION, wxT( "Add" ) );
	subMenu->Append( wxID_STORYSCENEEDITOR_DELETESECTION, wxT( "Delete" ) );
	contextMenu.AppendSubMenu( subMenu, wxT("Section") );

	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneSectionPanel::OnAddDialogChoice, m_sectionPanel, wxID_STORYSCENEEDITOR_ADDCHOICE );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneSectionPanel::OnRemoveSection, m_sectionPanel, wxID_STORYSCENEEDITOR_DELETESECTION );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditorScreenplayPanel::OnAddSection, m_sectionPanel->GetStorySceneEditor(), wxID_STORYSCENEEDITOR_ADDSECTION );
}

bool CEdStorySceneElementPanel::SetBackgroundColour( const wxColour& colour )
{
	bool hasColourChanged = __super::SetBackgroundColour( colour );
	if ( hasColourChanged == true )
	{
		for ( Uint32 i = 0; i < m_children.GetCount(); ++i )
		{
			m_children[ i ]->SetBackgroundColour( colour );
		}
	}

	return hasColourChanged;
}

void CEdStorySceneElementPanel::MarkElementAsSelected()
{
	SetBackgroundColour( wxColour( 229, 229, 255 ) );
	//OnSelected();	
}

void CEdStorySceneElementPanel::UnmarkElementAsSelected()
{
	SetBackgroundColour( *wxWHITE );
}

Bool CEdStorySceneElementPanel::IsSelected()
{
	return m_sectionPanel->IsElementSelected( this );
}

bool CEdStorySceneElementPanel::Destroy()
{
	OnDestroy();
	return wxPanel::Destroy();
}

void CEdStorySceneElementPanel::EmulateMenuEvent( Int32 menuItemId )
{
	wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
	commandEvent.SetId( menuItemId );
	commandEvent.SetEventObject( this );
	ProcessEvent( commandEvent );
}

Bool CEdStorySceneElementPanel::ConfirmElementChange()
{
	return wxMessageBox( 
		wxT( "You are trying to change type of element which is not empty. Are you sure?" ), 
		wxT( "Confirm operation" ), wxYES_NO | wxCENTRE ) == wxYES;
}

Uint32 CEdStorySceneElementPanel::GetStringWordCount( const String& text ) const
{
	Bool wasLastCharAWhiteChar = true;
	const Char* character = text.AsChar();
	Uint32 words = 0;
	
	while ( *character != 0 )
	{
		if ( *character == L' ' || *character == L'\t' || *character == L'\n' || *character == L'\r' )
		{
			wasLastCharAWhiteChar = true;
		}
		else
		{
			if ( wasLastCharAWhiteChar == true )
			{
				words += 1;
			}
			wasLastCharAWhiteChar = false;	
		}

		++character;
	}
	
	return words;
}

void CEdStorySceneElementPanel::OnCharPressed( wxKeyEvent& event )
{
	if ( m_sectionPanel->IsInElementSelectionMode() == false )
	{
		if ( event.GetKeyCode() == WXK_INSERT && event.GetModifiers() == wxMOD_CMD )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_ADDLINE );
		}
		else if ( event.GetKeyCode() == WXK_INSERT && event.GetModifiers() == ( wxMOD_CMD | wxMOD_SHIFT ) )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_INSERTLINE );
		}
		// 46 is a raw key code for delete which is the only way to distinguish it from backspace
		else if ( event.GetRawKeyCode() == 46 && event.GetModifiers() == wxMOD_CMD )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_DELETELINE );
		}
		else if ( ( event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN ) 
			&& event.GetModifiers() == ( wxMOD_CMD | wxMOD_SHIFT ) )
		{
			m_sectionPanel->EnterElementSelectionMode( this );
		}
		// Apparently 1 is ctrl+a
		else if ( event.GetKeyCode() == 1 && event.GetModifiers() & ( wxMOD_SHIFT ) )
		{
			m_sectionPanel->EnterElementSelectionMode( this );
		}
		// 22 is a key code for ctrl+v combination (it is translated earlier inside wxWidgets)
		else if ( event.GetKeyCode() == 22 && wxTheClipboard->IsSupported( TXT( "StorySceneElements" ) ) )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTEELEMENTS );
		}
		else if ( event.GetKeyCode() == 22 && wxTheClipboard->IsSupported( TXT( "StorySceneSections" ) ) )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTESECTIONS );
		}
		else
		{
			event.Skip();
		}
	}
	else
	{
		if ( event.GetKeyCode() == WXK_UP && ( event.GetModifiers() & ( wxMOD_CMD | wxMOD_SHIFT ) ) != 0 )
		{
			m_sectionPanel->HandleElementSelection( m_sectionPanel->GetElementBefore( this ) );
		}
		else if ( event.GetKeyCode() == WXK_DOWN && ( event.GetModifiers() & ( wxMOD_CMD | wxMOD_SHIFT ) ) != 0  )
		{
			m_sectionPanel->HandleElementSelection( m_sectionPanel->GetElementAfter( this ) );
		}
		// 3 is a key code for ctrl+c combination (it is translated earlier inside wxWidgets)
		else if ( event.GetKeyCode() == 3 )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_COPYELEMENTS );
		}
		// 24 is a key code for ctrl+x combination (it is translated earlier inside wxWidgets)
		else if ( event.GetKeyCode() == 24 )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_CUTELEMENTS );
		}
		// 22 is a key code for ctrl+v combination (it is translated earlier inside wxWidgets)
		else if ( event.GetKeyCode() == 22 && wxTheClipboard->IsSupported( TXT( "StorySceneElements" ) ) )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTEELEMENTS );
		}
		else if ( event.GetKeyCode() == 22 && wxTheClipboard->IsSupported( TXT( "StorySceneSections" ) ) )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTESECTIONS );
		}
		else if( event.GetKeyCode() == WXK_DELETE && event.GetModifiers() == wxMOD_CONTROL )
		{
			m_sectionPanel->GetStorySceneEditor()->DoEditDelete();
		}
		else
		{
			m_sectionPanel->LeaveElementSelectionMode();
			OnDeselected();
			event.Skip();
		}
	}
}

void CEdStorySceneElementPanel::OnCopy( wxCommandEvent& event )
{
	if ( m_sectionPanel->IsInElementSelectionMode() == true )
	{
		EmulateMenuEvent( wxID_STORYSCENEEDITOR_COPYELEMENTS );
		return;
	}

	if ( m_lastContextMenuObject->IsKindOf( CLASSINFO( wxTextCtrl ) ) )
	{
		wxTextCtrl* textControl = static_cast<wxTextCtrl*>( m_lastContextMenuObject );
		textControl->Copy();
	}
}

void CEdStorySceneElementPanel::OnCut( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_sectionPanel->IsInElementSelectionMode() == true )
		{
			// Get all events associated with dialog element, no matter section variant it belongs to.
			TDynArray< const CStorySceneEvent* > evts;
			TDynArray< CStorySceneSectionVariantId > sectionVariantIds;
			m_sectionPanel->GetSection()->EnumerateVariants( sectionVariantIds );
			for( auto sectionVariantId : sectionVariantIds )
			{
				m_sectionPanel->GetSection()->GetEventsForElement( evts, GetDialogElement(), sectionVariantId );
			}

			if( !evts.Empty() )
			{
				GFeedback->ShowMsg( TXT("Can't cut element"), TXT( "Can't cut element because it has some events associated with it." ) );
				return;
			}

			EmulateMenuEvent( wxID_STORYSCENEEDITOR_CUTELEMENTS );
			return;
		}

		if ( m_lastContextMenuObject->IsKindOf( CLASSINFO( wxTextCtrl ) ) )
		{
			wxTextCtrl* textControl = static_cast<wxTextCtrl*>( m_lastContextMenuObject );
			textControl->Cut();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnPaste( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( wxTheClipboard->IsSupported( TXT( "StorySceneElements" ) ) == true )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTEELEMENTS );
			return;
		}
		if ( wxTheClipboard->IsSupported( TXT( "StorySceneSections" ) ) == true )
		{
			EmulateMenuEvent( wxID_STORYSCENEEDITOR_PASTESECTIONS );
			return;
		}

		if ( m_lastContextMenuObject->IsKindOf( CLASSINFO( wxTextCtrl ) ) )
		{
			wxTextCtrl* textControl = static_cast<wxTextCtrl*>( m_lastContextMenuObject );
			textControl->Paste();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnAddDialogLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddDialogLine( lineIndex, true ) )
		{
			CAbstractStorySceneLine* newElement = m_sectionPanel->AddDialogLine( lineIndex, true );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex + 1 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnAddComment( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddComment( lineIndex, true ) )
		{
			CStorySceneComment* newElement = m_sectionPanel->AddComment( lineIndex, true );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex + 1 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnAddScriptLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddScriptLine( lineIndex, true ) )
		{
			CStorySceneScriptLine* newElement = m_sectionPanel->AddScriptLine( lineIndex, true );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex + 1 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnInsertDialogLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddDialogLine( lineIndex, false ) )
		{
			CAbstractStorySceneLine* newElement = m_sectionPanel->AddDialogLine( lineIndex, false );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnInsertComment( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddComment( lineIndex, false ) )
		{
			CStorySceneComment* newElement = m_sectionPanel->AddComment( lineIndex, false );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnInsertScriptLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );

		if ( m_sectionPanel->CanAddScriptLine( lineIndex, false ) )
		{
			CStorySceneScriptLine* newElement = m_sectionPanel->AddScriptLine( lineIndex, false );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnDeleteLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		const CStorySceneSection* section = m_sectionPanel->GetSection();
		CStorySceneElement* lineElement = GetDialogElement();

		// Get all events associated with line element.
		TDynArray< CStorySceneEvent* > lineEvts;
		for( CStorySceneEvent* evt : section->GetEventsFromAllVariants() )
		{
			if( evt->GetSceneElement() == lineElement )
			{
				lineEvts.PushBack( evt );
			}
		}

		// If line element has any events then create pause element that will replace it and take over all its events.
		if( !lineEvts.Empty() )
		{
			CEdSceneEditor* mediator = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor();

			// Get approved duration of line element from default variant (we consider default variant
			// the most important one so we use it even if user is currently using different variant).
			const CStorySceneSectionVariantId defVariantId = section->GetDefaultVariant();
			Float lineDuration = section->GetElementApprovedDuration( defVariantId, lineElement->GetElementID() );
			if( const Bool lineDurationNotApproved = ( lineDuration <= 0.0f ) )
			{
				lineDuration = lineElement->CalculateDuration();
			}

			// Create pause element in place of line element.
			CStoryScenePauseElement* pauseElement = CreateObject< CStoryScenePauseElement >( m_sectionPanel->GetSection() );
			pauseElement->SetDuration( lineDuration );
			const Uint32 insertionIndex = section->GetElements().GetIndex( lineElement );
			mediator->OnTimeline_AddSceneElement( section, pauseElement, insertionIndex );
			mediator->OnScreenplayPanel_RequestRebuildImmediate();

			// Approve pause element duration in all variants.
			TDynArray< CStorySceneSectionVariantId > sectionVariantIds;
			m_sectionPanel->GetSection()->EnumerateVariants( sectionVariantIds );
			for( const CStorySceneSectionVariantId sectionVariantId : sectionVariantIds )
			{
				mediator->OnTimeline_ApproveElementDuration( section, sectionVariantId, pauseElement, lineDuration );
			}

			// Associate line element events with pause element.
			for( CStorySceneEvent* evt : lineEvts )
			{
				if( evt->GetSceneElement() == lineElement )
				{
					evt->SetSceneElement( pauseElement );
				}
			}
		}

		// Delete line element.
		Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );
		if ( m_sectionPanel->CanRemoveDialogElement( lineIndex ) )
		{
			m_sectionPanel->RemoveDialogElement( this );
			m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->OnScreenplayPanel_RequestRebuildImmediate();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnCycleElementType( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		m_sectionPanel->ChangeDialogElementType( this, NextElementType() );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnEnterPressed( wxKeyEvent& event )
{
	wxWindow* eventSource = wxStaticCast( event.GetEventObject(), wxWindow );

	const Bool isEnterBlockedByChildEditor = ( eventSource->GetClientData() != NULL && eventSource->GetChildren().IsEmpty() == false );

	if ( event.GetKeyCode() == WXK_RETURN && isEnterBlockedByChildEditor == false )
	{
		const Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
		if( privilegeEditDialogs )
		{
			Int32 lineIndex = m_sectionPanel->FindDialogElementIndex( this );
			CAbstractStorySceneLine* newElement = m_sectionPanel->AddDialogLine( lineIndex );
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, m_sectionPanel, newElement, lineIndex + 1 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
		else
		{
			GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
		}
	}
	else
	{
		event.Skip();
	}
}

void CEdStorySceneElementPanel::OnElementContextMenu( wxContextMenuEvent& event )
{
	m_lastContextMenuObject = wxWindow::FindFocus();

	wxMenu contextMenu;

	FillContextMenu( contextMenu );

	PopupMenu( &contextMenu );
}

void CEdStorySceneElementPanel::OnElementChildFocus( wxChildFocusEvent& event )
{
	if( m_updatingGui )
	{
		return;
	}

	m_sectionPanel->GetStorySceneEditor()->OnElementPanelFocus( m_sectionPanel->GetSection(), GetDialogElement() );
}

void CEdStorySceneElementPanel::OnPasteElements( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		event.SetEventObject( this );
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnNavigationRequest( wxNavigationKeyEvent& event )
{
	Bool isNavigateDownOnLastElement 
		= ( event.GetDirection() == true && event.GetEventObject() == GetLastNavigableWindow() );
	Bool isNavigateUpOnFirstElement 
		= ( event.GetDirection() == false && event.GetEventObject() == GetFirstNavigableWindow() );
	
	if ( isNavigateDownOnLastElement || isNavigateUpOnFirstElement )
	{
		m_sectionPanel->GetEventHandler()->ProcessEvent( event );
	}
	else
	{
		event.Skip();
	}
}

void CEdStorySceneElementPanel::OnDestroy()
{
}

void CEdStorySceneElementPanel::OnElementLeftClick( wxMouseEvent& event )
{
	//m_sectionPanel->LeaveElementSelectionMode();
}

void CEdStorySceneElementPanel::OnLeftDownInElementChild( wxMouseEvent& event )
{
	Bool isShiftDown = wxIsShiftDown();
	Bool isCtrlDown = wxIsCtrlDown();

	if ( m_sectionPanel->IsInElementSelectionMode() == false && isCtrlDown == true && isShiftDown == true )
	{
		m_sectionPanel->EnterElementSelectionMode( this );
	}
	else if ( m_sectionPanel->IsInElementSelectionMode() == true && isShiftDown == false && isCtrlDown == false )
	{
		m_sectionPanel->LeaveElementSelectionMode();
		OnDeselected();
	}
	else if ( m_sectionPanel->IsInElementSelectionMode() == true && ( isShiftDown == true || isCtrlDown == true ) )
	{
		m_sectionPanel->HandleElementSelection( this, true );
	}
	else
	{
		event.Skip();
	}
}

Bool CEdStorySceneElementPanel::RefreshDialogObject( CObject* objectToRefresh )
{
	if ( GetDialogElement() == objectToRefresh )
	{
		RefreshData();
		return true;
	}
	return false;
}

void CEdStorySceneElementPanel::OnChangeElementToLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		ASSERT( m_elementType != SSET_Line );
		m_sectionPanel->ChangeDialogElementType( this, SSET_Line );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnChangeElementToComment( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		ASSERT( m_elementType != SSET_Comment );
		m_sectionPanel->ChangeDialogElementType( this, SSET_Comment );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnChangeElementToScriptLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		ASSERT( m_elementType != SSET_ScriptLine );
		m_sectionPanel->ChangeDialogElementType( this, SSET_ScriptLine );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnChangeElementToChoice( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		ASSERT( m_elementType != SSET_Choice );
		m_sectionPanel->ChangeDialogElementType( this, SSET_Choice );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::OnChangeElementToQuestChoiceLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		ASSERT( m_elementType != SSET_QuestChoiceLine );
		m_sectionPanel->ChangeDialogElementType( this, SSET_QuestChoiceLine );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneElementPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPreUndoStep ) ) 
	{
		if ( GetEventData< CEdUndoManager* >( data ) == m_undoManager )
		{
			if ( wxWindow* focused = wxWindow::FindFocus() )
			{
				if ( IsDescendant( focused ) )
				{
					// Steal the focus from the embedded editor to force finalization of the undo step.
					SetFocus();
				}
			}
		}
	}
}

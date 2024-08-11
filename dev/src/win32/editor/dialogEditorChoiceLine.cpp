#include "build.h"
#include "dialogEditorChoiceLine.h"

#include "dialogEditor.h"
#include "dialogEditorActions.h"
#include "undoDialogEditor.h"

#include "textControl.h"

#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/core/feedback.h"

wxIMPLEMENT_CLASS( CEdStorySceneChoiceLinePanel, wxPanel );

BEGIN_EVENT_TABLE( CEdStorySceneChoiceLinePanel, wxPanel )
	EVT_MENU( wxID_STORYSCENEEDITOR_COPYLINE, CEdStorySceneChoiceLinePanel::OnCopy )
	EVT_MENU( wxID_STORYSCENEEDITOR_CUTLINE, CEdStorySceneChoiceLinePanel::OnCut )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTELINE, CEdStorySceneChoiceLinePanel::OnPaste )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDCHOICELINE, CEdStorySceneChoiceLinePanel::OnAddChoiceLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_INSERTCHOICELINE, CEdStorySceneChoiceLinePanel::OnInsertChoiceLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_DELETECHOICELINE, CEdStorySceneChoiceLinePanel::OnDeleteChoiceLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_MOVECHOICELINEUP, CEdStorySceneChoiceLinePanel::OnMoveChoiceLineUp )
	EVT_MENU( wxID_STORYSCENEEDITOR_MOVECHOICELINEDOWN, CEdStorySceneChoiceLinePanel::OnMoveChoiceLineDown )
	EVT_MENU( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, CEdStorySceneChoiceLinePanel::OnMakeCopyUnique )
	EVT_CHILD_FOCUS( CEdStorySceneChoiceLinePanel::OnChoiceLineChildFocus )
END_EVENT_TABLE()

CEdStorySceneChoiceLinePanel::CEdStorySceneChoiceLinePanel( CEdStorySceneChoicePanel* choicePanel, CEdUndoManager* undoManager, Uint32 lineIndex )
	: m_choicePanel( choicePanel )
	, m_undoManager( undoManager )
	, m_lastContextMenuObject( NULL )
	, m_choiceLine( NULL )
	, m_choiceCommentField( NULL )
	, m_choiceContentField( NULL )
	, m_storySceneEditor( NULL )
{
	Create( choicePanel );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_indexLabel = new wxStaticText( this, wxID_ANY, wxT("1)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_indexLabel->Wrap( -1 );
	m_indexLabel->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	mainSizer->Add( m_indexLabel, 0, wxLEFT, 5 );

	wxPanel* choiceInteriorPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	choiceInteriorPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );
	wxBoxSizer* choicePanelSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* choiceTextSizer = new wxBoxSizer( wxHORIZONTAL );
	m_choiceActionIcon = new wxStaticBitmap( choiceInteriorPanel, wxID_ANY, wxNullBitmap );
	m_choiceActionIcon->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	m_choiceActionField = new wxStaticText( choiceInteriorPanel, wxID_ANY, wxEmptyString );
	m_choiceActionField->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );
	m_choiceActionField->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	m_choiceContentField = new CEdTextControl( choiceInteriorPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER|wxTE_RICH|wxNO_BORDER );
	m_choiceContentField->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );
	
	choiceTextSizer->Add( m_choiceActionIcon, 0, wxALL, 5 );
	choiceTextSizer->Add( m_choiceActionField, 0, wxALL, 5 );
	choiceTextSizer->Add( m_choiceContentField, 1, wxALL, 5 );

	choicePanelSizer->Add( choiceTextSizer, 1, wxEXPAND );
	//choicePanelSizer->Layout();

	m_choiceCommentField = new CEdTextControl( choiceInteriorPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTE_RICH );
	m_choiceCommentField->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );
	m_choiceCommentField->Hide();
	choicePanelSizer->Add( m_choiceCommentField, 0, wxEXPAND, 5 );

	m_postLinkPanel = new wxPanel( choiceInteriorPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_postLinkPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* postLinkPanelSizer;
	postLinkPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_postLinkPanel->SetSizer( postLinkPanelSizer );
	m_postLinkPanel->Layout();
	postLinkPanelSizer->Fit( m_postLinkPanel );
	choicePanelSizer->Add( m_postLinkPanel, 0, wxEXPAND, 5 );

	choiceInteriorPanel->SetSizer( choicePanelSizer );
	choiceInteriorPanel->Layout();
	choicePanelSizer->Fit( choiceInteriorPanel );
	mainSizer->Add( choiceInteriorPanel, 1, wxEXPAND|wxRIGHT, 5 );

	SetSizer( mainSizer );
	Layout();
	mainSizer->Fit( this );

	//////////////////////////////////////////////////////////////////////////

	SetChoiceLineIndex( lineIndex );

	m_storySceneEditor = m_choicePanel->GetSectionPanel()->GetStorySceneEditor();
	m_storySceneEditor->RegisterChoiceLine( this );

	m_choiceContentField->SetHint( wxT( "Choice line" ) );
	m_choiceCommentField->SetHint( wxT( "Choice comment" ) );

	CEdStorySceneHandlerFactory* handlerFactory = m_storySceneEditor->GetHandlerFactory();
	
	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateAutoExpandHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_choiceContentField );

	m_choicePanel->EnableShortcuts( m_choiceContentField, false );

	m_choiceContentField->Bind( wxEVT_RIGHT_DOWN, &CEdStorySceneChoiceLinePanel::OnContextMenu, this );
	m_choiceContentField->Bind( wxEVT_CHAR, &CEdStorySceneChoiceLinePanel::OnKeyPressed, this );
	m_choiceContentField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneChoiceLinePanel::OnPasteText, this );
	m_choiceContentField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneChoiceLinePanel::OnCutText, this );
	m_choiceContentField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneChoiceLinePanel::OnChoiceLinePaste, this );
	m_choiceContentField->Bind( wxEVT_LEFT_DOWN, &CEdStorySceneChoiceLinePanel::OnLeftDown, this );
	m_choiceContentField->Bind( wxEVT_KEY_DOWN, &CEdStorySceneChoiceLinePanel::OnEnterPressed, this );
	m_choiceContentField->Bind( wxEVT_EDTC_STRING_UPDATED, &CEdStorySceneChoiceLinePanel::OnChoiceLineChanged, this );
	m_choiceContentField->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneChoiceLinePanel::OnChoiceLineLostFocus, this );

	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_choiceCommentField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_choiceCommentField );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_choiceCommentField );

	m_choiceCommentField->Bind( wxEVT_RIGHT_DOWN, &CEdStorySceneChoiceLinePanel::OnContextMenu, this );
	m_choiceCommentField->Bind( wxEVT_CHAR, &CEdStorySceneChoiceLinePanel::OnKeyPressed, this );
	m_choiceCommentField->Bind( wxEVT_LEFT_DOWN, &CEdStorySceneChoiceLinePanel::OnLeftDown, this );
	m_choiceCommentField->Bind( wxEVT_EDTC_STRING_UPDATED, &CEdStorySceneChoiceLinePanel::OnCommentLineChanged, this );
	
	m_choiceContentField->Bind( wxEVT_NAVIGATION_KEY, &CEdStorySceneChoiceLinePanel::OnNavigate, this );

	SEvents::GetInstance().RegisterListener( CNAME( SceneChoiceLineLinkChanged ), this );

	ChangeFontSize( m_storySceneEditor->GetFontSize() );
}

CEdStorySceneChoiceLinePanel::~CEdStorySceneChoiceLinePanel()
{
	m_storySceneEditor->UnregisterChoiceLine( this );
	
	SEvents::GetInstance().UnregisterListener( this );

	if ( m_storySceneEditor->ShouldIgnoreLinkUpdateOnElementDelete() == true )
	{
		return;
	}

	while ( m_connectedSections.Empty() == false )
	{
		CEdSceneSectionPanel* nextSection = m_connectedSections.PopBack();

		wxControl* nextSectionControl =  *( m_nextSectionsLinks.FindPtr( nextSection ) );
		ASSERT( nextSectionControl );
		nextSection->UnregisterControlToThis( nextSectionControl );
		RemoveControl( nextSectionControl );
		m_nextSectionsLinks.Erase( nextSection );

		nextSection->RemovePreLink( m_choicePanel->GetSectionPanel() );
	}
}

void CEdStorySceneChoiceLinePanel::SetChoiceLineIndex( Uint32 index )
{
	m_indexLabel->SetLabel( wxString::Format( wxT( "%d)" ), index ) );
}

Bool CEdStorySceneChoiceLinePanel::IsEmpty()
{
	Bool hasDefaultValue = m_choiceContentField->GetValue().StartsWith( wxT( "Choice" ) );

	return m_choiceContentField->IsEmpty() || hasDefaultValue;
}

void CEdStorySceneChoiceLinePanel::UpdateColors()
{
	if ( m_choiceLine == NULL ) return;

	wxColour bgColor;
	if ( m_choiceLine->GetChoice()->IsSceneElementCopy() )
	{
		bgColor = wxColour( 255, 255, 0 );
	}
	else if ( m_choicePanel->IsSelected() == true )
	{
		bgColor = wxColour( 229, 229, 255 );
	}
	else if ( m_choiceLine->HasCondition() == true && m_choiceLine->HasMemo() == true ) 
	{
		bgColor = wxColour( 223, 188, 102 );
	}
	else if ( m_choiceLine->HasCondition() == true && m_choiceLine->HasMemo() == false ) 
	{
		bgColor = wxColour( 191, 255, 181 );
	}
	else if ( m_choiceLine->HasCondition() == false && m_choiceLine->HasMemo() == true ) 
	{
		bgColor = wxColour( 255, 188, 102 );
	}
	else
	{
		bgColor = wxColour( 255, 255, 255 );
	}

	m_choiceContentField->SetBackgroundColour( bgColor );
	m_choiceCommentField->SetBackgroundColour( bgColor );

//	To Debug content field sizes
// 	m_choiceContentField->SetBackgroundColour( wxColour( 0,255,0 ) );
// 	m_choiceCommentField->SetBackgroundColour( wxColour( 255,0,0 ) );
}

void CEdStorySceneChoiceLinePanel::SetFocus()
{
	m_choiceContentField->SetFocus();
}

wxString CEdStorySceneChoiceLinePanel::GetChoiceContent() const
{
	return m_choiceContentField->GetValue();
}

wxString CEdStorySceneChoiceLinePanel::GetChoiceComment() const
{
	return m_choiceCommentField->GetValue();
}

void CEdStorySceneChoiceLinePanel::SetChoiceContent( const wxString& text )
{
	m_choiceContentField->ChangeValue( text );
}

void CEdStorySceneChoiceLinePanel::SetChoiceLine( CStorySceneChoiceLine* choiceLine )
{
	m_choiceLine = choiceLine;
	RefreshData();
}

void CEdStorySceneChoiceLinePanel::RestoreSectionLink()
{
	CStorySceneLinkElement* connectedSection = m_choiceLine->GetNextElement();
	if ( connectedSection != NULL )
	{
		RefreshConnectedSections();
	}
}

void CEdStorySceneChoiceLinePanel::RefreshData()
{
	m_choiceContentField->ChangeValue( m_choiceLine->GetChoiceLine().AsChar() );
	m_choiceCommentField->ChangeValue( m_choiceLine->GetChoiceComment().AsChar() );
	m_choiceActionField->SetLabel( m_choiceLine->GetChoiceActionText().AsChar() );
	
	wxBitmap actionIcon = SEdResources::GetInstance().LoadBitmap( CEnum::ToString( m_choiceLine->GetChoiceActionIcon() ).AsChar() );
	if ( actionIcon.IsOk() == true )
	{
		m_choiceActionIcon->Show();
		m_choiceActionIcon->SetBitmap( actionIcon );
	}
	else
	{
		m_choiceActionIcon->Hide();
	}

	if ( !m_choiceCommentField->IsEmpty() )
	{
		m_choiceCommentField->Show();
		m_choiceCommentField->GetParent()->Layout();
	}

	Layout();

	UpdateColors();

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().DispatchEvent( CNAME( SceneChoiceLineChanged ), CreateEventData( m_choiceLine ) );
#endif
}

void CEdStorySceneChoiceLinePanel::RefreshConnectedSections()
{
	m_storySceneEditor->PreChangeSceneElements();

	while ( m_connectedSections.Empty() == false )
	{
		CEdSceneSectionPanel* nextSection = m_connectedSections.PopBack();

		wxControl* nextSectionControl =  *( m_nextSectionsLinks.FindPtr( nextSection ) );
		ASSERT( nextSectionControl );
		nextSection->UnregisterControlToThis( nextSectionControl );
		RemoveControl( nextSectionControl );
		m_nextSectionsLinks.Erase( nextSection );

		nextSection->RemovePreLink( m_choicePanel->GetSectionPanel() );
	}

	TDynArray< CStorySceneControlPart* > connectedControlParts;
	CStorySceneControlPart* nextControlPart 
		= Cast< CStorySceneControlPart >( m_choiceLine->GetNextElement() );
	if ( nextControlPart != NULL )
	{
		nextControlPart->CollectControlParts( connectedControlParts );
	}

	for ( TDynArray< CStorySceneControlPart* >::iterator iter = connectedControlParts.Begin();
		iter != connectedControlParts.End(); ++iter )
	{
		if ( (*iter)->IsA< CStorySceneSection >() == false )
		{
			continue;
		}

		CStorySceneSection* connectedSection = Cast< CStorySceneSection >( *iter );

		CEdSceneSectionPanel* connectedSectionPanel 
			= m_storySceneEditor->FindSectionPanel( connectedSection );
		
		ASSERT( connectedSectionPanel );
		if ( connectedSectionPanel )
		{
			m_connectedSections.PushBackUnique( connectedSectionPanel );
		}
	}

	for ( TDynArray< CEdSceneSectionPanel* >::iterator iter = m_connectedSections.Begin();
		iter != m_connectedSections.End(); ++iter )
	{
		CEdSceneSectionPanel* sectionPanel = *iter;
		AddPostLink( sectionPanel );
		sectionPanel->AddPreLink( m_choicePanel->GetSectionPanel(), this );
	}

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdStorySceneChoiceLinePanel::RemoveControl( wxWindow* control )
{
	wxWindow* parent = control->GetParent();
	parent->GetSizer()->Detach( control );
	parent->RemoveChild( control );
	delete control;
}

void CEdStorySceneChoiceLinePanel::ChangeFontSize( Int32 sizeChange )
{
	m_storySceneEditor->ChangeWindowFontSize( m_choiceCommentField, sizeChange );
	m_storySceneEditor->ChangeWindowFontSize( m_choiceContentField, sizeChange );
	m_storySceneEditor->ChangeWindowFontSize( m_indexLabel, sizeChange );

	for( THashMap< CEdSceneSectionPanel*, wxControl* >::iterator iter = m_nextSectionsLinks.Begin();
		iter != m_nextSectionsLinks.End(); ++iter )
	{
		m_storySceneEditor->ChangeWindowFontSize( iter->m_second, sizeChange );
	}
}

bool CEdStorySceneChoiceLinePanel::SetBackgroundColour( const wxColour& colour )
{
	if ( m_storySceneEditor == NULL || m_storySceneEditor->IsInitialized() == false )
	{
		return wxPanel::SetBackgroundColour( colour );
	}

	if ( m_choiceCommentField != NULL )
	{
		m_choiceCommentField->SetBackgroundColour( colour );
	}
	if ( m_choiceContentField != NULL )
	{
		m_choiceContentField->SetBackgroundColour( colour );
	}
	return wxPanel::SetBackgroundColour( colour );
}

void CEdStorySceneChoiceLinePanel::AddPostLink( CEdSceneSectionPanel* linkedSection )
{
	m_storySceneEditor->PreChangeSceneElements();

	wxStaticText* link = new wxStaticText( m_postLinkPanel, wxID_ANY, 
		linkedSection->GetSectionName(), wxDefaultPosition, wxDefaultSize, 0 );

	link->SetFont( wxFont( 7, 70, 90, 90, true, wxEmptyString ) );

	m_postLinkPanel->GetSizer()->Add( link, 0, wxALL, 1 );

	link->SetClientData( linkedSection );
	m_storySceneEditor->GetHandlerFactory()->CreateHyperlinkHandler()->ConnectTo( link );

	linkedSection->RegisterControlToThis( link );

	m_nextSectionsLinks.Insert( linkedSection, link );

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdStorySceneChoiceLinePanel::OnContextMenu( wxMouseEvent& event )
{
	wxMenu contextMenu;

	m_lastContextMenuObject = event.GetEventObject();

	contextMenu.Append( wxID_STORYSCENEEDITOR_INSERTCHOICELINE, wxT( "Insert choice line" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_ADDCHOICELINE, wxT( "Add choice line" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_DELETECHOICELINE, wxT( "Delete choice line" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_MOVECHOICELINEUP, wxT( "Move choice line up" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_MOVECHOICELINEDOWN, wxT( "Move choice line down" ) );
	contextMenu.AppendSeparator();
	contextMenu.Append( wxID_STORYSCENEEDITOR_DELETELINE, wxT( "Delete choice" ) );
	contextMenu.AppendSeparator();
	contextMenu.Append( wxID_STORYSCENEEDITOR_COPYLINE, wxT( "Copy" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_CUTLINE, wxT( "Cut" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_PASTELINE, wxT( "Paste" ) );
	contextMenu.AppendSeparator();

	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) == false )
	{
		contextMenu.Enable( wxID_STORYSCENEEDITOR_COPYLINE, false );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_CUTLINE, false );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_PASTELINE, false );
	}

	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdStorySceneElementPanel::OnDeleteLine, m_choicePanel, wxID_STORYSCENEEDITOR_DELETELINE );


	if ( m_choiceLine && m_choiceLine->GetChoice() && m_choiceLine->GetChoice()->IsSceneElementCopy() )
	{
		contextMenu.Append( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, wxT( "Make copy unique" ) );
	}

	PopupMenu( &contextMenu );
}

void CEdStorySceneChoiceLinePanel::OnAddChoiceLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 index = m_choicePanel->FindChoiceLineIndex( this );
		CStorySceneChoiceLine* newElement = m_choicePanel->AddChoiceLine( index, true );

		CUndoDialogChoiceLineExistance::PrepareCreationStep( *m_undoManager, m_choicePanel, newElement, index + 1 );
		CUndoDialogChoiceLineExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnInsertChoiceLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 index = m_choicePanel->FindChoiceLineIndex( this );
		CStorySceneChoiceLine* newElement = m_choicePanel->AddChoiceLine( index, false );

		CUndoDialogChoiceLineExistance::PrepareCreationStep( *m_undoManager, m_choicePanel, newElement, index );
		CUndoDialogChoiceLineExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnDeleteChoiceLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		Int32 index = m_choicePanel->FindChoiceLineIndex( this );
		CUndoDialogChoiceLineExistance::PrepareDeletionStep( *m_undoManager, m_choicePanel, m_choiceLine, index );
		CUndoDialogChoiceLineExistance::FinalizeStep( *m_undoManager );

		m_choicePanel->RemoveChoiceLine( this );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnCopy( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) )
	{
		wxTextCtrl* textControl = static_cast<wxTextCtrl*>( m_lastContextMenuObject );
		textControl->Copy();
	}
}

void CEdStorySceneChoiceLinePanel::OnCut( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) )
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

void CEdStorySceneChoiceLinePanel::OnPaste( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) )
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

void CEdStorySceneChoiceLinePanel::OnKeyPressed( wxKeyEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( event.GetKeyCode() == '(' && event.GetEventObject() != m_choiceCommentField )
		{
			m_storySceneEditor->PreChangeSceneElements();
			m_choiceCommentField->Show();
			m_choiceCommentField->SetFocus();
			m_storySceneEditor->PostChangeSceneElements( this );
		}
		else if ( event.GetKeyCode() == WXK_INSERT && event.GetModifiers() == wxMOD_CONTROL )
		{
			wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
			commandEvent.SetId( wxID_STORYSCENEEDITOR_ADDCHOICELINE );
			ProcessEvent( commandEvent );
		}
		else if ( event.GetKeyCode() == WXK_RETURN && event.GetEventObject() == m_choiceCommentField )
		{
			m_choiceContentField->SetFocus();
		}
		else if ( event.GetKeyCode() == WXK_DELETE && event.ControlDown() )
		{
			wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
			commandEvent.SetId( wxID_STORYSCENEEDITOR_DELETECHOICELINE );
			ProcessEvent( commandEvent );
		}
		else if ( event.GetKeyCode() == WXK_DOWN )
		{
			NavigateDown( static_cast< wxWindow* >( event.GetEventObject() ) );
		}
		else if ( event.GetKeyCode() == WXK_UP )
		{
			NavigateUp( static_cast< wxWindow* >( event.GetEventObject() ) );
		}
		else
		{
			CEdSceneEditorScreenplayPanel* editor = m_choicePanel->GetSectionPanel()->GetStorySceneEditor();
			CUndoDialogTextChange::PrepareChoiceLineStep( *m_undoManager, editor, m_choiceLine, event.GetKeyCode() );
			event.Skip();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnPasteText( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnCutText( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnChoiceLinePaste( wxClipboardTextEvent& event )
{
	CEdSceneEditorScreenplayPanel* editor = m_choicePanel->GetSectionPanel()->GetStorySceneEditor();
	CUndoDialogTextChange::PrepareChoiceLineStep( *m_undoManager, editor, m_choiceLine );
	event.Skip();
}

void CEdStorySceneChoiceLinePanel::OnChoiceLineLostFocus( wxFocusEvent& event )
{
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );
	event.Skip();
}

void CEdStorySceneChoiceLinePanel::OnEnterPressed( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_RETURN && event.GetEventObject() == m_choiceContentField )
	{
		wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
		commandEvent.SetId( wxID_STORYSCENEEDITOR_ADDCHOICELINE );
		ProcessEvent( commandEvent );
	}
	else
	{
		event.Skip();
	}
}

void CEdStorySceneChoiceLinePanel::OnNavigate( wxNavigationKeyEvent& event )
{
	if ( event.GetEventObject() == m_choiceContentField )
	{
		
		if ( event.GetDirection() == true 
			&& this == m_choicePanel->GetChoiceLines()[ m_choicePanel->GetChoiceLines().Size() - 1 ]
		)
		{
			m_choicePanel->GetEventHandler()->ProcessEvent( event );
		}
		else
		{
			event.SetEventObject( this );
			event.SetCurrentFocus( this );
			m_choicePanel->GetEventHandler()->ProcessEvent( event );
		}

	}
	else
	{
		event.Skip();
	}
}

void CEdStorySceneChoiceLinePanel::OnCommentLineChanged( wxCommandEvent& event )
{
	if ( m_choiceLine )
	{
		m_choiceLine->SetChoiceComment( m_choiceCommentField->GetValue().wc_str() );
	}

	if ( m_storySceneEditor != NULL )
	{
		m_storySceneEditor->RefreshWordCount();
	}

}
void CEdStorySceneChoiceLinePanel::OnChoiceLineChanged( wxCommandEvent& event )
{
	if ( m_choiceLine )
	{
		if ( m_choiceContentField->GetForegroundColour() == *wxLIGHT_GREY )
		{
			m_choiceLine->SetChoiceLine( String::EMPTY );
		}
		else
		{
			wxString choiceLineText = m_choiceContentField->GetValue();
			m_choiceLine->SetChoiceLine( choiceLineText.wc_str() );
		}

		if ( m_storySceneEditor != NULL )
		{
			m_storySceneEditor->RefreshWordCount();
		}
		
	}
	event.Skip();
}

void CEdStorySceneChoiceLinePanel::OnChoiceLineChildFocus( wxChildFocusEvent& event )
{
	m_storySceneEditor->OnChoiceLineChildFocus( m_choicePanel->GetSectionPanel()->GetSection(), m_choiceLine );
}

void CEdStorySceneChoiceLinePanel::OnMakeCopyUnique( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_storySceneEditor->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_choiceLine && m_choiceLine->GetChoice() && m_choiceLine->GetChoice()->IsSceneElementCopy() )
		{
			m_choiceLine->GetChoice()->MakeCopyUnique();

			CEdStorySceneChoicePanel::TChoiceLinePanelArray &panels = m_choicePanel->GetChoiceLines();
			for ( CEdStorySceneChoicePanel::TChoiceLinePanelArray::iterator panelI = panels.Begin(); panelI != panels.End(); ++panelI )
			{
				(*panelI)->UpdateColors();
			}
			UpdateColors();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoiceLinePanel::OnLeftDown( wxMouseEvent& event )
{
	Bool isShiftDown = wxIsShiftDown();
	Bool isCtrlDown = wxIsCtrlDown();

	CEdSceneSectionPanel* sectionPanel = m_choicePanel->GetSectionPanel();

	if ( sectionPanel->IsInElementSelectionMode() == false && isCtrlDown == true && isShiftDown == true )
	{
		sectionPanel->EnterElementSelectionMode( m_choicePanel );
	}
	else if ( sectionPanel->IsInElementSelectionMode() == true && isShiftDown == false && isCtrlDown == false )
	{
		sectionPanel->LeaveElementSelectionMode();
		OnDeselected();
	}
	else if ( sectionPanel->IsInElementSelectionMode() == true && ( isShiftDown == true || isCtrlDown == true ) )
	{
		sectionPanel->HandleElementSelection( m_choicePanel, true );
	}
	else
	{
		event.Skip();
	}
}

void CEdStorySceneChoiceLinePanel::OnSelected()
{
	m_choiceContentField->HideNativeCaret();
	m_choiceCommentField->HideNativeCaret();
	UpdateColors();
}

void CEdStorySceneChoiceLinePanel::OnDeselected()
{
	m_choiceContentField->ShowNativeCaret();
	m_choiceCommentField->ShowNativeCaret();
	UpdateColors();
}

void CEdStorySceneChoiceLinePanel::NavigateDown( wxWindow* currentWindow )
{
	if ( currentWindow == m_choiceCommentField )
	{
		m_choiceContentField->SetFocus();
	}
	else
	{
		wxNavigationKeyEvent navigationEvent;
		navigationEvent.SetCurrentFocus( this );
		navigationEvent.SetDirection( true );
		navigationEvent.SetEventObject( currentWindow );
		
		m_choicePanel->GetEventHandler()->ProcessEvent( navigationEvent );
	}
}

void CEdStorySceneChoiceLinePanel::NavigateUp( wxWindow* currentWindow )
{
	if ( currentWindow == m_choiceContentField && m_choiceCommentField->IsShown() == true )
	{
		m_choiceCommentField->SetFocus();
	}
	else
	{
		wxNavigationKeyEvent navigationEvent;
		navigationEvent.SetCurrentFocus( this );
		navigationEvent.SetDirection( false );
		navigationEvent.SetEventObject( currentWindow );

		m_choicePanel->GetEventHandler()->ProcessEvent( navigationEvent );
	}
}

void CEdStorySceneChoiceLinePanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SceneChoiceLineLinkChanged ) )
	{
		CStorySceneChoiceLine* line = GetEventData< CStorySceneChoiceLine* >( data );
		if ( line == GetChoiceLine() )
		{
			RefreshConnectedSections();
		}
	}
}

void CEdStorySceneChoiceLinePanel::OnMoveChoiceLineUp( wxCommandEvent& event )
{
	m_choicePanel->MoveChoiceLine( this, false );
}

void CEdStorySceneChoiceLinePanel::OnMoveChoiceLineDown( wxCommandEvent& event )
{
	m_choicePanel->MoveChoiceLine( this, true );
}

Bool CEdStorySceneChoiceLinePanel::RefreshDialogObject( CObject* objectToRefresh )
{
	if ( m_choiceLine == objectToRefresh )
	{
		RefreshData();
		return true;
	}
	return false;
}

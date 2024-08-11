#include "build.h"
#include "dialogEditorLine.h"

#include "dialogEditorActions.h"
#include "dialogEditorSpeakerInput.h"
#include "undoDialogEditor.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneLine.h"

#include "voiceEditor.h"

#include "textControl.h"
#include "../../common/engine/localizationManager.h"

#include "../../common/core/feedback.h"

// =================================================================================================
namespace {
// =================================================================================================

void AppendBrackets( wxTextCtrl* textField, wxString leftBracket, wxString rightBracket )
{
	long caretPosition = textField->GetInsertionPoint();
	wxString text = textField->GetValue();

	Bool isBracketAppended = false;

	if ( leftBracket.IsEmpty() == false && text.StartsWith( leftBracket ) == false )
	{
		text = leftBracket + text;
		caretPosition += 1;
		isBracketAppended = true;
	}

	if ( rightBracket.IsEmpty() == false && text.EndsWith( rightBracket ) == false )
	{
		text = text + rightBracket;
		isBracketAppended = true;
	}

	if ( isBracketAppended == true )
	{
		textField->ChangeValue( text );
		textField->SetInsertionPoint( caretPosition );
	}
}

wxString GetValueWithoutBrackets( wxTextCtrl* textField, wxString leftBracket, wxString rightBracket )
{
	wxString text = textField->GetValue();
	if ( text.StartsWith( leftBracket ) == true && text.EndsWith( rightBracket ) == true )
	{
		text = text.Mid
			(
			leftBracket.Length(),
			text.Length() - leftBracket.Length() - rightBracket.Length()
			);
	}
	return text;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

wxIMPLEMENT_CLASS( CEdStorySceneLineContentPanel, wxPanel );
wxIMPLEMENT_CLASS( CEdStorySceneLinePanel, CEdStorySceneElementPanel );

BEGIN_EVENT_TABLE( CEdStorySceneLinePanel, CEdStorySceneElementPanel )
	EVT_CHAR( CEdStorySceneLinePanel::OnCharPressed )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDLINECOMMENT, CEdStorySceneLinePanel::OnAddEditLineComment )
	EVT_MENU( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, CEdStorySceneLinePanel::OnMakeCopyUnique )
	EVT_MENU( wxID_STORYSCENEEDITOR_REC_VOICE_FOR_LINE, CEdStorySceneLinePanel::OnRecVoiceForLine )
END_EVENT_TABLE()

CEdStorySceneLinePanel::CEdStorySceneLinePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CAbstractStorySceneLine* dialogLine )
	: CEdStorySceneElementPanel( parent, sectionPanel, undoManager, SSET_Line )
	, m_speakerField( NULL )
	, m_commentField( NULL )
	, m_dialogLine( NULL )
{
	m_updatingGui = true;

	Create( parent );

	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* sizer;
	sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sizerHoriz = new wxBoxSizer( wxHORIZONTAL );

	CEdSceneEditor* sceneEditor = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor();

	m_speakerField = new CEdStorySceneLineSpeakerInput( sceneEditor->HACK_GetStoryScene(), this, wxTE_CENTRE|wxTE_NOHIDESEL|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxNO_BORDER|wxTE_RICH );
	m_speakerField->SetFont( wxFont( 12, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	sizerHoriz->AddStretchSpacer(2);
	sizerHoriz->Add(m_speakerField,2,wxEXPAND,0);

	m_speakingToField = new CEdStorySceneLineSpeakerInput( sceneEditor->HACK_GetStoryScene(), this, wxTE_NOHIDESEL|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxNO_BORDER|wxTE_RICH );
	m_speakingToField->SetFont( wxFont( 12, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	sizerHoriz->AddStretchSpacer(1);
	sizerHoriz->Add(m_speakingToField,1,wxEXPAND,0);

	sizer->Add( sizerHoriz, 4, wxALIGN_CENTER|wxEXPAND, 0 );

	m_commentField = new CEdTextControl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE|wxNO_BORDER|wxTE_PROCESS_ENTER|wxTE_RICH );
	m_commentField->SetFont( wxFont( 12, wxFONTFAMILY_ROMAN, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	sizer->Add( m_commentField, 4, wxALIGN_CENTER|wxEXPAND, 0 );

	this->SetSizer( sizer );
	this->Layout();
	sizer->Fit( this );

//////////////////////////////////////////////////////////////////////////

	CEdStorySceneHandlerFactory* handlerFactory = m_sectionPanel->GetStorySceneEditor()->GetHandlerFactory();

	AddContentLine();

	m_speakerField->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneLinePanel::OnSpeakerLostFocus, this );
	m_speakerField->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdStorySceneLinePanel::OnSpeakerChange, this );
	m_speakerField->Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdStorySceneLinePanel::OnSpeakerEnter, this );
	m_speakerField->Bind( wxEVT_CHAR, &CEdStorySceneLinePanel::OnSpeakerChar, this );
 	m_speakerField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneLinePanel::OnSpeakerPaste, this );
	m_speakerField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneLinePanel::OnSpeakerCut, this );

	m_commentField->Bind( wxEVT_SET_FOCUS, &CEdStorySceneLinePanel::OnCommentFocus, this );
	m_commentField->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneLinePanel::OnCommentLostFocus, this );
	m_commentField->Bind( wxEVT_CHAR, &CEdStorySceneLinePanel::OnLineCommentChar, this );
	m_commentField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneLinePanel::OnLineCommentPaste, this );
	m_commentField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneLinePanel::OnLineCommentCut, this );

	m_speakingToField->Bind( wxEVT_COMMAND_LISTBOX_SELECTED, &CEdStorySceneLinePanel::OnSpeakingToChange, this );
	m_speakingToField->Bind( wxEVT_CHAR, &CEdStorySceneLinePanel::OnSpeakingToChar, this );
	m_speakingToField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneLinePanel::OnSpeakingToPaste, this );
	m_speakingToField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneLinePanel::OnSpeakingToCut, this );

	m_speakingToField->SetHint( wxT( "Speaking to" ) );
	m_speakerField->SetHint( wxT( "SPEAKER" ) );
	m_commentField->SetHint( wxT( "(Comment)" ) );
	
	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_speakerField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_speakerField );
	EnableShortcuts( m_speakerField, false );

	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_speakingToField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_speakingToField );
	EnableShortcuts( m_speakingToField, false );

	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_commentField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_commentField );
	EnableShortcuts( m_commentField, false );

	ChangeFontSize( m_sectionPanel->GetStorySceneEditor()->GetFontSize() );

	m_updatingGui = false;
}

CEdStorySceneLinePanel::~CEdStorySceneLinePanel()
{
}

EStorySceneElementType CEdStorySceneLinePanel::NextElementType()
{
	return SSET_Comment;
}

void CEdStorySceneLinePanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CAbstractStorySceneLine >() )
	{
		m_dialogLine = Cast< CAbstractStorySceneLine >( storySceneElement );
		
		CEdSceneEditorScreenplayPanel* sceneEditor = m_sectionPanel->GetStorySceneEditor();
		CEdStorySceneHandlerFactory* handlerFactory = sceneEditor->GetHandlerFactory();

		handlerFactory->CreateTranslationHelperHandler( m_dialogLine->GetLocalizedComment(), this )->ConnectTo( m_commentField );

		RED_FATAL_ASSERT( storySceneElement->IsA< CStorySceneLine >(), "Encountered CAbstractStorySceneLine that's not CStorySceneLine" ); // it cannot be anything else
		handlerFactory->CreateTranslationHelperHandler( m_dialogLine->GetLocalizedContent(), this )->ConnectTo( m_contentField->GetField() );

		RefreshData();
	}
}

CStorySceneElement* CEdStorySceneLinePanel::GetDialogElement()
{
	return m_dialogLine;
}

void CEdStorySceneLinePanel::RefreshData()
{
	CName& voiceTag = m_dialogLine->GetVoiceTag();
	if ( voiceTag )
	{
		m_speakerField->ChangeValue( m_dialogLine->GetVoiceTag().AsString().AsChar() );
	}
	else
	{
		m_speakerField->ChangeValue( wxEmptyString );
	}

	CName speakingTo = m_dialogLine->GetSpeakingTo() ;
	if ( speakingTo )
	{
		m_speakingToField->ChangeValue( m_dialogLine->GetSpeakingTo().AsString().AsChar() );
	}
	else
	{
		m_speakingToField->ChangeValue( wxEmptyString );
	}

	m_contentField->GetField()->ChangeValue( m_dialogLine->GetContent().AsChar() );
	m_commentField->ChangeValue( m_dialogLine->GetComment().AsChar() );
	
	if ( m_commentField->IsEmpty() == false )
	{
		AppendBrackets( m_commentField, wxT( "(" ), wxT( ")" ) );
	}
	
	if ( m_dialogLine->GetComment().Empty() == true && m_commentField->IsShown() == true )
	{
		m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();
		m_commentField->Hide();
		m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
	}
	else if ( m_dialogLine->GetComment().Empty() == false && m_commentField->IsShown() == false )
	{
		m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();
		m_commentField->Show();
		m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
	}

	// RYTHON DEBUG BEGIN ==========================================
	UpdateColors();
	// RYTHON DEBUG END   ==========================================

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();
	m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->RefreshPropertiesPage();
}

void CEdStorySceneLinePanel::RefreshHelperData()
{
	/*wxArrayString actorNames;
	GetAutoCompleteSpeakerNames(actorNames);
	m_speakerField->AutoComplete( actorNames );*/
}

Bool CEdStorySceneLinePanel::IsEmpty()
{
	return IsTextFieldEmpty( m_speakerField ) && IsTextFieldEmpty( m_commentField ) && IsTextFieldEmpty( m_contentField->GetField() );
}

Bool CEdStorySceneLinePanel::IsTextFieldEmpty( wxTextCtrl* textField )
{
	return textField->IsEmpty() || textField->IsShown() == false;
}

void CEdStorySceneLinePanel::SetFocus()
{
	m_speakerField->SetFocus();
}

void CEdStorySceneLinePanel::ChangeFontSize( Int32 sizeChange )
{
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_speakerField, sizeChange );
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_speakingToField, sizeChange );
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_commentField, sizeChange );
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_contentField->GetField(), sizeChange );
}

void CEdStorySceneLinePanel::OnCommentFocus( wxFocusEvent& event )
{
	if ( m_commentField->GetValue().IsEmpty() == false )
	{
		m_commentField->SetInsertionPoint( m_commentField->GetLastPosition() - 1 );
	}
	event.Skip();
}

void CEdStorySceneLinePanel::OnCommentLostFocus( wxFocusEvent& event )
{
	AppendBrackets( m_commentField, wxT( "(" ), wxT( ")" ) );
	m_dialogLine->SetComment( GetValueWithoutBrackets( m_commentField, wxT( "(" ), wxT( ")" ) ).wc_str() );
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();
	m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->RefreshPropertiesPage();

	event.Skip();
}

void CEdStorySceneLinePanel::OnSpeakerEnter( wxCommandEvent& event )
{
	m_contentField->SetFocus();
}

void CEdStorySceneLinePanel::OnLineContentCharEnter( wxKeyEvent& event )
{
	int keyCode = event.GetKeyCode();

	if ( keyCode == '(' )
	{
		wxCommandEvent addEditLineCommentEvent( wxEVT_COMMAND_MENU_SELECTED );
		addEditLineCommentEvent.SetId( wxID_STORYSCENEEDITOR_ADDLINECOMMENT );
		AddPendingEvent( addEditLineCommentEvent );
	} 	
	else
	{		
		CUndoDialogTextChange::PrepareLineContentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine, keyCode );
		event.Skip();
	}
}

void CEdStorySceneLinePanel::OnLineContentPaste( wxClipboardTextEvent& event )
{
	CUndoDialogTextChange::PrepareLineContentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine );
	event.Skip();
}

void CEdStorySceneLinePanel::OnLineContentLostFocus( wxFocusEvent& event )
{
	m_dialogLine->SetContent( m_contentField->GetField()->GetValue().wc_str() );
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();

	event.Skip();
}

void CEdStorySceneLinePanel::OnAddEditLineComment( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

		m_commentField->Show();
		m_commentField->SetFocus();

		wxString commentText = m_dialogLine->GetComment().AsChar();
		if ( commentText.IsEmpty() == true )
		{
			m_commentField->SetValue( wxT( "(" ) );
			m_commentField->SetInsertionPoint( 1 );
		}
		else
		{
			m_commentField->SetValue( commentText );
			m_commentField->SetInsertionPoint( commentText.Length() + 1 );
		}
		m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnSpeakerChar( wxKeyEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CUndoDialogTextChange::PrepareLineVoiceTagStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine, event.GetKeyCode() );
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnSpeakerPaste( wxClipboardTextEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CUndoDialogTextChange::PrepareLineVoiceTagStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine );
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnSpeakerCut( wxClipboardTextEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnSpeakerLostFocus( wxFocusEvent& event )
{
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );
	event.Skip();
}

void CEdStorySceneLinePanel::OnSpeakerChange( wxCommandEvent& event )
{
	if ( m_dialogLine != NULL )
	{
		wxString txt = GetValueWithoutBrackets( m_speakerField, wxT( "$" ), wxT( "" ) ).Upper();
		m_dialogLine->SetVoiceTag( CName( txt.wc_str() ) );

		if ( m_dialogLine->GetSpeakingTo() == CName::NONE || m_dialogLine->GetSpeakingTo() == m_dialogLine->GetVoiceTag() )
		{
			const CName speakingTo = m_sectionPanel->GenerateSpeakingToForLine( m_dialogLine,  m_sectionPanel->FindDialogElementIndex( this ) );
			m_dialogLine->SetSpeakingTo( speakingTo );
			m_speakingToField->ChangeValue( speakingTo.AsString().AsChar() );
		}

		m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->RefreshPropertiesPage();

		event.Skip();
	}
}

void CEdStorySceneLinePanel::OnSpeakingToChange( wxCommandEvent& event )
{
	if ( m_dialogLine != NULL )
	{
		wxString txt = GetValueWithoutBrackets( m_speakingToField, wxT( "$" ), wxT( "" ) ).Upper();
		m_dialogLine->SetSpeakingTo( CName( txt.wc_str() ) );

		m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->RefreshPropertiesPage();

		event.Skip();
	}
}

void CEdStorySceneLinePanel::OnLineCommentChar( wxKeyEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CUndoDialogTextChange::PrepareLineCommentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine, event.GetKeyCode() );
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnLineCommentPaste( wxClipboardTextEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CUndoDialogTextChange::PrepareLineCommentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_dialogLine );
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnLineCommentCut( wxClipboardTextEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}


void CEdStorySceneLinePanel::OnSpeakingToChar( wxKeyEvent& event )
{
	event.Skip();
}

void CEdStorySceneLinePanel::OnSpeakingToPaste( wxCommandEvent& event )
{
	event.Skip();
}

void CEdStorySceneLinePanel::OnSpeakingToCut( wxCommandEvent& event )
{
	event.Skip();
}

void CEdStorySceneLinePanel::FillContextMenu( wxMenu& contextMenu )
{
	contextMenu.Append( wxID_STORYSCENEEDITOR_ADDLINECOMMENT, wxT( "Add director's comment" ) );
	contextMenu.AppendSeparator();
	
	CEdStorySceneElementPanel::FillContextMenu( contextMenu );
	
	Bool isContextOnContentField = false;
	if ( m_lastContextMenuObject == m_contentField->GetField() )
	{
		isContextOnContentField = true;
	}

	if ( ( isContextOnContentField == true
		|| m_lastContextMenuObject == m_commentField
		|| m_lastContextMenuObject == m_speakerField )
		&& m_sectionPanel->IsInElementSelectionMode() == false )
	{
		contextMenu.Enable( wxID_STORYSCENEEDITOR_COPYLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_CUTLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_PASTELINE, true );
	}

	if ( m_dialogLine && m_dialogLine->IsSceneElementCopy() )
	{
		contextMenu.AppendSeparator();
		contextMenu.Append( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, wxT( "Make copy unique" ) );
	}

	if ( m_dialogLine && m_dialogLine->IsA< CStorySceneLine >() )
	{
		contextMenu.AppendSeparator();
		contextMenu.Append( wxID_STORYSCENEEDITOR_REC_VOICE_FOR_LINE, wxT( "Record voice" ) );
	}
}

void CEdStorySceneLinePanel::OnSelected()
{
	m_speakerField->HideNativeCaret();
	m_commentField->HideNativeCaret();
	m_contentField->GetField()->HideNativeCaret();

	UpdateColors();
}

void CEdStorySceneLinePanel::OnDeselected()
{
	m_speakerField->ShowNativeCaret();
	m_commentField->ShowNativeCaret();
	m_contentField->GetField()->ShowNativeCaret();

	// RYTHON DEBUG BEGIN ==========================================
	UpdateColors();
	// RYTHON DEBUG END   ==========================================
}

bool CEdStorySceneLinePanel::SetBackgroundColour( const wxColour& colour )
{
	bool result = CEdStorySceneElementPanel::SetBackgroundColour( colour );
	if( m_contentField )
	{
		m_contentField->GetField()->SetBackgroundColour( colour );
	}

	return result;
}

void CEdStorySceneLinePanel::UpdateColors()
{
	if ( m_dialogLine )
	{
		wxColour bgColor;
		if ( m_dialogLine->IsSceneElementCopy() )
		{
			if( IsSelected() )
			{
				bgColor = wxColour( 255, 255, 128 );
			}
			else
			{
				bgColor = wxColour( 255, 255, 0 );
			}
		}
		else
		{
			if( IsSelected() )
			{
				bgColor = wxColour( 229, 229, 255 );
			}
			else
			{
				bgColor = wxColour( 255, 255, 255 );
			}
		}

		m_commentField->SetBackgroundColour( bgColor );
		m_contentField->GetField()->SetBackgroundColour( bgColor );
	}
}

void CEdStorySceneLinePanel::OnMakeCopyUnique( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_dialogLine )
		{
			m_dialogLine->MakeCopyUnique();
			UpdateColors();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::OnRecVoiceForLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_dialogLine && m_dialogLine->IsA< CStorySceneLine >() )
		{
			CStorySceneLine* line = static_cast< CStorySceneLine* >( m_dialogLine );

			String text = line->GetLocalizedContent()->GetString();

			CEdVoiceDialog dlg( this, line->GetVoiceFileName(), SLocalizationManager::GetInstance().GetCurrentLocale().ToLower(), text );
			if ( dlg.ShowModal() != 1 )
			{
				wxMessageBox( wxT("Couldn't generate lipsync file"), wxT("Error") );
			}
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLinePanel::AddContentLine()
{
	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

	CEdSceneEditorScreenplayPanel* sceneEditor = m_sectionPanel->GetStorySceneEditor();
	CEdStorySceneHandlerFactory* handlerFactory = sceneEditor->GetHandlerFactory();

	m_contentField = new CEdStorySceneLineContentPanel( this );
	this->GetSizer()->Add( m_contentField, 0, wxEXPAND, 5 );

	m_contentField->GetField()->Bind( wxEVT_CHAR, &CEdStorySceneLinePanel::OnLineContentCharEnter, this );
	m_contentField->GetField()->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneLinePanel::OnLineContentLostFocus, this );
	m_contentField->GetField()->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneLinePanel::OnLineContentPaste, this );

	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_contentField->GetField() );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_contentField->GetField() );
	handlerFactory->CreateAutoExpandHandler()->ConnectTo( m_contentField->GetField() );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_contentField->GetField() );

	m_contentField->GetField()->SetHint( wxT( "(dialog text)" ) );

	EnableShortcuts( m_contentField->GetField() );

	m_contentField->GetField()->SetFocus();

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );

	m_contentField->Bind( wxEVT_LEFT_DOWN, &CEdStorySceneLinePanel::OnPanelSelected, this );
}

void CEdStorySceneLinePanel::GetWordCount( Uint32& contentWords, Uint32& commentWords ) const
{
	if ( m_dialogLine == NULL )
	{
		return;
	}

	commentWords = GetStringWordCount( GetValueWithoutBrackets( m_commentField, wxT( "(" ), wxT( ")" ) ).wc_str() );
	contentWords = GetStringWordCount( m_contentField->GetField()->GetValue().wc_str() );
}

void CEdStorySceneLinePanel::GetAutoCompleteSpeakerNames( wxArrayString &actorNames )
{
	CEdSceneEditor* sceneEditor = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor();
	
	TDynArray< CName >	sceneActorNames;
	sceneEditor->HACK_GetStoryScene()->CollectVoicetags( sceneActorNames, true );

	for ( Uint32 i = 0; i < sceneActorNames.Size(); ++i )
	{
		if ( sceneActorNames[ i ] == CName::NONE )
		{
			continue;
		}
		actorNames.Add( sceneActorNames[ i ].AsString().AsChar() );
	}
	actorNames.Sort();
}

void CEdStorySceneLinePanel::ImplCommitChanges()
{
	// line text
	m_dialogLine->SetContent( m_contentField->GetField()->GetValue().wc_str() );

	// line comment
	m_dialogLine->SetComment( GetValueWithoutBrackets( m_commentField, wxT( "(" ), wxT( ")" ) ).wc_str() );

	// line speaker
	wxString lineSpeaker = GetValueWithoutBrackets( m_speakerField, wxT( "$" ), wxT( "" ) ).Upper();
	m_dialogLine->SetVoiceTag( CName( lineSpeaker.wc_str() ) );

	// line speaking to
	wxString lineSpeakingTo = GetValueWithoutBrackets( m_speakingToField, wxT( "$" ), wxT( "" ) ).Upper();
	m_dialogLine->SetSpeakingTo( CName( lineSpeakingTo.wc_str() ) );
}

//////////////////////////////////////////////////////////////////////////

CEdStorySceneLineContentPanel::CEdStorySceneLineContentPanel( wxWindow* parent )
	: wxPanel( parent )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

	m_contentField = new CEdTextControl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxTE_RICH|wxNO_BORDER|wxTE_NO_VSCROLL );
	m_contentField->SetFont( wxFont( 12, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	m_contentField->Bind( wxEVT_CHAR, &CEdStorySceneLineContentPanel::OnChar, this );
	m_contentField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneLineContentPanel::OnClipboard, this );
	m_contentField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneLineContentPanel::OnClipboard, this );

	sizer->Add( 0, 0, 1, wxEXPAND, 5 );
	sizer->Add( m_contentField, 4, wxEXPAND, 0 );
	sizer->Add( 0, 0, 1, wxEXPAND, 5 );

	this->SetSizer( sizer );
	this->Layout();
}

bool CEdStorySceneLineContentPanel::SetBackgroundColour( const wxColour& colour )
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

void CEdStorySceneLineContentPanel::OnChar( wxKeyEvent& event )
{
	CEdStorySceneLinePanel* parent = static_cast< CEdStorySceneLinePanel* >( GetParent() );
	Bool privilegeEditDialogs = parent->GetSectionPanel()->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;

	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneLineContentPanel::OnClipboard( wxClipboardTextEvent& event )
{
	CEdStorySceneLinePanel* parent = static_cast< CEdStorySceneLinePanel* >( GetParent() );
	Bool privilegeEditDialogs = parent->GetSectionPanel()->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;

	if( privilegeEditDialogs )
	{
		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

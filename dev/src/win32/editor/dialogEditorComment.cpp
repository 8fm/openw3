#include "build.h"
#include "dialogEditorComment.h"

#include "dialogEditorActions.h"
#include "textControl.h"

#include "undoDialogEditor.h"

#include "../../common/game/storySceneComment.h"
#include "../../common/core/feedback.h"

wxIMPLEMENT_CLASS( CEdStorySceneCommentPanel, CEdStorySceneElementPanel );

BEGIN_EVENT_TABLE( CEdStorySceneCommentPanel, CEdStorySceneElementPanel )
	EVT_MENU( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, CEdStorySceneCommentPanel::OnMakeCopyUnique )
END_EVENT_TABLE()

CEdStorySceneCommentPanel::CEdStorySceneCommentPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CStorySceneComment* dialogComment, Bool readOnly )	
	: CEdStorySceneElementPanel( parent, sectionPanel, undoManager, SSET_Comment )
	, m_comment( NULL )
{
	Create( parent );

	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

	long style = wxNO_BORDER|wxTE_PROCESS_ENTER|wxTE_MULTILINE|wxTE_RICH;

	if ( readOnly )
	{
		style |= wxTE_READONLY;
	}

	m_commentField = new CEdTextControl
	(
		this,
		wxID_ANY,
		wxEmptyString, 
		wxDefaultPosition,
		wxDefaultSize, 
		style
	);

	m_commentField->EnableEnterKeyWorkaround();
	m_commentField->SetFont( wxFont( 12, 70, 93, 90, false, wxT("Courier New") ) );
	m_commentField->SetMinSize( wxSize( -1, 2 * GetCharHeight() ) );

	sizer->Add( m_commentField, 1, wxEXPAND, 5 );

	this->SetSizer( sizer );
	this->Layout();

	m_commentField->SetInsertionPointEnd();

	ChangeFontSize( m_sectionPanel->GetStorySceneEditor()->GetFontSize() );

	Bind( wxEVT_LEFT_DOWN, &CEdStorySceneCommentPanel::OnPanelSelected, this );
}

CEdStorySceneCommentPanel::~CEdStorySceneCommentPanel(void)
{
}

void CEdStorySceneCommentPanel::InitializeElementHandling()
{
	CEdStorySceneHandlerFactory* handlerFactory = m_sectionPanel->GetStorySceneEditor()->GetHandlerFactory();

	EnableShortcuts( m_commentField );

	ConnectHandlers(handlerFactory);
}
void CEdStorySceneCommentPanel::ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory )
{
	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_commentField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_commentField );
	handlerFactory->CreateAutoExpandHandler()->ConnectTo( m_commentField );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_commentField );

	m_commentField->SetHint( wxT( "(Comment)" ) );

 	m_commentField->Bind( wxEVT_CHAR, &CEdStorySceneCommentPanel::OnCommentChar, this );
	m_commentField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdStorySceneCommentPanel::OnCommentPaste, this );
	m_commentField->Bind( wxEVT_COMMAND_TEXT_CUT, &CEdStorySceneCommentPanel::OnCommentCut, this );
 	m_commentField->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneCommentPanel::OnCommentFocusLost, this );
}

Bool CEdStorySceneCommentPanel::IsEmpty()
{
	return m_commentField->IsEmpty();
}

void CEdStorySceneCommentPanel::SetFocus()
{
	m_commentField->SetFocus();
}

EStorySceneElementType CEdStorySceneCommentPanel::NextElementType()
{
	Int32 index = GetParent()->GetChildren().IndexOf( this );
	Int32 numberOfSizerItems = GetParent()->GetChildren().GetCount();

	if ( m_sectionPanel->CanAddChoice() == true && index == numberOfSizerItems - 1 )
	{
		return SSET_Choice;
	}
	
	return SSET_Line;
}

void CEdStorySceneCommentPanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneComment >() )
	{
		m_comment = Cast< CStorySceneComment >( storySceneElement );
		m_sectionPanel->GetStorySceneEditor()->GetHandlerFactory()
			->CreateTranslationHelperHandler( m_comment->GetLocalizedComment(), this )->ConnectTo( m_commentField );
		RefreshData();
		UpdateColors();
	}
}

CStorySceneElement* CEdStorySceneCommentPanel::GetDialogElement()
{
	return m_comment;
}

void CEdStorySceneCommentPanel::RefreshData()
{
	m_commentField->ChangeValue( m_comment->GetCommentText().AsChar() );
}

void CEdStorySceneCommentPanel::FillContextMenu( wxMenu& contextMenu )
{
	CEdStorySceneElementPanel::FillContextMenu( contextMenu );

	if ( m_lastContextMenuObject == m_commentField )
	{
		contextMenu.Enable( wxID_STORYSCENEEDITOR_COPYLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_CUTLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_PASTELINE, true );
	}

	if ( m_comment && m_comment->IsSceneElementCopy() )
	{
		contextMenu.AppendSeparator();
		contextMenu.Append( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, wxT( "Make copy unique" ) );
	}
}

void CEdStorySceneCommentPanel::ChangeFontSize( Int32 sizeChange )
{
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_commentField, sizeChange );
}

void CEdStorySceneCommentPanel::OnCommentChar( wxKeyEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_comment )
		{
			CUndoDialogTextChange::PrepareCommentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_comment, event.GetKeyCode() );
		}

		event.Skip();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneCommentPanel::OnCommentPaste( wxCommandEvent& event )
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

void CEdStorySceneCommentPanel::OnCommentCut( wxCommandEvent& event )
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

void CEdStorySceneCommentPanel::OnCommentFocusLost( wxFocusEvent& event )
{
	m_comment->SetCommentText( m_commentField->GetValue().wc_str() );
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );

	m_sectionPanel->GetStorySceneEditor()->RefreshWordCount();

	event.Skip();
}

void CEdStorySceneCommentPanel::OnMakeCopyUnique( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_comment && m_comment->IsSceneElementCopy() )
		{
			m_comment->MakeCopyUnique();
			UpdateColors();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneCommentPanel::OnSelected()
{
	m_commentField->HideNativeCaret();
	UpdateColors();
}

void CEdStorySceneCommentPanel::OnDeselected()
{
	m_commentField->ShowNativeCaret();
	UpdateColors();
}

bool CEdStorySceneCommentPanel::SetBackgroundColour( const wxColour& colour )
{
	bool result = CEdStorySceneElementPanel::SetBackgroundColour( colour );

	UpdateColors();

	return result;
}

void CEdStorySceneCommentPanel::UpdateColors()
{
	if ( m_comment )
	{
		wxColour bgColor;
		if ( m_comment->IsSceneElementCopy() )
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
	}
}

void CEdStorySceneCommentPanel::GetWordCount( Uint32& contentWords, Uint32& commentWords ) const
{
	if ( m_comment == NULL )
	{
		return;
	}

	contentWords = GetStringWordCount( m_commentField->GetValue().wc_str() );
}

void CEdStorySceneCommentPanel::ImplCommitChanges()
{
	m_comment->SetCommentText( m_commentField->GetValue().wc_str() );
}

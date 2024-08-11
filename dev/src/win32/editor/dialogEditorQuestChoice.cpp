#include "build.h"
#include "dialogEditorQuestChoice.h"
#include "dialogEditorActions.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPhaseBlock.h"
#include "../../common/game/questContextDialogBlock.h"
#include "assetBrowser.h"
#include "textControl.h"
#include "undoDialogEditor.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdStorySceneQuestChoicePanel, CEdStorySceneElementPanel )
EVT_MENU( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, CEdStorySceneQuestChoicePanel::OnMakeCopyUnique )
END_EVENT_TABLE()

CEdStorySceneQuestChoicePanel::CEdStorySceneQuestChoicePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CStorySceneComment* dialogComment )	
: CEdStorySceneElementPanel( parent, sectionPanel, undoManager, SSET_QuestChoiceLine )
, m_questChoiceLine( NULL )
{
	Create( parent );

	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
	wxBoxSizer* linesSizer = new wxBoxSizer( wxVERTICAL );

	m_choiceLineField = new CEdTextControl
	(
		this,
		wxID_ANY,
		wxT(""), 
		wxDefaultPosition,
		wxDefaultSize, 
		wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_PROCESS_ENTER
	);

	m_choiceLineField->EnableEnterKeyWorkaround();
	m_choiceLineField->SetFont( wxFont( 12, 70, 93, 90, false, wxT("Courier New") ) );
	m_choiceLineField->SetMinSize( wxSize( -1, 2 * GetCharHeight() ) );

	m_targetScenePathField = new CEdTextControl
	(
		this,
		wxID_ANY,
		wxT(""), 
		wxDefaultPosition,
		wxDefaultSize,
		wxNO_BORDER
	);

	m_targetScenePathField->EnableEnterKeyWorkaround();
	m_targetScenePathField->SetFont( wxFont( 12, 70, 93, 90, false, wxT("Courier New") ) );
	m_targetScenePathField->SetMinSize( wxSize( -1, 2 * GetCharHeight() ) );
	m_targetScenePathField->SetEditable( false );
	m_targetScenePathField->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CEdStorySceneQuestChoicePanel::OnTargetDoubleClick ), NULL, this );

	wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
	wxBitmapButton* findContextChoicesBtn = new wxBitmapButton( this, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	findContextChoicesBtn->SetToolTip( wxT("Select a quest with injected choice options.") );
	findContextChoicesBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdStorySceneQuestChoicePanel::OnFindTargetScene ), NULL, this );

	linesSizer->Add( m_targetScenePathField, 1, wxEXPAND, 5 );
	linesSizer->Add( m_choiceLineField, 1, wxEXPAND, 5 );

	sizer->Add( linesSizer, 1, wxEXPAND, 5 );
	sizer->Add( findContextChoicesBtn, 0, wxALL, 5 );

	m_targetScenePathField->Hide();

	this->SetSizer( sizer );
	this->Layout();

	m_choiceLineField->SetInsertionPointEnd();

	ChangeFontSize( m_sectionPanel->GetStorySceneEditor()->GetFontSize() );
}

CEdStorySceneQuestChoicePanel::~CEdStorySceneQuestChoicePanel(void)
{
}

void CEdStorySceneQuestChoicePanel::InitializeElementHandling()
{
	CEdStorySceneHandlerFactory* handlerFactory = m_sectionPanel->GetStorySceneEditor()->GetHandlerFactory();

	EnableShortcuts( m_choiceLineField );

	ConnectHandlers(handlerFactory);
}
void CEdStorySceneQuestChoicePanel::ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory )
{
	handlerFactory->CreateCaretOnFocusHandler()->ConnectTo( m_choiceLineField );
	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_choiceLineField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_choiceLineField );
	handlerFactory->CreateAutoExpandHandler()->ConnectTo( m_choiceLineField );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_choiceLineField );

	m_choiceLineField->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdStorySceneQuestChoicePanel::OnCommentChanged, this );
	m_choiceLineField->Bind( wxEVT_CHAR, &CEdStorySceneQuestChoicePanel::OnCommentChar, this );
	m_choiceLineField->Bind( wxEVT_KILL_FOCUS, &CEdStorySceneQuestChoicePanel::OnCommentLostFocus, this );
}

Bool CEdStorySceneQuestChoicePanel::IsEmpty()
{
	Bool hasDefaultValue = ( m_choiceLineField->GetValue() == wxT( "(Quest Choice Line)" )  );

	return m_choiceLineField->IsEmpty() || hasDefaultValue;
}

void CEdStorySceneQuestChoicePanel::SetFocus()
{
	m_choiceLineField->SetFocus();
}

EStorySceneElementType CEdStorySceneQuestChoicePanel::NextElementType()
{
	Int32 index = GetParent()->GetChildren().IndexOf( this );
	Int32 numberOfSizerItems = GetParent()->GetChildren().GetCount();

	if ( m_sectionPanel->CanAddChoice() == true && index == numberOfSizerItems - 1 )
	{
		return SSET_Choice;
	}

	return SSET_ScriptLine;
}

void CEdStorySceneQuestChoicePanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneComment >() )
	{
		m_questChoiceLine = Cast< CStorySceneComment >( storySceneElement );
		m_sectionPanel->GetStorySceneEditor()->GetHandlerFactory()
			->CreateTranslationHelperHandler( m_questChoiceLine->GetLocalizedComment(), this )->ConnectTo( m_choiceLineField );
		RefreshData();
		UpdateColors();
	}
}

CStorySceneElement* CEdStorySceneQuestChoicePanel::GetDialogElement()
{
	return m_questChoiceLine;
}
void CEdStorySceneQuestChoicePanel::RefreshData()
{
	m_choiceLineField->ChangeValue( m_questChoiceLine->GetCommentText().AsChar() );
}

void CEdStorySceneQuestChoicePanel::FillContextMenu( wxMenu& contextMenu )
{
	CEdStorySceneElementPanel::FillContextMenu( contextMenu );

	if ( m_lastContextMenuObject == m_choiceLineField )
	{
		contextMenu.Enable( wxID_STORYSCENEEDITOR_COPYLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_CUTLINE, true );
		contextMenu.Enable( wxID_STORYSCENEEDITOR_PASTELINE, true );
	}

	if ( m_questChoiceLine && m_questChoiceLine->IsSceneElementCopy() )
	{
		contextMenu.AppendSeparator();
		contextMenu.Append( wxID_STORYSCENEEDITOR_MAKE_COPY_UNIQUE, wxT( "Make copy unique" ) );
	}
}

void CEdStorySceneQuestChoicePanel::ChangeFontSize( Int32 sizeChange )
{
	m_sectionPanel->GetStorySceneEditor()->ChangeWindowFontSize( m_choiceLineField, sizeChange );
}

bool CEdStorySceneQuestChoicePanel::SetBackgroundColour( const wxColour& colour )
{
	bool result = CEdStorySceneElementPanel::SetBackgroundColour( colour );

	UpdateColors();

	return result;
}

void CEdStorySceneQuestChoicePanel::UpdateColors()
{
	if ( m_questChoiceLine == NULL ) return;

	wxColour bgColor;
	if ( m_questChoiceLine->IsSceneElementCopy() )
	{
		bgColor = wxColour( 255, 255, 0 );
	}
	else
	{
		bgColor = wxColour( 255, 182, 219 );
	}

	m_choiceLineField->SetBackgroundColour( bgColor );
	m_targetScenePathField->SetBackgroundColour( bgColor );
	m_targetScenePathField->SetForegroundColour( wxColour( 145, 104, 125 ) );

}

String CEdStorySceneQuestChoicePanel::FindInjectTargetScenePath() const
{
	String questResourcePath;
	GetActiveResource( questResourcePath );
	CResource* questResource = GDepot->LoadResource( questResourcePath );

	if ( questResource == NULL || questResource->IsA< CQuestPhase >() == false )
	{
		return String::EMPTY;
	}

	CQuestPhase* quest = Cast< CQuestPhase >( questResource );

	CQuestGraph* questGraphWithInjectedChoices = quest->GetGraph();

	if ( questGraphWithInjectedChoices == NULL )
	{
		return String::EMPTY;
	}

	CStoryScene* hostScene = m_questChoiceLine->GetSection()->GetScene();

	// find context dialog block ( the ones that can inject new choice options )
	TDynArray< CQuestContextDialogBlock* > contextBlocks;
	TSet< CQuestGraph* > exploredGraphs;
	TDynArray< CQuestGraph* > graphsStack;

	graphsStack.PushBack( questGraphWithInjectedChoices );

	// analyze the graph
	while ( graphsStack.Empty() == false )
	{
		CQuestGraph* currentGraph = graphsStack.PopBack();
		if ( exploredGraphs.Find( currentGraph ) != exploredGraphs.End() )
		{
			continue;
		}
		else
		{
			exploredGraphs.Insert( currentGraph );
		}

		TDynArray< CGraphBlock* >& blocks = currentGraph->GraphGetBlocks();
		Uint32 count = blocks.Size();

		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( blocks[ i ]->IsA< CQuestContextDialogBlock >() )
			{
				contextBlocks.PushBack( Cast< CQuestContextDialogBlock >( blocks[ i ] ) );
			}
			else if ( blocks[ i ]->IsA< CQuestScopeBlock >() )
			{
				CQuestScopeBlock* scope = Cast< CQuestScopeBlock >( blocks[ i ] );
				graphsStack.PushBack( scope->GetGraph() );
			}
		}
	}


	// gather the new choice options
	Uint32 lineIdx = 1;
	for ( TDynArray< CQuestContextDialogBlock* >::const_iterator it = contextBlocks.Begin();
		it != contextBlocks.End(); ++it )
	{
		CQuestContextDialogBlock* block = *it;
		CStoryScene* targetScene = block->GetTargetScene();
		CStoryScene* sceneWithInjectedOptions = block->GetScene();

		THashMap< String, String > injectedChoices;
		if ( sceneWithInjectedOptions == hostScene )
		{
			return targetScene->GetFile()->GetDepotPath();
			// TODO: add many target scenes if such a functionality is supported
		}
	}

	return String::EMPTY;
}

void CEdStorySceneQuestChoicePanel::OnCommentChar( wxKeyEvent& event )
{
	CUndoDialogTextChange::PrepareCommentStep( *m_undoManager, m_sectionPanel->GetStorySceneEditor(), m_questChoiceLine, event.GetKeyCode() );
	event.Skip();
}

void CEdStorySceneQuestChoicePanel::OnCommentLostFocus( wxFocusEvent& event )
{
	CUndoDialogTextChange::FinalizeStep( *m_undoManager );
	event.Skip();
}

void CEdStorySceneQuestChoicePanel::OnCommentChanged( wxCommandEvent& event )
{
	if ( m_questChoiceLine )
	{
		m_questChoiceLine->SetCommentText( m_choiceLineField->GetValue().wc_str() );
	}
	event.Skip();
}

void CEdStorySceneQuestChoicePanel::OnMakeCopyUnique( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_questChoiceLine && m_questChoiceLine->IsSceneElementCopy() )
		{
			m_questChoiceLine->MakeCopyUnique();
			UpdateColors();
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneQuestChoicePanel::OnSelected()
{
	m_choiceLineField->HideNativeCaret();
	UpdateColors();
}

void CEdStorySceneQuestChoicePanel::OnDeselected()
{
	m_choiceLineField->ShowNativeCaret();
	UpdateColors();
}

void CEdStorySceneQuestChoicePanel::OnFindTargetScene( wxCommandEvent& event )
{
	String targetScenePath = FindInjectTargetScenePath() ;
	
	if ( targetScenePath.Empty() == true )	
	{
		return;
	}

	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();
	m_targetScenePathField->ChangeValue( targetScenePath.AsChar() );
	m_targetScenePathField->Show();
	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
	UpdateColors();
}

void CEdStorySceneQuestChoicePanel::OnTargetDoubleClick( wxMouseEvent& event )
{
	String targetScenePath = m_targetScenePathField->GetValue().wc_str();
	if ( targetScenePath.Empty() == false )
	{
		wxTheFrame->GetAssetBrowser()->OpenFile( targetScenePath );
	}
	
}

void CEdStorySceneQuestChoicePanel::ImplCommitChanges()
{}

#include "build.h"
#include "dialogEditorChoice.h"
#include "dialogEditorChoiceLine.h"
#include "dialogEditorInjectedChoiceLine.h"
#include "dialogEditorActions.h"
#include "undoDialogEditor.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPhaseBlock.h"
#include "../../common/game/questContextDialogBlock.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdStorySceneChoicePanel, CEdStorySceneElementPanel )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDCHOICELINE, CEdStorySceneChoicePanel::OnAddChoiceLine )
	EVT_NAVIGATION_KEY( CEdStorySceneChoicePanel::OnNavigate )
END_EVENT_TABLE()

CEdStorySceneChoicePanel::CEdStorySceneChoicePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager )
	: CEdStorySceneElementPanel( parent, sectionPanel, undoManager, SSET_Choice )
	, m_storySceneChoice( NULL )
	, m_questGraphWithInjectedChoices( NULL )
{
	Create( parent );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* sizer;
	sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* headerSizer;
	headerSizer = new wxBoxSizer( wxHORIZONTAL );

	m_choiceElementLabel = new wxStaticText( this, wxID_ANY, wxT( "Choice" ), wxDefaultPosition, wxDefaultSize, 0 );
	m_choiceElementLabel->Wrap( -1 );
	m_choiceElementLabel->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );

	headerSizer->Add( m_choiceElementLabel, 0, wxALL, 5 );

	wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
	m_findContextChoicesBtn = new wxBitmapButton( this, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_findContextChoicesBtn->SetToolTip( wxT("Select a quest with injected choice options.") );

	headerSizer->Add( m_findContextChoicesBtn, 0, wxALL, 5 );

	sizer->Add( headerSizer, 1, wxEXPAND, 5 );

	this->SetSizer( sizer );
	this->Layout();
	sizer->Fit( this );

	////////////////////////////////////////////////////////////

	//wxXmlResource::Get()->LoadPanel( this, parent, TEXT( "DialogEditorChoice" ) );
	//
	//m_findContextChoicesBtn = XRCCTRL( *this, "findContextChoicesBtn", wxBitmapButton );
	//m_choiceElementLabel = XRCCTRL( *this, "m_choiceElementLabel", wxStaticText );

	m_findContextChoicesBtn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdStorySceneChoicePanel::OnFindContextChoices, this );

	m_injectedChoiceLinesSizer = new wxBoxSizer( wxVERTICAL );
	m_regularChoiceLinesSizer = new wxBoxSizer( wxVERTICAL );
	GetSizer()->Add( m_injectedChoiceLinesSizer, 0, wxEXPAND, 5 );
	GetSizer()->Add( m_regularChoiceLinesSizer, 0, wxEXPAND, 5 );

	ChangeFontSize( m_sectionPanel->GetStorySceneEditor()->GetFontSize() );

	m_choiceElementLabel->Bind( wxEVT_LEFT_DOWN, &CEdStorySceneChoicePanel::OnClick, this );
	Bind( wxEVT_LEFT_DOWN, &CEdStorySceneChoicePanel::OnPanelSelected, this );
}

CEdStorySceneChoicePanel::~CEdStorySceneChoicePanel()
{
	m_undoManager->NotifyObjectRemoved( m_storySceneChoice );
}

Bool CEdStorySceneChoicePanel::IsEmpty()
{
	return m_choiceLines.Empty() || ( m_choiceLines.Size() == 1 && m_choiceLines[ 0 ]->IsEmpty() );
}

void CEdStorySceneChoicePanel::SetFocus()
{
	if ( m_choiceLines.Empty() == false )
	{
		m_choiceLines[ 0 ]->SetFocus();
	}
}

CStorySceneChoiceLine* CEdStorySceneChoicePanel::AddChoiceLine( Int32 index /*= wxNOT_FOUND*/, Bool after /*= true */ )
{
	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

	Uint32 initialLabelIndex = index + 1 + ( ( after == true ) ? 1 : 0 );
	CEdStorySceneChoiceLinePanel* choiceLine = new CEdStorySceneChoiceLinePanel( this, m_undoManager, initialLabelIndex );

	index = AddChoiceLinePanel(index, after, choiceLine);

	CStorySceneChoiceLine* line = m_storySceneChoice->AddChoiceLine( index );

	choiceLine->SetChoiceLine( line );

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );

	choiceLine->SetFocus();

	UpdateChoiceLineIndices();

	return line;
}

Int32 CEdStorySceneChoicePanel::AddChoiceLinePanel( Int32 index, Bool after, CEdStorySceneChoiceLinePanel* choiceLine )
{
	if
	(
		index == wxNOT_FOUND ||
		(
			index == m_choiceLines.Size() - 1 &&
			after == true
		) ||
		index >= m_choiceLines.SizeInt()
	)
	{
		m_regularChoiceLinesSizer->Add( choiceLine, 0, wxEXPAND, 5 );
		m_choiceLines.PushBack( choiceLine );

		index = -1;
	}
	else
	{
		if ( after == true )
		{
			index += 1;
		}

		m_regularChoiceLinesSizer->Insert( index, choiceLine, 0, wxEXPAND, 5 );
		
		m_choiceLines.Insert( index, choiceLine );
	}	
	return index;
}

void CEdStorySceneChoicePanel::CreateAndAddChoiceLine( Int32 index, CStorySceneChoiceLine* choiceLine )
{
	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

	CEdStorySceneChoiceLinePanel* choiceLinePanel = new CEdStorySceneChoiceLinePanel( this, m_undoManager, index + 1 );

	AddChoiceLinePanel( index, true, choiceLinePanel );
	choiceLinePanel->SetChoiceLine( choiceLine );

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
}

void CEdStorySceneChoicePanel::RemoveChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine )
{
	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

	m_storySceneChoice->RemoveChoiceLine( m_choiceLines.GetIndex( choiceLine ) );

	m_choiceLines.Remove( choiceLine );
	delete choiceLine;

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );

	UpdateChoiceLineIndices();
}

void CEdStorySceneChoicePanel::RemoveChoiceLine( Int32 index )
{
	RemoveChoiceLine( m_choiceLines[ index ] );
}

Int32 CEdStorySceneChoicePanel::FindChoiceLineIndex( CEdStorySceneChoiceLinePanel* choiceLine )
{
	return m_choiceLines.GetIndex( choiceLine );
}

void CEdStorySceneChoicePanel::UpdateChoiceLineIndices()
{
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		m_choiceLines[ i ]->SetChoiceLineIndex( i + 1 );
	}
}

void CEdStorySceneChoicePanel::SetStorySceneElement( CStorySceneElement* storySceneElement )
{
	if ( storySceneElement->IsA< CStorySceneChoice >() == false )
	{
		return;
	}

	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();

	m_storySceneChoice = Cast< CStorySceneChoice >( storySceneElement );

	while ( m_choiceLines.Empty() == false )
	{
		CEdStorySceneChoiceLinePanel* choiceLine = m_choiceLines.PopBack();
		delete choiceLine;
	}

	RefreshData();

	for ( Uint32 i = 0; i < m_storySceneChoice->GetNumberOfChoiceLines(); ++i )
	{
		CStorySceneChoiceLine* choiceLine = m_storySceneChoice->GetChoiceLine( i );
		Int32 index = i;
		CreateAndAddChoiceLine(index, choiceLine);

	}
	UpdateChoiceLineIndices();

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
}

CStorySceneElement* CEdStorySceneChoicePanel::GetDialogElement()
{
	return m_storySceneChoice;
}
EStorySceneElementType CEdStorySceneChoicePanel::NextElementType()
{
	return SSET_ScriptLine;
}

void CEdStorySceneChoicePanel::RefreshData()
{
	// refresh injected quest choices related functionality
	if ( m_storySceneChoice && m_storySceneChoice->IsQuestChoice() )
	{
		m_findContextChoicesBtn->Show( true );
		UpdateQuestGraphSelection();
	}
	else
	{
		m_findContextChoicesBtn->Show( false );
		m_questGraphWithInjectedChoices = NULL;
	}
	RefreshInjectedDialogs();

	// refresh regular choice lines
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		m_choiceLines[ i ]->RefreshData();
	}

	GetSizer()->Layout();
}

Bool CEdStorySceneChoicePanel::RefreshDialogObject( CObject* objectToRefresh )
{
	if ( m_storySceneChoice == objectToRefresh )
	{
		RefreshData();
		return true;
	}
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		if ( m_choiceLines[ i ]->RefreshDialogObject( objectToRefresh ) == true )
		{
			return true;
		}
	}
	return false;
}

void CEdStorySceneChoicePanel::FillContextMenu( wxMenu& contextMenu )
{
	contextMenu.Append( wxID_STORYSCENEEDITOR_ADDCHOICELINE, wxT( "Add choice line" ) );
	contextMenu.AppendSeparator();
	CEdStorySceneElementPanel::FillContextMenu( contextMenu );
}

void CEdStorySceneChoicePanel::ChangeFontSize( Int32 sizeChange )
{
	for ( TChoiceLinePanelArray::iterator iter = m_choiceLines.Begin(); iter != m_choiceLines.End(); ++iter )
	{
		(*iter)->ChangeFontSize( sizeChange );
	}
}


void CEdStorySceneChoicePanel::OnFindContextChoices( wxCommandEvent& event )
{
	UpdateQuestGraphSelection();
	RefreshInjectedDialogs();
}

void CEdStorySceneChoicePanel::UpdateQuestGraphSelection()
{
	m_questGraphWithInjectedChoices = NULL;

	// look up new choices
	if ( !m_storySceneChoice )
	{
		return;
	}

	String resourcePath;
	GetActiveResource( resourcePath );
	CResource* res = GDepot->LoadResource( resourcePath );

	if ( res != NULL && res->IsA< CQuestPhase >() == true )
	{
		CQuestPhase* quest = Cast< CQuestPhase >( res );
		m_questGraphWithInjectedChoices = quest->GetGraph();
	}

}

void CEdStorySceneChoicePanel::RefreshInjectedDialogs()
{
	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();
	m_injectedChoiceLinesSizer->Clear( true );

	if ( m_questGraphWithInjectedChoices )
	{
		CStoryScene* hostScene = m_storySceneChoice->GetSection()->GetScene();
		
		// find context dialog block ( the ones that can inject new choice options )
		TDynArray< CQuestContextDialogBlock* > blocks;
		FindContextDialogBlocks( *m_questGraphWithInjectedChoices, blocks );
		
		// gather the new choice options
		Uint32 lineIdx = 1;
		for ( TDynArray< CQuestContextDialogBlock* >::const_iterator it = blocks.Begin();
			  it != blocks.End(); ++it )
		{
			CQuestContextDialogBlock* block = *it;
			CStoryScene* targetScene = block->GetTargetScene();
			CStoryScene* sceneWithInjectedOptions = block->GetScene();

			THashMap< String, SSceneInjectedChoiceLineInfo > injectedChoices;
			if ( targetScene == hostScene )
			{
				block->GetChoices( injectedChoices );
			}

			for ( THashMap< String, SSceneInjectedChoiceLineInfo >::const_iterator it = injectedChoices.Begin();
				it != injectedChoices.End(); ++it )
			{
				CEdStorySceneInjectedChoiceLinePanel* choiceLine = new CEdStorySceneInjectedChoiceLinePanel( this, lineIdx++, it->m_second.m_choiceLine.GetString(), *sceneWithInjectedOptions );
				m_injectedChoiceLinesSizer->Add( choiceLine, 0, wxEXPAND, 5 );
			}
		}
	}

	m_injectedChoiceLinesSizer->Layout();
	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
}

///////////////////////////////////////////////////////////////////////////////

void CEdStorySceneChoicePanel::FindContextDialogBlocks( CQuestGraph& questGraph, TDynArray< CQuestContextDialogBlock* >& outBlocks ) const
{
	TSet< CQuestGraph* > exploredGraphs;

	TDynArray< CQuestGraph* > graphsStack;
	graphsStack.PushBack( &questGraph );

	// analyze the graph
	while ( !graphsStack.Empty() )
	{
		CQuestGraph* currGraph = graphsStack.PopBack();
		if ( exploredGraphs.Find( currGraph ) != exploredGraphs.End() )
		{
			continue;
		}
		else
		{
			exploredGraphs.Insert( currGraph );
		}

		TDynArray< CGraphBlock* >& blocks = currGraph->GraphGetBlocks();
		Uint32 count = blocks.Size();

		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( blocks[ i ]->IsA< CQuestContextDialogBlock >() )
			{
				outBlocks.PushBack( Cast< CQuestContextDialogBlock >( blocks[ i ] ) );
			}
			else if ( blocks[ i ]->IsA< CQuestScopeBlock >() )
			{
				CQuestScopeBlock* scope = Cast< CQuestScopeBlock >( blocks[ i ] );
				graphsStack.PushBack( scope->GetGraph() );
			}
		}
	}
}

void CEdStorySceneChoicePanel::OnChoiceLineChanged( wxCommandEvent& event )
{
	wxTextCtrl* eventSource = static_cast<wxTextCtrl*>( event.GetEventObject() );
	Int32 choiceLineIndex = -1;
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		if ( wxWindow::FindWindowById( eventSource->GetId(), m_choiceLines[ i ] ) != NULL )
		{
			choiceLineIndex = i;
			break;
		}
	}

	if ( choiceLineIndex != -1 && m_storySceneChoice != NULL )
	{
		CStorySceneChoiceLine* choiceLine = m_storySceneChoice->GetChoiceLine( choiceLineIndex );
		choiceLine->SetChoiceLine( eventSource->GetValue().wc_str() );
	}
}

void CEdStorySceneChoicePanel::OnAddChoiceLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = m_sectionPanel->GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CStorySceneChoiceLine* newElement = AddChoiceLine();
		CUndoDialogChoiceLineExistance::PrepareCreationStep( *m_undoManager, this, newElement, m_choiceLines.Size()-1 );
		CUndoDialogChoiceLineExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdStorySceneChoicePanel::OnNavigate( wxNavigationKeyEvent& event )
{
	
	Int32 choiceLineIndex = m_choiceLines.GetIndex( 
		static_cast<CEdStorySceneChoiceLinePanel*>( event.GetCurrentFocus() ) );

	if ( choiceLineIndex == -1 
		|| choiceLineIndex == 0 && event.GetDirection() == false 
		|| choiceLineIndex == m_choiceLines.Size() -1 && event.GetDirection() == true )
	{
		event.Skip();
	}
	else if ( event.GetDirection() == true )
	{
		m_choiceLines[ choiceLineIndex + 1 ]->SetFocus();
	}
	else if ( event.GetDirection() == false )
	{
		m_choiceLines[ choiceLineIndex - 1 ]->SetFocus();
	}
	
}

void CEdStorySceneChoicePanel::OnSelected()
{
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		m_choiceLines[ i ]->OnSelected();
	}
}

void CEdStorySceneChoicePanel::OnDeselected()
{
	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		m_choiceLines[ i ]->OnDeselected();
	}
}

wxWindow* CEdStorySceneChoicePanel::GetFirstNavigableWindow()
{
	if ( m_choiceLines.Empty() == true )
	{
		return NULL;
	}

	return m_choiceLines[ 0 ]->GetFirstNavigableWindow();
}

wxWindow* CEdStorySceneChoicePanel::GetLastNavigableWindow()
{
	if ( m_choiceLines.Empty() == true )
	{
		return NULL;
	}

	return m_choiceLines[ m_choiceLines.Size() - 1 ]->GetLastNavigableWindow();
}

void CEdStorySceneChoicePanel::OnClick( wxMouseEvent& event )
{
	if( m_sectionPanel->IsInElementSelectionMode() )
	{
		m_sectionPanel->LeaveElementSelectionMode();
	}

	m_sectionPanel->GetStorySceneEditor()->OnChoicePanelClick( m_storySceneChoice );

	m_choiceElementLabel->SetFocus();
}

void CEdStorySceneChoicePanel::NavigateToNextLine( CEdStorySceneChoiceLinePanel* currentLine )
{
	Int32 choiceLineIndex = m_choiceLines.GetIndex( currentLine );

	if ( choiceLineIndex == -1 || choiceLineIndex == m_choiceLines.Size() -1 )
	{
		Navigate( true );
		return;
	}
	
	m_choiceLines[ choiceLineIndex + 1 ]->GetFirstNavigableWindow()->SetFocus();
}

void CEdStorySceneChoicePanel::NavigateToPrevLine( CEdStorySceneChoiceLinePanel* currentLine )
{
	Int32 choiceLineIndex = m_choiceLines.GetIndex( currentLine );

	if ( choiceLineIndex == -1 || choiceLineIndex == 0 )
	{
		return;
	}

	m_choiceLines[ choiceLineIndex - 1 ]->GetLastNavigableWindow()->SetFocus();
}

void CEdStorySceneChoicePanel::GetWordCount( Uint32& contentWords, Uint32& commentWords ) const
{
	contentWords = 0;
	commentWords = 0;

	for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
	{
		commentWords += GetStringWordCount( m_choiceLines[ i ]->GetChoiceComment().wc_str() );
		contentWords += GetStringWordCount( m_choiceLines[ i ]->GetChoiceContent().wc_str() );
	}
}

Bool CEdStorySceneChoicePanel::MoveChoiceLine( CEdStorySceneChoiceLinePanel* choiceLinePanel, Bool moveDown )
{
	Int32 choiceLineIndex = FindChoiceLineIndex( choiceLinePanel );
	if ( choiceLineIndex == -1 )
	{
		return false;
	}

	Int32 newChoiceLineIndex = m_storySceneChoice->MoveChoiceLine( choiceLineIndex, moveDown );

	if ( newChoiceLineIndex == choiceLineIndex )
	{
		return false;
	}

	m_sectionPanel->GetStorySceneEditor()->PreChangeSceneElements();
	m_regularChoiceLinesSizer->Detach( choiceLinePanel );
	m_regularChoiceLinesSizer->Insert( newChoiceLineIndex, choiceLinePanel, 0, wxEXPAND | wxALL, 5 );

	m_choiceLines.Swap( choiceLineIndex, newChoiceLineIndex );

	UpdateChoiceLineIndices();
	Layout();

	m_sectionPanel->GetStorySceneEditor()->PostChangeSceneElements( this );
	

	return true;
}

void CEdStorySceneChoicePanel::ImplCommitChanges()
{}

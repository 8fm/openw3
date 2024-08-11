#include "build.h"
#include "dialogEditorSection.h"
#include "dialogSectionTitle.h"
#include "dialogEditorLine.h"
#include "dialogEditorChoice.h"
#include "dialogEditorComment.h"
#include "dialogEditorQuestChoice.h"
#include "dialogEditorScriptLine.h"
#include "dialogEditorCutscene.h"
#include "tagViewEditor.h"
#include "undoDialogEditor.h"
#include "dialogEditorActions.h"

#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneOutput.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneCutscene.h"
#include "dialogEditorVideo.h"
#include "../../common/game/storySceneVideo.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdSceneSectionPanel, wxPanel )
	EVT_SET_FOCUS( CEdSceneSectionPanel::OnSectionFocus )
	EVT_CHILD_FOCUS( CEdSceneSectionPanel::OnSectionChildFocus )
	EVT_NAVIGATION_KEY( CEdSceneSectionPanel::OnNavigationRequest )
	EVT_BUTTON( XRCID( "ExpandPseudoButton" ), CEdSceneSectionPanel::OnExpandClicked )
	EVT_BUTTON( XRCID( "CollapsePseudoButton" ), CEdSceneSectionPanel::OnCollapseClicked )
	EVT_LEFT_DCLICK( CEdSceneSectionPanel::OnDoubleClick )
	EVT_CONTEXT_MENU( CEdSceneSectionPanel::OnContextMenu )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDLINE, CEdSceneSectionPanel::OnAddDialogLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDCHOICE, CEdSceneSectionPanel::OnAddDialogChoice )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDCOMMENT, CEdSceneSectionPanel::OnAddComment )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDQUESTCHOICE, CEdSceneSectionPanel::OnAddQuestChoice )
	EVT_MENU( wxID_STORYSCENEEDITOR_DELETESECTION, CEdSceneSectionPanel::OnRemoveSection )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDSCRIPTLINE, CEdSceneSectionPanel::OnAddScriptLine )
	EVT_MENU( wxID_STORYSCENEEDITOR_CUTELEMENTS, CEdSceneSectionPanel::OnCutElements )
	EVT_MENU( wxID_STORYSCENEEDITOR_COPYELEMENTS, CEdSceneSectionPanel::OnCopyElements )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTEELEMENTS, CEdSceneSectionPanel::OnPasteElements )
	EVT_MENU( wxID_STORYSCENEEDITOR_SHIFTSECTIONDOWN, CEdSceneSectionPanel::OnShiftSectionPanelDown )
	EVT_MENU( wxID_STORYSCENEEDITOR_SHIFTSECTIONUP, CEdSceneSectionPanel::OnShiftSectionPanelUp )

	EVT_MENU( wxID_STORYSCENEEDITOR_COPYSECTIONS, CEdSceneSectionPanel::OnCopySection )
	EVT_MENU( wxID_STORYSCENEEDITOR_CUTSECTIONS, CEdSceneSectionPanel::OnCutSection )
	EVT_MENU( wxID_STORYSCENEEDITOR_PASTESECTIONS, CEdSceneSectionPanel::OnPasteSection )

END_EVENT_TABLE()

CEdSceneSectionPanel::CEdSceneSectionPanel( wxWindow* parent, CEdSceneEditorScreenplayPanel* dialogEditor, CStorySceneSection* dialogSection, CEdUndoManager* undoManager )
	: m_storySceneEditor( dialogEditor )
	, m_undoManager( undoManager )
	, m_hasChoice( false )
	, m_hasQuestChoiceLine( false )
	, m_dialogSection( NULL )
	, m_firstSelectedElement( NULL )
	, m_sectionNameField( NULL )
	, m_sectionFlagsStatic( NULL )
{
	ASSERT( m_undoManager != NULL );

	Create( parent );
	this->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer231 = new wxBoxSizer( wxVERTICAL );

	wxPanel* m_panel109 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxCLIP_CHILDREN|wxTAB_TRAVERSAL );
	m_panel109->SetFont( wxFont( 7, 70, 90, 90, false, wxEmptyString ) );
	m_panel109->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer2331 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer250 = new wxBoxSizer( wxHORIZONTAL );

	m_sectionNameField = new CEdDialogSectionTitle( m_panel109, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxNO_BORDER );
	m_sectionNameField->SetFont( wxFont( 16, 70, 90, 92, false, wxT("Courier New") ) );

	bSizer250->Add( m_sectionNameField, 1, wxEXPAND|wxLEFT, 5 );

	m_expandPseudoButton = new wxStaticText( m_panel109, wxID_ANY, wxT("+"), wxDefaultPosition, wxDefaultSize, 0 );
	m_expandPseudoButton->Wrap( -1 );
	m_expandPseudoButton->Hide();

	bSizer250->Add( m_expandPseudoButton, 0, wxALL, 5 );

	m_collapsePseudoButton = new wxStaticText( m_panel109, wxID_ANY, wxT("-"), wxDefaultPosition, wxDefaultSize, 0 );
	m_collapsePseudoButton->Wrap( -1 );
	bSizer250->Add( m_collapsePseudoButton, 0, wxALL, 5 );

	bSizer2331->Add( bSizer250, 0, wxEXPAND, 1 );

	m_sectionFlagsStatic = new wxStaticText( m_panel109, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sectionFlagsStatic->Wrap( -1 );
	bSizer2331->Add( m_sectionFlagsStatic, 0, wxALL|wxEXPAND, 5 );

	m_linkPanel = new wxPanel( m_panel109, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxNO_BORDER );
	m_linkPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer232 = new wxBoxSizer( wxVERTICAL );

	m_linkPanel->SetSizer( bSizer232 );
	m_linkPanel->Layout();
	bSizer232->Fit( m_linkPanel );
	bSizer2331->Add( m_linkPanel, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 0 );

	m_panel109->SetSizer( bSizer2331 );
	m_panel109->Layout();
	bSizer2331->Fit( m_panel109 );
	bSizer231->Add( m_panel109, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5 );

	m_dialogPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	m_dialogPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer233 = new wxBoxSizer( wxVERTICAL );

	m_dialogPanel->SetSizer( bSizer233 );
	m_dialogPanel->Layout();
	bSizer233->Fit( m_dialogPanel );
	bSizer231->Add( m_dialogPanel, 0, wxALL|wxEXPAND, 5 );

	m_postLinkPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxNO_BORDER );
	m_postLinkPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer2321 = new wxBoxSizer( wxVERTICAL );

	m_postLinkPanel->SetSizer( bSizer2321 );
	m_postLinkPanel->Layout();
	bSizer2321->Fit( m_postLinkPanel );
	bSizer231->Add( m_postLinkPanel, 0, wxEXPAND | wxALL, 0 );

	this->SetSizer( bSizer231 );
	this->Layout();


	//////////////////////////////////////////////////////////////////////////

	m_linkPanel->Show( false );
	m_linkPanel->Enable( false );

	m_sectionNameField->SetHint( wxT( "Section name" ) );

	m_sectionNameField->Bind( wxEVT_KILL_FOCUS, &CEdSceneSectionPanel::OnNameFieldLostFocus, this );
	m_sectionNameField->Bind( wxEVT_CHAR, &CEdSceneSectionPanel::OnCharPressed, this );
	m_sectionNameField->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdSceneSectionPanel::OnNameChanged, this );
	m_sectionNameField->Bind( wxEVT_COMMAND_TEXT_ENTER, &CEdSceneSectionPanel::OnSectionNameEnter, this );
	m_sectionNameField->Bind( wxEVT_COMMAND_TEXT_PASTE, &CEdSceneSectionPanel::OnSectionNamePaste, this );

	CEdStorySceneHandlerFactory* handlerFactory = GetStorySceneEditor()->GetHandlerFactory();
	IEdDialogEditorHandler* arrowTraverseHandler = handlerFactory->CreateArrowTraverseHandler();
	IEdDialogEditorHandler* scrollOnFocusHandler = handlerFactory->CreateScrollOnFocusHandler();
	
	arrowTraverseHandler->ConnectTo( m_sectionNameField );
	scrollOnFocusHandler->ConnectTo( m_sectionNameField );

	handlerFactory->CreatePseudoButtonHandler()->ConnectTo( m_collapsePseudoButton );
	handlerFactory->CreatePseudoButtonHandler()->ConnectTo( m_expandPseudoButton );

	RED_FATAL_ASSERT( dialogSection, "CEdSceneSectionPanel::CEdSceneSectionPanel(): section must not be nullptr." );
	SetDialogSection( dialogSection );
}

CEdSceneSectionPanel::~CEdSceneSectionPanel()
{
	m_undoManager->NotifyObjectRemoved( m_dialogSection );
	
	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		m_dialogElements[ i ]->Destroy();
	}
	m_dialogElements.Clear();
}

void CEdSceneSectionPanel::CommitChanges()
{
	for( CEdStorySceneElementPanel* el : m_dialogElements )
	{
		el->CommitChanges();
	}
}

wxString CEdSceneSectionPanel::GetSectionName()
{
	return m_sectionNameField->GetValue();
}

void CEdSceneSectionPanel::SetFocus()
{
	wxPanel::SetFocus();
	m_sectionNameField->SetFocus();
}

Bool CEdSceneSectionPanel::CanAddDialogLine( Int32 index, Bool after ) const
{
	return m_dialogSection ? m_dialogSection->CanAddDialogLine( index, after ) : false;
}

Bool CEdSceneSectionPanel::CanAddRandomLine( Int32 index, Bool after ) const
{
	return m_dialogSection ? m_dialogSection->CanAddRandomLine( index, after ) : false;
}

Bool CEdSceneSectionPanel::CanAddQuestChoice( Int32 index, Bool after ) const
{
	return m_dialogSection ? m_dialogSection->CanAddQuestChoice( index, after ) : false;
}

Bool CEdSceneSectionPanel::CanAddComment( Int32 index, Bool after ) const
{
	return m_dialogSection ? m_dialogSection->CanAddComment( index, after ) : false;
}

Bool CEdSceneSectionPanel::CanAddScriptLine( Int32 index, Bool after ) const
{
	return m_dialogSection ? m_dialogSection->CanAddScriptLine( index, after ) : false;
}

Bool CEdSceneSectionPanel::CanAddChoice()
{
	return m_dialogSection ? m_dialogSection->CanAddChoice() : false;
}

CStorySceneQuestChoiceLine* CEdSceneSectionPanel::AddQuestChoice()
{
	m_storySceneEditor->PreChangeSceneElements();

	CEdStorySceneQuestChoicePanel* sectionComment = new CEdStorySceneQuestChoicePanel( m_dialogPanel, this, m_undoManager );
	sectionComment->InitializeElementHandling();

	AddDialogLineElement( sectionComment, 0, false );

	CStorySceneQuestChoiceLine* dialogComment = m_dialogSection->SetQuestChoice();
	sectionComment->SetStorySceneElement( dialogComment );

	m_storySceneEditor->PostChangeSceneElements( this );

	m_hasQuestChoiceLine = true;

	sectionComment->SetFocus();

	return dialogComment;
}

CStorySceneComment* CEdSceneSectionPanel::AddComment( Int32 index /*= wxNOT_FOUND*/, Bool after /*= true */ )
{
	m_storySceneEditor->PreChangeSceneElements();

	CEdStorySceneCommentPanel* sectionComment = new CEdStorySceneCommentPanel( m_dialogPanel, this, m_undoManager );
	sectionComment->InitializeElementHandling();

	Int32 insertedIndex = AddDialogLineElement( sectionComment, index, after );
	
	CStorySceneComment* dialogComment = m_dialogSection->AddComment( insertedIndex );
	sectionComment->SetStorySceneElement( dialogComment );

	m_storySceneEditor->PostChangeSceneElements( this );

	sectionComment->SetFocus();

	return dialogComment;
}

CAbstractStorySceneLine* CEdSceneSectionPanel::AddDialogLine( Int32 index /*= -1*/, Bool after /*= true */ )
{
	m_storySceneEditor->PreChangeSceneElements();

	CEdStorySceneLinePanel* dialogLinePanel = new CEdStorySceneLinePanel( m_dialogPanel, this, m_undoManager );

	Int32 insertedIndex = AddDialogLineElement( dialogLinePanel, index, after );

	// Extract previous element
	CStorySceneElement* previousElement = NULL;
	if( insertedIndex > 0 )
	{
		previousElement = m_dialogElements[ insertedIndex - 1 ]->GetDialogElement();
	}

	CAbstractStorySceneLine* dialogLine = m_dialogSection->AddDialogLineAfter( previousElement );
	dialogLine->SetSpeakingTo( GenerateSpeakingToForLine( dialogLine,index ) );
	dialogLinePanel->SetStorySceneElement( dialogLine );

	m_storySceneEditor->UpdateTimeline();

	m_storySceneEditor->PostChangeSceneElements( this );

	dialogLinePanel->SetFocus();

	return dialogLine;
}


CName CEdSceneSectionPanel::GenerateSpeakingToForLine( const CAbstractStorySceneLine* line ,Uint32 index ) const 
{
	CName speakingTo;

	const CStorySceneDialogsetInstance* dialogset = m_dialogSection->GetScene()->GetFirstDialogsetAtSection( m_dialogSection );

	if ( dialogset )
	{
		TDynArray<CName> actorNames;
		dialogset->GetAllActorNames( actorNames );
		if ( actorNames.Size() == 2 && line->GetVoiceTag() )
		{
			return actorNames[0] != line->GetVoiceTag() ? actorNames[0] : actorNames[1];
		}
	}

	const CStorySceneLine*  prevLine = m_storySceneEditor->GetPrevLine( line );
	if( prevLine )
	{
		if( prevLine->GetVoiceTag() != line->GetVoiceTag() )
		{
			speakingTo = prevLine->GetVoiceTag();
		}
		else
		{
			speakingTo = prevLine->GetSpeakingTo();
		}
	}

	for ( Uint32 i = index; i <= m_dialogSection->GetNumberOfElements() && speakingTo == CName::NONE; ++i )
	{
		const CStorySceneLine*  nextLine = Cast<CStorySceneLine>( m_dialogSection->GetElement(i) );
		if( nextLine )
		{
			CName nextLineVoicetag	= nextLine->GetVoiceTag();		
			if( nextLineVoicetag != line->GetVoiceTag() )
			{
				speakingTo	= nextLineVoicetag;
			}
		}
	}
	return speakingTo;
}

void CEdSceneSectionPanel::GenerateSpeakingToForSection()
{
	for( Uint32 i = 0; i < m_dialogSection->GetNumberOfElements(); i++ )
	{
		CAbstractStorySceneLine* sceneLine = Cast<CAbstractStorySceneLine>( m_dialogSection->GetElement(i) );
		if( sceneLine )
		{
			CName speakingTo = sceneLine->GetSpeakingTo();
			if( speakingTo == CName::NONE )
			{
				speakingTo = GenerateSpeakingToForLine( sceneLine, i );
				sceneLine->SetSpeakingTo(speakingTo );
			}			
		}
	}
}

CStorySceneScriptLine* CEdSceneSectionPanel::AddScriptLine( Int32 index /*= wxNOT_FOUND*/, Bool after /*= true */ )
{
	m_storySceneEditor->PreChangeSceneElements();

	CEdSceneScriptLinePanel* scriptLinePanel = new CEdSceneScriptLinePanel( m_dialogPanel, this, m_undoManager );
	scriptLinePanel->InitializeElementHandling();

	Int32 insertedIndex = AddDialogLineElement( scriptLinePanel, index, after );

	// Extract previous element
	CStorySceneElement* previousElement = NULL;
	if( insertedIndex > 0 )
	{
		previousElement = m_dialogElements[ insertedIndex - 1 ]->GetDialogElement();
	}

	CStorySceneScriptLine* scriptLine = m_dialogSection->AddScriptLineAfter( previousElement );
	scriptLinePanel->SetStorySceneElement( scriptLine );

	m_storySceneEditor->PostChangeSceneElements( this );

	scriptLinePanel->SetFocus();

	return scriptLine;
}

CStorySceneChoice* CEdSceneSectionPanel::AddChoice()
{
	m_storySceneEditor->PreChangeSceneElements();

	CEdStorySceneChoicePanel* choicePanel = new CEdStorySceneChoicePanel( m_dialogPanel, this, m_undoManager );
	AddDialogLineElement( choicePanel, -1, true );
	
	CStorySceneChoice* storySceneChoice = m_dialogSection->AddChoice();
	choicePanel->SetStorySceneElement( storySceneChoice );

	m_hasChoice = true;

	m_storySceneEditor->PostChangeSceneElements( this );
	
	choicePanel->AddChoiceLine( wxNOT_FOUND, true );

	return storySceneChoice;
}

Int32 CEdSceneSectionPanel::AddDialogLineElement( CEdStorySceneElementPanel* storySceneElement, Int32 index, Bool after )
{
	Int32 numberOfSizerItems = m_dialogPanel->GetSizer()->GetChildren().GetCount();

	if ( index == wxNOT_FOUND || ( index == m_dialogElements.Size() && after == true ) )
	{
		m_dialogPanel->GetSizer()->Add( storySceneElement, 0, wxEXPAND | wxALL, 5 );
		m_dialogElements.PushBack( storySceneElement );
		index = -1;
	}
	else
	{
		if ( after == true )
		{
			index += 1;
		}
		storySceneElement->MoveBeforeInTabOrder( m_dialogPanel->GetChildren()[ index ] );

		m_dialogPanel->GetSizer()->Insert( index, storySceneElement, 0, wxEXPAND | wxALL, 5 );
		m_dialogElements.Insert( index, storySceneElement );
	}

	

	return index;
}

Bool CEdSceneSectionPanel::CanRemoveDialogElement( CEdStorySceneElementPanel* element ) const
{
	return m_dialogSection ? m_dialogSection->CanRemoveElement( element->GetDialogElement() ) : false;
}

Bool CEdSceneSectionPanel::CanRemoveDialogElement( Uint32 elementIndex ) const
{
	return CanRemoveDialogElement( m_dialogElements[ elementIndex ] );
}

void CEdSceneSectionPanel::RemoveDialogElement( CEdStorySceneElementPanel* element )
{
	if ( CanRemoveDialogElement( element ) )
	{
		m_storySceneEditor->PreChangeSceneElements();

		Uint32 elementIndex = m_dialogElements.GetIndex( element );
		if ( element->GetElementType() == SSET_Choice )
		{
			m_hasChoice = false;
			m_dialogSection->RemoveChoice();
		}
		else if ( element->GetElementType() == SSET_QuestChoiceLine )
		{
			m_hasQuestChoiceLine = false;
			m_dialogSection->RemoveChoice();
		}
		else
		{
			m_dialogSection->RemoveElement( element->GetDialogElement() );
		}

		m_dialogElements.Remove( element );

		m_selectedElements.Erase( element );
		if ( m_firstSelectedElement == element )
		{
			m_firstSelectedElement = NULL;
		}

		element->Destroy();

		m_storySceneEditor->UpdateTimeline();

		m_storySceneEditor->PostChangeSceneElements( this );

		if ( elementIndex > 0 && elementIndex < m_dialogElements.Size() )
		{
			m_dialogElements[ elementIndex - 1 ]->SetFocus();
		}
	}
}

void CEdSceneSectionPanel::RemoveDialogElement( Uint32 elementIndex )
{
	RemoveDialogElement( m_dialogElements[ elementIndex ] );
}

void CEdSceneSectionPanel::ChangeDialogElementType( CEdStorySceneElementPanel* element, EStorySceneElementType type )
{
	Int32 index = FindDialogElementIndex( element );

	if ( index == wxNOT_FOUND )
		return;

	CStorySceneElement* oldElement = element->GetDialogElement();

	CUndoDialogElementExistance::PrepareDeletionStep( *m_undoManager, this, oldElement, index );
	RemoveDialogElement( element );

	CStorySceneElement* newElement;
	if ( type == SSET_Comment )
	{
		newElement = AddComment( index, false );
	}
	else if ( type == SSET_Choice )
	{
		newElement = AddChoice();
	}
	else if ( type == SSET_QuestChoiceLine )
	{
		newElement = AddQuestChoice();
	}
	else if ( type == SSET_Line )
	{
		newElement = AddDialogLine( index, false );
	}
	else if ( type == SSET_ScriptLine )
	{
		newElement = AddScriptLine( index, false );	
	}

	CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, index );
	CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
}

void CEdSceneSectionPanel::SetDialogSection( CStorySceneSection* section )
{
	m_storySceneEditor->PreChangeSceneElements();

	while ( m_dialogElements.Empty() == false )
	{
		CEdStorySceneElementPanel* storySceneElement = m_dialogElements.PopBack();
		storySceneElement->Destroy();
	}

	m_dialogSection = section;

	if ( section == NULL )
	{
		m_storySceneEditor->PostChangeSceneElements( this );
		return;
	}

	RefreshData();

	for ( Int32 i = (Int32)m_dialogSection->GetNumberOfElements()-1; i>=0; --i )
	{
		CStorySceneElement* element = m_dialogSection->GetElement( i );
		if ( !element )
		{
			m_dialogSection->RemoveElement( m_dialogSection->GetElement( i ) );
		}
	}

	// add new elements
	for ( Uint32 i = 0; i < m_dialogSection->GetNumberOfElements(); ++i )
	{
		CStorySceneElement* element = m_dialogSection->GetElement( i );

		if ( element->GetElementID().Empty() == true )
		{
			element->GenerateElementID();
		}

		CreateAndAddStorySceneElementPanel( element );
	}

	CStorySceneElement* choiceElement = m_dialogSection->GetChoice();
	if ( choiceElement != NULL )
	{
		if ( choiceElement->GetElementID().Empty() == true )
		{
			choiceElement->GenerateElementID();
		}

		CreateAndAddStorySceneElementPanel( choiceElement );
	}

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::RefreshData()
{
	String sectionName = m_dialogSection->GetName();

	//temp debug info about settings - may evolve
	//sectionName += TXT( "[" ) + m_dialogSection->GetScene()->GetSettingNameAtSection( m_dialogSection ).AsString() + TXT( "]" );

	m_sectionNameField->ChangeValue( sectionName.AsChar() );

	String sectionFlagsText = TXT( "" );
	if ( m_dialogSection->IsGameplay() == true )
	{
		sectionFlagsText += TXT( "GAMEPLAY; " );
	}
	if ( m_dialogSection->IsImportant() == true )
	{
		sectionFlagsText += TXT( "IMPORTANT; " );
	}
		
	m_sectionFlagsStatic->SetLabel( sectionFlagsText.AsChar() );
	m_sectionFlagsStatic->Show( sectionFlagsText.Empty() == false 
		&& m_storySceneEditor->ShouldShowOnlyScriptTexts() == false );

	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		m_dialogElements[ i ]->RefreshData();
	}
}


void CEdSceneSectionPanel::RefreshHelperData()
{
	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		m_dialogElements[ i ]->RefreshHelperData();
	}
}

Bool CEdSceneSectionPanel::RefreshDialogObject( CObject* objectToRefresh )
{
	if ( m_dialogSection == objectToRefresh )
	{
		RefreshData();
		return true;
	}
	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		if ( m_dialogElements[ i ]->RefreshDialogObject( objectToRefresh ) == true )
		{
			return true;
		}
	}
	return false;

}

void CEdSceneSectionPanel::RemovePreLink( CEdSceneSectionPanel* linkedSection )
{
	m_storySceneEditor->PreChangeSceneElements();

	THashMap< CEdSceneSectionPanel*, wxControl* >::iterator linkToRemoveIter 
		= m_previousSectionsLinks.Find( linkedSection );

	if ( linkToRemoveIter == m_previousSectionsLinks.End() )
	{
		m_storySceneEditor->PostChangeSceneElements( this );
		return;
	}

	wxControl* linkedControl = linkToRemoveIter->m_second;
	
	ASSERT( linkedControl );
	linkedSection->UnregisterControlToThis( linkedControl );
	RemoveControl( linkedControl );
	m_previousSectionsLinks.Erase( linkedSection );

	if ( m_previousSectionsLinks.Empty() == true )
	{
		m_linkPanel->Enable( false );
		m_linkPanel->Show( false );
	}

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::RefreshConnectedSections()
{
	m_storySceneEditor->PreChangeSceneElements();

	ClearPostLinks();


	TDynArray< CStorySceneControlPart* > connectedControlParts;

	for ( Uint32 i = 0; i < m_dialogSection->GetNumberOfInputPaths(); ++i )
	{
		CStorySceneLinkElement* nextLinkElement = m_dialogSection->GetInputPathLinkElement( i )->GetNextElement();
		CStorySceneControlPart* nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement );
		if ( nextControlPart == NULL && nextLinkElement != NULL)
		{
			nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement->GetParent() );
		}
		if ( nextControlPart != NULL )
		{
			nextControlPart->CollectControlParts( connectedControlParts );
		}
	}

	


	for ( TDynArray< CStorySceneControlPart* >::iterator iter = connectedControlParts.Begin();
		iter != connectedControlParts.End(); ++iter )
	{
		CStorySceneControlPart* controlPart = *iter;
		if ( controlPart->IsA< CStorySceneOutput >() == true )
		{
			CStorySceneOutput* sceneOutput = Cast< CStorySceneOutput >( controlPart );
			AddOutputMarker( sceneOutput->GetName() );
		}
		else if ( controlPart->IsA< CStorySceneSection >() == true )
		{
			CStorySceneSection* connectedSection = Cast< CStorySceneSection >( controlPart );
			m_connectedSections.PushBackUnique( m_storySceneEditor->FindSectionPanel( connectedSection ) );
		}
	}

	for ( TDynArray< CEdSceneSectionPanel* >::iterator iter = m_connectedSections.Begin();
		iter != m_connectedSections.End(); ++iter )
	{
		if ( CEdSceneSectionPanel* sectionPanel = *iter )
		{
			AddPostLink( sectionPanel );
			sectionPanel->AddPreLink( this, this );
		}
	}

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::RegisterControlToThis( wxControl* control )
{
	m_controlsToThis.PushBackUnique( control );
}

void CEdSceneSectionPanel::UnregisterControlToThis( wxControl* control )
{
	m_controlsToThis.Remove( control );
}

void CEdSceneSectionPanel::RemoveControl( wxWindow* control )
{
	wxWindow* parent = control->GetParent();
	parent->GetSizer()->Detach( control );
	parent->RemoveChild( control );
	control->Destroy();
}

void CEdSceneSectionPanel::AddPostLink( CEdSceneSectionPanel* linkedSection )
{
	m_storySceneEditor->PreChangeSceneElements();

	m_postLinkPanel->Show();

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

void CEdSceneSectionPanel::AddPreLink( CEdSceneSectionPanel* linkedSectionPanel, wxWindow* linkWindow )
{
	m_storySceneEditor->PreChangeSceneElements();

	wxStaticText* link = new wxStaticText( m_linkPanel, wxID_ANY, 
		linkedSectionPanel->GetSectionName(), wxDefaultPosition, wxDefaultSize, 0 );

	link->SetFont( wxFont( 7, 70, 90, 90, true, wxEmptyString ) );

	link->SetClientData( linkWindow );

	m_storySceneEditor->GetHandlerFactory()->CreateHyperlinkHandler()->ConnectTo( link );

	m_linkPanel->GetSizer()->Add( link, 0, wxALL, 1 );
	m_linkPanel->Show();
	m_linkPanel->Enable();

	m_previousSectionsLinks.Insert( linkedSectionPanel, link );
	linkedSectionPanel->RegisterControlToThis( link );

	m_storySceneEditor->PostChangeSceneElements( this );
}


void CEdSceneSectionPanel::AddOutputMarker( const String& outputName )
{
	m_storySceneEditor->PreChangeSceneElements();

	m_postLinkPanel->Show();

	wxStaticText* outputLabel = new wxStaticText( m_postLinkPanel, wxID_ANY, 
		wxString::Format( TXT( "ENDING( %s )" ), outputName.AsChar() ), 
		wxDefaultPosition, wxDefaultSize, 0 );

	outputLabel->SetFont( wxFont( 7, 70, 90, 92, true, wxEmptyString ) );
	outputLabel->SetForegroundColour( *wxBLACK );

	m_postLinkPanel->GetSizer()->Add( outputLabel, 0, wxALL, 1 );

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::RemoveLink( CEdSceneSectionPanel* nextSection )
{
	wxControl** nextSectionControlPtr = m_nextSectionsLinks.FindPtr( nextSection );

	if( nextSectionControlPtr )
	{
		wxControl* nextSectionControl =  *( nextSectionControlPtr );
		ASSERT( nextSectionControl );

		nextSection->UnregisterControlToThis( nextSectionControl );
		RemoveControl( nextSectionControl );
		m_nextSectionsLinks.Erase( nextSection );

		nextSection->RemovePreLink( this );
	}

}

void CEdSceneSectionPanel::ClearPostLinks()
{
	while ( m_connectedSections.Empty() == false )
	{
		CEdSceneSectionPanel* nextSection = m_connectedSections.PopBack();

		RemoveLink( nextSection );
	}
	m_postLinkPanel->DestroyChildren();
}

void CEdSceneSectionPanel::ClearPreLinks()
{
	TDynArray< CEdSceneSectionPanel* > keys;
	m_previousSectionsLinks.GetKeys( keys );

	for( Uint32 i = 0; i < keys.Size(); ++i )
	{
		keys[ i ]->RemoveLink( this );
	}

	m_previousSectionsLinks.Clear();
}

void CEdSceneSectionPanel::PutSelectionIntoUndoSteps( Bool finalize )
{
	for ( auto selIt = m_selectedElements.Begin(); selIt != m_selectedElements.End(); ++selIt )
	{
		CUndoDialogElementExistance::PrepareDeletionStep( *m_undoManager, this, (*selIt)->GetDialogElement(), m_dialogElements.GetIndex( *selIt ) );
	}

	if ( finalize )
	{
		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
	}
}

void CEdSceneSectionPanel::PasteElementsAtLocation( Int32 pasteLocationIndex )
{
	if( m_selectedElements.Size() != 1 )
	{
		GFeedback->ShowMsg( TXT( "Screenplay panel" ), TXT( "Please indicate paste position by selecting exactly one scene element." ) );
		return;
	}

	Bool clipboardOpened = wxTheClipboard->Open();

	if ( clipboardOpened == false ) {
		return;
	}

	CClipboardData elementClipboardData( TXT( "StorySceneElements" ) );

	if ( wxTheClipboard->IsSupported( elementClipboardData.GetDataFormat() ) == true 
		&& wxTheClipboard->GetData( elementClipboardData ) == true )
	{
		CMemoryFileReader reader( elementClipboardData.GetData(), 0 );
		CDependencyLoader loader( reader, NULL );
		DependencyLoadingContext loadingContext;
		loadingContext.m_parent = m_dialogSection;

		if ( loader.LoadObjects( loadingContext ) == false )
		{
			wxTheClipboard->Close();
			return;
		}

		// Get index at which selected scene element exists in CStorySceneSection::m_sceneElements. This index
		// is different from pasteLocationIndex that can be used with CEdSceneSectionPanel::m_dialogElements.
		// These indexes differ because CStorySceneSection::m_sceneElements contains all scene elements while
		// CEdSceneSectionPanel::m_dialogElements contains only story scene comments and dialog lines.
		CStorySceneElement* selectedSceneElement = ( *m_selectedElements.Begin() )->GetDialogElement();
		const Uint32 selectedSceneElementIndex = m_dialogSection->GetElements().GetIndex( selectedSceneElement );

		Bool wasCopy = elementClipboardData.IsCopy();

		m_storySceneEditor->PreChangeSceneElements();

		for ( TDynArray< CObject* >::iterator iter = loadingContext.m_loadedRootObjects.Begin();
			iter != loadingContext.m_loadedRootObjects.End(); ++iter )
		{
			CStorySceneElement* sceneElement = SafeCast< CStorySceneElement >( *iter );

			if ( wasCopy == true )
			{
				sceneElement->ClearElementID();
				sceneElement->GenerateElementID();
				sceneElement->MakeCopyUnique();
			}

			if ( sceneElement->GetElementID().Empty() == true )
			{
				sceneElement->GenerateElementID();
			}

			m_dialogSection->AddSceneElement( sceneElement, selectedSceneElementIndex );

			CreateAndAddStorySceneElementPanel( sceneElement, pasteLocationIndex );

			m_dialogSection->NotifyAboutElementAdded( sceneElement );
			if ( sceneElement->IsA< CStorySceneChoice >() )
			{
				CStorySceneChoice* sceneChoice = Cast< CStorySceneChoice >( sceneElement );
				for ( Uint32 i = 0; i < sceneChoice->GetNumberOfChoiceLines(); ++i )
				{
					sceneChoice->NotifyAboutChoiceLineAdded( i );
				}
			}

			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, sceneElement, pasteLocationIndex );
		}

		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );

		m_storySceneEditor->PostChangeSceneElements( this );
		m_storySceneEditor->RefreshDialog();

		const Bool cutOperation = !wasCopy;
		if( cutOperation )
		{
			wxTheClipboard->Clear();
		}
	}

	wxTheClipboard->Close();
}

void CEdSceneSectionPanel::ExpandSection()
{
	m_storySceneEditor->PreChangeSceneElements();
	m_collapsePseudoButton->Show();
	m_dialogPanel->Show();
	m_expandPseudoButton->Hide();
	Fit();
	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::CollapseSection()
{
	m_storySceneEditor->PreChangeSceneElements();
	m_collapsePseudoButton->Hide();
	m_dialogPanel->Hide();
	m_expandPseudoButton->Show();
	Fit();
	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::HandleElementSelection( CEdStorySceneElementPanel* element, Bool mouseMode /*= false */ )
{
	if ( element == NULL || IsInElementSelectionMode() == false )
	{
		return;
	}

	Bool isElementSelected = IsElementSelected( element );

	if ( mouseMode == false )
	{
		ClearSelectedElements();

		Int32 firstSelectedPanelIndex = FindDialogElementIndex( m_firstSelectedElement );
		Int32 elementIndex = FindDialogElementIndex( element );

		Uint32 selectionStartIndex = min( firstSelectedPanelIndex, elementIndex );
		Uint32 selectionEndIndex = max( firstSelectedPanelIndex, elementIndex );

		for( Uint32 i = selectionStartIndex; i <= selectionEndIndex; ++i )
		{
			m_dialogElements[ i ]->MarkElementAsSelected();
			m_selectedElements.Insert( m_dialogElements[ i ] );
		}
	}
	else
	{
		if ( wxIsShiftDown() == true )
		{
			ClearSelectedElements();
			
			Int32 firstSelectedPanelIndex = FindDialogElementIndex( m_firstSelectedElement );
			Int32 elementIndex = FindDialogElementIndex( element );
			
			Uint32 selectionStartIndex = min( firstSelectedPanelIndex, elementIndex );
			Uint32 selectionEndIndex = max( firstSelectedPanelIndex, elementIndex );
			
			for( Uint32 i = selectionStartIndex; i <= selectionEndIndex; ++i )
			{
				m_dialogElements[ i ]->MarkElementAsSelected();
				m_selectedElements.Insert( m_dialogElements[ i ] );
			}
		}
		else if ( wxIsCtrlDown() == true )
		{
			if ( isElementSelected == true )
			{
				element->UnmarkElementAsSelected();
				m_selectedElements.Erase( element );
			}
			else
			{
				element->MarkElementAsSelected();
				m_selectedElements.Insert( element );
			}
		}
		else
		{
			if( isElementSelected )
			{
				element->UnmarkElementAsSelected();
				m_selectedElements.Erase( element );

				if( m_selectedElements.Empty() )
				{
					LeaveElementSelectionMode();
				}
			}
			else
			{
				LeaveElementSelectionMode();
				EnterElementSelectionMode( element );
			}
		}
	}

	element->SetFocus();
	element->OnSelected();
	Refresh();
}

bool CEdSceneSectionPanel::IsElementSelected( CEdStorySceneElementPanel* element )
{
	return ( m_selectedElements.Find( element ) != m_selectedElements.End() );
}

void CEdSceneSectionPanel::ClearSelectedElements()
{
	for( THashSet< CEdStorySceneElementPanel* >::iterator iter = m_selectedElements.Begin();
		iter != m_selectedElements.End(); ++iter )
	{
		CEdStorySceneElementPanel* selectedElement = *iter;
		selectedElement->UnmarkElementAsSelected();
	}
	m_selectedElements.Clear();
	Refresh();
}

void CEdSceneSectionPanel::EnterElementSelectionMode( CEdStorySceneElementPanel* initialElement )
{
	m_storySceneEditor->SetSectionPanelHandlingElementSelection( this );

	initialElement->MarkElementAsSelected();
	m_selectedElements.Insert( initialElement );
	m_firstSelectedElement = initialElement;
	
	initialElement->SetFocus();
	initialElement->OnSelected();

	Refresh();
}

void CEdSceneSectionPanel::LeaveElementSelectionMode()
{
	ClearSelectedElements();
	m_storySceneEditor->SetSectionPanelHandlingElementSelection( NULL );
}

Int32 CEdSceneSectionPanel::FindDialogElementIndex( wxWindow* storySceneElement )
{
	wxSizerItem* lineSizerItem = m_dialogPanel->GetSizer()->GetItem( storySceneElement );
	return m_dialogPanel->GetSizer()->GetChildren().IndexOf( lineSizerItem );
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::GetElementAfter( CEdStorySceneElementPanel* element )
{
	Int32 elementIndex = m_dialogElements.GetIndex( element );
	if ( elementIndex == -1 || elementIndex == m_dialogElements.Size() - 1 )
	{
		return NULL;
	}
	return m_dialogElements[ elementIndex + 1 ];
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::GetElementBefore( CEdStorySceneElementPanel* element )
{
	Int32 elementIndex = m_dialogElements.GetIndex( element );
	if ( elementIndex == -1 || elementIndex == 0 )
	{
		return NULL;
	}
	return m_dialogElements[ elementIndex - 1 ];
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::GetPanelByElement( CStorySceneElement* element )
{
	for ( auto elIt = m_dialogElements.Begin(); elIt != m_dialogElements.End(); ++elIt )
	{
		if ( (*elIt)->GetDialogElement() == element )
		{
			return *elIt;
		}
	}

	return NULL;
}

Bool CEdSceneSectionPanel::IsLastElementChild( wxWindow* window )
{
	if ( m_dialogElements.Empty() == true && HasChoice() == false )
	{
		return window == m_sectionNameField;
	}
	else
	{
		return m_dialogElements[ m_dialogElements.Size() - 1 ]->FindWindow( window->GetId() ) != NULL;
	}
}

Bool CEdSceneSectionPanel::IsFirstElementChild( wxWindow* window )
{
	if ( m_dialogElements.Empty() == true )
	{
		return false;
	}
	return m_dialogElements[ 0 ]->FindWindow( window->GetId() ) != NULL;
}

void CEdSceneSectionPanel::CopyElements( Bool generateNewIDs /*= true */ )
{
	if ( m_selectedElements.Empty() == false )
	{
		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		CDependencySaver saver( writer, NULL );

		TDynArray<CObject*> toSave;

		for ( TDynArray< CEdStorySceneElementPanel* >::iterator iter = m_dialogElements.Begin();
			iter != m_dialogElements.End(); ++iter )
		{
			CEdStorySceneElementPanel* elementPanel = *iter;
			if ( m_selectedElements.Find( elementPanel ) != m_selectedElements.End() )
			{
				CStorySceneElement* element = elementPanel->GetDialogElement();
				toSave.Insert( 0, element );
			}
		}

		// Save
		DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&)toSave );
		if ( saver.SaveObjects( context ) == false )
		{
			WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
			return;
		}

		if ( wxTheClipboard->Open() )
		{
			wxTheClipboard->SetData( new CClipboardData( TXT( "StorySceneElements" ), buffer, generateNewIDs ) );
			wxTheClipboard->Close();
		}
	}
}

void CEdSceneSectionPanel::CutElements()
{
	CopyElements( false );
	PutSelectionIntoUndoSteps( true );
	RemoveSelectedElements();
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::CreateAndAddStorySceneElementPanel( CStorySceneElement* element, Int32 addLocationIndex )
{
	CEdStorySceneElementPanel* elementPanel = NULL;
	if ( element->IsA< CStorySceneLine >() )
	{
		elementPanel = new CEdStorySceneLinePanel( m_dialogPanel, this, m_undoManager );
	}
	else if ( element->IsExactlyA< CStorySceneComment >() )
	{
		CEdStorySceneCommentPanel* commentPanel 
				= new CEdStorySceneCommentPanel( m_dialogPanel, this, m_undoManager );
			commentPanel->InitializeElementHandling();
			elementPanel = commentPanel;
	}
	else if ( element->IsExactlyA< CStorySceneQuestChoiceLine >() )
	{
		if ( m_hasQuestChoiceLine == false )
		{
			CEdStorySceneQuestChoicePanel* commentPanel 
				= new CEdStorySceneQuestChoicePanel( m_dialogPanel, this, m_undoManager );
			commentPanel->InitializeElementHandling();
			elementPanel = commentPanel;

			m_hasQuestChoiceLine = true;
		}
	}
	else if ( element->IsA< CStorySceneChoice >() && m_hasChoice == false)
	{
		elementPanel = new CEdStorySceneChoicePanel( m_dialogPanel, this, m_undoManager );

		m_hasChoice = true;
	}
	else if ( element->IsA< CStorySceneScriptLine >() )
	{
		CEdSceneScriptLinePanel* scriptLinePanel = new CEdSceneScriptLinePanel( m_dialogPanel, this, m_undoManager );
		scriptLinePanel->InitializeElementHandling();
		elementPanel = scriptLinePanel;
	}
	else if ( element->IsA< CStorySceneCutscenePlayer >() )
	{
		CEdSceneCutsceneHeaderPanel* cutsceneDescPanel = new CEdSceneCutsceneHeaderPanel( m_dialogPanel, this, m_undoManager  );
		cutsceneDescPanel->InitializeElementHandling();
		elementPanel = cutsceneDescPanel;
	}
	else if ( element->IsA< CStorySceneVideoElement >() )
	{
		CEdSceneVideoDescriptionPanel* videoDescPanel = new CEdSceneVideoDescriptionPanel( m_dialogPanel, this, m_undoManager  );
		videoDescPanel->InitializeElementHandling();
		elementPanel = videoDescPanel;
	}

	if ( elementPanel != NULL )
	{
		elementPanel->SetStorySceneElement( element );
		
		addLocationIndex = ( addLocationIndex == -1 ) ? m_dialogElements.Size() : addLocationIndex;
		
		AddDialogLineElement( elementPanel, addLocationIndex, false );
	}
	return elementPanel;
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::CreateAndAddStorySceneElementPanel( CStorySceneElement* element )
{
	return CreateAndAddStorySceneElementPanel( element, -1 );
}

void CEdSceneSectionPanel::MarkSectionAsSelected()
{
	SetBackgroundColour( wxColour( 229, 229, 255 ) );
}

void CEdSceneSectionPanel::UnmarkSectionAsSelected()
{
	SetBackgroundColour( *wxWHITE );
}

void CEdSceneSectionPanel::RemoveSelectedElements()
{
	m_storySceneEditor->PreChangeSceneElements();
	
	while ( m_selectedElements.Begin() != m_selectedElements.End() )
	{
		CEdStorySceneElementPanel* selectedElement = *( m_selectedElements.Begin() );
		RemoveDialogElement( selectedElement );
	}

	m_storySceneEditor->PostChangeSceneElements( this );
}

void CEdSceneSectionPanel::ChangeFontSize( Int32 sizeChange )
{
	m_storySceneEditor->ChangeWindowFontSize( m_sectionNameField, sizeChange );

	m_storySceneEditor->ChangeWindowFontSize( m_sectionFlagsStatic, sizeChange );
	
	for ( TDynArray< CEdStorySceneElementPanel* >::iterator iter = m_dialogElements.Begin();
		iter != m_dialogElements.End(); ++iter )
	{
		(*iter)->ChangeFontSize( sizeChange );
	}

	for( THashMap< CEdSceneSectionPanel*, wxControl* >::iterator iter = m_nextSectionsLinks.Begin();
		iter != m_nextSectionsLinks.End(); ++iter )
	{
		m_storySceneEditor->ChangeWindowFontSize( iter->m_second, sizeChange );
	}

	for( THashMap< CEdSceneSectionPanel*, wxControl* >::iterator iter = m_previousSectionsLinks.Begin();
		iter != m_previousSectionsLinks.End(); ++iter )
	{
		m_storySceneEditor->ChangeWindowFontSize( iter->m_second, sizeChange );
	}
}

bool CEdSceneSectionPanel::SetBackgroundColour( const wxColour& colour )
{
	if ( m_storySceneEditor == NULL || m_storySceneEditor->IsInitialized() == false )
	{
		return wxPanel::SetBackgroundColour( colour );
	}

	if ( m_sectionNameField != NULL )
	{
		m_sectionNameField->SetBackgroundColour( colour );
	}
	
	for ( TDynArray< CEdStorySceneElementPanel* >::iterator iter = m_dialogElements.Begin();
		iter != m_dialogElements.End(); ++iter )
	{
		(*iter)->SetBackgroundColour( colour );
	}
	return wxPanel::SetBackgroundColour( colour );
}

CEdStorySceneElementPanel* CEdSceneSectionPanel::GetParentStorySceneElementPanel( wxWindow* window )
{
	wxWindow* parent = window->GetParent();
	while( parent != NULL && parent->IsKindOf( CLASSINFO( CEdStorySceneElementPanel ) ) == false )
	{
		parent = parent->GetParent();
	}
	CEdStorySceneElementPanel* parentElement = NULL;
	if ( parent != NULL )
	{
		parentElement = wxStaticCast( parent, CEdStorySceneElementPanel );
	}
	return parentElement;
}

wxWindow* CEdSceneSectionPanel::GetLastNavigableWindow()
{
	if ( m_dialogElements.Empty() == true )
	{
		return m_sectionNameField;
	}
	return m_dialogElements[ m_dialogElements.Size() - 1 ]->GetLastNavigableWindow();
}

wxWindow* CEdSceneSectionPanel::GetNextNavigableWindow( wxWindow* currentWindow )
{
	CEdStorySceneElementPanel* elementPanel = NULL;
	if ( currentWindow == m_sectionNameField )
	{
		if ( m_dialogElements.Empty() == false )
		{
			elementPanel = m_dialogElements[ 0 ];
			if ( elementPanel->IsShown() == true )
			{
				return elementPanel->GetFirstNavigableWindow();
			}
		}
	}
	else
	{
		elementPanel = GetParentStorySceneElementPanel( currentWindow );
	}
	
	elementPanel = GetElementAfter( elementPanel );
	while ( elementPanel != NULL )
	{
		if ( elementPanel->IsShown() == true )
		{
			return elementPanel->GetFirstNavigableWindow();
		}
		elementPanel = GetElementAfter( elementPanel );
	}
	
	return NULL;
}

wxWindow* CEdSceneSectionPanel::GetPrevNavigableWindow( wxWindow* currentWindow )
{
	if ( currentWindow == m_sectionNameField )
	{
		return NULL;
	}

	CEdStorySceneElementPanel* elementPanel = NULL;	
	elementPanel = GetParentStorySceneElementPanel( currentWindow );
	elementPanel = GetElementBefore( elementPanel );
	
	while ( elementPanel != NULL )
	{
		if ( elementPanel->IsShown() == true )
		{
			return elementPanel->GetLastNavigableWindow();
		}
		elementPanel = GetElementBefore( elementPanel );
	}

	if ( elementPanel == NULL )
	{
		return m_sectionNameField;
	}

	return NULL;
}

void CEdSceneSectionPanel::OnContextMenu( wxContextMenuEvent& event )
{
	wxMenu contextMenu;

	contextMenu.Append( wxID_STORYSCENEEDITOR_SHIFTSECTIONDOWN, wxT( "Move section down" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_SHIFTSECTIONUP, wxT( "Move section up" ) );
	contextMenu.AppendSeparator();
	contextMenu.Append( wxID_STORYSCENEEDITOR_COPYSECTIONS, wxT( "Copy section" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_CUTSECTIONS, wxT( "Cut section" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_PASTESECTIONS, wxT( "Paste section" ) );
	contextMenu.AppendSeparator();
	contextMenu.Append( wxID_STORYSCENEEDITOR_ADDSECTION, wxT( "Add section" ) );
	contextMenu.Append( wxID_STORYSCENEEDITOR_DELETESECTION, wxT( "Delete section" ) );
	contextMenu.AppendSeparator();
	wxMenu* subMenu = new wxMenu();

	if ( m_dialogSection && m_dialogSection->CanAddDialogLine( 0, false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDLINE, wxT( "Dialog line" ) );
	}
	if ( m_dialogSection && m_dialogSection->CanAddQuestChoice( 0, false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDQUESTCHOICE, wxT( "Quest choice line" ) );
	}
	if ( m_dialogSection && m_dialogSection->CanAddComment( 0, false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDCOMMENT, wxT( "Comment" ) );
	}
	if ( m_dialogSection && m_dialogSection->CanAddScriptLine( 0, false ) )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDSCRIPTLINE, wxT( "Script line" ) );
	}
	if ( m_dialogSection && m_dialogSection->CanAddChoice() )
	{
		subMenu->Append( wxID_STORYSCENEEDITOR_ADDCHOICE, wxT( "Choice" ) );
	}
	contextMenu.AppendSubMenu( subMenu, wxT("Add element") );

	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditorScreenplayPanel::OnAddSection, m_storySceneEditor, wxID_STORYSCENEEDITOR_ADDSECTION );

	PopupMenu( &contextMenu );
}

void CEdSceneSectionPanel::OnSceneElementAdded( CStorySceneElement* element, CStorySceneSection* section )
{
	if ( section->IsA< CStorySceneCutsceneSection >() && element->IsA< CStorySceneCutscenePlayer >() )
	{
		ASSERT( element->IsA< CStorySceneCutscenePlayer >() );
		m_storySceneEditor->PreChangeSceneElements();

		if ( element->GetElementID().Empty() == true )
		{
			element->GenerateElementID();
		}

		CreateAndAddStorySceneElementPanel( element );

		m_storySceneEditor->PostChangeSceneElements( this );
	}
}

void CEdSceneSectionPanel::OnAddComment( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CStorySceneElement* newElement = AddComment( 0, false );
		CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, 0 );
		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnAddQuestChoice( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_hasQuestChoiceLine == false )
		{
			CStorySceneElement* newElement = AddQuestChoice();
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, 0 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnAddDialogLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CStorySceneElement* newElement = AddDialogLine( 0, false );
		CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, 0 );
		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnAddScriptLine( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CStorySceneElement* newElement = AddScriptLine( 0, false );
		CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, 0 );
		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnAddDialogChoice( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		if ( m_hasChoice == false )
		{
			CStorySceneElement* newElement = AddChoice();
			CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, m_dialogElements.Size()-1 );
			CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnRemoveSection( wxCommandEvent& event )
{
	// NOTE: undo is handled by the graph that listens to the remove event
	m_storySceneEditor->RemoveSection( m_dialogSection );
}

void CEdSceneSectionPanel::OnSectionNameEnter( wxCommandEvent& event )
{
	if ( m_dialogElements.Empty() == true )
	{
		CAbstractStorySceneLine* newElement = AddDialogLine();
		CUndoDialogElementExistance::PrepareCreationStep( *m_undoManager, this, newElement, 0 );
		CUndoDialogElementExistance::FinalizeStep( *m_undoManager );
	}
	m_dialogElements[ 0 ]->SetFocus();
}

void CEdSceneSectionPanel::OnNameFieldLostFocus( wxFocusEvent& event )
{
	for ( TDynArray< wxControl* >::iterator iter = m_controlsToThis.Begin();
		iter != m_controlsToThis.End(); ++iter )
	{
		(*iter)->SetLabel( GetSectionName() );
	}
	m_storySceneEditor->UpdateAvailableSections();

	CUndoDialogTextChange::FinalizeStep( *m_undoManager );

	event.Skip();
}

void CEdSceneSectionPanel::OnSectionFocus( wxFocusEvent& event )
{
	m_storySceneEditor->OnSectionPanelSectionFocus( GetSection() );
	event.Skip();
}

void CEdSceneSectionPanel::OnSectionChildFocus( wxChildFocusEvent& event )
{
	ASSERT( GetSection() == m_dialogSection );
	m_storySceneEditor->OnSectionPanelSectionChildFocus( GetSection() );
}

void CEdSceneSectionPanel::OnCharPressed( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_INSERT && event.ControlDown() && event.AltDown() == false )
	{
		wxCommandEvent commandEvent( wxEVT_COMMAND_MENU_SELECTED );
		commandEvent.SetId( wxID_STORYSCENEEDITOR_ADDLINE );
		AddPendingEvent( commandEvent );
	}
	else if ( event.GetKeyCode() == WXK_RETURN )
	{
		wxWindow* eventSource = static_cast<wxWindow*>( event.GetEventObject() );
		eventSource->Navigate();
		event.Skip();
	}
	else if ( event.GetKeyCode() == WXK_PAGEDOWN && event.GetModifiers() == wxMOD_ALT )
	{
		m_storySceneEditor->ShiftSectionPanel( this, true );
	}
	else if ( event.GetKeyCode() == WXK_PAGEUP && event.GetModifiers() == wxMOD_ALT )
	{
		m_storySceneEditor->ShiftSectionPanel( this, false );
	}
	else
	{
		CUndoDialogTextChange::PrepareNameStep( *m_undoManager, GetStorySceneEditor(), m_dialogSection, event.GetKeyCode() );
		event.Skip();
	}
}

void CEdSceneSectionPanel::OnSectionNamePaste( wxClipboardTextEvent& event )
{
	CUndoDialogTextChange::PrepareNameStep( *m_undoManager, GetStorySceneEditor(), m_dialogSection );
	event.Skip();
}

void CEdSceneSectionPanel::OnDelegatedMouseScroll( wxMouseEvent& event )
{
	if ( event.GetEventType() == wxEVT_MOUSEWHEEL )
	{
		event.ResumePropagation( wxEVENT_PROPAGATE_MAX );
	}
	event.Skip();
}

void CEdSceneSectionPanel::OnNameChanged( wxCommandEvent& event )
{
	if ( m_dialogSection != NULL )
	{
		m_dialogSection->SetName( m_sectionNameField->GetValue().wc_str() );
	}
}

void CEdSceneSectionPanel::OnChildFocus( wxChildFocusEvent& event )
{
	wxPanel::OnChildFocus( event );
}

void CEdSceneSectionPanel::OnCollapseClicked( wxCommandEvent& event )
{
	CollapseSection();

}

void CEdSceneSectionPanel::OnExpandClicked( wxCommandEvent& event )
{
	ExpandSection();

}

void CEdSceneSectionPanel::OnCopyElements( wxCommandEvent& event )
{
	CopyElements( true );
}

void CEdSceneSectionPanel::OnCutElements( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CutElements();
		GetStorySceneEditor()->GetSceneEditor()->OnScreenplayPanel_RequestRebuildImmediate();
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnPasteElements( wxCommandEvent& event )
{
	Bool privilegeEditDialogs = GetStorySceneEditor()->GetSceneEditor()->GetRedUserPrivileges().m_editDialogs;
	if( privilegeEditDialogs )
	{
		CEdStorySceneElementPanel* eventSourcePanel = wxStaticCast( event.GetEventObject(), CEdStorySceneElementPanel );
		Int32 pasteLocationIndex = FindDialogElementIndex( eventSourcePanel );

		PasteElementsAtLocation( pasteLocationIndex );
	}
	else
	{
		GFeedback->ShowMsg( TXT( "Scene Editor" ), TXT( "Requested operation requires edit_dialogs privilege." ) );
	}
}

void CEdSceneSectionPanel::OnShiftSectionPanelDown( wxCommandEvent& event )
{
	if ( m_storySceneEditor->ShiftSectionPanel( this, true ) )
	{
		CUndoDialogSectionMove::CreateDownStep( *m_undoManager, m_storySceneEditor, this );
	}
}

void CEdSceneSectionPanel::OnShiftSectionPanelUp( wxCommandEvent& event )
{
	if ( m_storySceneEditor->ShiftSectionPanel( this, false ) )
	{
		CUndoDialogSectionMove::CreateUpStep( *m_undoManager, m_storySceneEditor, this );
	}
}

void CEdSceneSectionPanel::OnNavigationRequest( wxNavigationKeyEvent& event )
{
	if ( event.GetEventObject()->IsKindOf( CLASSINFO( wxTextCtrl ) ) == false )
	{
		event.Skip();
		return;
	}

	wxWindow* eventSource = wxStaticCast( event.GetEventObject(), wxWindow );

	if ( event.GetDirection() == false )
	{
		wxWindow* prevWindow = GetPrevNavigableWindow( eventSource );
		if ( prevWindow != NULL )
		{
			prevWindow->SetFocus();
		}
		else
		{
			CEdSceneSectionPanel* sectionPanelBeforeThis 
				= m_storySceneEditor->GetSectionPanelBefore( this );
			if ( sectionPanelBeforeThis != NULL )
			{
				sectionPanelBeforeThis->GetLastNavigableWindow()->SetFocus();
			}
		}
	}
	else
	{
		wxWindow* nextWindow = GetNextNavigableWindow( eventSource );
		if ( nextWindow != NULL )
		{
			nextWindow->SetFocus();
		}
		else
		{
			CEdSceneSectionPanel* sectionPanelAfterThis 
				= m_storySceneEditor->GetSectionPanelAfter( this );
			if ( sectionPanelAfterThis != NULL )
			{
				sectionPanelAfterThis->GetFirstNavigableWindow()->SetFocus();
			}
		}
	}
}

void CEdSceneSectionPanel::OnDoubleClick( wxMouseEvent& event )
{
	LeaveElementSelectionMode();

	m_storySceneEditor->HandleSectionSelection( this );

	event.Skip();
}

void CEdSceneSectionPanel::OnCopySection( wxCommandEvent& event )
{
	wxMouseEvent mouseEvent;
	OnDoubleClick( mouseEvent );

	m_storySceneEditor->OnEditCopy( event );
}

void CEdSceneSectionPanel::OnCutSection( wxCommandEvent& event )
{
	wxMouseEvent mouseEvent;
	OnDoubleClick( mouseEvent );

	m_storySceneEditor->OnEditCut( event );
}

void CEdSceneSectionPanel::OnPasteSection( wxCommandEvent& event )
{	
	wxMouseEvent mouseEvent;
	OnDoubleClick( mouseEvent );

	m_storySceneEditor->OnEditPaste( event );
}

void CEdSceneSectionPanel::MarkDebugElement( const CStorySceneElement* element )
{
	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		if ( m_dialogElements[ i ]->GetDialogElement() == element )
		{
			m_dialogElements[ i ]->SetBackgroundColour( wxColour( 255, 255, 0 ) );

			m_storySceneEditor->ScrollDialogTextToShowElement( m_dialogElements[ i ] );
		}
		else
		{
			m_dialogElements[ i ]->SetBackgroundColour( wxColour( 255, 255, 255 ) );
		}
	}
	Refresh();
}

void CEdSceneSectionPanel::GetWordCount( Uint32& contentWords, Uint32& commentWords ) const
{
	contentWords = 0;
	commentWords = 0;

	for ( Uint32 i = 0; i < m_dialogElements.Size(); ++i )
	{
		Uint32 elementTotalWords = 0;
		Uint32 elementCommentWords = 0;
		m_dialogElements[ i ]->GetWordCount( elementTotalWords, elementCommentWords );
		contentWords += elementTotalWords;
		commentWords += elementCommentWords;
	}
}

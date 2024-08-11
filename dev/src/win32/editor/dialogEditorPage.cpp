#include "build.h"
#include "dialogEditorPage.h"

#include "assetBrowser.h"
#include "dialogEditorSection.h"
#include "dialogEditorChoiceLine.h"
#include "dialogGraphEditor.h"
#include "gridEditor.h"

#include "dialogEditorActions.h"
#include "dialogTimeline.h"

#include "dialogPreview.h"

#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneChoiceLine.h"
#include "..\..\common\engine\localizedContent.h"
#include "../../common/game/storySceneAbstractLine.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneGraphBlock.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneGraph.h"
#include "..\..\common\engine\localizationManager.h"
#include "../../common/game/actor.h"
#include "../../common/core/feedback.h"

#include "gridCustomColumns.h"
#include "gridCustomTypes.h"
#include "../../common/core/diskFile.h"

//CGatheredResource resVoicetagTable( TXT( "globals\\scenes\\scene_voice_tags.csv" ), RGF_NotCooked );

//RED_DEFINE_STATIC_NAME( FileReloadToConfirm )
//RED_DEFINE_STATIC_NAME( FileReloadConfirm )
//RED_DEFINE_STATIC_NAME( FileReload )

BEGIN_EVENT_TABLE( CEdSceneEditorScreenplayPanel, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID( "NextSectionButton" ), CEdSceneEditorScreenplayPanel::OnNextSection )
	EVT_BUTTON( XRCID( "PrevSectionButton" ), CEdSceneEditorScreenplayPanel::OnPrevSection )
	EVT_BUTTON( XRCID( "AddSectionButton" ), CEdSceneEditorScreenplayPanel::OnAddSection )
	EVT_CHOICE( XRCID( "CurrentSectionChoice" ), CEdSceneEditorScreenplayPanel::OnChangeSection )
	EVT_MENU( wxID_STORYSCENEEDITOR_ADDSECTION, CEdSceneEditorScreenplayPanel::OnAddSection )
	EVT_MENU( wxID_STORYSCENEEDITOR_REFRESHDIALOG, CEdSceneEditorScreenplayPanel::OnRefreshDialog )
	EVT_BUTTON( XRCID( "ResetStringDB" ), CEdSceneEditorScreenplayPanel::OnResetStringDbIds )
//	EVT_TOGGLEBUTTON( XRCID( "DebugButton" ), CEdSceneEditorScreenplayPanel::OnToggleDebug )
//	EVT_MENU( XRCID( "playButton" ), CEdSceneEditorScreenplayPanel::OnPlayPause )
//	EVT_MENU( XRCID( "resetButton"), CEdSceneEditorScreenplayPanel::OnReset )
	EVT_TIMER( XRCID( "wordTimer" ), CEdSceneEditorScreenplayPanel::OnWordCountTimer )
END_EVENT_TABLE()

IMPLEMENT_CLASS( CEdSceneEditorScreenplayPanel, wxPanel );

CEdSceneEditorScreenplayPanel::CEdSceneEditorScreenplayPanel( wxWindow* parent, CEdSceneEditor* sceneEditor ) 
	: m_currentSectionPanel( NULL )
	, m_changeElementsCounter( 0 )
	, m_isInitialized( false )
	, m_sectionPanelHandlingElementSelection( NULL )
	, m_shouldIgnoreLinkUpdateOnElementDelete( false )
	, m_fontSize( 0 )
	, m_showOnlyScriptTexts( false )
	//, m_sceneDebugEnabled( false )
	, m_isRefreshInProgress( false )
	, m_wordCountTimer( NULL )
	, m_sceneEditor( sceneEditor )
	, m_mediator( sceneEditor )
{
	wxXmlResource::Get()->LoadPanel( this, parent, TEXT( "DialogScreenplayPanel" ) );

	m_statusLabel = XRCCTRL( *this, "StatusBar", wxStaticText );
	m_dialogTextPanel = XRCCTRL( *this, "DialogTextPanel", wxScrolledWindow );
	//wxPanel* propertiesPanel = XRCCTRL( *this, "PropertiesPanel", wxPanel );



	m_handlerFactory.SetDialogEditor( this );

	m_dialogTextPanel->SetAutoLayout( true );
	m_dialogTextPanel->SetScrollRate( 5, 15 );
	m_dialogTextPanel->Connect( wxEVT_RIGHT_UP, 
		wxMouseEventHandler( CEdSceneEditorScreenplayPanel::OnTextPanelContextMenu ), NULL, this );
	m_dialogTextPanel->Connect( wxEVT_LEFT_DOWN, 
		wxMouseEventHandler( CEdSceneEditorScreenplayPanel::OnTextPanelClick ), NULL, this );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionRemoved ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionAdded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionElementAdded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSectionLinksChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurrentLocaleChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( StorySceneDebugRefresh ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneSettingChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

	SetMinSize( wxSize( 800, 600 ) );
	SetMaxSize( wxSize( 1200, 900 ) );

	Fit();
	Layout();
	
	Refresh();

	m_wordCountTimer = new CEdTimer( this, XRCID( "wordTimer" ) );
	m_wordCountTimer->Stop();
	//SetSceneDefinition( scene );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdSceneEditorScreenplayPanel::OnBtnApplyScreenplayChanges, this, XRCID("btnApplyScreenplayChanges") );
}

CEdSceneEditorScreenplayPanel::~CEdSceneEditorScreenplayPanel(void)
{
	m_wordCountTimer->Stop();

	m_shouldIgnoreLinkUpdateOnElementDelete = true;

	// Delete sections
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		delete m_sectionPanels[ i ];
	}
	m_sectionPanels.Clear();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdSceneEditorScreenplayPanel::CommitChanges()
{
	for( CEdSceneSectionPanel* sectionPanel :  m_sectionPanels )
	{
		sectionPanel->CommitChanges();
	}
}

void CEdSceneEditorScreenplayPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == HACK_GetStoryScene() )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetName().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );

		if ( reloadInfo.m_newResource->IsA<CStoryScene>() )
		{
			CStoryScene* oldScene = (CStoryScene*)(reloadInfo.m_oldResource);
			CStoryScene* newScene = (CStoryScene*)(reloadInfo.m_newResource);

			ASSERT(newScene != HACK_GetStoryScene());

			if (oldScene == HACK_GetStoryScene())
			{
				ReloadScreenplay();
				RefreshDialog();

				wxTheFrame->GetAssetBrowser()->OnEditorReload(newScene, this);
			}
		}
	}
	else if ( name == CNAME( SceneSectionRemoved ) )
	{
		CStorySceneSection* sceneSection = GetEventData< CStorySceneSection* >( data );
		if ( sceneSection->GetScene() == HACK_GetStoryScene() )
		{
			CEdSceneSectionPanel* sectionPanel = FindSectionPanel( sceneSection );
			if ( sectionPanel != NULL )
			{
				RemoveSectionPanel( sectionPanel );
			}
		}
		
	}
	else if ( name == CNAME( SceneSectionAdded ) )
	{
		CStorySceneSection* sceneSection = GetEventData< CStorySceneSection* >( data );
		if ( sceneSection->GetScene() == HACK_GetStoryScene() )
		{
			AddSectionPanel( sceneSection );
		}
	}
	else if ( name == CNAME( SceneSectionLinksChanged ) )
	{
		CStorySceneSection* sceneSection = GetEventData< CStorySceneSection* >( data );
		if ( sceneSection->GetScene() == HACK_GetStoryScene() )
		{
			CEdSceneSectionPanel* sectionPanel = FindSectionPanel( sceneSection );
			if ( sectionPanel != NULL )
			{
				sectionPanel->RefreshConnectedSections();
			}
		}
	}
	else if ( name == CNAME( SceneSectionElementAdded ) )
	{
		const SStorySceneElementInfo& elementInfo = GetEventData< SStorySceneElementInfo >( data );
		CStorySceneSection* sceneSection = elementInfo.m_section;
		if ( sceneSection->GetScene() == HACK_GetStoryScene() )
		{
			CEdSceneSectionPanel* sectionPanel = FindSectionPanel( sceneSection );
			if ( sectionPanel != NULL )
			{
				sectionPanel->OnSceneElementAdded( elementInfo.m_element, sceneSection );
				if ( sectionPanel == m_currentSectionPanel && HACK_GetPreview() != NULL )
				{
					//GetPreview()->SetupSectionPreview( sceneSection );
				}
			}
		}
	}
	else if ( name == CNAME( CurrentLocaleChanged ) )
	{
		RefreshDialog();
	}
	else if ( name == CNAME( SceneSettingChanged ) )
	{
		CStoryScene* scene = GetEventData< CStoryScene* >( data );
		if ( scene == HACK_GetStoryScene() )
		{
			//GetPreview()->RefreshPositions();
		}
	}
	else if ( name == CNAME( EditorPropertyPostChange ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		CObject* changedObject = propertyData.m_typedObject.AsObject();
		if ( changedObject != NULL )
		{
			CStoryScene* changedObjectParentScene = changedObject->FindParent< CStoryScene >();
			if ( changedObjectParentScene != NULL && changedObjectParentScene == HACK_GetStoryScene() )
			{
				RunLaterOnce( [ this, changedObject ]() {
					RefreshDialogObject( changedObject );
				} );
			}
		}
		
	}

}

void CEdSceneEditorScreenplayPanel::AddSectionPanel( CStorySceneSection* section )
{
	RED_FATAL_ASSERT( section, "CEdSceneEditorScreenplayPanel::AddSectionPanel(): section must not be nullptr." );

	PreChangeSceneElements();

	CEdSceneSectionPanel* sectionPanel = new CEdSceneSectionPanel( m_dialogTextPanel, this, section, m_undoManager );
	Int32 index = HACK_GetStoryScene()->GetSectionIndex( section );
	ASSERT ( index != -1 );
	m_dialogTextPanel->GetSizer()->Insert( index, sectionPanel, 0, wxEXPAND | wxALL, 5 );

	m_sectionPanels.PushBack( sectionPanel );

	PostChangeSceneElements( m_dialogTextPanel );

	RefreshDialog();

	UpdateAvailableSections();
	sectionPanel->SetFocus();
}

void CEdSceneEditorScreenplayPanel::RemoveSectionPanel( CEdSceneSectionPanel* sectionPanel )
{
	ASSERT( sectionPanel != NULL );
	PreChangeSceneElements();

	m_sectionPanels.Remove( sectionPanel );
	if ( sectionPanel == m_currentSectionPanel )
	{
		m_currentSectionPanel = NULL;
	}

	m_selectedSections.Erase( sectionPanel );

	sectionPanel->ClearPreLinks();

	delete sectionPanel;
	PostChangeSceneElements( m_dialogTextPanel );
	UpdateAvailableSections();
}

void CEdSceneEditorScreenplayPanel::SetCurrentSection( CStorySceneSection* section )
{
	CEdSceneSectionPanel* sectionPanel = FindSectionPanel( section );
	if ( sectionPanel == NULL )
	{
		return;
	}

	m_currentSectionPanel = sectionPanel;
}

CEdSceneSectionPanel* CEdSceneEditorScreenplayPanel::GetSectionPanel( Uint32 index )
{
	if ( index < 0 || index >= m_sectionPanels.Size() )
	{
		return NULL;
	}
	return m_sectionPanels[ index ];
}

Int32 CEdSceneEditorScreenplayPanel::GetSectionPanelIndex( CEdSceneSectionPanel* sectionPanel )
{
	return m_sectionPanels.GetIndex( sectionPanel );
}

Bool CEdSceneEditorScreenplayPanel::ShiftSectionPanel( CEdSceneSectionPanel* sectionPanel, Bool isMovingDown )
{
	Uint32 sectionPanelIndex = m_sectionPanels.GetIndex( sectionPanel );
	Uint32 otherSectionPanelIndex 
		= ( isMovingDown == true ? sectionPanelIndex + 1 : sectionPanelIndex - 1 );
	
	return MoveSectionPanel( sectionPanel, otherSectionPanelIndex );
}

Bool CEdSceneEditorScreenplayPanel::MoveSectionPanel( CEdSceneSectionPanel* sectionPanel, Uint32 newSectionPanelIndex )
{
	Uint32 sectionPanelIndex = m_sectionPanels.GetIndex( sectionPanel );

	if ( sectionPanelIndex >= m_sectionPanels.Size() || newSectionPanelIndex >= m_sectionPanels.Size() )
	{
		return false;
	}

	CEdSceneSectionPanel* otherSectionPanel = m_sectionPanels[ newSectionPanelIndex ];

	PreChangeSceneElements();
	m_dialogTextPanel->GetSizer()->Detach( sectionPanel );
	m_dialogTextPanel->GetSizer()->Insert( 
		newSectionPanelIndex, sectionPanel, 0, wxEXPAND | wxALL, 5 );

	PostChangeSceneElements( m_dialogTextPanel );

	m_sectionPanels.Swap( sectionPanelIndex, newSectionPanelIndex );

	sectionPanel->SetFocus();

	return true;
}

Bool CEdSceneEditorScreenplayPanel::MoveSectionPanel( const CStorySceneSection* section, Uint32 newSectionPanelIndex )
{
	CEdSceneSectionPanel* sectionPanel = FindSectionPanel( section );
	if ( sectionPanel )
	{
		return MoveSectionPanel( sectionPanel, newSectionPanelIndex );
	}
	else
	{
		return false;
	}
}

void CEdSceneEditorScreenplayPanel::UpdateAvailableSections()
{
	wxArrayString sectionNames;
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		sectionNames.Add( m_sectionPanels[ i ]->GetSectionName() );
	}

	//wxChoice* sectionChoice = XRCCTRL( *this, "CurrentSectionChoice", wxChoice );
	//sectionChoice->Clear();
	//sectionChoice->Append( sectionNames );
}

void CEdSceneEditorScreenplayPanel::RegisterChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine )
{
	m_choiceLines.PushBack( choiceLine );

	UpdateAvailableSections();
}

void CEdSceneEditorScreenplayPanel::UnregisterChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine )
{
	m_choiceLines.Remove( choiceLine );
}

void CEdSceneEditorScreenplayPanel::ReloadScreenplay()
{
	m_isRefreshInProgress = true;
	
	PreChangeSceneElements();

	m_currentSectionPanel = NULL;
	while( m_sectionPanels.Empty() == false )
	{
		delete m_sectionPanels.PopBack();
	}

	HACK_GetSceneGraphEditor()->SetGraph( HACK_GetStoryScene()->GetGraph() );

	for ( Uint32 i = 0; i < HACK_GetStoryScene()->GetNumberOfSections(); ++i )
	{
		CEdSceneSectionPanel* section 
			= new CEdSceneSectionPanel( m_dialogTextPanel, this, HACK_GetStoryScene()->GetSection( i ), m_undoManager );

		m_dialogTextPanel->GetSizer()->Add( section, 0, wxEXPAND | wxALL, 5 );

		AnalizeSection( HACK_GetStoryScene()->GetSection( i ) );

		m_sectionPanels.PushBack( section );
	}

	UpdateAvailableSections();

	for ( Uint32 j = 0; j < m_choiceLines.Size(); ++j )
	{
		m_choiceLines[ j ]->RestoreSectionLink();
	}
	for ( TDynArray< CEdSceneSectionPanel* >::iterator iter = m_sectionPanels.Begin();
		iter != m_sectionPanels.End(); ++iter )
	{
		(*iter)->RefreshConnectedSections();
	}

	//RefreshDialog();
	PostChangeSceneElements();
	m_isRefreshInProgress = false;
}

void CEdSceneEditorScreenplayPanel::AnalizeSection( CStorySceneSection* section )
{
	if ( section->IsA< CStorySceneCutsceneSection >() )
	{
		CStorySceneCutsceneSection* csSection = Cast< CStorySceneCutsceneSection >( section );
		CCutsceneTemplate* csTempl = csSection->GetCsTemplate();

		if ( section->GetScene() && section->GetScene()->GetFile() && csTempl && csTempl->GetFile() )
		{
			const String scenePath = section->GetScene()->GetFile()->GetDepotPath();
			if ( !csTempl->IsFileUsedThisCutscene( scenePath ) ) 
			{
				GFeedback->ShowMsg( TXT("Message"), TXT("Cutscene [%s] is used by this scene. Checkout cutscene file to save this information inside cutscene resource."), csTempl->GetFile()->GetFileName().AsChar() );
				if ( csTempl->SaveFileWithThisCutscene( scenePath ) )
				{
					csTempl->Save();
				}
			}
		}
	}
}

void CEdSceneEditorScreenplayPanel::RefreshDialog()
{
	if ( m_isRefreshInProgress == true )
	{
		return;
	}

	m_isRefreshInProgress = true;
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		m_sectionPanels[ i ]->RefreshData();
	}
	m_isInitialized = true;
	
	m_isRefreshInProgress = false;
	RefreshWordCount();
}

Bool CEdSceneEditorScreenplayPanel::RefreshDialogObject( CObject* objectToRefresh )
{
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		if ( m_sectionPanels[ i ]->RefreshDialogObject( objectToRefresh ) == true )
		{
			return true;
		}
	}
	return false;
}

void CEdSceneEditorScreenplayPanel::RefreshHelperData()
{
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		m_sectionPanels[ i ]->RefreshHelperData();
	}
}

void CEdSceneEditorScreenplayPanel::RefreshWordCount()
{
	if ( m_isRefreshInProgress == true )
	{
		return;
	}

	if ( m_wordCountTimer != NULL )
	{
		m_wordCountTimer->Start( 500, true );
	}

	/*Uint32 contentWords = 0;
	Uint32 commentWords = 0;

	for ( Uint32 i = 0; i < m_sections.Size(); ++i )
	{
	CEdSceneSectionPanel* sectionPanel = m_sections[ i ];

	Uint32 sectionContentWords = 0;
	Uint32 sectionCommentWords = 0;

	sectionPanel->GetWordCount( sectionContentWords, sectionCommentWords );

	contentWords += sectionContentWords;
	commentWords += sectionCommentWords;
	}

	String statusString = String::Printf( TXT( "Words in scene: %d (%d in content, %d in comments)" ), 
	contentWords + commentWords, contentWords, commentWords );

	m_sceneScriptEditor->SetStatus( statusString );*/
}

void CEdSceneEditorScreenplayPanel::ReinitializeSceneScript()
{
	m_isInitialized = false;
	RefreshDialog();
	m_isInitialized = true;
}

void CEdSceneEditorScreenplayPanel::PreChangeSceneElements()
{
	if ( m_changeElementsCounter == 0 )
	{
		Freeze();
	}
	m_changeElementsCounter += 1;
}

void CEdSceneEditorScreenplayPanel::PostChangeSceneElements( wxWindow* windowToRefreshLayout /*= NULL */ )
{
	if ( m_changeElementsCounter == 1 )
	{
		Int32 scrollPos = m_dialogTextPanel->GetScrollPos( wxVERTICAL );

		if ( windowToRefreshLayout == NULL )
		{
			m_dialogTextPanel->GetSizer()->Fit( m_dialogTextPanel );
			m_dialogTextPanel->GetParent()->Layout();
		}
		else
		{
			windowToRefreshLayout->GetParent()->Layout();

			wxSize newSize = m_dialogTextPanel->GetBestVirtualSize();
			m_dialogTextPanel->SetVirtualSize( newSize );
		}

		m_dialogTextPanel->Scroll( -1, scrollPos );

		HACK_GetSceneGraphEditor()->Repaint();

		Thaw();
	}
	
	m_changeElementsCounter -= 1;

	m_mediator->OnScreenplayPanel_PostChangeSceneElements();
}

void CEdSceneEditorScreenplayPanel::UpdateTimeline()
{
	//GetPreview()->SetupSectionPreview( ( m_currentSectionPanel != NULL ) ? m_currentSectionPanel->GetSection() : NULL );
}

void CEdSceneEditorScreenplayPanel::ScrollDialogTextToShowElement( wxWindow* dialogTextElement )
{
	wxScrolledWindow* scrollWindow = GetDialogTextPanel();

	



	int stepx, stepy;
	int startx, starty;

	wxSize view( scrollWindow->GetClientSize() );
	scrollWindow->GetScrollPixelsPerUnit(&stepx, &stepy);
	scrollWindow->GetViewStart( &startx, &starty );

	wxRect eventSourceWindowRect( 
		scrollWindow->ScreenToClient( dialogTextElement->GetScreenPosition() ), 
		dialogTextElement->GetSize() );

	Bool shouldScroll = false;

	int diff = 0;

	if ( eventSourceWindowRect.GetTop() < 0 )
	{
		diff = eventSourceWindowRect.GetTop();
		shouldScroll = true;
	}
	else if ( eventSourceWindowRect.GetBottom() > view.y )
	{
		diff = eventSourceWindowRect.GetBottom() - view.y + 1;
		diff += stepy - 1;
		shouldScroll = true;
	}

	if ( shouldScroll == true && stepy > 0 )
	{
		starty = (starty * stepy + diff) / stepy;
		scrollWindow->Freeze();
		scrollWindow->Scroll( startx, starty );
		scrollWindow->Thaw();
	}

	
}

void CEdSceneEditorScreenplayPanel::FocusOnFirstSection()
{
	if ( m_sectionPanels.Empty() == false )
	{
		m_sectionPanels[ 0 ]->SetFocus();
	}
}

CEdSceneSectionPanel* CEdSceneEditorScreenplayPanel::FindSectionPanel( const CStorySceneSection* section )
{
	for ( TDynArray< CEdSceneSectionPanel* >::iterator iter = m_sectionPanels.Begin();
		iter != m_sectionPanels.End(); ++iter )
	{
		if ( (*iter)->GetSection() == section )
		{
			return *iter;
		}
	}
	return NULL;
}

void CEdSceneEditorScreenplayPanel::HandleSectionSelection( CEdSceneSectionPanel* sectionPanel )
{
	if ( wxIsCtrlDown() == false )
	{
		ClearSectionSelection();
	}

	Bool isSectionSelected = ( m_selectedSections.Find( sectionPanel ) != m_selectedSections.End() );

	if ( sectionPanel == NULL )
	{
		return;
	}

	if ( isSectionSelected == false )
	{
		m_selectedSections.Insert( sectionPanel );
		sectionPanel->MarkSectionAsSelected();
		HACK_GetSceneGraphEditor()->SelectSectionBlock( sectionPanel->GetSection(), true );
	}
	else
	{
		m_selectedSections.Erase( sectionPanel );
		sectionPanel->UnmarkSectionAsSelected();
		HACK_GetSceneGraphEditor()->SelectSectionBlock( sectionPanel->GetSection(), false );
	}
	sectionPanel->Refresh();
}

void CEdSceneEditorScreenplayPanel::ClearSectionSelection()
{
	for ( THashSet< CEdSceneSectionPanel* >::iterator iter = m_selectedSections.Begin();
		iter != m_selectedSections.End(); ++iter )
	{
		( *iter )->UnmarkSectionAsSelected();
		( *iter )->Refresh();
	}
	m_selectedSections.Clear();
}
void CEdSceneEditorScreenplayPanel::SetSectionPanelHandlingElementSelection( CEdSceneSectionPanel* sectionPanel )
{
	if ( m_sectionPanelHandlingElementSelection != NULL 
		&& m_sectionPanelHandlingElementSelection != sectionPanel )
	{
		m_sectionPanelHandlingElementSelection->ClearSelectedElements();
	}
	m_sectionPanelHandlingElementSelection = sectionPanel;
}

Bool CEdSceneEditorScreenplayPanel::IsFirstSectionPanel( CEdSceneSectionPanel* sectionPanel )
{
	return m_sectionPanels[ 0 ] == sectionPanel;
}

Bool CEdSceneEditorScreenplayPanel::IsLastSectionPanel( CEdSceneSectionPanel* sectionPanel )
{
	return m_sectionPanels[ m_sectionPanels.Size() - 1 ] == sectionPanel;
}

void CEdSceneEditorScreenplayPanel::ChangeWindowFontSize( wxWindow* window, Int32 sizeChange )
{
	if ( sizeChange == 0 )
	{
		return;
	}
	
	wxColour windowColor = window->GetForegroundColour();
	
	wxFont windowFont = window->GetFont();
	windowFont.SetPointSize( windowFont.GetPointSize() + sizeChange );
	
	window->SetFont( windowFont );

	window->SetOwnForegroundColour( *wxWHITE );
	window->SetOwnForegroundColour( windowColor );
	
	window->Update();
}

void CEdSceneEditorScreenplayPanel::ChangeFontSize( Int32 sizeChange )
{
	PreChangeSceneElements();
	for ( TDynArray< CEdSceneSectionPanel* >::iterator iter = m_sectionPanels.Begin();
		iter != m_sectionPanels.End(); ++iter )
	{
		(*iter)->ChangeFontSize( sizeChange );
	}

	m_fontSize += sizeChange;

	ReinitializeSceneScript();
	PostChangeSceneElements( m_dialogTextPanel );
}

void CEdSceneEditorScreenplayPanel::OnTextPanelContextMenu( wxMouseEvent& event )
{
	wxMenu contextMenu;

	contextMenu.Append( wxID_STORYSCENEEDITOR_ADDSECTION, TXT( "Add section" ) );
	contextMenu.AppendSeparator();

	contextMenu.Append( wxID_STORYSCENEEDITOR_PASTESECTIONS, TXT( "Paste sections" ) );

	if ( wxTheClipboard->IsSupported( TXT( "StorySceneSections" ) ) == false )
	{
		contextMenu.Enable( wxID_STORYSCENEEDITOR_PASTESECTIONS, false );
	}

	PopupMenu( &contextMenu );
}

void CEdSceneEditorScreenplayPanel::OnAddSection( wxCommandEvent& event )
{
	HACK_GetSceneGraphEditor()->OnAddSection( event );
}

void CEdSceneEditorScreenplayPanel::OnAddCutscene( wxCommandEvent& event )
{
	HACK_GetSceneGraphEditor()->OnAddCutscene( event );
}

void CEdSceneEditorScreenplayPanel::OnChangeSection( wxCommandEvent& event )
{
	Int32 sectionIndex = event.GetSelection();
	m_sectionPanels[ sectionIndex ]->SetFocus();

	//GetPreview()->SetupSectionPreview( m_sectionPanels[ sectionIndex ]->GetSection() );

	HACK_GetSceneGraphEditor()->CenterOnSectionBlock( m_sectionPanels[ sectionIndex ]->GetSection() );
}

void CEdSceneEditorScreenplayPanel::OnNextSection( wxCommandEvent& event )
{
	if ( m_currentSectionPanel == NULL )
	{
		return;
	}

	Uint32 sectionIndex = m_sectionPanels.GetIndex( m_currentSectionPanel );
	if ( sectionIndex == -1 )
	{
		m_currentSectionPanel->SetFocus();
		return;
	}

	if ( sectionIndex < m_sectionPanels.Size() - 1 )
	{
		m_sectionPanels[ sectionIndex + 1 ]->SetFocus();
	}
	else
	{
		m_currentSectionPanel->SetFocus();
	}
}

void CEdSceneEditorScreenplayPanel::OnPrevSection( wxCommandEvent& event )
{
	if ( m_currentSectionPanel == NULL )
	{
		return;
	}

	Uint32 sectionIndex = m_sectionPanels.GetIndex( m_currentSectionPanel );
	if ( sectionIndex == -1 )
	{
		m_currentSectionPanel->SetFocus();
		return;
	}

	if ( sectionIndex > 0 )
	{
		m_sectionPanels[ sectionIndex - 1 ]->SetFocus();
	}
	else
	{
		m_currentSectionPanel->SetFocus();
	}
}

void CEdSceneEditorScreenplayPanel::RemoveSection( CStorySceneSection* section )
{
	m_mediator->OnScreenplayPanel_RemoveSection( section );
}

void CEdSceneEditorScreenplayPanel::OnExit( wxCommandEvent& event )
{
	Close();
}

void CEdSceneEditorScreenplayPanel::OnTextPanelClick( wxMouseEvent& event )
{
	GetSceneEditor()->SetPropertiesBrowserObject( HACK_GetStoryScene(), nullptr );
}

void CEdSceneEditorScreenplayPanel::OnRefreshDialog( wxCommandEvent& event )
{
	RefreshDialog();
}

CEdSceneSectionPanel* CEdSceneEditorScreenplayPanel::GetSectionPanelAfter( CEdSceneSectionPanel* sectionPanel )
{
	Uint32 sectionPanelIndex = m_sectionPanels.GetIndex( sectionPanel );
	if ( sectionPanelIndex == -1 || sectionPanelIndex == m_sectionPanels.Size() - 1 )
	{
		return NULL;
	}
	return m_sectionPanels[ sectionPanelIndex + 1 ];
}

CEdSceneSectionPanel* CEdSceneEditorScreenplayPanel::GetSectionPanelBefore( CEdSceneSectionPanel* sectionPanel )
{
	Uint32 sectionPanelIndex = m_sectionPanels.GetIndex( sectionPanel );
	if ( sectionPanelIndex == -1 || sectionPanelIndex == 0 )
	{
		return NULL;
	}
	return m_sectionPanels[ sectionPanelIndex - 1 ];
}

void CEdSceneEditorScreenplayPanel::OnResetStringDbIds( wxCommandEvent& event )
{
	SLocalizationManager::GetInstance().ResetIDs( HACK_GetStoryScene() );
}

void CEdSceneEditorScreenplayPanel::ToggleShowOnlyScriptTexts()
{
	m_showOnlyScriptTexts = !m_showOnlyScriptTexts;
}

void CEdSceneEditorScreenplayPanel::EnableShowOnlyScriptTexts( Bool enable /* = true */ )
{
	m_showOnlyScriptTexts = enable;
}

Bool CEdSceneEditorScreenplayPanel::IsShowOnlyScriptTextsEnabled() const
{
	return m_showOnlyScriptTexts;
}

void CEdSceneEditorScreenplayPanel::OnEditDelete( wxCommandEvent& event )
{
	DoEditDelete();
}

void CEdSceneEditorScreenplayPanel::DoEditDelete()
{
	if ( m_selectedSections.Empty() == false )
	{
		wxCommandEvent deleteEvent( wxEVT_COMMAND_MENU_SELECTED );
		deleteEvent.SetId( wxID_STORYSCENEEDITOR_DELETESECTION );
		HACK_GetSceneGraphEditor()->GetEventHandler()->ProcessEvent( deleteEvent );
	}
	else if ( m_sectionPanelHandlingElementSelection != NULL )
	{
		m_sectionPanelHandlingElementSelection->RemoveSelectedElements();
	}
}

void CEdSceneEditorScreenplayPanel::OnEditCopy( wxCommandEvent& event )
{
	if ( m_selectedSections.Empty() == false )
	{
		HACK_GetSceneGraphEditor()->OnEditCopy( event );
	}
	else if ( m_sectionPanelHandlingElementSelection != NULL )
	{
		m_sectionPanelHandlingElementSelection->OnCopyElements( event );
	}
}

void CEdSceneEditorScreenplayPanel::OnEditCut( wxCommandEvent& event )
{
	if ( m_selectedSections.Empty() == false )
	{
		HACK_GetSceneGraphEditor()->OnEditCut( event );
	}
	else if ( m_sectionPanelHandlingElementSelection != NULL )
	{
		m_sectionPanelHandlingElementSelection->OnCutElements( event );
	}
}

void CEdSceneEditorScreenplayPanel::OnEditPaste( wxCommandEvent& event )
{
	if ( m_selectedSections.Empty() == false )
	{
		HACK_GetSceneGraphEditor()->OnEditPaste( event );
	}
	else if ( m_sectionPanelHandlingElementSelection != NULL )
	{
		m_sectionPanelHandlingElementSelection->OnPasteElements( event );
	}
}

void CEdSceneEditorScreenplayPanel::OnWordCountTimer( wxTimerEvent& event )
{
	Uint32 contentWords = 0;
	Uint32 commentWords = 0;

	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		CEdSceneSectionPanel* sectionPanel = m_sectionPanels[ i ];

		Uint32 sectionContentWords = 0;
		Uint32 sectionCommentWords = 0;

		sectionPanel->GetWordCount( sectionContentWords, sectionCommentWords );

		contentWords += sectionContentWords;
		commentWords += sectionCommentWords;
	}

	String statusString = String::Printf( TXT( "Words in scene: %d (%d in content, %d in comments)" ), 
		contentWords + commentWords, contentWords, commentWords );

	m_statusLabel->SetLabel( statusString.AsChar() );
}

void CEdSceneEditorScreenplayPanel::OnBtnApplyScreenplayChanges( wxCommandEvent& event )
{
	GetSceneEditor()->OnScreenplayPanel_ApplyChanges();
}

void CEdSceneEditorScreenplayPanel::OnElementPanelFocus( const CStorySceneSection* section, CStorySceneElement* e )
{
	ClearSectionSelection();

	m_mediator->OnScreenplayPanel_ElementPanelFocus( section, e );
}

void CEdSceneEditorScreenplayPanel::OnChoiceLineChildFocus( const CStorySceneSection* section, CStorySceneChoiceLine* line )
{
	m_mediator->OnScreenplayPanel_ChoiceLineChildFocus( section, line );
}

void CEdSceneEditorScreenplayPanel::OnSectionPanelSectionFocus( const CStorySceneSection* section )
{
	m_mediator->OnScreenplayPanel_SectionPanelSectionFocus( section );
}

void CEdSceneEditorScreenplayPanel::OnSectionPanelSectionChildFocus( CStorySceneSection* section )
{
	m_mediator->OnScreenplayPanel_SectionPanelSectionChildFocus( section );

	ClearSectionSelection();
}

void CEdSceneEditorScreenplayPanel::OnChoicePanelClick( CStorySceneChoice* ch )
{
	m_mediator->OnScreenplayPanel_ChoicePanelClick( ch );
}

CName CEdSceneEditorScreenplayPanel::GetPrevSpeakerName( const CStorySceneElement* currElement )
{
	return m_mediator->OnScreenplayPanel_GetPrevSpeakerName( currElement );
}

const CStorySceneLine* CEdSceneEditorScreenplayPanel::GetPrevLine( const CStorySceneElement* currElement )
{
	return m_mediator->OnScreenplayPanel_GetPrevLine( currElement );
}

void CEdSceneEditorScreenplayPanel::OnGenerateSpeakingToData( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_sectionPanels.Size(); ++i )
	{
		m_sectionPanels[i]->GenerateSpeakingToForSection();
	}
	RefreshDialog();
	//GFeedback->ShowMultiBoolDialog()
	//m_mediator->OnScreenplayPanel_GetPrevSpeakerName()
}

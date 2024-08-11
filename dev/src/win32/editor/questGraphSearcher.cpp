#include "build.h"
#include "questGraphSearcher.h"
#include "questEditor.h"
#include "questGraphEditor.h"
#include "rewardEditor.h"
#include "assetBrowser.h"
#include "../../common/game/rewardManager.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPauseConditionBlock.h"
#include "../../common/game/questConditionBlock.h"
#include "../../common/game/questContextDialogBlock.h"
#include "../../common/game/questInteractionDialogBlock.h"
#include "../../common/game/questCondition.h"
#include "../../common/game/questScriptBlock.h"
#include "../../common/game/questSceneBlock.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/questFactsDBChangingBlock.h"
#include "../../common/game/questRewardBlock.h"
#include "../../common/game/questPhaseBlock.h"
#include "../../common/game/journalBlock.h"

#include "../../games/r4/journalQuestBlock.h"
#include "../../games/r4/journalQuestTrackBlock.h"
#include "../../games/r4/journalQuestObjectiveCounterGraphBlock.h"
#include "../../games/r4/journalQuestMappinStateBlock.h"
#include "../../games/r4/journalQuestMonsterKnownGraphBlock.h"

#include "../../common/core/depot.h"
#include "../../common/core/xmlFileWriter.h"
#include "../../common/core/xmlFileReader.h"

///////////////////////////////////////////////////////////////////////////////

void CQuestGraphSearcher::SQuestBlockData::Execute( CQuestGraphSearcher& parent )
{
	parent.OpenQuestBlockInEditor( block );
}

CObject* CQuestGraphSearcher::SQuestBlockData::GetObject() 
{ 
	return block; 
}

void CQuestGraphSearcher::SSpawnsetData::Execute( CQuestGraphSearcher& parent )
{
	parent.EditSpawnset( spawnset );
}

CObject* CQuestGraphSearcher::SSpawnsetData::GetObject() 
{ 
	return spawnset; 
}

///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CQuestGraphSearcher, wxDialog )
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID( "m_notebook15" ), CQuestGraphSearcher::OnPageChanged )	
END_EVENT_TABLE()

CQuestGraphSearcher::CQuestGraphSearcher( CEdQuestGraphEditor& parent )
	: m_parent( parent )
{
	wxXmlResource::Get()->LoadDialog( this, &parent, wxT( "QuestBlocksFinder" ) );
	m_blockTypeChoice = XRCCTRL( *this, "blockTypeChoice", wxChoice );
	m_blockTypeChoice->Clear();

	TDynArray< CClass* > blockClasses;
	CEdQuestEditor::GetEditableBlockClasses( NULL, blockClasses );
	for ( TDynArray< CClass* >::const_iterator it = blockClasses.Begin(); it != blockClasses.End(); ++it )
	{
		if ( CQuestGraphBlock* defaultBlock = (*it)->GetDefaultObject< CQuestGraphBlock >() )
		{
			m_blockTypeChoice->AppendString( defaultBlock->GetBlockName().AsChar() );
			m_blockClasses.Insert( defaultBlock->GetBlockName().AsChar(), *it );
		}
	}

	m_conditionTypeChoice = XRCCTRL( *this, "conditionTypeChoice", wxChoice );
	m_conditionTypeChoice->Clear();
	{

		struct Pred
		{
			mutable CName str1;
			mutable CName str2;
			Bool operator()( CClass* a, CClass* b ) const
			{	
				str1 = a->GetName();
				str2 = b->GetName();
				return str1 < str2;
			}

		} pred;

		TDynArray< CClass* > conditionClasses;
		SRTTI::GetInstance().EnumClasses( ClassID< IQuestCondition >(), conditionClasses );
		conditionClasses.Remove( ClassID< IQuestCondition >() );
		Sort( conditionClasses.Begin(), conditionClasses.End(), pred );

		for ( TDynArray< CClass* >::const_iterator it = conditionClasses.Begin(); it != conditionClasses.End(); ++it )
		{
			if ( (*it)->IsAbstract() )
			{
				continue;
			}
			m_conditionTypeChoice->AppendString( (*it)->GetName().AsString().AsChar() );
			m_conditionClasses.Insert( (*it)->GetName().AsString(), *it );
		}
	}

	// fill the list of script function names
	m_functionName = XRCCTRL( *this, "functionNameChoice", wxChoice );
	TDynArray< CFunction* > functions;
	SRTTI::GetInstance().EnumFunctions( functions );

	// Add classes
	TDynArray< String > functionNames;
	for ( Uint32 i=0; i<functions.Size(); i++ )
	{
		CFunction* func = functions[i];
		if ( func && func->IsQuest() )
		{
			functionNames.PushBack( func->GetName().AsString() );
		}
	}
	// Sort list and add to combo box
	Sort( functionNames.Begin(), functionNames.End() );
	for ( Uint32 i=0; i<functionNames.Size(); i++ )
	{
		m_functionName->AppendString( functionNames[i].AsChar() );
	}

	// fill the item names combo
	m_rewardNameChoice = XRCCTRL( *this, "rewardNameChoice", wxChoice );
	{
		// Get items
		TDynArray< CName > rewards;
#ifdef REWARD_EDITOR
		CEdRewardEditor::GetRewardNames( rewards );
#else
		const CRewardManager* defMgr = GCommonGame->GetRewardManager();
		defMgr->GetRewards( rewards );

#endif

		wxArrayString rewardNames;
		for (Uint32 i = 0; i < rewards.Size(); ++i)
		{
			rewardNames.Add( rewards[i].AsString().AsChar() );
		}

		// Append sorted items
		rewardNames.Sort();
		m_rewardNameChoice->Freeze();
		m_rewardNameChoice->Append( rewardNames );
		m_rewardNameChoice->Thaw();

		m_rewardNameChoice->Refresh();
	}


	m_blockName = XRCCTRL( *this, "blockName", wxTextCtrl );
	m_guidVal = XRCCTRL( *this, "blockGUID", wxTextCtrl );
	m_storyPhaseName = XRCCTRL( *this, "storyPhaseName", wxTextCtrl );
	m_factIdText = XRCCTRL( *this, "factIdText", wxTextCtrl );

	m_searchByTypeBtn = XRCCTRL( *this, "byTypeSearchButton", wxButton );
	m_searchByTypeBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByType ), NULL, this );

	m_searchByNameBtn = XRCCTRL( *this, "byNameSearchButton", wxButton );
	m_searchByNameBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByName ), NULL, this );

	m_searchByFunctionBtn = XRCCTRL( *this, "byFunctionSearchButton", wxButton );
	m_searchByFunctionBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByFunction ), NULL, this );

	m_searchByFactBtn = XRCCTRL( *this, "byFactSearchButton", wxButton );
	m_searchByFactBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByFact ), NULL, this );

	m_searchByRewardBtn = XRCCTRL( *this, "byRewardSearchButton", wxButton );
	m_searchByRewardBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByReward ), NULL, this );

	m_searchByConditionBtn = XRCCTRL( *this, "byConditionSearchButton", wxButton );
	m_searchByConditionBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByCondition ), NULL, this );

	m_searchByGUIDBtn = XRCCTRL( *this, "byGUIDSearchButton", wxButton );
	m_searchByGUIDBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchByGUID ), NULL, this );

	m_searchForSpawnsetBtn = XRCCTRL( *this, "spawnsetSearchButton", wxButton );
	m_searchForSpawnsetBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchForSpawnset ), NULL, this );

	m_searchForOutdatedBtn = XRCCTRL( *this, "outdatedSearchButton", wxButton );
	m_searchForOutdatedBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchForOutdated ), NULL, this );

	m_searchForInvalidBtn = XRCCTRL( *this, "invalidSearchButton", wxButton );
	m_searchForInvalidBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchForInvalid ), NULL, this );

	m_searchForZeroGUIDBtn = XRCCTRL( *this, "zeroGUIDSearchButton", wxButton );
	m_searchForZeroGUIDBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnSearchForZeroGUID ), NULL, this );

	m_importFromXML = XRCCTRL( *this, "importFromXMLButton", wxButton );
	m_importFromXML->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::ImportFromXML ), NULL, this );

	m_exportToXML = XRCCTRL( *this, "exportToXMLButton", wxButton );
	m_exportToXML->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::ExportToXML ), NULL, this );
	//m_exportToXML->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::mcinekTEMPSHITreplacer ), NULL, this );

	m_findCutscenesBtn = XRCCTRL( *this, "findCutscenesBtn", wxButton );
	m_findCutscenesBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestGraphSearcher::FindCutscenes ), NULL, this );

	m_results = XRCCTRL( *this, "results", wxListBox );
	m_results->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CQuestGraphSearcher::OnResultDblClicked ), NULL, this );
}

void CQuestGraphSearcher::OnSearchByType( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByType );
}

void CQuestGraphSearcher::OnSearchByName( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByName );
}

void CQuestGraphSearcher::OnSearchByFunction( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByFunction );
}

void CQuestGraphSearcher::OnSearchByFact( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByFact );
}

void CQuestGraphSearcher::OnSearchByReward( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByReward );
}

void CQuestGraphSearcher::OnSearchByCondition( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByCondition );
}

void CQuestGraphSearcher::OnSearchByGUID( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByGUID );
}

void CQuestGraphSearcher::OnSearchForOutdated( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::ValidateSingleGraph );
}

void CQuestGraphSearcher::OnSearchForInvalid( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByInvalidJournalPath );
}

void CQuestGraphSearcher::OnSearchForZeroGUID( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::FindByZeroGUID );
}

void CQuestGraphSearcher::OnSearchForSpawnset( wxCommandEvent& event )
{
	m_results->Clear();
	CName seekedPhaseName = CName( m_storyPhaseName->GetValue().wc_str() );

	// find all spawnsets
	TDynArray< CDiskFile* > foundFiles;
	GDepot->Search( TXT(".w2comm"), foundFiles );


	// analyze them one by one, memorizing those that contain the phase name we're looking for
	TDynArray< CCommunity* > matchingSpawnsets;
	for ( TDynArray< CDiskFile* >::iterator it = foundFiles.Begin(); it != foundFiles.End(); ++it )
	{
		CDiskFile* file = *it;
		if ( !file->IsLoaded() )
		{
			ResourceLoadingContext context;
			file->Load( context );
		}

		CCommunity* spawnset = Cast< CCommunity >( file->GetResource() );
		if ( spawnset )
		{
			THashSet< CName > outNames;
			spawnset->GetPhaseNames( outNames );

			if ( outNames.Find( seekedPhaseName ) != outNames.End() )
			{
				matchingSpawnsets.PushBack( spawnset ); 
			}
		}
	}

	// output the results
	for ( TDynArray< CCommunity* >::iterator it = matchingSpawnsets.Begin(); it != matchingSpawnsets.End(); ++it )
	{
		CDiskFile* file = (*it)->GetFile();
		m_results->Append( file ? file->GetDepotPath().AsChar() : (*it)->GetFriendlyName().AsChar(), 
			new SSpawnsetData( *it ) );
	}
}

void CQuestGraphSearcher::FindCutscenes( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::CutscenesFinder );
}

void CQuestGraphSearcher::ImportFromXML( wxCommandEvent& event )
{
	m_results->Clear();

	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	// load the xml
	wxFileDialog loadFileDialog( this, wxT("Import From XML"), wxT( "." ), wxT( "questFindResults.xml" ), wxT( "*.xml" ), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	String loadPath = loadFileDialog.GetPath().wc_str();
	IFile* file = GFileManager->CreateFileReader( loadPath, FOF_Buffered | FOF_AbsolutePath );
	ASSERT( file != NULL );
	if ( !file )
	{
		return;
	}

	// read blocks definitions from the XML
	m_importedBlocksGUIDs.Clear();
	CXMLFileReader reader( *file );
	String val;
	while ( reader.BeginNode( TXT( "object" ) ) )
	{
		while ( reader.BeginNode( TXT("property") ) )
		{
			if ( !reader.Attribute( TXT( "type" ), val ) )
			{
				continue;
			}
			if ( reader.BeginNode( TXT( "CGUID" ) ) )
			{
				reader.Value( val );
				m_importedBlocksGUIDs.PushBack( CGUID( val.AsChar() ) );
				reader.EndNode();
			}

			reader.EndNode();
		}
		reader.EndNode();
	}

	// find the blocks
	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::SearchForDefinitionsFromXML );

	// cleanup
	m_importedBlocksGUIDs.Clear();
	delete file;
}

void CQuestGraphSearcher::ExportToXML( wxCommandEvent& event )
{
	// create the xml file
	wxFileDialog saveFileDialog( this, wxT("Export To XML"), wxT( "." ), wxT( "questFindResults.xml" ), wxT( "*.xml" ), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	String savePath = saveFileDialog.GetPath().wc_str();
	IFile* file = GFileManager->CreateFileWriter( savePath, FOF_Buffered | FOF_AbsolutePath );
	ASSERT( file != NULL );
	if ( !file )
	{
		return;
	}

	// serialize the selected blocks to the xml
	TDynArray< CObject* > objectsToSave;
	Uint32 count = m_results->GetCount();
	for ( Uint32 i = 0; i < count; ++i )
	{
		IResult* data = dynamic_cast< IResult* >( m_results->GetClientObject( i ) );
		if ( data )
		{
			objectsToSave.PushBack( data->GetObject() );
		}
	}

	CXMLFileWriter* writer = new CXMLFileWriter( *file );

	writer->BeginNode( TXT("redxml") );

	for ( Uint32 i = 0; i < objectsToSave.Size(); ++i )
	{
		const CQuestGraphBlock* block = Cast< CQuestGraphBlock >( objectsToSave[ i ] );
		if ( block )
		{
			String entry = block->GetSearchCaption();
			writer->BeginNode( TXT("block") );
			writer->Attribute( TXT("entry"), entry );
			writer->EndNode();
		}
	}
	writer->EndNode();
	writer->Flush();

	// cleanup
	delete writer;
	delete file;
}

void CQuestGraphSearcher::OnResultDblClicked( wxCommandEvent& event )
{
	Int32 selection = event.GetSelection();
	if ( selection < 0 )
	{
		return;
	}

	IResult* data = dynamic_cast< IResult* >( m_results->GetClientObject( selection ) );
	if ( data )
	{
		data->Execute( *this );
		return;
	}
}

void CQuestGraphSearcher::OnPageChanged( wxBookCtrlEvent & event )
{
	//
	// This is checked here because after changing active tab in this dialog window, focus stays on search button from other, not-active tab.
	// Unfortunately wxButton "default" property works in scope of whole dialog window, so after setting default=true for all search buttons from all tabs, focus is set on the last one and stays there.
	//
	auto notebook = XRCCTRL( *this, "m_notebook15", wxNotebook );
	auto currentTab = notebook->GetPage( event.GetSelection() );
	wxButton* buttonToFocus = nullptr;

	if ( currentTab == XRCCTRL( *this, "byTypePage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byTypeSearchButton", wxButton );
	}		
	else if ( currentTab == XRCCTRL( *this, "byNamePage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byNameSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "byConditionPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byConditionSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "byScriptFunction", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byFunctionSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "byFact", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byFactSearchButton", wxButton );		
	}
	else if ( currentTab == XRCCTRL( *this, "byReward", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byRewardSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "byGUIDPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "byGUIDSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "spawnsetPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "spawnsetSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "outdatedPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "outdatedSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "invalidPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "invalidSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "zeroGuidPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "zeroGUIDSearchButton", wxButton );
	}
	else if ( currentTab == XRCCTRL( *this, "fromXMLPage", wxPanel ) )
	{
		buttonToFocus = XRCCTRL( *this, "importFromXMLButton", wxButton );
	}

	if ( buttonToFocus )
	{
		buttonToFocus->SetFocus();
	}
}

void CQuestGraphSearcher::SetBlocksList( TDynArray< CQuestGraphBlock* >& blocks )
{
	m_results->Clear();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_results->Append( blocks[ i ]->GetSearchCaption().AsChar(), new SQuestBlockData( blocks[ i ] ) );
	}
}

void CQuestGraphSearcher::GraphBFS( CQuestGraph& rootGraph, SearchMethod method )
{
	TSet< CQuestGraph* > exploredGraphs;

	TDynArray< CQuestGraph* > graphsStack;
	graphsStack.PushBack( &rootGraph );

	TDynArray< CQuestGraphBlock* > foundBlocks;

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

		(this->*method)( *currGraph, foundBlocks );

		TDynArray< CGraphBlock* >& blocks = currGraph->GraphGetBlocks();
		for ( TDynArray< CGraphBlock* >::iterator it = blocks.Begin();
			it != blocks.End(); ++it )
		{
			CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( *it );
			if ( questBlock && questBlock->IsA< CQuestScopeBlock >() )
			{
				CQuestScopeBlock* scope = Cast< CQuestScopeBlock >( questBlock );
				graphsStack.PushBack( scope->GetGraph() );
			}
		}
	}

	// inform the user about the outdated blocks
	if ( foundBlocks.Empty() )
	{
		m_results->Append( wxT( "No blocks found" ) );
	}
	else
	{
		SetBlocksList( foundBlocks );
	}
}

void CQuestGraphSearcher::ValidateSingleGraph( CQuestGraph& graph, 
											  TDynArray< CQuestGraphBlock* >& outdatedBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );
		if ( questBlock && questBlock->NeedsUpdate() )
		{
			outdatedBlocks.PushBack( questBlock );
		}
	}
}

void CQuestGraphSearcher::FindByName( CQuestGraph& graph, 
									 TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	String searchedName = m_blockName->GetValue().wc_str();
	if ( searchedName.GetLength() == 0 )
	{
		return;
	}

	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );
		if ( questBlock && MatchBlockName( questBlock->GetBlockName() ) )
		{
			foundBlocks.PushBack( questBlock );
		}
	}
}

void CQuestGraphSearcher::FindByFunction( CQuestGraph& graph, 
									 TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	const Int32 selection = m_functionName->GetSelection();
	if ( selection == wxNOT_FOUND )
	{
		return;
	}

	String selectedFunctionName = m_functionName->GetString( selection ).wc_str();
	if ( selectedFunctionName.Empty() )
	{
		return;
	}

	CName selectedFunction( selectedFunctionName );

	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestScriptBlock* questScriptBlock = Cast< CQuestScriptBlock >( blocks[ i ] );
		if ( questScriptBlock && selectedFunction == questScriptBlock->GetFunctionName()  )
		{
			foundBlocks.PushBack( questScriptBlock );
		}
	}
}

void CQuestGraphSearcher::FindByFact( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	String selectedFactId = m_factIdText->GetValue().wc_str();
	if ( selectedFactId.Empty() )
	{
		return;
	}

	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestFactsDBChangingBlock* questFactsDBBlock = Cast< CQuestFactsDBChangingBlock >( blocks[ i ] );
		if ( questFactsDBBlock && selectedFactId == questFactsDBBlock->GetFactId().AsChar()  )
		{
			foundBlocks.PushBack( questFactsDBBlock );
		}
	}
}

void CQuestGraphSearcher::FindByReward( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	const Int32 selection = m_rewardNameChoice->GetSelection();
	if ( selection == wxNOT_FOUND )
	{
		return;
	}

	String itemName = m_rewardNameChoice->GetString( selection ).wc_str();
	CName item( itemName );

	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestRewardBlock* rewardBlock = Cast< CQuestRewardBlock >( blocks[ i ] );
		if ( !rewardBlock )
		{
			continue;
		}

		if( rewardBlock->GetRewardName() == item )
		{
			foundBlocks.PushBack( rewardBlock );
		}
	}
}

void CQuestGraphSearcher::FindByGUID( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	CGUID searchedGUID = CGUID( m_guidVal->GetValue().wc_str() );

	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );
		if ( questBlock && questBlock->GetGUID() == searchedGUID )
		{
			foundBlocks.PushBack( questBlock );
		}
	}
}

void CQuestGraphSearcher::FindByType( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );
		if ( questBlock && MatchBlockType( questBlock ) )
		{
			foundBlocks.PushBack( questBlock );
		}
	}
}

void CQuestGraphSearcher::FindByCondition( CQuestGraph& graph, 
									 TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestPauseConditionBlock* pauseCond = Cast< CQuestPauseConditionBlock >( blocks[ i ] );
		if ( pauseCond )
		{
			TDynArray< IQuestCondition* >& conditions = pauseCond->GetConditions();
			for ( TDynArray< IQuestCondition* >::iterator condIt = conditions.Begin();
				condIt != conditions.End(); ++condIt )
			{
				if ( *condIt && MatchConditionType( *condIt ) )
				{
					foundBlocks.PushBack( pauseCond );
				}
			}

			continue;
		}

		CQuestConditionBlock* cond = Cast< CQuestConditionBlock >( blocks[ i ] );
		if ( cond && cond->GetCondition() )		
		{
			if ( MatchConditionType( cond->GetCondition() ) )
			{
				foundBlocks.PushBack( cond );
			}
		}
	}
}

void CQuestGraphSearcher::SearchForDefinitionsFromXML( CQuestGraph& graph, 
									 TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );
		TDynArray< CGUID >::iterator guidIt = questBlock ? Find( m_importedBlocksGUIDs.Begin(), m_importedBlocksGUIDs.End(), questBlock->GetGUID() ) : m_importedBlocksGUIDs.End();
		if ( guidIt !=  m_importedBlocksGUIDs.End() )
		{
			// we found one
			m_importedBlocksGUIDs.Remove( *guidIt );
			foundBlocks.PushBack( questBlock );
		}
	}
}

void CQuestGraphSearcher::CutscenesFinder( CQuestGraph& graph, 
														TDynArray< CQuestGraphBlock* >& blocksWithCutscenes ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestGraphBlock* questBlock = SafeCast< CQuestGraphBlock >( blocks[ i ] );

		if( questBlock )
		{
			CStoryScene* scene = NULL;
			if ( questBlock->IsA< CQuestContextDialogBlock >() )
			{
				CQuestContextDialogBlock* sceneBlock = Cast< CQuestContextDialogBlock >( questBlock );
				scene = sceneBlock->GetScene();
			}
			else if ( questBlock->IsA< CQuestInteractionDialogBlock >() )
			{
				CQuestInteractionDialogBlock* sceneBlock = Cast< CQuestInteractionDialogBlock >( questBlock );
				scene = sceneBlock->GetScene();
			}
			else if ( questBlock->IsA< CQuestSceneBlock >() )
			{
				CQuestSceneBlock* sceneBlock = Cast< CQuestSceneBlock >( questBlock );
				scene = sceneBlock->GetScene();
			}

			if ( !scene )
			{
				continue;
			}

			if ( HasCutscenes( *scene ) )
			{
				blocksWithCutscenes.PushBack( questBlock );
			}
		}
	}
}

void CQuestGraphSearcher::FindByInvalidJournalPath( CQuestGraph& graph, 
												   TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		if (	blocks[ i ]->IsA< CJournalBlock >() ||
				blocks[ i ]->IsA< CJournalQuestBlock >() ||
				blocks[ i ]->IsA< CJournalQuestTrackBlock >() ||
				blocks[ i ]->IsA< CJournalQuestMappinStateBlock >() ||
				blocks[ i ]->IsA< CJournalQuestObjectiveCounterGraphBlock >() ||
				blocks[ i ]->IsA< CJournalQuestMonsterKnownGraphBlock >() )
		{
			CQuestGraphBlock* block = Cast< CQuestGraphBlock >( blocks[ i ] );
			if ( block )
			{
				if ( !block->IsValid() )
				{
					foundBlocks.PushBackUnique( block );
				}
			}
			else
			{
				RED_HALT("Not a CQuestGraphBlock");
			}
		}
	}
}

void CQuestGraphSearcher::FindByZeroGUID( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( blocks[ i ]->IsA< CQuestGraphBlock >() )
		{
			CQuestGraphBlock* block = Cast< CQuestGraphBlock >( blocks[ i ] );
			if ( block )
			{
				if ( block->GetGUID() == CGUID::ZERO )
				{
					foundBlocks.PushBackUnique( block );
				}
			}
		}
	}
}

Bool CQuestGraphSearcher::HasCutscenes( CStoryScene& scene ) const
{
	// list all inputs
	TDynArray< CStorySceneCutsceneSection* > blocks;
	scene.CollectControlParts( blocks );

	return !blocks.Empty();
}

Bool CQuestGraphSearcher::MatchBlockName( const String& blockName ) const
{
	return blockName == String( m_blockName->GetValue().wc_str() );
}

Bool CQuestGraphSearcher::MatchBlockType( CQuestGraphBlock* questBlock ) const
{
	const Int32 selection = m_blockTypeChoice->GetSelection();
	if ( selection == wxNOT_FOUND )
	{
		return false;
	}

	String selectedTypeName = m_blockTypeChoice->GetString( selection ).wc_str();
	CClass* foundClass = m_blockClasses[ selectedTypeName ];
	return ( foundClass && questBlock->GetClass() == foundClass );
}

Bool CQuestGraphSearcher::MatchConditionType( IQuestCondition* condition ) const
{
	const Int32 selection = m_conditionTypeChoice->GetSelection();
	if ( selection == wxNOT_FOUND )
	{
		return false;
	}

	String selectedTypeName = m_conditionTypeChoice->GetString( selection ).wc_str();
	CClass* foundClass = m_conditionClasses[ selectedTypeName ];
	return ( foundClass && condition->GetClass() == foundClass );
}

void CQuestGraphSearcher::OpenQuestBlockInEditor( CQuestGraphBlock* block )
{
	TDynArray< CQuestGraphBlock* > stack;
	m_parent.FindBlock( block, stack );
	m_parent.OpenBlock( stack );
}

void CQuestGraphSearcher::EditSpawnset( CCommunity* spawnset )
{
	CEdAssetBrowser* assetBrowser = wxTheFrame->GetAssetBrowser();
	assetBrowser->SelectAndOpenFile( spawnset->GetFile()->GetDepotPath() );
}

#if 0

Int32 mcinekTEMPSHIT_replaceCounter = 0;

void CQuestGraphSearcher::mcinekTEMPSHITreplacer( wxCommandEvent& event )
{
	CQuestGraph* topLevelGraph = m_parent.GetTopLevelGraph();
	if ( !topLevelGraph )
	{
		return;
	}

	mcinekTEMPSHIT_replaceCounter = 0;

	GraphBFS( *topLevelGraph, &CQuestGraphSearcher::mcinekTEMPSHITsearch );

	wxMessageBox( String::Printf( TXT( "Replaced %d blocks" ), mcinekTEMPSHIT_replaceCounter ).AsChar() );
}


void CQuestGraphSearcher::mcinekTEMPSHITsearch( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const
{
	TDynArray< CGraphBlock* >& blocks = graph.GraphGetBlocks();
	Uint32 count = blocks.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestScriptBlock* scriptBlock = Cast< CQuestScriptBlock >( blocks[ i ] );
		if( scriptBlock == NULL )
		{
			continue;
		}

		if( scriptBlock->GetFunctionName() != TXT( "QStartMinigameEx" ) )
		{
			continue;
		}

		// Get connections
		TDynArray< CGraphSocket* > inputConnections;
		TDynArray< CGraphSocket* > winConnections;
		TDynArray< CGraphSocket* > loseConnections;

		// Iterate every socket
		const TDynArray< CGraphSocket* >& sockets = scriptBlock->GetSockets();
		for( Uint32 i = 0; i < sockets.Size(); ++i )
		{
			CGraphSocket* socket = sockets[ i ];
			TDynArray< CGraphConnection* > connections = socket->GetConnections();
			TDynArray< CGraphSocket* >* toAdd = NULL;

			if( socket->GetName() == CNAME( In ) )
			{
				toAdd = &inputConnections;
			}
			else if( socket->GetName() == CNAME( True ) )
			{
				toAdd = &winConnections;
			}
			else if( socket->GetName() == CNAME( False ) )
			{
				toAdd = &loseConnections;
			}

			if( toAdd == NULL )
			{
				// Skip socket
				continue;
			}

			// Add destinations of connections
			for( Uint32 z = 0; z < connections.Size(); ++z )
			{
				CGraphConnection* connection = connections[ z ];
				if( connection->GetSource() == socket && connection->GetDestination() != NULL )
				{
					toAdd->PushBack( connection->GetDestination() );
				}
				else if( connection->GetSource() != NULL )
				{
					toAdd->PushBack( connection->GetSource() );
				}
			}
		}

		// Create new block
		GraphBlockSpawnInfo binfo( CDicePokerBlock::GetStaticClass() );
		binfo.m_position = scriptBlock->GetPosition();
		CDicePokerBlock* dicePokerBlock = static_cast< CDicePokerBlock* >( graph.GraphCreateBlock( binfo ) );

		// Recreate connections
		CGraphSocket* socket = dicePokerBlock->FindSocket( CNAME( In ) );
		for( Uint32 z = 0; z < inputConnections.Size(); ++z )
		{
			inputConnections[ z ]->ConnectTo( socket );
		}

		socket = dicePokerBlock->FindSocket( CDicePokerBlock::OUT_PLAYER_WON );
		for( Uint32 z = 0; z < winConnections.Size(); ++z )
		{
			socket->ConnectTo( winConnections[ z ] );
		}

		socket = dicePokerBlock->FindSocket( CDicePokerBlock::OUT_PLAYER_LOST );
		for( Uint32 z = 0; z < loseConnections.Size(); ++z )
		{
			socket->ConnectTo( loseConnections[ z ] );
		}

		// Fill properties
		for( Uint32 z = 0; z < scriptBlock->m_parameters.Size(); ++z )
		{
			const QuestScriptParam& param = scriptBlock->m_parameters[ z ];
			if( param.m_name == TXT( "minigameTemplate" ) )
			{
				THandle< CEntityTemplate > handle;
				VERIFY( param.m_value.AsType< THandle< CEntityTemplate > >( handle ) );
				dicePokerBlock->m_minigameTemplate = handle.Get();
			}
			else if( param.m_name == TXT( "spawnPointTag" ) )
			{
				VERIFY( param.m_value.AsType< CName >( dicePokerBlock->m_spawnPointTag ) );
			}
			else if( param.m_name == TXT( "opponent" ) )
			{
				VERIFY( param.m_value.AsType< CName >( dicePokerBlock->m_opponentTag ) );
			}
			else if( param.m_name == TXT( "endWithBlackscreen" ) )
			{
				VERIFY( param.m_value.AsType< Bool >( dicePokerBlock->m_endWithBlackscreen ) );
			}
		}

		// Remove oldblock from graph
		graph.GraphRemoveBlock( scriptBlock );

		mcinekTEMPSHIT_replaceCounter++;
	}
}

#endif

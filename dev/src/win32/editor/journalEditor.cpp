#include "build.h"
#include "journalEditor.h"
#include "chooseItemDialog.h"
#include "stringSelectorTool.h"
#include "journalSelectorWidget.h"
#include "journalTree.h"

#include "../../games/r4/journal.h"
#include "../../games/r4/r4JournalManager.h"
#include "../../common/engine/gameResource.h"
#include "../../games/r4/r4GameResource.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

wxDEFINE_EVENT( wxEVT_JOURNAL_CHOICE_POPULATED, wxCommandEvent );

RED_DEFINE_STATIC_NAME( EAreaName )
RED_DEFINE_STATIC_NAME( contentType )

enum eEventIds
{
	EventID_CreateQuestGroup = wxID_HIGHEST,
	EventID_CreateQuest,
	EventID_CreateQuestPhase,
	EventID_CreateQuestObjective,
	EventID_CreateQuestMapPin,
	EventID_CreateQuestItemTag,
	EventID_CreateQuestMonsterTag,
	EventID_CreateQuestDescription,

	EventID_CreateCharacterGroup,
	EventID_CreateCharacter,
	EventID_CreateCharacterDescription,

	EventID_CreateGlossaryGroup,
	EventID_CreateGlossaryEntry,
	EventID_CreateGlossaryEntryDescription,

	EventID_CreateTutorialGroup,
	EventID_CreateTutorialEntry,

	EventID_CreateItemGroup,
	EventID_CreateItemSubGroup,
	EventID_CreateItemEntry,
	EventID_CreateItemComponent,

	EventID_CreateCreatureGroup,
	EventID_CreateCreatureVirtualGroup,
	EventID_CreateCreature,
	EventID_CreateCreatureDescription,
	EventID_CreateCreatureHuntingClue,
	EventID_CreateCreatureGameplayHint,
	EventID_CreateCreatureVitalSpot,

	EventID_CreateStoryBookChapter,
	EventID_CreateStoryBookPage,
	EventID_CreateStoryBookPageDescription,

	EventID_CreatePlaceGroup,
	EventID_CreatePlace,
	EventID_CreatePlaceDescription,

	EventID_DuplicateQuestObjective,
	EventID_DuplicateQuestMapPin,

	EventID_SortChildren,

	EventID_SelectItem,
	EventID_SelectItems,
	EventID_SelectString,
	EventID_SelectObject,
	EventID_ClearPath,

	EventID_DeleteItem,

	EventID_SC_CheckOut,
	EventID_SC_Save,
	EventID_SC_SaveAndCheckin,
	EventID_SC_Revert,
	EventID_SC_Delete,
	EventID_SC_NotSynced,

	// Generic property changes
	EventID_LocalizedStringTextChange,
	EventID_LocalizedStringIDChange,
	EventID_StringChange,
	EventID_StringChoiceChange,
	EventID_UintChange,
	EventID_ChoiceChange,
	EventID_CNameChange,
	EventID_CNameChoiceChange,
	EventID_JournalPathChange,
	EventID_CheckBoxChange,
	EventID_FloatChange,
	EventID_ObjectHandleChange,
	EventID_CNameArrayChange,

	// Custom property changes
	EventID_FilenameChange,
	EventID_NameChange,

	EventID_QuestTypeChange,
	EventID_QuestObjective_TypeChange,

	EventID_Max
};

CEdJournalEditor::CEdJournalEditor( wxWindow* parent, THandle< CJournalPath > defaultPath )
:	wxSmartLayoutPanel( parent, TEXT( "JournalEditor" ), false ),
	m_treePopupMenu( NULL ),
	m_stringSelectorTool( NULL ),
	m_previousGroup( NULL ),
	m_huntingClueChoiceWidget( NULL )
{
	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_JOURNAL" ) ) );
	SetIcon( iconSmall );

	LoadOptionsFromConfig();

	// Extract widgets
	m_Tree = XRCCTRL( *this, "journalTree", CEdJournalTree );
	ASSERT( m_Tree != NULL );

	m_Tree->Initialize();

	m_splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );
	ASSERT( m_splitter != NULL );
	m_splitter->Unsplit();

	m_subContainer = XRCCTRL( *this, "SubContainer", wxPanel );
	ASSERT( m_subContainer != NULL );

	// Don't bother doing anything more if the journal directory hasn't been defined
	if( !m_Tree->IsDirectoryDefined() )
	{
		return;
	}

	for( Uint32 i = 0; i < CEdJournalTree::TreeCategory_Max; ++i )
	{
		m_Tree->AddCategoryRoot( static_cast< CEdJournalTree::eTreeCategory >( i ) );

		m_Tree->PopulateTreeSection( static_cast< CEdJournalTree::eTreeCategory >( i ) );
	}

	// custom setup for CJournalCreatureVirtualGroup->CJournalCreatureGroup link
	TDynArray< CDirectory * > bestiaryMainDirectory;
	TDynArray< CDirectory * > bestiaryDirectories = CEdJournalTree::GetCreaturesDirectories();
	if ( bestiaryDirectories.Size() > 0 )
	{
		bestiaryMainDirectory.PushBack( bestiaryDirectories[ 0 ] );
	}
	m_linkedJournalTypes.Insert( ClassID< CJournalCreatureVirtualGroup >(), SLinkedJournalTypeDescriptor( ClassID< CJournalCreatureVirtualGroup >(), ClassID< CJournalCreatureGroup >(), bestiaryMainDirectory ) );

	CreateWidgets( ClassID< CJournalQuestGroup >(), true );
	CreateWidgets( ClassID< CJournalQuest >(), true );
	CreateWidgets( ClassID< CJournalQuestPhase >(), false );

	CreateWidgets( ClassID< CJournalQuestDescriptionGroup >(), false );
	CreateWidgets( ClassID< CJournalQuestDescriptionEntry >(), false );

	CreateWidgets( ClassID< CJournalQuestObjective >(), false );
	CreateWidgets( ClassID< CJournalQuestMapPin >(), false );
	CreateWidgets( ClassID< CJournalQuestEnemyTag >(), false );
	CreateWidgets( ClassID< CJournalQuestItemTag >(), false );

	CreateWidgets( ClassID< CJournalCharacterGroup >(), true );
	CreateWidgets( ClassID< CJournalCharacter >(), true );
	CreateWidgets( ClassID< CJournalCharacterDescription >(), false );

	CreateWidgets( ClassID< CJournalGlossaryGroup >(), true );
	CreateWidgets( ClassID< CJournalGlossary >(), true );
	CreateWidgets( ClassID< CJournalGlossaryDescription >(), false );

	CreateWidgets( ClassID< CJournalTutorialGroup >(), true );
	CreateWidgets( ClassID< CJournalTutorial >(), true );

	CreateWidgets( ClassID< CJournalItemGroup >(), true );
	CreateWidgets( ClassID< CJournalItemSubGroup >(), true );
	CreateWidgets( ClassID< CJournalItem >(), true );
	CreateWidgets( ClassID< CJournalItemComponent >(), false );

	CreateWidgets( ClassID< CJournalCreatureGroup >(), true );
	CreateWidgets( ClassID< CJournalCreatureVirtualGroup >(), true );
	CreateWidgets( ClassID< CJournalCreature >(), true );
	CreateWidgets( ClassID< CJournalCreatureDescriptionGroup >(), false );
	CreateWidgets( ClassID< CJournalCreatureDescriptionEntry >(), false );
	CreateWidgets( ClassID< CJournalCreatureHuntingClueGroup >(), false );
	CreateWidgets( ClassID< CJournalCreatureHuntingClue >(), false );
	CreateWidgets( ClassID< CJournalCreatureGameplayHintGroup >(), false );
	CreateWidgets( ClassID< CJournalCreatureGameplayHint >(), false );
	CreateWidgets( ClassID< CJournalCreatureVitalSpotEntry >(), false );
	CreateWidgets( ClassID< CJournalCreatureVitalSpotGroup >(), false );

	CreateWidgets( ClassID< CJournalStoryBookChapter >(), true );
	CreateWidgets( ClassID< CJournalStoryBookPage >(), true );
	CreateWidgets( ClassID< CJournalStoryBookPageDescription >(), false );

	CreateWidgets( ClassID< CJournalPlaceGroup >(), true );
	CreateWidgets( ClassID< CJournalPlace >(), true );
	CreateWidgets( ClassID< CJournalPlaceDescription >(), false );

	Bind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdJournalEditor::OnTreeRightClick, this, XRCID( "journalTree" ) );
	Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdJournalEditor::OnCategorySelectionChanged, this, XRCID( "journalTree" ) );
	
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestGroup, this, GetID( EventID_CreateQuestGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuest, this, GetID( EventID_CreateQuest ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestPhase, this, GetID( EventID_CreateQuestPhase ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestObjective, this, GetID( EventID_CreateQuestObjective ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestMapPin, this, GetID( EventID_CreateQuestMapPin ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestItemTag, this, GetID( EventID_CreateQuestItemTag ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestMonsterTag, this, GetID( EventID_CreateQuestMonsterTag ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestDescription, this, GetID( EventID_CreateQuestDescription ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacterGroup, this, GetID( EventID_CreateCharacterGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacter, this, GetID( EventID_CreateCharacter ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacterDescription, this, GetID( EventID_CreateCharacterDescription ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryGroup, this, GetID( EventID_CreateGlossaryGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryEntry, this, GetID( EventID_CreateGlossaryEntry ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryDescription, this, GetID( EventID_CreateGlossaryEntryDescription ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewTutorialGroup, this, GetID( EventID_CreateTutorialGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewTutorialEntry, this, GetID( EventID_CreateTutorialEntry ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemGroup, this, GetID( EventID_CreateItemGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemSubGroup, this, GetID( EventID_CreateItemSubGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemEntry, this, GetID( EventID_CreateItemEntry ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemComponent, this, GetID( EventID_CreateItemComponent ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureGroup, this, GetID( EventID_CreateCreatureGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureVirtualGroup, this, GetID( EventID_CreateCreatureVirtualGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreature, this, GetID( EventID_CreateCreature ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureDescription, this, GetID( EventID_CreateCreatureDescription ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureHuntingClue, this, GetID( EventID_CreateCreatureHuntingClue ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureVitalSpot, this, GetID( EventID_CreateCreatureVitalSpot ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureGameplayHint, this, GetID( EventID_CreateCreatureGameplayHint ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookChapter, this, GetID( EventID_CreateStoryBookChapter ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookPage, this, GetID( EventID_CreateStoryBookPage ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookPageDescription, this, GetID( EventID_CreateStoryBookPageDescription ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlaceGroup, this, GetID( EventID_CreatePlaceGroup ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlace, this, GetID( EventID_CreatePlace ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlaceDescription, this, GetID( EventID_CreatePlaceDescription ) );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectItemDialog, this, GetID( EventID_SelectItem ) );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectItemsDialog, this, GetID( EventID_SelectItems ) );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectStringDialog, this, GetID( EventID_SelectString ) );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnUseObject, this, GetID( EventID_SelectObject ) );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnClearPath, this, GetID( EventID_ClearPath ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCCheckOut, this, GetID( EventID_SC_CheckOut ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCSaveItem, this, GetID( EventID_SC_Save ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCSaveAndCheckIn, this, GetID( EventID_SC_SaveAndCheckin ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCRevert, this, GetID( EventID_SC_Revert ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCDeleteItem, this, GetID( EventID_SC_Delete ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDuplicateQuestObjective, this, GetID( EventID_DuplicateQuestObjective ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDuplicateQuestMapPin, this, GetID( EventID_DuplicateQuestMapPin ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSortChildren, this, GetID( EventID_SortChildren ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDeleteItem, this, GetID( EventID_DeleteItem ) );
	
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnStringChange, this, GetID( EventID_StringChange ) );
	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnStringChange, this, GetID( EventID_StringChoiceChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnUintChange, this, GetID( EventID_UintChange ) );
	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnChoiceChange, this, GetID( EventID_ChoiceChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnLocalizedStringTextChange, this, GetID( EventID_LocalizedStringTextChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnCNameChanged, this, GetID( EventID_CNameChange ) );
	Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdJournalEditor::OnCheckBoxChange, this, GetID( EventID_CheckBoxChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnFloatChange, this, GetID( EventID_FloatChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnObjectHandleChange, this, GetID( EventID_ObjectHandleChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnNameArrayChange, this, GetID( EventID_CNameArrayChange ) );

	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnCNameChanged, this, GetID( EventID_CNameChoiceChange ) );
	
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnNameChange, this, GetID( EventID_NameChange ) );
	Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnFilenameChange, this, GetID( EventID_FilenameChange ) );

	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnQuestObjectiveTypeChange, this, GetID( EventID_QuestObjective_TypeChange ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnSaveAll, this, XRCID( "saveAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckInAll, this, XRCID( "checkAllIn" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnSelectTargetDLC, this, XRCID("fileSelectTargetDLC") );
	Bind( wxEVT_CLOSE_WINDOW, &CEdJournalEditor::OnCloseWindow, this );

	Bind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdJournalEditor::OnTreeDrag, this );
	Bind( wxEVT_COMMAND_TREE_END_DRAG, &CEdJournalEditor::OnTreeDrop, this );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCollapseAll, this, XRCID( "collapseAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnExpandAll, this, XRCID( "expandAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnResetWindowSize, this, XRCID( "resetWindowSize" ) );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckDuplicatedUniqueScriptTags, this, XRCID( "checkDuplicatedUniqueScriptTags" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckDuplicatedGUIDs, this, XRCID( "checkDuplicatedGUIDs" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnGenerateMissingUniqueScriptTags, this, XRCID( "generateMissingUniqueScriptTags" ) );

	if( defaultPath )
	{
		m_Tree->ExpandPath( defaultPath );
	}

	InitTargetDLC();

	OnOpenInitialEntriesResource();
}

CEdJournalEditor::~CEdJournalEditor()
{
	OnCloseInitialEntriesResource();

	Unbind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdJournalEditor::OnTreeRightClick, this, XRCID( "journalTree" ) );
	Unbind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdJournalEditor::OnCategorySelectionChanged, this, XRCID( "journalTree" ) );
		
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestGroup, this, GetID( EventID_CreateQuestGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuest, this, GetID( EventID_CreateQuest ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestPhase, this, GetID( EventID_CreateQuestPhase ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestObjective, this, GetID( EventID_CreateQuestObjective ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestMapPin, this, GetID( EventID_CreateQuestMapPin ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestItemTag, this, GetID( EventID_CreateQuestItemTag ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestMonsterTag, this, GetID( EventID_CreateQuestMonsterTag ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewQuestDescription, this, GetID( EventID_CreateQuestDescription ) );


	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacterGroup, this, GetID( EventID_CreateCharacterGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacter, this, GetID( EventID_CreateCharacter ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCharacterDescription, this, GetID( EventID_CreateCharacterDescription ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryGroup, this, GetID( EventID_CreateGlossaryGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryEntry, this, GetID( EventID_CreateGlossaryEntry ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewGlossaryDescription, this, GetID( EventID_CreateGlossaryEntryDescription ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewTutorialGroup, this, GetID( EventID_CreateTutorialGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewTutorialEntry, this, GetID( EventID_CreateTutorialEntry ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemGroup, this, GetID( EventID_CreateItemGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemSubGroup, this, GetID( EventID_CreateItemSubGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemEntry, this, GetID( EventID_CreateItemEntry ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewItemComponent, this, GetID( EventID_CreateItemComponent ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureGroup, this, GetID( EventID_CreateCreatureGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureVirtualGroup, this, GetID( EventID_CreateCreatureVirtualGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreature, this, GetID( EventID_CreateCreature ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureDescription, this, GetID( EventID_CreateCreatureDescription ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureHuntingClue, this, GetID( EventID_CreateCreatureHuntingClue ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureGameplayHint, this, GetID( EventID_CreateCreatureGameplayHint ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewCreatureVitalSpot, this, GetID( EventID_CreateCreatureVitalSpot ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookChapter, this, GetID( EventID_CreateStoryBookChapter ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookPage, this, GetID( EventID_CreateStoryBookPage ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewStoryBookPageDescription, this, GetID( EventID_CreateStoryBookPageDescription ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlaceGroup, this, GetID( EventID_CreatePlaceGroup ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlace, this, GetID( EventID_CreatePlace ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCreateNewPlaceDescription, this, GetID( EventID_CreatePlaceDescription ) );

	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectItemDialog, this, GetID( EventID_SelectItem ) );
	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectItemsDialog, this, GetID( EventID_SelectItems ) );
	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnSelectStringDialog, this, GetID( EventID_SelectString ) );

	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnUseObject, this, GetID( EventID_SelectObject ) );

	Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdJournalEditor::OnClearPath, this, GetID( EventID_ClearPath ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCCheckOut, this, GetID( EventID_SC_CheckOut ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCSaveItem, this, GetID( EventID_SC_Save ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCSaveAndCheckIn, this, GetID( EventID_SC_SaveAndCheckin ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCRevert, this, GetID( EventID_SC_Revert ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSCDeleteItem, this, GetID( EventID_SC_Delete ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDuplicateQuestObjective, this, GetID( EventID_DuplicateQuestObjective ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDuplicateQuestMapPin, this, GetID( EventID_DuplicateQuestMapPin ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupSortChildren, this, GetID( EventID_SortChildren ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnPopupDeleteItem, this, GetID( EventID_DeleteItem ) );

	Unbind( wxEVT_MENU_CLOSE, &CEdJournalEditor::OnCreateNewQuestGroup, this );

	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnStringChange, this, GetID( EventID_StringChange ) );
	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnStringChange, this, GetID( EventID_StringChoiceChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnUintChange, this, GetID( EventID_UintChange ) );
	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnChoiceChange, this, GetID( EventID_ChoiceChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnLocalizedStringTextChange, this, GetID( EventID_LocalizedStringTextChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnCNameChanged, this, GetID( EventID_CNameChange ) );
	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnCNameChanged, this, GetID( EventID_CNameChoiceChange ) );
    Unbind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdJournalEditor::OnCheckBoxChange, this, GetID( EventID_CheckBoxChange ) );
    Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnFloatChange, this, GetID( EventID_FloatChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnObjectHandleChange, this, GetID( EventID_ObjectHandleChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnNameArrayChange, this, GetID( EventID_CNameArrayChange ) );

	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnNameChange, this, GetID( EventID_NameChange ) );
	Unbind( wxEVT_COMMAND_TEXT_UPDATED, &CEdJournalEditor::OnFilenameChange, this, GetID( EventID_FilenameChange ) );

	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnQuestObjectiveTypeChange, this, GetID( EventID_QuestObjective_TypeChange ) );
	
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnSaveAll, this, XRCID( "saveAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckInAll, this, XRCID( "checkAllIn" ) );
	Unbind( wxEVT_CLOSE_WINDOW, &CEdJournalEditor::OnCloseWindow, this );

	Unbind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdJournalEditor::OnTreeDrag, this );
	Unbind( wxEVT_COMMAND_TREE_END_DRAG, &CEdJournalEditor::OnTreeDrop, this );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCollapseAll, this, XRCID( "collapseAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnExpandAll, this, XRCID( "expandAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnResetWindowSize, this, XRCID( "resetWindowSize" ) );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckDuplicatedUniqueScriptTags, this, XRCID( "checkDuplicatedUniqueScriptTags" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnCheckDuplicatedGUIDs, this, XRCID( "checkDuplicatedGUIDs" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdJournalEditor::OnGenerateMissingUniqueScriptTags, this, XRCID( "generateMissingUniqueScriptTags" ) );

	m_widgetData.ClearPtr();
}

void CEdJournalEditor::OnCloseWindow( wxCloseEvent& event )
{
	if( m_modifiedMenuItems.Size() > 0 )
	{
		Int32 userAnswer = YesNoCancel( TXT( "Changes not saved. Save?" ) );
		if( userAnswer == IDYES )
		{
			SaveAll();
		}
		else if( userAnswer == IDCANCEL )
		{
			return;
		}
	}

	Destroy();
}

void CEdJournalEditor::OnTreeRightClick( wxEvent& event )
{
	if( event.IsKindOf( CLASSINFO( wxTreeEvent ) ) )
	{
		wxTreeEvent& treeEvent = static_cast< wxTreeEvent& >( event );

		if( m_treePopupMenu )
		{
			delete m_treePopupMenu;
		}

		m_treePopupMenu = new wxMenu();

		m_popupMenuItem = treeEvent.GetItem();
		ASSERT( m_popupMenuItem.IsOk() );

		m_Tree->SelectItem( m_popupMenuItem );

		PopulateCommonMenuOptions( m_popupMenuItem, m_treePopupMenu );

		PopupMenu( m_treePopupMenu );
	}
}

void CEdJournalEditor::PopulateMenuCreationOptions( const wxTreeItemId& item, wxMenu* menu ) const
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );
	ASSERT( itemData != NULL );

	if( itemData->m_entry->IsA< CJournalQuestRoot >() )
	{
		menu->Append( GetID( EventID_CreateQuestGroup ), wxT( "Create new quest group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestGroup >() )
	{
		menu->Append( GetID( EventID_CreateQuest ), wxT( "Create new quest" ) );
		menu->Append( GetID( EventID_CreateQuestGroup ), wxT( "Create new quest group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuest >() )
	{
		menu->Append( GetID( EventID_CreateQuestPhase ), wxT( "Create new phase" ) );
		menu->Append( GetID( EventID_CreateQuest ), wxT( "Create new quest" ) );
			
	}
	else if( itemData->m_entry->IsA< CJournalQuestPhase >() )
	{
		menu->Append( GetID( EventID_CreateQuestObjective ), wxT( "Create new objective" ) );
		menu->Append( GetID( EventID_CreateQuestPhase ), wxT( "Create new phase" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete phase" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestDescriptionGroup >() )
	{
		menu->Append( GetID( EventID_CreateQuestDescription ), wxT( "Create new description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestDescriptionEntry >() )
	{
		menu->Append( GetID( EventID_CreateQuestDescription ), wxT( "Create new description" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestObjective >() )
	{
		menu->Append( GetID( EventID_CreateQuestMapPin ), wxT( "Create new map pin" ) );
		menu->Append( GetID( EventID_CreateQuestItemTag ), wxT( "Create new item tag" ) );
		menu->Append( GetID( EventID_CreateQuestMonsterTag ), wxT( "Create new enemy tag" ) );
		menu->Append( GetID( EventID_CreateQuestObjective ), wxT( "Create new objective" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_SortChildren ), wxT( "DBG - Print children" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DuplicateQuestObjective ), wxT( "Duplicate objective" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete objective" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestMapPin >() )
	{
		menu->Append( GetID( EventID_CreateQuestMapPin ), wxT( "Create new map pin" ) );  
		menu->Append( GetID( EventID_CreateQuestItemTag ), wxT( "Create new item tag" ) );
		menu->Append( GetID( EventID_CreateQuestMonsterTag ), wxT( "Create new enemy tag" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DuplicateQuestMapPin ), wxT( "Duplicate map pin" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete map pin" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestItemTag >() )
	{
		menu->Append( GetID( EventID_CreateQuestMapPin ), wxT( "Create new map pin" ) );
		menu->Append( GetID( EventID_CreateQuestItemTag ), wxT( "Create new item tag" ) );
		menu->Append( GetID( EventID_CreateQuestMonsterTag ), wxT( "Create new enemy tag" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete item tag" ) );
	}
	else if( itemData->m_entry->IsA< CJournalQuestEnemyTag >() )
	{
		menu->Append( GetID( EventID_CreateQuestMapPin ), wxT( "Create new map pin" ) );
		menu->Append( GetID( EventID_CreateQuestItemTag ), wxT( "Create new item tag" ) );
		menu->Append( GetID( EventID_CreateQuestMonsterTag ), wxT( "Create new enemy tag" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete enemy tag" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCharacterRoot >() )
	{
		menu->Append( GetID( EventID_CreateCharacterGroup ), wxT( "Create new character group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCharacterGroup >() )
	{
		menu->Append( GetID( EventID_CreateCharacter ), wxT( "Create new character" ) );
		menu->Append( GetID( EventID_CreateCharacterGroup ), wxT( "Create new character group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCharacter >() )
	{
		menu->Append( GetID( EventID_CreateCharacterDescription ), wxT( "Create new character description" ) );
		menu->Append( GetID( EventID_CreateCharacter ), wxT( "Create new character" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCharacterDescription >() )
	{
		menu->Append( GetID( EventID_CreateCharacterDescription ), wxT( "Create new character description" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalGlossaryRoot >() )
	{
		menu->Append( GetID( EventID_CreateGlossaryGroup ), wxT( "Create new glossary group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalGlossaryGroup >() )
	{
		menu->Append( GetID( EventID_CreateGlossaryEntry ), wxT( "Create new glossary entry" ) );
		menu->Append( GetID( EventID_CreateGlossaryGroup ), wxT( "Create new glossary group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalGlossary >() )
	{
		menu->Append( GetID( EventID_CreateGlossaryEntryDescription ), wxT( "Create new glossary description" ) );
		menu->Append( GetID( EventID_CreateGlossaryEntry ), wxT( "Create new glossary entry" ) );
	}
	else if( itemData->m_entry->IsA< CJournalGlossaryDescription >() )
	{
		menu->Append( GetID( EventID_CreateGlossaryEntryDescription ), wxT( "Create new glossary description" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalTutorialRoot >() )
	{
		menu->Append( GetID( EventID_CreateTutorialGroup ), wxT( "Create new tutorial group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalTutorialGroup >() )
	{
		menu->Append( GetID( EventID_CreateTutorialEntry ), wxT( "Create new tutorial entry" ) );
		menu->Append( GetID( EventID_CreateTutorialGroup ), wxT( "Create new tutorial group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalTutorial >() )
	{
		menu->Append( GetID( EventID_CreateTutorialEntry ), wxT( "Create new tutorial entry" ) );
	}
	else if( itemData->m_entry->IsA< CJournalItemRoot >() )
	{
		menu->Append( GetID( EventID_CreateItemGroup ), wxT( "Create new item group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalItemGroup >() )
	{
		menu->Append( GetID( EventID_CreateItemSubGroup ), wxT( "Create new item sub group" ) );
		menu->Append( GetID( EventID_CreateItemGroup ), wxT( "Create new item group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalItemSubGroup >() )
	{
		menu->Append( GetID( EventID_CreateItemEntry ), wxT( "Create new item" ) );
		menu->Append( GetID( EventID_CreateItemSubGroup ), wxT( "Create new item sub group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalItem >() )
	{
		menu->Append( GetID( EventID_CreateItemComponent ), wxT( "Create new item ingredient" ) );
		menu->Append( GetID( EventID_CreateItemEntry ), wxT( "Create new item" ) );
	}
	else if( itemData->m_entry->IsA< CJournalItemComponent >() )
	{
		menu->Append( GetID( EventID_CreateItemComponent ), wxT( "Create new item ingredient" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete ingredient" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureRoot >() )
	{
		menu->Append( GetID( EventID_CreateCreatureGroup ), wxT( "Create new creature group" ) );
		menu->Append( GetID( EventID_CreateCreatureVirtualGroup ), wxT( "Create new creature virtual group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreature ), wxT( "Create new creature" ) );
		menu->Append( GetID( EventID_CreateCreatureGroup ), wxT( "Create new creature group" ) );
		menu->Append( GetID( EventID_CreateCreatureVirtualGroup ), wxT( "Create new creature virtual group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureVirtualGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreature ), wxT( "Create new creature" ) );
		menu->Append( GetID( EventID_CreateCreatureGroup ), wxT( "Create new creature group" ) );
		menu->Append( GetID( EventID_CreateCreatureVirtualGroup ), wxT( "Create new creature virtual group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreature >() )
	{
		menu->Append( GetID( EventID_CreateCreature ), wxT( "Create new creature" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureDescriptionGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreatureDescription ), wxT( "Create new description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureDescriptionEntry >() )
	{
		menu->Append( GetID( EventID_CreateCreatureDescription ), wxT( "Create new description" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}

	else if( itemData->m_entry->IsA< CJournalCreatureVitalSpotGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreatureVitalSpot ), wxT( "Create new vital spot" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureVitalSpotEntry >() )
	{
		menu->Append( GetID( EventID_CreateCreatureVitalSpot ), wxT( "Create new vital spot" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}

	else if( itemData->m_entry->IsA< CJournalCreatureHuntingClueGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreatureHuntingClue ), wxT( "Create new clue" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureHuntingClue >() )
	{
		menu->Append( GetID( EventID_CreateCreatureHuntingClue ), wxT( "Create new clue" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete clue" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureGameplayHintGroup >() )
	{
		menu->Append( GetID( EventID_CreateCreatureGameplayHint ), wxT( "Create new gameplay hint" ) );
	}
	else if( itemData->m_entry->IsA< CJournalCreatureGameplayHint >() )
	{
		menu->Append( GetID( EventID_CreateCreatureGameplayHint ), wxT( "Create new gameplay hint" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete gameplay hint" ) );
	}
	else if( itemData->m_entry->IsA< CJournalStoryBookRoot >() )
	{
		menu->Append( GetID( EventID_CreateStoryBookChapter ), wxT( "Create new chapter" ) );
	}
	else if( itemData->m_entry->IsA< CJournalStoryBookChapter >() )
	{
		menu->Append( GetID( EventID_CreateStoryBookChapter ), wxT( "Create new chapter" ) );
		menu->Append( GetID( EventID_CreateStoryBookPage ), wxT( "Create new page" ) );
	}
	else if( itemData->m_entry->IsA< CJournalStoryBookPage >() )
	{
		menu->Append( GetID( EventID_CreateStoryBookPage ), wxT( "Create new page" ) );
		menu->Append( GetID( EventID_CreateStoryBookPageDescription ), wxT( "Create new page description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalStoryBookPageDescription >() )
	{
		menu->Append( GetID( EventID_CreateStoryBookPageDescription ), wxT( "Create new page description" ) );
	}
	else if( itemData->m_entry->IsA< CJournalPlaceRoot >() )
	{
		menu->Append( GetID( EventID_CreatePlaceGroup ), wxT( "Create new place group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalPlaceGroup >() )
	{
		menu->Append( GetID( EventID_CreatePlace ), wxT( "Create new place" ) );
		menu->Append( GetID( EventID_CreatePlaceGroup ), wxT( "Create new place group" ) );
	}
	else if( itemData->m_entry->IsA< CJournalPlace >() )
	{
		menu->Append( GetID( EventID_CreatePlaceDescription ), wxT( "Create new place description" ) );
		menu->Append( GetID( EventID_CreatePlace ), wxT( "Create new place" ) );
	}
	else if( itemData->m_entry->IsA< CJournalPlaceDescription >() )
	{
		menu->Append( GetID( EventID_CreatePlaceDescription ), wxT( "Create new place description" ) );
		menu->AppendSeparator();
		menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete description" ) );
	}
}

void CEdJournalEditor::PopulateCommonMenuOptions( const wxTreeItemId& item, wxMenu* menu ) const
{
	CEdJournalTreeItemData* resourceItemData = NULL;

	wxTreeItemId resourceItem;
	m_Tree->GetResourceItem( item, resourceItem, &resourceItemData );

	if( resourceItemData && resourceItemData->m_resource )
	{
		CDiskFile* file = resourceItemData->m_resource->GetFile();
		if( file )
		{
			if( file->IsLocal() )
			{
				// In theory we shouldn't ever have anything not in perforce, but if a file is added outside the editor then it'll appear as local
				PopulateMenuCreationOptions( item, menu );
				menu->AppendSeparator();

				menu->Append( GetID( EventID_SC_Save ), wxT( "Save and Add to Source Control\tctrl+s" ) );
				menu->AppendSeparator();
				menu->Append( GetID( EventID_SC_Delete ), wxT( "Delete" ) );
			}
			else if( file->IsCheckedIn() )
			{
				menu->Append( GetID( EventID_SC_CheckOut ), wxT( "Check out\tctrl-o" ) );
				menu->AppendSeparator();
				menu->Append( GetID( EventID_SC_Delete ), wxT( "Delete and Remove from Source Control" ) );
			}
			else if( file->IsCheckedOut() )
			{
				PopulateMenuCreationOptions( item, menu );
				menu->AppendSeparator();

				menu->Append( GetID( EventID_SC_Save ), wxT( "Save\tctrl+s" ) );
				menu->Append( GetID( EventID_SC_SaveAndCheckin ), wxT( "Save and Check in\tctrl+shift+c" ) );
				menu->Append( GetID( EventID_SC_Revert ), wxT( "Revert" ) );
				menu->AppendSeparator();
				menu->Append( GetID( EventID_SC_Delete ), wxT( "Delete and Remove from Source Control" ) );
			}
			else if( file->IsAdded() )
			{
				PopulateMenuCreationOptions( item, menu );
				menu->AppendSeparator();

				menu->Append( GetID( EventID_SC_Save ), wxT( "Save\tctrl+s" ) );
				menu->Append( GetID( EventID_SC_SaveAndCheckin ), wxT( "Save and Check in\tctrl+shift+c" ) );
				menu->AppendSeparator();
				menu->Append( GetID( EventID_SC_Delete ), wxT( "Delete and Remove from Source Control" ) );
			}
			else if( file->IsNotSynced() )
			{
				menu->Append( GetID( EventID_SC_NotSynced ), wxT( "Not synced" ) );
			}
		}
		else
		{
			// No file - newly created
			PopulateMenuCreationOptions( item, menu );

			menu->AppendSeparator();
			menu->Append( GetID( EventID_SC_Save ), wxT( "Save and Add to Source Control" ) );
			
			CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );

			if( itemData == resourceItemData )
			{
				menu->AppendSeparator();
				menu->Append( GetID( EventID_DeleteItem ), wxT( "Delete Item" ) );
			}
		}
	}
	else
	{
		PopulateMenuCreationOptions( item, menu );
	}
}

// The addition is done in this weird order to avoid overflow
Uint32 CEdJournalEditor::CalculateOrderFront( const wxTreeItemId& parentItem ) const
{
	Uint32 b = 0xffffffff;

	wxTreeItemIdValue unused;
	wxTreeItemId firstChild = m_Tree->GetFirstChild( parentItem, unused );

	if( firstChild.IsOk() )
	{
		CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( firstChild ) );

		b = itemData->m_entry->GetOrder();
	}

	return CalculateOrder( 0, b );
}

Uint32 CEdJournalEditor::CalculateOrder( const wxTreeItemId& startItem ) const
{
	CEdJournalTreeItemData* itemData = NULL;

	// No uint max?
	Uint32 a = 0;
	Uint32 b = 0xffffffff;

	// Could be invalid, in which case assume no siblings and stick with default value
	if( startItem.IsOk() )
	{
		wxTreeItemId endItem = m_Tree->GetNextSibling( startItem );

		// Being invalid means we have the last item in the list
		if( endItem.IsOk() )
		{
			itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( endItem ) );
			ASSERT( itemData->m_entry != NULL );

			b = itemData->m_entry->GetOrder();
		}

		itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( startItem ) );

		ASSERT( itemData->m_entry != NULL );
		a = itemData->m_entry->GetOrder();
	}

	return CalculateOrder( a, b );
}

Uint32 CEdJournalEditor::CalculateOrder( Uint32 a, Uint32 b ) const
{
	ASSERT( a != b );
	
	// TODO: Space out orders when there's no space left
	ASSERT( a < b - 1 );

	Uint32 order = ( a / 2 );
	order += ( b / 2 );

	return order;
}

void CEdJournalEditor::VerifyParent( wxTreeItemId& parentItem, CJournalBase* journalEntry ) const
{
	CEdJournalTreeItemData* parentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) );
	ASSERT( parentData->m_entry != NULL );

	if( !journalEntry->IsParentClass( parentData->m_entry ) )
	{
		parentItem = m_Tree->GetItemParent( parentItem );
		ASSERT( journalEntry->IsParentClass( static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) )->m_entry ) );
	}
}

wxTreeItemId CEdJournalEditor::CreateNewResourceItem( wxTreeItemId parentItem, CJournalBase* journalEntry, CDirectory* directory )
{
	VerifyParent( parentItem, journalEntry );

	// Make sure the entry's parent belong to the current target DLC
	CEdJournalTreeItemData* parentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) );
	if ( parentData->m_directory && ( ( m_targetDLC.GetLength() == 0 && parentData->m_directory->GetDepotPath().BeginsWith( TXT("dlc\\") ) ) ||
									  ( m_targetDLC.GetLength() != 0 && !parentData->m_directory->GetDepotPath().BeginsWith( String::Printf( TXT("dlc\\%ls\\"), m_targetDLC.AsChar() ).AsChar() ) ) ) )
	{
		wxMessageBox( wxT("The parent node does not belong to the current target DLC") );
		journalEntry->Discard();
		return 0;
	}

	// Resource
	CJournalResource* journalResource = ::CreateObject< CJournalResource >();
	journalEntry->SetParent( journalResource );
	journalResource->Set( journalEntry );

	return CreateNewItem( parentItem, journalEntry, journalResource, directory );
}

wxTreeItemId CEdJournalEditor::CreateNewSubItem( wxTreeItemId parentItem, CJournalContainerEntry* journalEntry, const Char* baseName /*= NULL*/ )
{
	VerifyParent( parentItem, journalEntry );

	// Grab parent quest
	CEdJournalTreeItemData* parentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) );
	ASSERT( parentData->m_entry != NULL );

	// Make sure the entry's parent belong to the current target DLC
	if ( parentData->m_directory && ( ( m_targetDLC.GetLength() == 0 && parentData->m_directory->GetDepotPath().BeginsWith( TXT("dlc\\") ) ) ||
									  ( m_targetDLC.GetLength() != 0 && !parentData->m_directory->GetDepotPath().BeginsWith( String::Printf( TXT("dlc\\%ls\\"), m_targetDLC.AsChar() ).AsChar() ) ) ) )
	{
		wxMessageBox( wxT("The parent node does not belong to the current target DLC") );
		journalEntry->Discard();
		return 0;
	}

	wxTreeItemId retVal = CreateNewItem( parentItem, journalEntry, NULL, NULL, baseName );

	ASSERT( parentData->m_entry->IsA< CJournalContainer >() );

	CJournalContainer* parent = static_cast< CJournalContainer* >( parentData->m_entry );

	// Add to quest
	parent->AddChildEntry( journalEntry );

	return retVal;
}

wxTreeItemId CEdJournalEditor::CreateNewItem( wxTreeItemId parentItem, CJournalBase* journalEntry, CJournalResource* resource, CDirectory* directory, const Char* baseName /*= NULL*/ )
{
	// Order
	wxTreeItemId lastChildItem = m_Tree->GetLastChild( parentItem );
	Uint32 order = CalculateOrder( lastChildItem );

	if( journalEntry->IsA< CJournalChildBase >() )
	{
		// Grab Parent
		CEdJournalTreeItemData* parentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) );
	
		// Upcast
		CJournalChildBase* child = static_cast< CJournalChildBase* >( journalEntry );

		// Initialise
		child->Initialize( parentData->m_entry->GetGUID(), order, baseName );
	}
	else
	{
		journalEntry->Initialize( order );
	}

	// Add to tree
	wxTreeItemId newItem = m_Tree->AddItemAppend( parentItem, journalEntry->GetName().AsChar(), new CEdJournalTreeItemData( journalEntry, resource, directory ) );

	// Select
	m_Tree->SelectItem( newItem );

	// Mark as modified
	MarkItemModified( newItem );

	return newItem;
}

void CEdJournalEditor::OnCreateNewQuestGroup( wxEvent& event )
{
	CJournalQuestGroup* group = ::CreateObject< CJournalQuestGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Quest ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetQuestsDirectories() ) );
}

void CEdJournalEditor::OnCreateNewQuest( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuest* quest = ::CreateObject< CJournalQuest >();
	wxTreeItemId newQuestItem = CreateNewResourceItem( m_popupMenuItem, quest, GetTargetDLCDirectoryFrom( CEdJournalTree::GetQuestsDirectories() ) );
	if ( !newQuestItem.IsOk() )
	{
		return;
	}

	CJournalQuestDescriptionGroup* descriptionsGroup	= ::CreateObject< CJournalQuestDescriptionGroup >();
	CreateNewSubItem( newQuestItem, descriptionsGroup );
	m_Tree->SelectItem( quest );

}

void CEdJournalEditor::OnCreateNewQuestPhase( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestPhase* phase = ::CreateObject< CJournalQuestPhase >();
	CreateNewSubItem( m_popupMenuItem, phase );
}

void CEdJournalEditor::OnCreateNewQuestDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestDescriptionEntry* entry = ::CreateObject< CJournalQuestDescriptionEntry >();
	CreateNewSubItem( m_popupMenuItem, entry );
}


void CEdJournalEditor::OnCreateNewQuestObjective( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestObjective* objective = ::CreateObject< CJournalQuestObjective >();
	CreateNewSubItem( m_popupMenuItem, objective );
}

void CEdJournalEditor::OnCreateNewQuestMapPin( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestMapPin* mappin = ::CreateObject< CJournalQuestMapPin >();
	CreateNewSubItem( m_popupMenuItem, mappin );
}

void CEdJournalEditor::OnCreateNewQuestItemTag( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestItemTag* tag = ::CreateObject< CJournalQuestItemTag >();
	CreateNewSubItem( m_popupMenuItem, tag );
}

void CEdJournalEditor::OnCreateNewQuestMonsterTag( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalQuestEnemyTag* tag = ::CreateObject< CJournalQuestEnemyTag >();
	CreateNewSubItem( m_popupMenuItem, tag );
}

void CEdJournalEditor::OnCreateNewCharacterGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCharacterGroup* character = ::CreateObject< CJournalCharacterGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Character ), character, GetTargetDLCDirectoryFrom( CEdJournalTree::GetCharactersDirectories() ) );
}

void CEdJournalEditor::OnCreateNewCharacter( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCharacter* character = ::CreateObject< CJournalCharacter >();
	CreateNewResourceItem( m_popupMenuItem, character, GetTargetDLCDirectoryFrom( CEdJournalTree::GetCharactersDirectories() ) );
}

void CEdJournalEditor::OnCreateNewCharacterDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCharacterDescription* character = ::CreateObject< CJournalCharacterDescription >();
	CreateNewSubItem( m_popupMenuItem, character );
}

void CEdJournalEditor::OnCreateNewGlossaryGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalGlossaryGroup* group = ::CreateObject< CJournalGlossaryGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Glossary ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetGlossaryDirectories() ) );
}

void CEdJournalEditor::OnCreateNewGlossaryEntry( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalGlossary* entry = ::CreateObject< CJournalGlossary >();
	CreateNewResourceItem( m_popupMenuItem, entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetGlossaryDirectories() ) );
}

void CEdJournalEditor::OnCreateNewGlossaryDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalGlossaryDescription* entry = ::CreateObject< CJournalGlossaryDescription >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewTutorialGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalTutorialGroup* group = ::CreateObject< CJournalTutorialGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Tutorial ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetTutorialDirectories() ) );
}

void CEdJournalEditor::OnCreateNewTutorialEntry( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalTutorial* entry = ::CreateObject< CJournalTutorial >();
	CreateNewResourceItem( m_popupMenuItem, entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetTutorialDirectories() ) );
}

void CEdJournalEditor::OnCreateNewItemGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalItemGroup* entry = ::CreateObject< CJournalItemGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Items ), entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetItemsDirectories() ) );
}

void CEdJournalEditor::OnCreateNewItemSubGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalItemSubGroup* entry = ::CreateObject< CJournalItemSubGroup >();
	CreateNewResourceItem( m_popupMenuItem, entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetItemsDirectories() ) );
}

void CEdJournalEditor::OnCreateNewItemEntry( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalItem* entry = ::CreateObject< CJournalItem >();
	CreateNewResourceItem( m_popupMenuItem, entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetItemsDirectories() ) );
}

void CEdJournalEditor::OnCreateNewItemComponent( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalItemComponent* entry = ::CreateObject< CJournalItemComponent >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewCreatureGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureGroup* group = ::CreateObject< CJournalCreatureGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Creatures ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetCreaturesDirectories() ) );
}

void CEdJournalEditor::OnCreateNewCreatureVirtualGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureVirtualGroup* group = ::CreateObject< CJournalCreatureVirtualGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Creatures ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetCreaturesDirectories() ) );
}

void CEdJournalEditor::OnCreateNewCreature( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreature* entry = ::CreateObject< CJournalCreature >();
	wxTreeItemId newCreatureItem = CreateNewResourceItem( m_popupMenuItem, entry, GetTargetDLCDirectoryFrom( CEdJournalTree::GetCreaturesDirectories() ) );
	if ( !newCreatureItem.IsOk() )
	{
		return;
	}

	CJournalCreatureDescriptionGroup* descriptionGroup		= ::CreateObject< CJournalCreatureDescriptionGroup >();
	CJournalCreatureHuntingClueGroup* huntingClueGroup		= ::CreateObject< CJournalCreatureHuntingClueGroup >();
	CJournalCreatureGameplayHintGroup* gameplayhintGroup	= ::CreateObject< CJournalCreatureGameplayHintGroup >();
	CJournalCreatureVitalSpotGroup* vitalSpotGroup			= ::CreateObject< CJournalCreatureVitalSpotGroup >();

	CreateNewSubItem( newCreatureItem, descriptionGroup );
	CreateNewSubItem( newCreatureItem, huntingClueGroup );
	CreateNewSubItem( newCreatureItem, gameplayhintGroup );
	CreateNewSubItem( newCreatureItem, vitalSpotGroup );

	m_Tree->SelectItem( newCreatureItem );
}

void CEdJournalEditor::OnCreateNewCreatureDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureDescriptionEntry* entry = ::CreateObject< CJournalCreatureDescriptionEntry >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewCreatureHuntingClue( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureHuntingClue* entry = ::CreateObject< CJournalCreatureHuntingClue >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewCreatureGameplayHint( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureGameplayHint* entry = ::CreateObject< CJournalCreatureGameplayHint >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewCreatureVitalSpot( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalCreatureVitalSpotEntry* entry = ::CreateObject< CJournalCreatureVitalSpotEntry >();
	CreateNewSubItem( m_popupMenuItem, entry );
}

void CEdJournalEditor::OnCreateNewStoryBookChapter( wxEvent& event )
{
	CJournalStoryBookChapter* chapter = ::CreateObject< CJournalStoryBookChapter >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_StoryBook ), chapter, GetTargetDLCDirectoryFrom( CEdJournalTree::GetStoryBookDirectories() ) );
}

void CEdJournalEditor::OnCreateNewStoryBookPage( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalStoryBookPage* page = ::CreateObject< CJournalStoryBookPage >();
	CreateNewResourceItem( m_popupMenuItem, page, GetTargetDLCDirectoryFrom( CEdJournalTree::GetStoryBookDirectories() ) );
}

void CEdJournalEditor::OnCreateNewStoryBookPageDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalStoryBookPageDescription* pageDescription = ::CreateObject< CJournalStoryBookPageDescription >();
	CreateNewSubItem( m_popupMenuItem, pageDescription );
}

void CEdJournalEditor::OnCreateNewPlaceGroup( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalPlaceGroup* group = ::CreateObject< CJournalPlaceGroup >();
	CreateNewResourceItem( m_Tree->GetCategoryRootItem( CEdJournalTree::TreeCategory_Places ), group, GetTargetDLCDirectoryFrom( CEdJournalTree::GetPlacesDirectories() ) );
}

void CEdJournalEditor::OnCreateNewPlace( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalPlace* place = ::CreateObject< CJournalPlace >();
	CreateNewResourceItem( m_popupMenuItem, place, GetTargetDLCDirectoryFrom( CEdJournalTree::GetPlacesDirectories() ) );
}

void CEdJournalEditor::OnCreateNewPlaceDescription( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CJournalPlaceDescription* placeDescription = ::CreateObject< CJournalPlaceDescription >();
	CreateNewSubItem( m_popupMenuItem, placeDescription );
}

void CEdJournalEditor::OnPopupDuplicateQuestObjective( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( m_popupMenuItem ) );
	if ( itemData )
	{
		CJournalBase* base = itemData->m_entry;
		if ( base && base->IsA< CJournalQuestObjective >() )
		{
			String newBaseName = base->GetName() + TXT(" Copy");
			CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( base->Clone( base->GetParent() ) );
			objective->Reset();
			wxTreeItemId objectiveItem = CreateNewSubItem( m_popupMenuItem, objective, newBaseName.AsChar() );

			m_Tree->PopulateDuplicatedTreeContainers( objectiveItem, objective, true );
		}
	}
}

void CEdJournalEditor::OnPopupDuplicateQuestMapPin( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( m_popupMenuItem ) );
	if ( itemData )
	{
		CJournalBase* base = itemData->m_entry;
		if ( base && base->IsA< CJournalQuestMapPin >()  )
		{
			String newBaseName = base->GetName() + TXT(" Copy");
			CJournalQuestMapPin* mappin = Cast< CJournalQuestMapPin >( base->Clone( base->GetParent() ) );
			mappin->RecreateGUID();
			CreateNewSubItem( m_popupMenuItem, mappin, newBaseName.AsChar() );
		}
	}
}

void CEdJournalEditor::OnPopupSortChildren( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( m_popupMenuItem ) );
	if ( itemData )
	{
		CJournalContainer* container = Cast< CJournalContainer >( itemData->m_entry );
		if ( container )
		{
			RED_LOG( RED_LOG_CHANNEL( JournalOrder ), TXT("------------------") );
			for ( Uint32 i = 0; i < container->GetNumChildren(); ++i )
			{
				CJournalContainerEntry* entry = Cast< CJournalContainerEntry >( container->GetChild( i ) );
				if ( entry )
				{
					RED_LOG( RED_LOG_CHANNEL( JournalOrder ), TXT("%d %d %s"), i, entry->GetIndex(), entry->GetName().AsChar() );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( JournalOrder ), TXT("%d huh?"), i );
				}
			}

		}
	}
}

wxTextCtrl* CEdJournalEditor::GetWidget( wxCommandEvent& event )
{
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( wxWindow ) ) );

	wxWindow* buttonWindow = static_cast< wxWindow* >( eventObject );

	// Can't do much type checking with a void pointer unfortunately :(
	SEdWidgetData* data = static_cast< SEdWidgetData* >( buttonWindow->GetClientData() );
	ASSERT( data != NULL );
	ASSERT( data->dataType == SEdWidgetData::DT_WidgetPtr );

	return static_cast< wxTextCtrl* >( data->widget );
}

void CEdJournalEditor::OnSelectItemDialog( wxCommandEvent& event )
{
	wxTextCtrl* textCtrl = GetWidget( event );

	CName item( textCtrl->GetValue() );

	CEdChooseItemDialog* dialog = new CEdChooseItemDialog( this, item );
	dialog->SetClientData( textCtrl );

	dialog->Bind( wxEVT_CHOOSE_ITEM_OK, wxCommandEventHandler( CEdJournalEditor::OnSelectItemDialogOK ), this );

	dialog->ShowModal();
}

void CEdJournalEditor::OnSelectItemDialogOK( wxCommandEvent& event )
{
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( CEdChooseItemDialog ) ) );

	CEdChooseItemDialog* dialog = static_cast< CEdChooseItemDialog* >( eventObject );

	wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( dialog->GetClientData() );
	
	// Call SetValue() and NOT ChangeValue() as we want a text change event to spawn
	textCtrl->SetValue( event.GetString() );
}

void CEdJournalEditor::OnSelectItemsDialog( wxCommandEvent& event )
{
	wxTextCtrl* textCtrl = GetWidget( event );

	CEdChooseItemDialog* dialog = new CEdChooseItemDialog( this, CName::NONE );
	dialog->SetClientData( textCtrl );
	dialog->Bind( wxEVT_CHOOSE_ITEM_OK, wxCommandEventHandler( CEdJournalEditor::OnSelectItemsDialogOK ), this );
	dialog->ShowModal();
}

void CEdJournalEditor::OnSelectItemsDialogOK( wxCommandEvent& event )
{
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( CEdChooseItemDialog ) ) );

	CEdChooseItemDialog* dialog = static_cast< CEdChooseItemDialog* >( eventObject );
	wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( dialog->GetClientData() );

	const wxString& newItem = event.GetString();
	if ( newItem.empty() )
	{
		return;
	}
	wxString items = textCtrl->GetValue();
	if ( !items.empty() )
	{
		items += wxString( "\n" );
	}
	items += newItem;

	// Call SetValue() and NOT ChangeValue() as we want a text change event to spawn
	textCtrl->SetValue( items );
}

void CEdJournalEditor::OnSelectStringDialog( wxCommandEvent& event )
{
	if( !m_stringSelectorTool )
	{
		m_stringSelectorTool = new CEdStringSelector( this );

		m_stringSelectorTool->Bind( wxEVT_CHOOSE_STRING_OK, wxCommandEventHandler( CEdJournalEditor::OnSelectStringDialogOK ), this );
		m_stringSelectorTool->Bind( wxEVT_CHOOSE_STRING_CANCEL, wxEventHandler( CEdJournalEditor::OnSelectStringDialogCancel ), this );
		 
		wxTextCtrl* textCtrl = GetWidget( event );
		m_stringSelectorTool->SetClientData( textCtrl );

		m_stringSelectorTool->Show();
	}
}

void CEdJournalEditor::OnSelectStringDialogOK( wxCommandEvent& event )
{
	// Set string id in resource
	wxTreeItemId selectedItem = m_Tree->GetSelection();

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );

	Uint32 stringID = event.GetInt();

	ASSERT( event.GetEventObject() == m_stringSelectorTool );

	LOG_EDITOR( TXT( "STRING SELECTED: %u" ), stringID );

	wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( m_stringSelectorTool->GetClientData() );
	
	SEdWidgetData* data = static_cast< SEdWidgetData* >( textCtrl->GetClientData() );

	LocalizedString intermediary;
	intermediary.SetIndex( stringID );

	SetProperty( data, intermediary );

	// Fill text control with localised string
	textCtrl->SetValue( intermediary.GetString().AsChar() );

	m_stringSelectorTool = NULL;
}

void CEdJournalEditor::OnSelectStringDialogCancel( wxEvent& event )
{
	m_stringSelectorTool = NULL;
}

void CEdJournalEditor::OnFilenameChange( wxCommandEvent& event )
{
	wxTreeItemId selectedItem = m_Tree->GetSelection();
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );

	itemData->m_filename = String( event.GetString() );
	// !!! FILENAMES NEED TO BE LOWERCASE !!!
	itemData->m_filename.MakeLower();

	event.Skip();
}

SEdWidgetData* CEdJournalEditor::GetWidgetDataFromEvent( wxCommandEvent& event )
{
	// I would have thought that a widget would pass it's Client Data pointer through to an
	// event it creates, but apparently not, so we have to go via this route instead

	// Grab originating control
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( wxWindow ) ) );

	wxWindow* windowObject = static_cast< wxWindow* >( eventObject );

	// Grab property data
	return static_cast< SEdWidgetData* >( windowObject->GetClientData() );
}

template< typename T >
void CEdJournalEditor::SetProperty( SEdWidgetData* propData, const T& propertyValue )
{
	ASSERT( propData->dataType == SEdWidgetData::DT_PropertyName );

	wxTreeItemId selectedItem = m_Tree->GetSelection();

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );

	// Grab Property
	CClass* rtticlass = itemData->m_entry->GetClass();
	CProperty* prop = rtticlass->FindProperty( propData->propertyName );

	prop->Set( itemData->m_entry, &propertyValue );

	MarkItemModified( selectedItem );
}

template< typename T >
void CEdJournalEditor::SetProperty( wxCommandEvent& event, const T& propertyValue )
{
	SEdWidgetData* propData = GetWidgetDataFromEvent( event );

	SetProperty( propData, propertyValue );

	event.Skip();
}

void CEdJournalEditor::OnStringChange( wxCommandEvent& event )
{
	// convert to red string
	String str( event.GetString() );
	SetProperty( event, str );
}

void CEdJournalEditor::OnUintChange( wxCommandEvent& event )
{
	// convert to red string
	String str( event.GetString() );

	// Get Number
	Uint32 value;
	FromString( str, value );

	SetProperty( event, value );

	event.Skip();
}

void CEdJournalEditor::OnChoiceChange( wxCommandEvent& event )
{
	Uint32 choice = event.GetInt();
	SetProperty( event, choice );

	OnChangeQuestTypeProperty( event );
}

void CEdJournalEditor::OnQuestObjectiveTypeChange( wxCommandEvent& event )
{
	OnChoiceChange( event );
}

void CEdJournalEditor::OnLocalizedStringTextChange( wxCommandEvent& event )
{
	wxTreeItemId selectedItem = m_Tree->GetSelection();

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );

	// Grab originating control
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( wxWindow ) ) );

	wxWindow* windowObject = static_cast< wxWindow* >( eventObject );

	// convert to red string
	String stringData( event.GetString() );

	// Grab property data
	SEdWidgetData* data = static_cast< SEdWidgetData* >( windowObject->GetClientData() );

	// Grab localised string
	LocalizedString* localizedString = GetPropertyPointer< LocalizedString >( itemData->m_entry, windowObject );

	// Grab resource parent
	wxTreeItemId resourceItem;
	CEdJournalTreeItemData* resourceItemData;
	m_Tree->GetResourceItem( selectedItem, resourceItem, &resourceItemData );
	ASSERT( resourceItemData != NULL, TXT( "There must be an item with a resource somewhere up the tree if a user is editing a field!" ) );

	// Altered strings need to be associated with the resource item
	TLocalizedStringEntryArray& itemLocalizedStringChanges = m_localizedStringChanges.GetRef( resourceItemData->m_entry->GetGUID() );

	// Associate string with journal and journal resource
	LocalizedStringEntry entry( localizedString, JOURNAL_STRINGDB_PROPERTY_NAME, resourceItemData->m_resource );
	itemLocalizedStringChanges.PushBackUnique( entry );

	// Pass the string through
	localizedString->SetString( stringData );

	MarkItemModified( selectedItem );

	event.Skip();
}

void CEdJournalEditor::OnNameChange( wxCommandEvent& event )
{
	// Change text in tree entry (preserve any DLC name too)
	wxTreeItemId selectedItem = m_Tree->GetSelection();
	wxString previousName = m_Tree->GetItemText( selectedItem );
	int index = previousName.Index( wxT(" (dlc:") );
	if ( index != -1 )
	{
		wxString dlcName = previousName.Mid( index );
		m_Tree->SetItemText( selectedItem, event.GetString() + dlcName );
	}
	else
	{
		m_Tree->SetItemText( selectedItem, event.GetString() );
	}

	OnStringChange( event );
}

void CEdJournalEditor::OnCNameChanged( wxCommandEvent& event )
{
	// convert to red string
	String str( event.GetString() );

	CName name( str );

	SetProperty( event, name );
}

void CEdJournalEditor::OnCheckBoxChange( wxCommandEvent& event )
{
	SetProperty( event, event.IsChecked() );

	// set property to be saved in separate file
	OnChangeActiveProperty( event );
}

void CEdJournalEditor::OnFloatChange( wxCommandEvent& event )
{
	// convert to red string
	String str( event.GetString() );

	// Get Number
	Float value;
	FromString( str, value );

	SetProperty( event, value );

	event.Skip();
}

void CEdJournalEditor::OnPathChanged( wxCommandEvent& event )
{
	// Grab originating control
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( CJournalSelectorWidget ) ) );
	
	CJournalSelectorWidget* widget = static_cast< CJournalSelectorWidget* >( eventObject );
	
	THandle< CJournalPath > path = widget->GetPath();

	wxTreeItemId selectedItem = m_Tree->GetSelection();
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );

	SetProperty( static_cast< SEdWidgetData* >( widget->GetClientData() ), path );

	event.Skip();
}

void CEdJournalEditor::OnPathCleared( wxCommandEvent& event )
{
	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject != NULL );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( CJournalSelectorWidget ) ) );

	CJournalSelectorWidget* widget = static_cast< CJournalSelectorWidget* >( eventObject );

	wxTreeItemId selectedItem = m_Tree->GetSelection();
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );

	SetProperty( static_cast< SEdWidgetData* >( widget->GetClientData() ), THandle< CJournalPath >() );

	event.Skip();
}

void CEdJournalEditor::OnObjectHandleChange( wxCommandEvent& event )
{
	String resource( event.GetString() );

	wxTreeItemId selectedItem = m_Tree->GetSelection();

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData );

	wxObject* eventObject = event.GetEventObject();
	ASSERT( eventObject );
	ASSERT( eventObject->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) );

	wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( eventObject );
	ASSERT( textCtrl );

	SEdWidgetData* data = static_cast< SEdWidgetData* >( textCtrl->GetClientData() );
	ASSERT( data );

	CClass* rtticlass = itemData->m_entry->GetClass();
	ASSERT( rtticlass );
	CProperty* prop = rtticlass->FindProperty( data->propertyName );
	ASSERT( prop );

	CPropertyDataBuffer buffer( prop->GetType() );
	if ( prop->GetType()->FromString( buffer.Data(), resource ) )
	{
		prop->Set( itemData->m_entry, buffer.Data() );
		textCtrl->ChangeValue( resource.AsChar() );
		MarkItemModified( selectedItem );
	}

	event.Skip();
}

void CEdJournalEditor::OnUseObject( wxCommandEvent& event )
{
	String resource;
	if ( GetActiveResource( resource ) )
	{
		wxTreeItemId selectedItem = m_Tree->GetSelection();

		CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
		ASSERT( itemData );

		wxTextCtrl* textCtrl = GetWidget( event );
		ASSERT( textCtrl );
		SEdWidgetData* data = static_cast< SEdWidgetData* >( textCtrl->GetClientData() );
		ASSERT( data );

		CClass* rtticlass = itemData->m_entry->GetClass();
		ASSERT( rtticlass );
		CProperty* prop = rtticlass->FindProperty( data->propertyName );
		ASSERT( prop );

		CPropertyDataBuffer buffer( prop->GetType() );
		if ( prop->GetType()->FromString( buffer.Data(), resource ) )
		{
			prop->Set( itemData->m_entry, buffer.Data() );
			textCtrl->ChangeValue( resource.AsChar() );
			MarkItemModified( selectedItem );
		}
	}

	event.Skip();
}

void CEdJournalEditor::OnClearPath( wxCommandEvent& event )
{
	wxTreeItemId selectedItem = m_Tree->GetSelection();

	wxTextCtrl* textCtrl = GetWidget( event );
	ASSERT( textCtrl );

	if ( textCtrl && textCtrl->IsKindOf( wxCLASSINFO( CJournalSelectorWidget ) ) )
	{
		CJournalSelectorWidget* selector = static_cast< CJournalSelectorWidget* >( textCtrl );
		selector->OnItemCleared();
		selector->ChangeValue( TXT("") );
	}
}

void CEdJournalEditor::OnNameArrayChange( wxCommandEvent& event )
{
	// convert to red string
	String str( event.GetString() );
	
	TDynArray< CName > names;
	TDynArray< String > itemStrings = str.Split( TXT("\n"), true );
	for ( Uint32 i = 0; i < itemStrings.Size(); ++i )
	{
		if ( !itemStrings[ i ].Empty() )
		{
			names.PushBack( CName( itemStrings[ i ] ) );
		}
	}

	SetProperty( event, names );
}

void CEdJournalEditor::OnSaveAll( wxEvent& event )
{
	SaveAll();
}

void CEdJournalEditor::SaveAll()
{
	// Save Strings first - this may affect the ids which are written to the resource files
	SaveAllLocalizedStrings();

	Uint32 failedSaves = 0;

	TDynArray< wxTreeItemId > failedSaveItems;

	while( m_modifiedMenuItems.Size() > failedSaveItems.Size() )
	{
		wxTreeItemId modifiedMenuItem = m_modifiedMenuItems.PopBack();
		eError error = SaveItem( modifiedMenuItem );
		if( error != Error_None )
		{
			failedSaveItems.PushBack( modifiedMenuItem );
		}
	}

	m_modifiedMenuItems.CopyFast( failedSaveItems );

	SaveInitialEntriesResource();

	//TODO: Display failed saves to user
}

void CEdJournalEditor::CheckinAll()
{
	SaveAll();

	TDynArray< CDiskFile* > filesToSubmit;

	for( Uint32 i = 0; i < m_CheckedOutMenuItems.Size(); ++i )
	{
		CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( m_CheckedOutMenuItems[ i ] ) );
		ASSERT( itemData != NULL );
		ASSERT( itemData->m_resource != NULL );

		filesToSubmit.PushBack( itemData->m_resource->GetFile() );
	}
	
	CJournalInitialEntriesResource* resource = nullptr;

	for ( Uint32 i = 0; i < sizeof( m_initalEntriesResource ) / sizeof( m_initalEntriesResource[ 0 ] ); ++i )
	{
		CJournalInitialEntriesResource* resource = m_initalEntriesResource[ i ];
		if ( resource )
		{
			CDiskFile* file = resource->GetFile();
			if ( file )
			{
				if ( file->IsCheckedOut() || file->IsAdded() )
				{
					filesToSubmit.PushBack( file );
				}
			}
		}
	}

	if( filesToSubmit.Size() > 0 )
	{
		GVersionControl->Submit( filesToSubmit );
	}
}

void CEdJournalEditor::SaveAllLocalizedStrings()
{
	// If this points to an item, only those strings will be saved. If it's invalid, then all strings will be saved
	m_localizedStringsToSave = CGUID();

	SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );

	m_localizedStringChanges.Clear();
}

void CEdJournalEditor::OnCheckInAll( wxEvent& event )
{
	CheckinAll();
}

void CEdJournalEditor::OnSelectTargetDLC( wxEvent& event )
{
	TDynArray< String > targets;
	targets.PushBack( wxT("(global)") );

	Int32 selection = 0;

	// Scan DLC folder
	CDirectory* dlc = GDepot->FindPath( TXT("dlc\\") );
	if ( dlc != nullptr )
	{
		const Char* targetDLC = m_targetDLC.AsChar();
		for ( CDirectory* dlcDir : dlc->GetDirectories() )
		{
			const Char* dirName = dlcDir->GetName().AsChar();
			if ( Red::System::StringCompareNoCase( dirName, targetDLC ) == 0)
			{
				selection = targets.SizeInt();
			}
			targets.PushBack( dirName );
		}
	}

	// Make sure there is a DLC directory to select
	if ( targets.Size() == 1 )
	{
		wxMessageBox( wxT("No DLC directories found, please make a DLC dictory under dlc\\ first"), wxT("No DLC"), wxOK	| wxCENTER, this );
		return;
	}

	// Show a selection dialog for DLCs
	if ( FormattedDialogBox( this, wxT("Select Target DLC"), wxString::Format( wxT("H{L%s=200|V{B@'&OK'|B'&Cancel'}}=150"), CEdFormattedDialog::ArrayToList( targets ).AsChar() ), &selection ) != 0 )
	{
		return;
	}

	// Store the name and update title
	if ( selection == 0 )
	{
		SetTargetDLC( String::EMPTY );
	}
	else
	{
		SetTargetDLC( targets[selection] );
	}
}

void CEdJournalEditor::InitTargetDLC()
{
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("JournalTargetDLC"), m_targetDLC );

	if( m_targetDLC.Empty() )
	{
		SetTitle( wxT("Journal Editor") );
	}
	else
	{
		CDirectory* dlcJournal = GDepot->FindPath( String::Printf( TXT("dlc\\%ls\\journal\\"), m_targetDLC.AsChar() ) );
		if ( !dlcJournal )
		{
			m_targetDLC = String::EMPTY;
			SetTitle( wxT("Journal Editor") );
		}
		else
		{
			SetTitle( wxString::Format( wxT("Journal Editor (Target DLC: %s)"), m_targetDLC.AsChar() ) );
		}
	}	
}

void CEdJournalEditor::SetTargetDLC( const String& targetDLC )
{
	// Store the name and update title
	m_targetDLC = targetDLC;
	if ( m_targetDLC.Empty() )
	{
		SetTitle( wxT("Journal Editor") );
	}
	else
	{
		SetTitle( wxString::Format( wxT("Journal Editor (Target DLC: %s)"), m_targetDLC.AsChar() ) );

		// Make sure that the selected DLC has a journal directory
		CDirectory* dlcJournal = GDepot->FindPath( String::Printf( TXT("dlc\\%ls\\journal\\"), m_targetDLC.AsChar() ) );
		if ( !dlcJournal )
		{
			CDirectory* dir = GDepot->CreatePath( String::Printf( TXT("dlc\\%ls\\journal\\"), m_targetDLC.AsChar() ) );
			dir->CreateOnDisk();
		}
	}
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("JournalTargetDLC"), m_targetDLC );
}

void CEdJournalEditor::OnPopupSCCheckOut( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	CheckOutItem( m_popupMenuItem );
}

void CEdJournalEditor::OnPopupSCSaveItem( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );
	
	DisplayError( SaveItem( m_popupMenuItem ), m_popupMenuItem );	
}

void CEdJournalEditor::OnPopupSCSaveAndCheckIn( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );
	
	eError error = SaveItem( m_popupMenuItem );

	if( error == Error_None )
	{
		CEdJournalTreeItemData* itemData = NULL;
		wxTreeItemId resourceItem;
		m_Tree->GetResourceItem( m_popupMenuItem, resourceItem, &itemData );
		ASSERT( itemData && itemData->m_resource && itemData->m_resource->GetFile() );

		CDiskFile* file = itemData->m_resource->GetFile();
		file->Submit();

		MarkItemUnmodified( resourceItem );

		m_Tree->MarkItemLocked( m_popupMenuItem, true );
	}
	else
	{
		DisplayError( error, m_popupMenuItem );
	}
}

void CEdJournalEditor::OnPopupSCRevert( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );
	RevertItem( m_popupMenuItem );
}

void CEdJournalEditor::OnPopupSCDeleteItem( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );

	wxTreeItemId resourceItem;
	CEdJournalTreeItemData* itemData = NULL;
	m_Tree->GetResourceItem( m_popupMenuItem, resourceItem, &itemData );

	DeleteItem( resourceItem );
}

void CEdJournalEditor::OnPopupDeleteItem( wxEvent& event )
{
	ASSERT( m_popupMenuItem.IsOk() );
	DeleteItem( m_popupMenuItem );
}

void CEdJournalEditor::RevertItem( const wxTreeItemId& item )
{
	wxTreeItemId itemToRevert = item;
	CEdJournalTreeItemData* itemToRevertData = NULL;

	m_Tree->GetResourceItem( item, itemToRevert, &itemToRevertData );
	ASSERT( itemToRevertData );

	String confirmationMessage = String::Printf( TXT( "Are you sure you want to revert \"%s\" (Filename: %s.%s)?" ), itemToRevertData->m_entry->GetName().AsChar(), itemToRevertData->m_filename.AsChar(), itemToRevertData->m_resource->GetFileExtension() );

	if( itemToRevertData->m_entry->IsA< CJournalContainer >() )
	{
		CJournalContainer* container = static_cast< CJournalContainer* >( itemToRevertData->m_entry );

		String affectedChildren( TXT( "\nThis will also affect the following entries:\n\n" ) );

		for( Uint32 i = 0; i < container->GetNumChildren(); ++i )
		{
			affectedChildren += TXT( "-- " ) + container->GetChild( i )->GetName() + TXT( "\n" );
		}

		confirmationMessage += affectedChildren;
	}

	if( wxMessageBox( confirmationMessage.AsChar(), wxT( "Revert" ), wxYES_NO, this ) == wxYES )
	{
		itemToRevertData->m_resource->GetFile()->Revert();
		MarkItemUnmodified( itemToRevert );

		if( item == m_Tree->GetSelection() )
		{
			CategorySelectionChanged( item );
		}
	}
}

void CEdJournalEditor::CheckOutItem( const wxTreeItemId& item )
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );

	if( !itemData->m_resource )
	{
		wxTreeItemId parentItem = m_Tree->GetItemParent( item );
		ASSERT( parentItem.IsOk() );

		CheckOutItem( parentItem );
	}
	else
	{
		itemData->m_resource->GetFile()->CheckOut();

		CategorySelectionChanged( item );
	}

	m_Tree->MarkItemLocked( m_popupMenuItem, false );
}

CEdJournalEditor::eError CEdJournalEditor::SaveItem( const wxTreeItemId& menuItem )
{
	wxTreeItemId resourceItem;
	CEdJournalTreeItemData* resourceItemData = NULL;
	m_Tree->GetResourceItem( menuItem, resourceItem, &resourceItemData );
	ASSERT( resourceItemData != NULL, TXT( "It should be impossible for a user to save a journal entry that has no resource" ) );

	eError errorVal = Error_None;

	TLocalizedStringEntryArray* itemLocalizedStringChanges = m_localizedStringChanges.FindPtr( resourceItemData->m_entry->GetGUID() );

	if( itemLocalizedStringChanges )
	{
		m_localizedStringsToSave = resourceItemData->m_entry->GetGUID();

		SLocalizationManager::GetInstance().UpdateStringDatabase( this, true );
	}

	errorVal = SaveItem( resourceItemData );

	if( errorVal == Error_None )
	{
		MarkItemSaved( resourceItem );

		// If the user is saving the currently selected thing, refresh the widgets
		if( m_Tree->GetSelection() == menuItem )
		{
			CategorySelectionChanged( menuItem );
		}
	}

	return errorVal;
}

CEdJournalEditor::eError CEdJournalEditor::SaveItem( CEdJournalTreeItemData* itemData )
{
	ASSERT( itemData->m_resource != NULL );

	if( itemData->m_resource->GetFile() )
	{
		itemData->m_resource->Save();
	}
	else
	{
		ASSERT( itemData->m_directory != NULL );

		if( itemData->m_filename.Empty() )
		{
			// No filename defined
			LOG_EDITOR( TXT( "Cannot save: No filename specified for item -> %s" ), itemData->m_entry->GetName().AsChar() );

			return Error_NoFilename;
		}

		String filename = String::Printf( TXT( "%s.%s" ), itemData->m_filename.AsChar(), CJournalResource::GetFileExtension() );

		// Check filename
		if( itemData->m_directory->FindLocalFile( filename ) )
		{
			// File already exists
			LOG_EDITOR( TXT( "Cannot save: File already exists -> %s" ), filename.AsChar() );
			return Error_DuplicateFilename;
		}

		// Save
		itemData->m_resource->SaveAs( itemData->m_directory, filename );
	}

	// Add to source control
	CDiskFile* file = itemData->m_resource->GetFile();

	if( file )
	{
		// update source control status
		file->GetStatus();

		// Add it to source control if it isn't already
		if( file->IsLocal() )
		{
			file->Add();
		}
	}

	return Error_None;
}

void CEdJournalEditor::GetResourceChildren( const wxTreeItemId& item, TDynArray< CEdJournalTreeItemData* >& children ) const
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = m_Tree->GetFirstChild( item, cookie );

	while( child.IsOk() )
	{
		CEdJournalTreeItemData* childData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( child ) );
		ASSERT( childData != NULL );
		
		if( childData->m_resource )
		{
			children.PushBack( childData );
			
			// Recurse
			GetResourceChildren( child, children );
		}

		child = m_Tree->GetNextChild( item, cookie );
	}
}

CEdJournalEditor::eError CEdJournalEditor::DeleteItem( const wxTreeItemId& item )
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );
	ASSERT( itemData != NULL );

	String confirmationMessage;

	if( itemData->m_entry->IsA< CJournalContainerEntry >() )
	{
		confirmationMessage = String::Printf( TXT( "Are you sure you want to delete \"%s\" and any children it has?" ), itemData->m_entry->GetName().AsChar() );
		if( wxMessageBox( confirmationMessage.AsChar(), wxT( "Delete Item" ), wxYES_NO, this ) == wxYES )
		{
			// Child
			CJournalContainerEntry* child = static_cast< CJournalContainerEntry* >( itemData->m_entry );

			// Grab Parent
			wxTreeItemId parentItem = m_Tree->GetItemParent( item );
			CEdJournalTreeItemData* parentItemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( parentItem ) );
			ASSERT( parentItemData != NULL );
			
			if( parentItemData->m_entry->IsA< CJournalContainer >() )
			{
				CJournalContainer* parent = static_cast< CJournalContainer* >( parentItemData->m_entry );

				// Delete
				parent->RemoveChildEntry( child );
			}
			else
			{
				// Topmost container class
				ASSERT( itemData->m_resource );
				
				if( itemData->m_resource->GetFile() )
				{
					itemData->m_resource->GetFile()->Delete();
				}

				itemData->m_resource->SetParent( NULL );
			}

			// Remove the item that was just deleted
			m_modifiedMenuItems.Remove( item );

			m_Tree->DeleteItem( item );

			MarkItemModified( parentItem );
		}
	}
	else
	{
		ASSERT( itemData->m_resource );
		
		TDynArray< CEdJournalTreeItemData* > childrenToDelete;

		childrenToDelete.PushBack( itemData );
		GetResourceChildren( item, childrenToDelete );

		confirmationMessage = TXT( "Are you sure you want to delete the following files:\n" );
		String lineEnd = String::Printf( TXT( ".%s)\n" ), CJournalResource::GetFileExtension() );

		for( Uint32 i = 0; i < childrenToDelete.Size(); ++i )
		{
			confirmationMessage += childrenToDelete[ i ]->m_entry->GetName();

			if( childrenToDelete[ i ]->m_filename.Empty() )
			{
				confirmationMessage += TXT( " (No filename specified)\n" );
			}
			else
			{
				confirmationMessage += TXT( " (" ) + childrenToDelete[ i ]->m_filename + lineEnd;
			}
		}

		if( wxMessageBox( confirmationMessage.AsChar(), wxT( "Delete Item" ), wxYES_NO, this ) == wxYES )
		{
			for( Uint32 i = 0; i < childrenToDelete.Size(); ++i )
			{
				CDiskFile* file = childrenToDelete[ i ]->m_resource->GetFile();
				
				// File may be null if resource is newly created
				if( file != NULL )
				{
					file->Delete();
				}

				// For garbage collection
				childrenToDelete[ i ]->m_resource->SetParent( NULL );

				m_modifiedMenuItems.Remove( childrenToDelete[ i ]->GetId() );
			}

			// Remove the item that was just deleted
			m_modifiedMenuItems.Remove( item );

			m_Tree->DeleteItem( item );
		}

	}

	return Error_None;
}

void CEdJournalEditor::MarkItemModified( const wxTreeItemId& item, Bool force )
{
	if( m_Tree->MarkItemModified( item, force ) )
	{
		m_modifiedMenuItems.PushBackUnique( item );
		m_CheckedOutMenuItems.Remove( item );
	}
}

void CEdJournalEditor::MarkItemSaved( const wxTreeItemId& item, Bool force )
{
	if( m_Tree->MarkItemSaved( item, force ) )
	{
		m_modifiedMenuItems.Remove( item );
		m_CheckedOutMenuItems.PushBackUnique( item );
	}
}

void CEdJournalEditor::MarkItemUnmodified( const wxTreeItemId& item, Bool force )
{
	if( m_Tree->MarkItemUnmodified( item, force ) )
	{
		m_CheckedOutMenuItems.Remove( item );
	}
}

Bool CEdJournalEditor::CanModifyItem( const wxTreeItemId& item ) const
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );
	ASSERT( itemData != NULL );

	Bool enabled = false;

	if( itemData->m_resource )
	{
		return itemData->m_resource->CanModify();
	}
	else
	{
		wxTreeItemId parentItem = m_Tree->GetItemParent( item );
		ASSERT( parentItem.IsOk() );

		return CanModifyItem( parentItem );
	}
}

void CEdJournalEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( CurrentLocaleChanged ) )
	{
		CategorySelectionChanged( m_Tree->GetSelection() );
	}
}

CProperty* CEdJournalEditor::GetProperty( CJournalBase* entry, wxWindow* widget )
{
	// Grab property data
	SEdWidgetData* propData = static_cast< SEdWidgetData* >( widget->GetClientData() );

	// Grab Property
	CClass* rtticlass = entry->GetClass();
	CProperty* prop = rtticlass->FindProperty( propData->propertyName );

	return prop;
}

template< typename T >
T* CEdJournalEditor::GetPropertyPointer( CJournalBase* entry, wxWindow* widget )
{
	CProperty* prop = GetProperty( entry, widget );

	void* propPtr = prop->GetOffsetPtr( entry );

	return static_cast< T* >( propPtr );
}

template< typename T >
void CEdJournalEditor::GetProperty( CJournalBase* entry, wxWindow* widget, T& outValue )
{
	CProperty* prop = GetProperty( entry, widget );

	prop->Get( entry, &outValue );
}

void CEdJournalEditor::SetWidgetValueFromProperty( CJournalBase* entry, wxWindow* widget )
{
	String strVal;
	Uint32 numVal;
    Bool   boolVal;
    Float  floatVal;

	// Get Value
	if( widget->GetId() == GetID( EventID_StringChange ) || widget->GetId() == GetID( EventID_StringChoiceChange ) || widget->GetId() == GetID( EventID_NameChange ) )
	{
		GetProperty( entry, widget, strVal );
	}
	else if( widget->GetId() == GetID( EventID_ChoiceChange ) || widget->GetId() == GetID( EventID_UintChange ) )
	{
		GetProperty( entry, widget, numVal );
		strVal = ToString( numVal );
	}
	else if( widget->GetId() == GetID( EventID_FloatChange ) )
	{
		GetProperty( entry, widget, floatVal );
		strVal = ToString( floatVal );
	}
	else if( widget->GetId() == GetID( EventID_LocalizedStringTextChange ) )
	{
		LocalizedString intermediary;
		GetProperty( entry, widget, intermediary );

		strVal = intermediary.GetString();
	}
	else if( widget->GetId() == GetID( EventID_CNameChange ) || widget->GetId() == GetID( EventID_CNameChoiceChange ) )
	{
		CName intermediary;
		GetProperty( entry, widget, intermediary );

		if( intermediary != CName::NONE )
		{
			strVal = intermediary.AsString();
		}
	}
	else if( widget->GetId() == GetID( EventID_JournalPathChange ) )
	{
		THandle< CJournalPath > intermediary;
		GetProperty( entry, widget, intermediary );
		ASSERT( widget->IsKindOf( wxCLASSINFO( CJournalSelectorWidget ) ) );
		CJournalSelectorWidget* selectorCtrl = static_cast< CJournalSelectorWidget* >( widget );
		selectorCtrl->SetPath( intermediary );
		return;
	}
    else if( widget->GetId() == GetID( EventID_CheckBoxChange ) )
	{
		GetProperty( entry, widget, boolVal );

		// get property value from different place if required
		OnUpdateActiveProperty( entry, widget, boolVal );
	}
    else if( widget->GetId() == GetID( EventID_ObjectHandleChange ) )
	{
		TSoftHandle< CEntityTemplate > entityTemplate;
		GetProperty( entry, widget, entityTemplate );
		strVal = entityTemplate.GetPath();
	}
	else if( widget->GetId() == GetID( EventID_CNameArrayChange ) )
	{
		TDynArray< CName > names;
		GetProperty( entry, widget, names );
		for ( Uint32 i = 0; i < names.Size(); ++i )
		{
			strVal += names[ i ].AsString();
			if ( i < names.Size() - 1 )
			{
				strVal += TXT( "\n" );
			}
		}
	}
	else
	{
		ASSERT( false );
	}

	// Pass to widget
	if( widget->IsKindOf( wxCLASSINFO( wxTextCtrl ) ) )
	{
		wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( widget );
	    textCtrl->ChangeValue( strVal.AsChar() );
	}
	else if( widget->IsKindOf( wxCLASSINFO( wxChoice ) ) )
	{
		wxChoice* choiceCtrl = static_cast< wxChoice* >( widget );

		if( widget->GetId() == GetID( EventID_StringChoiceChange ) || widget->GetId() == GetID( EventID_CNameChoiceChange ) )
		{
			numVal = choiceCtrl->FindString( strVal.AsChar() );
		}

		choiceCtrl->SetSelection( numVal );

		wxCommandEvent event( wxEVT_JOURNAL_CHOICE_POPULATED );
		event.SetEventObject( choiceCtrl );
		event.SetString( strVal.AsChar() );
		event.SetInt( numVal );
		choiceCtrl->GetEventHandler()->ProcessEvent( event );
	}
    else if( widget->IsKindOf( wxCLASSINFO( wxCheckBox ) ) )
	{
		wxCheckBox* checkBox = static_cast< wxCheckBox* >( widget );
        checkBox->SetValue( boolVal );
	}
	else
	{
		ASSERT( false );
	}
}

wxStaticText* CEdJournalEditor::CreateLabel( wxSizer* sizer, TWidgetGroup& widgetGroup, const wxString& text )
{
	wxStaticText* label = new wxStaticText( m_subContainer, wxID_ANY, text + L':', wxDefaultPosition, wxDefaultSize, 0 );
	label->Wrap( -1 );

	if( sizer->IsKindOf( wxCLASSINFO( wxGridBagSizer ) ) )
	{
		wxGridBagSizer* bagSizer = static_cast< wxGridBagSizer* >( sizer );

		int row = bagSizer->GetEffectiveRowsCount();
		wxGBPosition position( row, 0 );

		bagSizer->Add( label, position, wxDefaultSpan, wxALL, 5 );
	}
	else
	{
		sizer->Add( label, 0, wxALL, 5 );
	}

	widgetGroup.PushBack( label );

	return label;
}

void CEdJournalEditor::AddDataToWidget( wxWindow* widget, const CName& name )
{
	SEdWidgetData* data = new SEdWidgetData( name );

	Uint32 widgetDataIndex = m_widgetData.Grow();
	m_widgetData[ widgetDataIndex ] = data;

	widget->SetClientData( data );
}

wxTextCtrl* CEdJournalEditor::CreateTextWidget( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& tooltip, Bool isDescription, const wxValidator& validator )
{
	long style = ( isDescription )? wxTE_MULTILINE | wxTE_RICH | wxTE_RICH2 : 0;

	wxTextCtrl* textCtrl = new wxTextCtrl( m_subContainer, windowID, wxT( "DEFAULT TEXT" ), wxDefaultPosition, wxDefaultSize, style, validator );

	AddDataToWidget( textCtrl, name );

	textCtrl->SetToolTip( tooltip );

	int proportion = ( isDescription )? 1 : 0;

	if( sizer->IsKindOf( wxCLASSINFO( wxGridBagSizer ) ) )
	{
		wxGridBagSizer* bagSizer = static_cast< wxGridBagSizer* >( sizer );

		int row = bagSizer->GetEffectiveRowsCount() - 1;
		wxGBPosition position( row, 1 );

		bagSizer->Add( textCtrl, position, wxDefaultSpan, wxALL|wxEXPAND, 5 );
	}
	else
	{
		sizer->Add( textCtrl, proportion, wxALL|wxEXPAND, 5 );
	}

	widgetGroup.PushBack( textCtrl );

	return textCtrl;
}

wxChoice* CEdJournalEditor::CreateChoiceWidget( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& tooltip, const wxArrayString& choices )
{
	wxChoice* choiceCtrl = new wxChoice( m_subContainer, windowID, wxDefaultPosition, wxDefaultSize, choices );

	choiceCtrl->SetToolTip( tooltip );

	AddDataToWidget( choiceCtrl, name );

	if( sizer->IsKindOf( wxCLASSINFO( wxGridBagSizer ) ) )
	{
		wxGridBagSizer* bagSizer = static_cast< wxGridBagSizer* >( sizer );

		int row = bagSizer->GetEffectiveRowsCount() - 1;
		wxGBPosition position( row, 1 );

		bagSizer->Add( choiceCtrl, position, wxDefaultSpan, wxALL|wxEXPAND, 5 );
	}
	else
	{
		sizer->Add( choiceCtrl, 0, wxALL|wxEXPAND, 5 );
	}

	widgetGroup.PushBack( choiceCtrl );

	return choiceCtrl;
}

wxButton* CEdJournalEditor::CreateSelectButton( wxGridBagSizer* sizer, TWidgetGroup& widgetGroup, wxTextCtrl* linkedControl, Int32 eventID, const wxString& label, const wxBitmap* bitmap /*= NULL*/ )
{
	wxButton* selectItemBtn = NULL;

	if ( bitmap )
	{
		selectItemBtn = new wxBitmapButton( m_subContainer, eventID, *bitmap );
		wxSize size = selectItemBtn->GetSize();
		size.x = size.y;
		selectItemBtn->SetInitialSize( size );
	}
	else
	{
		selectItemBtn = new wxButton( m_subContainer, eventID, label );
	}

	SEdWidgetData* data = new SEdWidgetData( linkedControl );
	m_widgetData.PushBack( data );
	selectItemBtn->SetClientData( data );

	int row = sizer->GetEffectiveRowsCount() - 1;
	sizer->Add( selectItemBtn, wxGBPosition( row, 2 ), wxDefaultSpan, wxALL, 5 );

	widgetGroup.PushBack( selectItemBtn );

	return selectItemBtn;
}

wxCheckBox* CEdJournalEditor::CreateCheckBox( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& toolTip )
{
	wxCheckBox* checkBox = new wxCheckBox( m_subContainer, windowID, wxT( "" ), wxDefaultPosition, wxDefaultSize );

	AddDataToWidget( checkBox, name );

	checkBox->SetToolTip( toolTip );

	if( sizer->IsKindOf( wxCLASSINFO( wxGridBagSizer ) ) )
	{
		wxGridBagSizer* bagSizer = static_cast< wxGridBagSizer* >( sizer );

		int row = bagSizer->GetEffectiveRowsCount() - 1;
		wxGBPosition position( row, 1 );

		bagSizer->Add( checkBox, position, wxDefaultSpan, wxALL|wxEXPAND, 5 );
	}
	else
	{
		sizer->Add( checkBox, 0, wxALL|wxEXPAND, 5 );
	}

	widgetGroup.PushBack( checkBox );

	return checkBox;
}

void CEdJournalEditor::CreateWidgets( const CClass* journalEntryClass, Bool needFile )
{
	ASSERT( journalEntryClass != NULL );

	TWidgetGroup& widgetGroup = m_controls[ journalEntryClass ];

	// Layout
	wxGridBagSizer* sizer = new wxGridBagSizer;
	wxGBSpan twoColSpan( 1, 2 );

	// File
	if( needFile )
	{
		// Filename
		CreateLabel( sizer, widgetGroup, wxT( "Filename" ) );
		wxTextCtrl* ctrl = CreateTextWidget( sizer, widgetGroup, GetID( EventID_FilenameChange ), CNAME( fileName ), wxT( "Filename, must be unique" ), false, wxTextValidator( wxFILTER_ALPHANUMERIC ) );
		sizer->SetItemSpan( ctrl, twoColSpan );
	}

	// Iterate through properties
	TDynArray< CProperty* > properties;
	journalEntryClass->GetProperties( properties );

	for( Uint32 i = 0; i < properties.Size(); ++i )
	{
		if( properties[ i ]->IsEditable() && !properties[ i ]->IsReadOnly() )
		{
			const CName& propertyType = properties[ i ]->GetType()->GetName();

			CreateLabel( sizer, widgetGroup, properties[ i ]->GetHint().AsChar() );

			if( propertyType == GetTypeName< LocalizedString >() )
			{
				Bool isDescription = properties[ i ]->GetName() == CNAME( description );

				wxTextCtrl* ctrl = CreateTextWidget
				(
					sizer,
					widgetGroup,
					GetID( EventID_LocalizedStringTextChange ),
					properties[ i ]->GetName(),
					properties[ i ]->GetHint().AsChar(),
					isDescription
				);

				CreateSelectButton( sizer, widgetGroup, ctrl, EventID_SelectString, wxT( "Select" ) );

				if( isDescription )
				{
					sizer->AddGrowableRow( sizer->GetRows() - 1 );
				}
			}
			else if( propertyType == GetTypeName< String >() )
			{
				if ( properties[ i ]->GetCustomEditorType() == TXT( "LinkedObjectPath" ) && m_linkedJournalTypes.KeyExist( journalEntryClass ) )
				{
					wxArrayString choiceStrings;
					TDynArray< String > files;
					CollectLinkedResourcedForType( journalEntryClass, files );

					choiceStrings.push_back( TXT( "" ) );
					for ( const String& s : files )
					{
						choiceStrings.push_back( s.AsChar() );
					}

					wxChoice* ctrl = CreateChoiceWidget
						(
						sizer,
						widgetGroup,
						GetID( EventID_StringChoiceChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						choiceStrings
						);

					sizer->SetItemSpan( ctrl, twoColSpan );
				}
				else
				{
					wxWindowID windowID = ( properties[ i ]->GetName() == CNAME( baseName ) )? GetID( EventID_NameChange ) : GetID( EventID_StringChange );

					wxTextCtrl* ctrl = CreateTextWidget
						(
						sizer,
						widgetGroup,
						windowID,
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar()
						);

					sizer->SetItemSpan( ctrl, twoColSpan );
				}
			}
			else if( propertyType == GetTypeName< Uint32 >() || propertyType == GetTypeName< Uint16 >() || propertyType == GetTypeName< Uint8 >() || propertyType == GetTypeName< Int32 >() )
			{
				if ( properties[ i ]->GetCustomEditorType() == TXT( "WorldSelection_Quest" ) ||
					 properties[ i ]->GetCustomEditorType() == TXT( "WorldSelection_Objective" ) )
				{
					wxArrayString choiceStrings;

					if ( properties[ i ]->GetCustomEditorType() == TXT( "WorldSelection_Objective" ) )
					{
						choiceStrings.push_back( wxT( "Inherit from parent" ) );
					}

					CEnum* en = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
					if ( en )
					{
						const TDynArray< CName > &enumVals = en->GetOptions();

						for ( Uint32 i=0; i<enumVals.Size(); i++ )
						{
							choiceStrings.push_back( enumVals[i].AsString().AsChar() );
						}
					}

					wxChoice* ctrl = CreateChoiceWidget
						(
						sizer,
						widgetGroup,
						GetID( EventID_ChoiceChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						choiceStrings
						);

					sizer->SetItemSpan( ctrl, twoColSpan );
				}
				else if( properties[ i ]->GetCustomEditorType() == TXT( "HuntingClueItem" ) )
				{
					wxArrayString choiceStrings;
					choiceStrings.push_back( wxT( "Please select category" ) );

					m_huntingClueChoiceWidget = CreateChoiceWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_ChoiceChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						choiceStrings
					);

					sizer->SetItemSpan( m_huntingClueChoiceWidget, twoColSpan );
				}
				else
				{
					wxTextCtrl* ctrl = CreateTextWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_UintChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						false,
						wxTextValidator( wxFILTER_NUMERIC )
					);

					sizer->SetItemSpan( ctrl, twoColSpan );
				}
			}
			else if( propertyType == GetTypeName< Float >() )
			{
				wxTextCtrl* ctrl = CreateTextWidget
					(
					sizer,
					widgetGroup,
					GetID( EventID_FloatChange ),
					properties[ i ]->GetName(),
					properties[ i ]->GetHint().AsChar(),
					false,
					wxTextValidator( wxFILTER_NUMERIC )
					//wxFloatingPointValidator<float>()
					);

				sizer->SetItemSpan( ctrl, twoColSpan );
			}
			else if( propertyType == GetTypeName< CName >() )
			{
				// Item
				if( properties[ i ]->GetCustomEditorType() == TXT( "ChooseItem" ) )
				{
					wxTextCtrl* ctrl = CreateTextWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_CNameChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar()
					);

					CreateSelectButton( sizer, widgetGroup, ctrl, EventID_SelectItem, wxT( "Select" ) );
				}
				else if( properties[ i ]->GetCustomEditorType() == TXT( "MapPin" ) )
				{
					//TODO: Select from list?
					wxTextCtrl* ctrl = CreateTextWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_CNameChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar()
					);
					sizer->SetItemSpan( ctrl, twoColSpan );
				}
				else if( properties[ i ]->GetCustomEditorType() == TXT( "MapPinType" ) )
				{
					//TODO: Select from list?
					wxTextCtrl* ctrl = CreateTextWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_CNameChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar()
					);
					sizer->SetItemSpan( ctrl, twoColSpan );
				}
				else if( properties[ i ]->GetCustomEditorType() == TXT( "HuntingClueCategory" ) )
				{
					wxArrayString choiceStrings;

					CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );
					ASSERT( gameResource != NULL, TXT( "No game resource defined (Journal Editor shouldn't be able to reach this point" ) );

					const TDynArray< CEnum* >& huntingClueCategories = gameResource->GetHuntingClueCategories();

					if( huntingClueCategories.Empty() )
					{
						choiceStrings.push_back( TXT( "No Hunting clue categories defined - please check game resource" ) );
					}
					else
					{
						for( Uint32 i = 0; i < huntingClueCategories.Size(); ++i )
						{
							choiceStrings.push_back( huntingClueCategories[ i ]->GetName().AsString().AsChar() );
						}
					}

					wxChoice* ctrl = CreateChoiceWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_CNameChoiceChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						choiceStrings
					);

					ctrl->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalEditor::OnHuntingClueCategorySelected, this );
					ctrl->Bind( wxEVT_JOURNAL_CHOICE_POPULATED, &CEdJournalEditor::OnHuntingClueCategorySelected, this );
					
					sizer->SetItemSpan( ctrl, twoColSpan );
				}
				else
				{
					wxTextCtrl* ctrl = CreateTextWidget
					(
						sizer,
						widgetGroup,
						GetID( EventID_CNameChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar()
					);

				sizer->SetItemSpan( ctrl, twoColSpan );
				}
			}
			else if( propertyType == GetTypeName< THandle< CJournalPath > >() )
			{
				const CName& name = properties[ i ]->GetName();

				const CClass* typeSelectable = NULL;
				Uint32 pathFlags = 0;

				if( properties[ i ]->GetCustomEditorType() == TXT( "JournalPropertyBrowserCreature" ) )
				{
					pathFlags =  FLAG( CEdJournalTree::TreeCategory_Creatures );
					typeSelectable = ClassID< CJournalCreature >();
				}
				else
				{
					HALT( "Custom property editor '%s' has not been defined for the journal", properties[ i ]->GetCustomEditorType().AsChar() );
				}

				CJournalSelectorWidget* selectorCtrl = new CJournalSelectorWidget( m_subContainer, EventID_JournalPathChange, pathFlags, typeSelectable );

				AddDataToWidget( selectorCtrl, name );

				if( sizer->IsKindOf( wxCLASSINFO( wxGridBagSizer ) ) )
				{
					wxGridBagSizer* bagSizer = static_cast< wxGridBagSizer* >( sizer );

					int row = bagSizer->GetEffectiveRowsCount() - 1;
					wxGBPosition position( row, 1 );

					bagSizer->Add( selectorCtrl, position, wxDefaultSpan, wxALL|wxEXPAND, 5 );
				}
				else
				{
					sizer->Add( selectorCtrl, 0, wxALL|wxEXPAND, 5 );
				}

				selectorCtrl->Bind( wxEVT_PATH_SELECTED, &CEdJournalEditor::OnPathChanged, this );
				selectorCtrl->Bind( wxEVT_PATH_CLEARED,  &CEdJournalEditor::OnPathCleared, this );
				widgetGroup.PushBack( selectorCtrl );

				CreateSelectButton( sizer, widgetGroup, selectorCtrl, EventID_ClearPath, wxT( "Clear" ) );
			}

			else if( propertyType == GetTypeName< TSoftHandle< CEntityTemplate > >() ||
			         propertyType == GetTypeName< TSoftHandle< CQuestPhase > >() )
			{
				wxTextCtrl* ctrl = CreateTextWidget
				(
					sizer,
					widgetGroup,
					GetID( EventID_ObjectHandleChange ),
					properties[ i ]->GetName(),
					properties[ i ]->GetHint().AsChar()
				);
				wxBitmap selectBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
				CreateSelectButton( sizer, widgetGroup, ctrl, EventID_SelectObject, wxEmptyString, &selectBitmap );
			}
			else if( properties[ i ]->GetType()->GetType() == RT_Enum )
			{
				CEnum* choiceEnum = static_cast< CEnum* >( properties[ i ]->GetType() );

				const TDynArray< CName >& choices = choiceEnum->GetOptions();
				
				wxArrayString choiceStrings;

				for( Uint32 iChoice = 0; iChoice < choices.Size(); ++iChoice )
				{
					choiceStrings.push_back( choices[ iChoice ].AsString().AsChar() );
				}

				wxChoice* ctrl = CreateChoiceWidget
				(
					sizer,
					widgetGroup,
					GetID( EventID_ChoiceChange ),
					properties[ i ]->GetName(),
					properties[ i ]->GetHint().AsChar(),
					choiceStrings
				);

				sizer->SetItemSpan( ctrl, twoColSpan );
			}
			else if( propertyType == GetTypeName< Bool >() )
			{
				wxCheckBox* ctrl = CreateCheckBox
					(
					sizer,
					widgetGroup,
                    GetID( EventID_CheckBoxChange ),
					properties[ i ]->GetName(),
					properties[ i ]->GetHint().AsChar()
					);

				sizer->SetItemSpan( ctrl, twoColSpan );
			}
			else if( propertyType == GetTypeName< TDynArray< CName > >() )
			{
				if( properties[ i ]->GetCustomEditorType() == TXT( "ChooseItem" ) )
				{
					wxTextCtrl* ctrl = CreateTextWidget
						(
						sizer,
						widgetGroup,
						GetID( EventID_CNameArrayChange ),
						properties[ i ]->GetName(),
						properties[ i ]->GetHint().AsChar(),
						true );

					sizer->AddGrowableRow( sizer->GetRows() - 1 );
					CreateSelectButton( sizer, widgetGroup, ctrl, EventID_SelectItems, wxT( "Add" ) );
				}
			}

		}
	}

	sizer->AddGrowableCol( 1 );
	sizer->SetFlexibleDirection( wxBOTH );
	sizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	// Add to frame
	wxSizer* containingSizer = m_subContainer->GetSizer();
	containingSizer->Add( sizer, 1, wxEXPAND, 5 );

	m_subContainer->Layout();
	m_splitter->UpdateSize();

	HideSubsectionWidgets( journalEntryClass );
}

void CEdJournalEditor::ShowSubsectionWidgets( const TWidgetGroup& widgetGroup, const wxTreeItemId& item )
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );
	ASSERT( itemData != NULL );

	Bool hasFile = itemData->m_resource && itemData->m_resource->GetFile();

	Bool enabled = CanModifyItem( item );

	for( Uint32 i = 0; i < widgetGroup.Size(); ++i )
	{
		SEdWidgetData* data = static_cast< SEdWidgetData* >( widgetGroup[ i ]->GetClientData() );

		widgetGroup[ i ]->Enable( enabled );

		if( data && data->dataType == SEdWidgetData::DT_PropertyName )
		{
			if( data->propertyName == CNAME( fileName ) )
			{
				wxTextCtrl* textCtrl = static_cast< wxTextCtrl* >( widgetGroup[ i ] );
				textCtrl->SetValue( itemData->m_filename.AsChar() );
				textCtrl->Enable( !( hasFile ) );
			}
			else
			{
				SetWidgetValueFromProperty( itemData->m_entry, widgetGroup[ i ] );
			}
		}

		widgetGroup[ i ]->Show();
	}

	m_splitter->UpdateSize();
	m_subContainer->Layout();
}

void CEdJournalEditor::HideSubsectionWidgets( const CClass* widgetGroupKey )
{
	ASSERT( widgetGroupKey != NULL );

	TWidgetGroup& widgetGroup = m_controls[ widgetGroupKey ];

	for( Uint32 i = 0; i < widgetGroup.Size(); ++i )
	{
		widgetGroup[ i ]->Hide();
	}
}

void CEdJournalEditor::OnCategorySelectionChanged( wxTreeEvent& event )
{
	wxTreeItemId selectedItem = event.GetItem();

	m_popupMenuItem = selectedItem;

	CategorySelectionChanged( selectedItem );
}

void CEdJournalEditor::CategorySelectionChanged( const wxTreeItemId& item )
{
	ASSERT( item.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );

	const CClass* classDesc = itemData->m_entry->GetClass();

	if( m_previousGroup != NULL && m_previousGroup != classDesc )
	{
		HideSubsectionWidgets( m_previousGroup );

		m_previousGroup = NULL;
	}

	if( m_controls.KeyExist( classDesc ) )
	{
		if( !m_splitter->IsSplit() )
		{
			m_splitter->SplitVertically( m_splitter->GetWindow1(), m_subContainer );
		}

		ShowSubsectionWidgets( m_controls[ classDesc ], item );
		m_previousGroup = classDesc;
	}
	else
	{
		if( m_splitter->IsSplit() )
		{
			m_splitter->Unsplit();
		}
	}

	wxMenu* newMenu = new wxMenu();
	PopulateCommonMenuOptions( item, newMenu );

	wxMenu* oldMenu = GetMenuBar()->Replace( 1, newMenu, wxT( "&Item" ) );
	delete oldMenu;

	Layout();
}

void CEdJournalEditor::OnTreeDrag( wxTreeEvent& event )
{
	wxTreeItemId draggedItem = event.GetItem();

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( draggedItem ) );
	ASSERT( itemData, TXT( "No data associated with journal item being dragged" ) );
	ASSERT( itemData->m_entry, TXT( "No entry associated with journal item being dragged!" ) );

	// Make sure it's not a category root (Which can't be drag and dropped)
	if( m_Tree->GetItemParent( draggedItem ) != m_Tree->GetRootItem() )
	{
		wxTreeItemId resourceItem; 
		CEdJournalTreeItemData* resourceData;
		m_Tree->GetResourceItem( draggedItem, resourceItem, &resourceData );

		// If it exists on perforce as is not checked out, do not allow it to be dragged
		if( !resourceData->m_resource->GetFile() || !resourceData->m_resource->GetFile()->IsCheckedIn() )
		{
			m_draggedItem = event.GetItem();
			event.Allow();
			LOG_EDITOR( TXT( "Dragging %s" ), itemData->m_entry->GetName().AsChar() );
		}
	}
}

void CEdJournalEditor::OnTreeDrop( wxTreeEvent& event )
{
	CEdJournalTreeItemData* sourceData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( m_draggedItem ) );
	ASSERT( sourceData && sourceData->m_entry );

	LOG_EDITOR( TXT( "Dropping %s" ), sourceData->m_entry->GetName().AsChar() );

	wxTreeItemId destItem = event.GetItem();

	CEdJournalTreeItemData* destData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( destItem ) );
	ASSERT( destData );

	Bool isSameType = sourceData->m_entry->GetClass()->IsA( destData->m_entry->GetClass() );
	Bool isParentType = ( !isSameType )? sourceData->m_entry->IsParentClass( destData->m_entry ) : false;

	if( isSameType || isParentType )
	{
		wxTreeItemId destParentItem;
		wxTreeItemId newItem;

		if( isSameType )
		{
			destParentItem = m_Tree->GetItemParent( destItem );

			Uint32 order = CalculateOrder( destItem );
			sourceData->m_entry->SetOrder( order );

			// Create new entry with same details as dragged item after the item it was "dropped" on
			newItem = m_Tree->AddItemInsert( destParentItem, destItem, sourceData->m_entry->GetName().AsChar(), new CEdJournalTreeItemData( sourceData ) );
		}
		else// if( isParentType )
		{
			destParentItem = destItem;

			Uint32 order = CalculateOrderFront( destParentItem );
			sourceData->m_entry->SetOrder( order );

			newItem = m_Tree->AddItemPrepend( destParentItem, sourceData->m_entry->GetName().AsChar(), new CEdJournalTreeItemData( sourceData ) );
		}

		ASSERT( destParentItem.IsOk() );

		CEdJournalTreeItemData* destParentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( destParentItem ) );
		ASSERT( destParentData );

		if( destParentData->m_entry )
		{
			if( sourceData->m_entry->IsA< CJournalChildBase >() )
			{
				CJournalChildBase* childEntry = static_cast< CJournalChildBase* >( sourceData->m_entry );

				childEntry->SetParentGUID( destParentData->m_entry->GetGUID() );
			}

			if( destParentData->m_entry->IsA< CJournalContainer >() )
			{
				ASSERT( sourceData->m_entry->IsA< CJournalContainerEntry >() );

				wxTreeItemId sourceParent = m_Tree->GetItemParent( m_draggedItem );
				CEdJournalTreeItemData* sourceParentData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( sourceParent ) );

				ASSERT( sourceParentData->m_entry->IsA< CJournalContainer >() );

				CJournalContainer* sourceParentContainer = static_cast< CJournalContainer* >( sourceParentData->m_entry );
				CJournalContainer* destParentContainer = static_cast< CJournalContainer* >( destParentData->m_entry );
				CJournalContainerEntry* movedItem = static_cast< CJournalContainerEntry* >( sourceData->m_entry );

				sourceParentContainer->RemoveChildEntry( movedItem );
				destParentContainer->AddChildEntry( movedItem );
			}
		}

		MarkItemModified( newItem, true );

		// Copy over children
		CopyChildren( m_draggedItem, newItem );

		// This will cause the journal path cache to update the existing entry now that it's position in the tree has changed
		// Or alternatively, populate the cache with this newly constructed path
		CJournalPath::ConstructPathFromTargetEntry( sourceData->m_entry, sourceData->m_resource, true );

		// Remove original entry
		m_modifiedMenuItems.Remove( m_draggedItem );
		m_CheckedOutMenuItems.Remove( m_draggedItem );
		m_Tree->DeleteItem( m_draggedItem );

		//m_Tree->SelectItem( newItem );
	}
}

void CEdJournalEditor::CopyChildren( const wxTreeItemId& sourceParentItem, const wxTreeItemId& destParentItem )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId sourceChild = m_Tree->GetFirstChild( sourceParentItem, cookie );

	while( sourceChild.IsOk() )
	{
		CEdJournalTreeItemData* sourceData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( sourceChild ) );
		
		wxTreeItemId destChild = m_Tree->AddItemAppend( destParentItem, sourceData->m_entry->GetName().AsChar(), new CEdJournalTreeItemData( sourceData ) );

		// Recursion ftw!
		CopyChildren( sourceChild, destChild );

		sourceChild = m_Tree->GetNextChild( sourceParentItem, cookie );
	}
}

void CEdJournalEditor::DisplayError( eError error, const wxTreeItemId& item )
{
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( item ) );

	switch( error )
	{
	case Error_None:
		break;

	case Error_NoFilename:
		{
			String msg = String::Printf( TXT( "No filename specified for item \"%s\"" ), itemData->m_entry->GetName() );
			wxMessageBox( msg.AsChar(), wxT( "Not Saved" ), wxOK, this );
		}
		break;

	case Error_DuplicateFilename:
		{
			String msg = String::Printf( TXT( "Filename \"%s\" already exists, please choose another for item \"%s\"" ), itemData->m_filename.AsChar(), itemData->m_entry->GetName() );
			wxMessageBox( msg.AsChar(), wxT( "Not Saved" ), wxOK, this );
		}
		break;

	default:
		HALT( "Unknown Journal Error" );
	}
}

void CEdJournalEditor::OnCollapseAll( wxEvent& event )
{
	m_Tree->CollapseAll();
}

void CEdJournalEditor::OnExpandAll( wxEvent& event )
{
	m_Tree->ExpandAll();
}

void CEdJournalEditor::OnResetWindowSize( wxEvent& event )
{
	SetSize( 650, 700 );
}

void CEdJournalEditor::OnCheckDuplicatedUniqueScriptTags( wxEvent& event )
{
	Uint32 duplicatedCount = 0;
	THashMap< CName, TDynArray< CJournalBase* > > journalEntries;

	m_Tree->CollectEntries( journalEntries );

	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("JOURNAL UST VALIDATOR") );
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );
	for ( THashMap< CName, TDynArray< CJournalBase* > >::iterator itEntry = journalEntries.Begin(); itEntry != journalEntries.End(); ++itEntry )
	{
		if ( itEntry->m_second.Size() > 1 )
		{
			duplicatedCount++;
			RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("Unique Script Tag: [%ls]"), itEntry->m_first.AsChar() );
			for ( Uint32 i = 0; i < itEntry->m_second.Size(); i++ )
			{
				CJournalBase* base = itEntry->m_second[ i ];
				if ( base )
				{
					THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( base );
					RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("   - [%s]   -   [%s]   -   [%s]"), base->GetFriendlyName().AsChar(), base->GetUniqueScriptIdentifier().AsChar(), path ? path->GetFriendlyName().AsChar() : TXT("null") );
				}
			}
		}
	}
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );

	if ( duplicatedCount > 0 )
	{
		GFeedback->ShowError( TXT("There are duplicated GUIDs in journal entries\nLook at 'JournalValidator' log channel for detailed information") );
	}
	else
	{
		GFeedback->ShowMsg( TXT("Duplicated GUIDs check"), TXT("All journal entries seem to be fine (%d checked)"), journalEntries.Size() );
	}
}

void CEdJournalEditor::OnCheckDuplicatedGUIDs( wxEvent& event )
{
	Uint32 duplicatedCount = 0;
	THashMap< CGUID, TDynArray< CJournalBase* > > journalEntries;

	m_Tree->CollectEntries( journalEntries );

	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("JOURNAL GUID VALIDATOR") );
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );
	for ( THashMap< CGUID, TDynArray< CJournalBase* > >::iterator itEntry = journalEntries.Begin(); itEntry != journalEntries.End(); ++itEntry )
	{
		if ( itEntry->m_second.Size() > 1 )
		{
			duplicatedCount++;
			RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("GUID: %u-%u-%u-%u"), itEntry->m_first.guid[ 0 ], itEntry->m_first.guid[ 1 ], itEntry->m_first.guid[ 2 ], itEntry->m_first.guid[ 3 ] );
			for ( Uint32 i = 0; i < itEntry->m_second.Size(); i++ )
			{
				CJournalBase* base = itEntry->m_second[ i ];
				if ( base )
				{
					THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( base );
					RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("   - [%s]   -   [%s]   -   [%s]"), base->GetFriendlyName().AsChar(), base->GetUniqueScriptIdentifier().AsChar(), path ? path->GetFriendlyName().AsChar() : TXT("null") );
				}
			}
		}
	}
	RED_LOG( RED_LOG_CHANNEL( JournalValidator ), TXT("===============================================") );

	if ( duplicatedCount > 0 )
	{
		GFeedback->ShowError( TXT("There are duplicated GUIDs in journal entries\nLook at 'JournalValidator' log channel for detailed information") );
	}
	else
	{
		GFeedback->ShowMsg( TXT("Duplicated GUIDs check"), TXT("All journal entries seem to be fine (%d checked)"), journalEntries.Size() );
	}
}

void CEdJournalEditor::OnGenerateMissingUniqueScriptTags( wxEvent& event )
{
	m_Tree->GenerateMissingUniqueScriptTags();
}

void CEdJournalEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT( "/Frames/JournalEditor" ) );
}

void CEdJournalEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT( "/Frames/JournalEditor" ) );
}

void CEdJournalEditor::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings )
{
	if( !m_localizedStringsToSave.IsZero() )
	{
		// Save specified strings
		TLocalizedStringEntryArray& itemLocalizedStringChanges = m_localizedStringChanges.GetRef( m_localizedStringsToSave );

		localizedStrings.CopyFast( itemLocalizedStringChanges );

		m_localizedStringChanges.Erase( m_localizedStringsToSave );

		m_localizedStringsToSave = CGUID();
	}
	else
	{
		// Save all strings
		for( THashMap< CGUID, TLocalizedStringEntryArray >::iterator iter = m_localizedStringChanges.Begin(); iter != m_localizedStringChanges.End(); ++iter )
		{
			TLocalizedStringEntryArray& itemLocalizedStringChanges = iter->m_second;

			for( Uint32 i = 0; i < itemLocalizedStringChanges.Size(); ++i )
			{
				localizedStrings.PushBack( itemLocalizedStringChanges[ i ] );
			}
		}
	}
}

Uint32 CEdJournalEditor::GetTargetDLCDirectoryIndexFrom( const TDynArray< CDirectory* >& directories ) const
{
	// Empty target DLC == global one, the first directory is always set to the global
	if ( m_targetDLC.GetLength() == 0 )
	{
		return 0;
	}

	// Otherwise find the entry that begins with dlc\<target
	const String leftSide = String::Printf( TXT("dlc\\%ls\\"), m_targetDLC.AsChar() );
	for ( Uint32 i=0; i < directories.Size(); ++i )
	{
		if ( directories[i]->GetDepotPath().BeginsWith( leftSide ) )
		{
			return i;
		}
	}

	// Not found, use global
	return 0;
}

CDirectory* CEdJournalEditor::GetTargetDLCDirectoryFrom( const TDynArray< CDirectory* >& directories ) const
{
	Uint32 index = GetTargetDLCDirectoryIndexFrom( directories );
	return directories[index];
}

void CEdJournalEditor::OnHuntingClueCategorySelected( wxCommandEvent& event )
{
	CName categoryName( event.GetString().wc_str() );

	DoHuntingClueCategorySelected( categoryName );

	event.Skip();
}

void CEdJournalEditor::DoHuntingClueCategorySelected( const CName& categoryName )
{
	if( m_huntingClueChoiceWidget )
	{
		const CEnum* category = SRTTI::GetInstance().FindEnum( categoryName );

		if( category )
		{
			wxArrayString options;

			for( Uint32 i = 0; i < category->GetOptions().Size(); ++i )
			{
				options.Add( category->GetOptions()[ i ].AsString().AsChar() );
			}

			m_huntingClueChoiceWidget->Set( options );
		}
		else
		{
			m_huntingClueChoiceWidget->Clear();
		}
	}
}

void CEdJournalEditor::OnChangeQuestTypeProperty( wxCommandEvent& event )
{
	SEdWidgetData* propData = GetWidgetDataFromEvent( event );
	ASSERT( propData != NULL );
	if ( !propData )
	{
		return;
	}
	ASSERT( propData->dataType == SEdWidgetData::DT_PropertyName );
	if ( !propData || propData->dataType != SEdWidgetData::DT_PropertyName )
	{
		return;
	}

	wxTreeItemId selectedItem = m_Tree->GetSelection();
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );
	if ( !itemData )
	{
		return;
	}

	CJournalBase* entry = itemData->m_entry;
	ASSERT( entry != NULL );
	if ( !entry )
	{
		return;
	}

	// make sure it is property from desired classes
	if ( !IsQuestTypePropertyClass( entry ) )
	{
		return;
	}
	// make sure it's correct widget
	if ( !IsQuestTypePropertyWidgetData( propData ) )
	{
		return;
	}

	Uint32 regularCount = 0, monsterHuntCount = 0, treasureHuntCount = 0;

	for ( Uint32 i = EJCT_Vanilla; i < EJCT_Count; ++i )
	{
		CountQuests( i, regularCount, monsterHuntCount, treasureHuntCount );

		CJournalInitialEntriesResource* resource = m_initalEntriesResource[ i ];
		if ( resource )
		{
			resource->SetQuestCount( regularCount, monsterHuntCount, treasureHuntCount );
		}
	}
}

void CEdJournalEditor::OnChangeActiveProperty( wxCommandEvent& event )
{
	SEdWidgetData* propData = GetWidgetDataFromEvent( event );
	ASSERT( propData != NULL );
	if ( !propData )
	{
	    return;
	}
	ASSERT( propData->dataType == SEdWidgetData::DT_PropertyName );
	if ( !propData || propData->dataType != SEdWidgetData::DT_PropertyName )
	{
	    return;
	}

	wxTreeItemId selectedItem = m_Tree->GetSelection();
	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( m_Tree->GetItemData( selectedItem ) );
	ASSERT( itemData != NULL );
	if ( !itemData )
	{
		return;
	}

	CJournalBase* entry = itemData->m_entry;
	ASSERT( entry != NULL );
	if ( !entry )
	{
		return;
	}

	// make sure it is property from desired classes
	if ( !IsActivePropertyClass( entry ) )
	{
	    return;
	}
	// make sure it's correct widget
	if ( !IsActivePropertyWidgetData( propData ) )
	{
		return;
	}

	// get journal path from entry
	THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( entry );
	ASSERT( path );
	if ( !path )
	{
		return;
	}

	// and add it or remove
	CJournalInitialEntriesResource* resource = m_initalEntriesResource[ EJCT_Vanilla ];
	if ( resource )
	{
		if ( event.IsChecked() )
		{
			resource->AddEntry( path );
		}
		else
		{
			resource->RemoveEntry( path );
		}
	}
}

void CEdJournalEditor::OnUpdateActiveProperty( CJournalBase* entry, wxWindow* widget, Bool& boolVal )
{
	ASSERT( entry );
	ASSERT( widget );
	if ( !entry || !widget )
	{
	    return;
	}

	SEdWidgetData* propData = static_cast< SEdWidgetData* >( widget->GetClientData() );
	ASSERT( propData );
	if ( !propData )
	{
		return;
	}

	// make sure the resource we're going to read from exists
	if ( !m_initalEntriesResource[ EJCT_Vanilla ] )
	{
		return;
	}

	// make sure it's correct class
	if ( !IsActivePropertyClass( entry ) )
	{
		return;
	}

	// make sure it's correct widget
	if ( !IsActivePropertyWidgetData( propData ) )
	{
		return;
	}

	// check if it is just recently added entry and resource file hasn't been created yet
	if ( !IsParentResourceSavedToFile( entry ) )
	{
		// disable widget and don't update it from separate resource
		widget->Enable( false );
		return;
	}

	// get journal path from entry
	THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( entry );
	ASSERT( path );
	if ( !path )
	{
		return;
	}

	// and set bool value depending if this entry exists in initial entries resource
	boolVal = m_initalEntriesResource[ EJCT_Vanilla ]->ExistsEntry( path );
}

void CEdJournalEditor::OnOpenInitialEntriesResource()
{
	// load resource if exists
	String filename;
	
	for ( Uint32 i = 0; i < sizeof( m_initalEntriesResource ) / sizeof( m_initalEntriesResource[ 0 ] ); ++i )
	{
		filename = CWitcherJournalManager::GetInitialEntriesPathAndFilename( i );
		m_initalEntriesResource[ i ] = Cast< CJournalInitialEntriesResource >( GDepot->LoadResource( filename ) );
	}

	if ( m_initalEntriesResource[ EJCT_Vanilla ] )
	{
		m_initalEntriesResource[ EJCT_Vanilla ]->AddToRootSet();

		// clear out invalid entries just in case
		TDynArray< THandle< CJournalPath > >& entries = m_initalEntriesResource[ EJCT_Vanilla ]->GetEntries();
		for ( Uint32 i = 0; i < entries.Size(); )
		{
			if ( !entries[ i ]->IsValid() )
			{
				entries.Erase( entries.Begin() + i );
				m_initalEntriesResource[ EJCT_Vanilla ]->MarkModified();
			}
			else
			{
				i++;
			}
		}
	}
	else
	{
		m_initalEntriesResource[ EJCT_Vanilla ] = ::CreateObject< CJournalInitialEntriesResource >();
		if ( m_initalEntriesResource[ EJCT_Vanilla ] )
		{
			m_initalEntriesResource[ EJCT_Vanilla ]->AddToRootSet();
		}
	}

	for ( Uint32 i = EJCT_EP1; i < EJCT_Count; ++i )
	{
		if ( m_initalEntriesResource[ i ] )
		{
			m_initalEntriesResource[ i ]->AddToRootSet();
		}
		else
		{
			m_initalEntriesResource[ i ] = ::CreateObject< CJournalInitialEntriesResource >();
			if ( m_initalEntriesResource[ i ] )
			{
				m_initalEntriesResource[ i ]->AddToRootSet();
			}
		}
	}
}

void CEdJournalEditor::OnCloseInitialEntriesResource()
{
	SaveInitialEntriesResource();

	for ( Uint32 i = 0; i < sizeof( m_initalEntriesResource ) / sizeof( m_initalEntriesResource[ 0 ] ); ++i )
	{
		if ( m_initalEntriesResource[ i ] )
		{
			m_initalEntriesResource[ i ]->RemoveFromRootSet();
		}
	}
}

void CEdJournalEditor::SaveInitialEntriesResource()
{
    // if there is a resource and it's modified, save it
	for ( Uint32 i = 0; i < sizeof( m_initalEntriesResource ) / sizeof( m_initalEntriesResource[ 0 ] ); ++i )
	{
		CJournalInitialEntriesResource* resource = m_initalEntriesResource[ i ];
		if ( resource )
		{
			String filename = CWitcherJournalManager::GetInitialEntriesPathAndFilename( i );

			CDiskFile* file = resource->GetFile();
			if ( file )
			{
				if ( file->IsModified() )
				{
					ASSERT( GDepot->FindFile( filename ) );

					file->Save();
				}
			}
			else
			{
				ASSERT( !GDepot->FindFile( filename ) );

				CDirectory* dir = GDepot->FindPath( filename.AsChar() );
				if ( !dir )
				{
					dir = GDepot->CreatePath( filename.AsChar() );
				}
				if ( dir )
				{
					resource->SaveAs( dir, CWitcherJournalManager::GetInitialEntriesFilename( i ) );
				}
			}
		}
	}
}

Bool CEdJournalEditor::IsQuestTypePropertyClass( const CJournalBase* entry )
{
	return entry->IsA< CJournalQuest >();
}

Bool CEdJournalEditor::IsQuestTypePropertyWidgetData( const SEdWidgetData* propData )
{
	ASSERT( propData );
	if ( !propData )
	{
		return false;
	}
	if ( propData->propertyName != CNAME( type ) && propData->propertyName != CNAME( contentType ) )
	{
		return false;
	}
	return true;
}

Bool CEdJournalEditor::IsActivePropertyClass( const CJournalBase* entry )
{
	// this classes should have defined a property for m_active
	return entry->IsA< CJournalStoryBookPageDescription >() ||
		   entry->IsA< CJournalStoryBookPage >() ||
		   entry->IsA< CJournalStoryBookChapter >() ||
           entry->IsA< CJournalTutorial >() ||
           entry->IsA< CJournalCreature >() ||
           entry->IsA< CJournalCreatureDescriptionEntry >() ||
           entry->IsA< CJournalCreatureHuntingClue >() ||
           entry->IsA< CJournalCreatureVitalSpotEntry >() ||
           entry->IsA< CJournalCharacter >() ||
           entry->IsA< CJournalCharacterDescription >() ||
           entry->IsA< CJournalGlossary >() ||
		   entry->IsA< CJournalGlossaryDescription >() ||
           entry->IsA< CJournalQuestDescriptionEntry >() ||
		   entry->IsA< CJournalPlace >() ||
		   entry->IsA< CJournalPlaceDescription >();
}

Bool CEdJournalEditor::IsActivePropertyWidgetData( const SEdWidgetData* propData )
{
	ASSERT( propData );
	if ( !propData )
	{
		return false;
	}
	if ( propData->propertyName != CNAME( active ) )
	{
	    return false;
	}
	return true;
}

Bool CEdJournalEditor::IsParentResourceSavedToFile( const CJournalBase* entry )
{
	CObject* parentObject = entry->GetParent();

	while ( parentObject && !parentObject->IsA< CJournalResource >() )
	{
		parentObject = parentObject->GetParent();
	}

	if ( parentObject )
	{
		CJournalResource* resource = static_cast< CJournalResource* >( parentObject );
		if ( resource )
		{
			return !!resource->GetFile();
		}
	}

	return false;
}

void CEdJournalEditor::CollectLinkedResourcedForType( const CClass* entryType, TDynArray< String > & outFilenames )
{
	if ( SLinkedJournalTypeDescriptor* linkedTypeDesc = m_linkedJournalTypes.FindPtr( entryType ) )
	{
		RED_ASSERT( entryType == linkedTypeDesc->m_baseClass );
		for ( CDirectory* dir : linkedTypeDesc->m_resourceDirectories )
		{
			CDirectoryFileList files = dir->GetFiles();
			for ( const auto& file : files )
			{
				String path = file->GetDepotPath();
				CJournalResource* res = Cast< CJournalResource >( GDepot->LoadResource( path ) );
				if ( res != nullptr && res->Get() && res->Get()->IsA( linkedTypeDesc->m_linkedClass ) )
				{
					outFilenames.PushBack( path );
				}
			}
		}
	}
	else
	{
		RED_ASSERT( false, TXT( "Cannot find proper SLinkedJournalTypeDescriptor" ) );
	}
}

void CEdJournalEditor::CountQuests( Uint32 contentType, Uint32& regularQuestCount, Uint32& monsterHuntQuestCount, Uint32& treasureHuntQuestCount )
{
	regularQuestCount = 0;
	monsterHuntQuestCount = 0;
	treasureHuntQuestCount = 0;

	for ( CDirectory* dir : CEdJournalTree::GetQuestsDirectories() )
	{
		TDynArray< String > questResources;
		dir->FindResourcesByExtension( TXT("journal"), questResources, true, true );

		for ( Uint32 i = 0; i < questResources.Size(); i++ )
		{
			CJournalResource* res = Cast< CJournalResource >( GDepot->LoadResource( questResources[ i ] ) );
			if ( res )
			{
				CJournalQuest* quest = Cast< CJournalQuest >( res->Get() );
				if ( quest )
				{
					if ( quest->GetContentType() != contentType )
					{
						continue;
					}

					switch ( quest->GetType() )
					{
					case QuestType_Story:
					case QuestType_Chapter:
					case QuestType_Side:
						regularQuestCount++;
						break;
					case QuestType_MonsterHunt:
						monsterHuntQuestCount++;
						break;
					case QuestType_TreasureHunt:
						treasureHuntQuestCount++;
						break;
					}
				}
			}
		}
	}
}
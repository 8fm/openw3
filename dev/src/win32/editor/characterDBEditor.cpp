OrderAfter
#include "build.h"
#include "characterDBEditor.h"
#include "characterResourceContainer.h"
#include "chooseItemDialog.h"
#include "stringSelectorTool.h"
#include "characterFolderTree.h"
#include "characterInheritanceTree.h"
#include "characterEdCustomTypes.h"
#include "gridEditor.h"
#include "gridCustomColumns.h"
#include "gridCustomTypes.h"

#include "../../common/engine/gameResource.h"
#include "../../common/core/property.h"
#include "../../games/r6/r6GameResource.h"
#include "../../common/core/depot.h"

CEdCharacterDBEditor::CEdCharacterDBEditor( wxWindow* parent, CCharacterResource* characterResource, Bool readOnlyMode )
	: wxSmartLayoutPanel( parent, TEXT( "CharacterDBEditor" ), false )
	, m_characterGrid( nullptr )
	, m_invalidData( true )
	, m_readOnlyMode( readOnlyMode )
	, m_changingSelection( false )
{
	m_resourceContainer = new CEdCharacterResourceContainer();

	m_resourceContainer->AddToRootSet();

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_CHARACTER_DB" ) ) ); 
	SetIcon( iconSmall );

	LoadOptionsFromConfig();

	m_notebook = XRCCTRL( *this, "TreeNotebook", wxNotebook );
	RED_FATAL_ASSERT( m_notebook, "Editor should have specified notebook" );
	m_notebook->ChangeSelection( VC_Folder );
	
	m_folderTree = XRCCTRL( *this, "FolderTree", CEdCharacterFolderTree );
	RED_FATAL_ASSERT( m_folderTree, "Editor should have specified folder tree");

	m_inheritanceTree = XRCCTRL( *this, "InheritanceTree", CEdCharacterInheritanceTree );
	RED_FATAL_ASSERT( m_inheritanceTree, "Editor should have specified inheritance tree" );

	m_folderTree->Initialize( *m_resourceContainer, m_inheritanceTree, readOnlyMode );

	m_inheritanceTree->Initialize( m_folderTree, readOnlyMode );

	// Don't bother doing anything more if the character DB directory hasn't been defined
	if ( !IsRootDirectoryDefined() )
	{
		SetBackgroundColour( wxColour( 224, 130, 130 ) );
		CEdHelpBubble* bubble = new CEdHelpBubble( this, TXT( "The path to the character DB must be defined\nin the Game Configuration Resource" ) );
		bubble->Show();
		m_invalidData = false;
		return;
	}

	m_viewChoice = XRCCTRL( *this, "ViewChoice", wxChoice );
	RED_FATAL_ASSERT( m_viewChoice, "Editor should have specified notebookview choice ui control" );
	m_viewChoice->Select( VC_Folder );

	wxPanel* characterPanel = XRCCTRL( *this, "CharacterPanel", wxPanel );
	RED_FATAL_ASSERT( characterPanel, "Editor should have specified character grid panel" );
		
	m_folderTree->PopulateTree( GetRootDirectory() );
	m_inheritanceTree->PopulateTree( *m_resourceContainer );

	m_folderTree->SelectItem( m_folderTree->GetRootItem() );
	m_folderTree->GatherCharactersForSelectedNode( m_visibleCharacters );

	m_characterGrid = new CGridEditor( characterPanel );
	SetGridObject();
	m_characterGrid->SetDefaultObjectParent( m_resourceContainer );
	m_characterGrid->RegisterCustomColumnDesc( TXT( "Voice Tag" ), new CGridVoicetagColumnDesc( 4 ) );
	m_characterGrid->RegisterCustomType( new CGridCharacterNameCellDesc( *m_resourceContainer ) );
	m_characterGrid->RegisterCustomType( new CGridTagListDesc );

	m_characterGrid->RegisterCustomCellAttrCustomizer( new CCharacterCellAttrCustomizer() );
	
	characterPanel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
	characterPanel->GetSizer()->Add( m_characterGrid, 1, wxEXPAND );

	characterPanel->Layout();

	Bind( wXEVT_GRID_EDITING, &CEdCharacterDBEditor::OnGridEdit, this );
	Bind( wxEVT_GRID_VALUE_CHANGED, &CEdCharacterDBEditor::OnGridChanged, this );
	
	Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdCharacterDBEditor::OnViewChanged, this, XRCID( "ViewChoice" )  );
	Bind( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, &CEdCharacterDBEditor::OnNotebookPageChanged, this );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnCollapseAll, this, XRCID( "collapseAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnExpandAll, this, XRCID( "expandAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnSaveAll, this, XRCID( "saveAll" ) );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnCheckInAll, this, XRCID( "checkAllIn" ) );
	Bind( wxEVT_CLOSE_PANEL, &CEdCharacterDBEditor::OnCloseWindow, this );

	Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdCharacterDBEditor::OnInheritanceTreeNodeSelectionChanged, this, XRCID( "InheritanceTree" ) );
	Bind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdCharacterDBEditor::OnFolderTreeNodeSelectionChanged, this, XRCID( "FolderTree" ) );
	Bind( wXEVT_TREE_CHARACTER_NAME_CHANGED, &CEdCharacterDBEditor::OnDataChanged, this, XRCID( "FolderTree" ) );

	wxMenuBar* menuBar = GetMenuBar();
	RED_FATAL_ASSERT( menuBar, "Editor should have specified menu bar" );

	wxMenuItem *saveAllItem = menuBar->FindItem( XRCID( "saveAll" ) );
	RED_FATAL_ASSERT( saveAllItem, "Editor should have specified saveAll menu item" );
	saveAllItem->Enable( !m_readOnlyMode );

	wxMenuItem *checkAllInItem = menuBar->FindItem( XRCID( "checkAllIn" ) );
	RED_FATAL_ASSERT( saveAllItem, "Editor should have specified checkAllIn menu item" );
	checkAllInItem->Enable( !m_readOnlyMode );

	if( characterResource != nullptr )
	{
		m_folderTree->ExpandPath( characterResource );
	}
}

CEdCharacterDBEditor::~CEdCharacterDBEditor()
{
	Unbind( wXEVT_GRID_EDITING, &CEdCharacterDBEditor::OnGridEdit, this );
	Unbind( wxEVT_GRID_VALUE_CHANGED, &CEdCharacterDBEditor::OnGridChanged, this );

	Unbind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdCharacterDBEditor::OnViewChanged, this );
	Unbind( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, &CEdCharacterDBEditor::OnNotebookPageChanged, this );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnCollapseAll, this, XRCID( "collapseAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnExpandAll, this, XRCID( "expandAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnSaveAll, this, XRCID( "saveAll" ) );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterDBEditor::OnCheckInAll, this, XRCID( "checkAllIn" ) );
	Unbind( wxEVT_CLOSE_PANEL, &CEdCharacterDBEditor::OnCloseWindow, this );
	
	Unbind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdCharacterDBEditor::OnInheritanceTreeNodeSelectionChanged, this, XRCID( "InheritanceTree" ) );
	Unbind( wxEVT_COMMAND_TREE_SEL_CHANGED, &CEdCharacterDBEditor::OnFolderTreeNodeSelectionChanged, this, XRCID( "FolderTree" ) );
	Unbind( wXEVT_TREE_CHARACTER_NAME_CHANGED, &CEdCharacterDBEditor::OnDataChanged, this, XRCID( "FolderTree" ) );

	if ( m_resourceContainer )
	{
		m_resourceContainer->RemoveFromRootSet();
		m_resourceContainer = nullptr;
	}
}

void CEdCharacterDBEditor::OnCollapseAll( wxEvent& event )
{
	m_folderTree->CollapseAll();
	m_inheritanceTree->CollapseAll();
}

void CEdCharacterDBEditor::OnExpandAll( wxEvent& event )
{
	m_folderTree->ExpandAll();
	m_inheritanceTree->ExpandAll();
}

void CEdCharacterDBEditor::OnSaveAll( wxEvent& event )
{
	m_folderTree->SaveOrShowFaileMessage( m_folderTree->GetRootItem(), false );
}

void CEdCharacterDBEditor::OnCheckInAll( wxEvent& event )
{
	m_folderTree->SaveOrShowFaileMessage( m_folderTree->GetRootItem(), true );
}

const CCharacter* CEdCharacterDBEditor::GetSelectedCharacter() const 
{
	if ( m_visibleCharacters.Size() > 0 )
	{
		return m_visibleCharacters[0];
	}
	return nullptr;	
}

void CEdCharacterDBEditor::OnGridEdit( CGridEvent& event )
{
	if ( m_readOnlyMode )
	{
		event.Veto();
		return;
	}

	m_editingCharactersTree.Clear();

	const IRTTIType* gridType = nullptr;
	void* gridData = nullptr;
	event.GetGridData( gridType, gridData );
	RED_FATAL_ASSERT( gridType->GetType() == RT_Class && ( ( CClass * )gridType )->GetName() == CName( TXT("CCharacter") ), "Grid should have CCharacter data" );

	CCharacter* character = ( CCharacter* ) gridData;

	const CProperty* prop = event.GetCellProperty();

	m_editingCharactersTree.PushBack( character );

	m_inheritanceTree->GetInheritanceTree( character, prop, m_editingCharactersTree );

	if ( !m_inheritanceTree->CheckOutInheritanceTree( character, prop ) )
	{
		event.Veto();
	}
}

void CEdCharacterDBEditor::OnGridChanged( CGridEvent& event )
{
	RED_FATAL_ASSERT( !m_readOnlyMode, "Grid can be changed only in not read only mode" );

	const IRTTIType* gridType = nullptr;
	void* gridData = nullptr;
	event.GetGridData( gridType, gridData );
	RED_FATAL_ASSERT( gridType->GetType() == RT_Class && ( ( CClass * )gridType )->GetName() == CName( TXT("CCharacter") ), "Grid should have CCharacter data" );

	CCharacter* character = ( CCharacter* ) gridData;

	RED_FATAL_ASSERT( m_editingCharactersTree.Size() > 0, "if grid is changed list of editing value can't be empty" );
	RED_FATAL_ASSERT( m_editingCharactersTree[0] == character, "Character which is changed has to be a root node on editing tree" );

	m_folderTree->MarkModified( character );

	const CProperty* prop = event.GetCellProperty();

	for ( Uint32 i = 1; i < m_editingCharactersTree.Size(); i++ )
	{
		m_editingCharactersTree[i]->UpdateInheritedProperty( prop );
	}
	m_invalidData = true;
}

void CEdCharacterDBEditor::OnCloseWindow( wxCloseEvent& event )
{
	if( m_folderTree->IsModified() )
	{
		Int32 userAnswer = YesNoCancel( TXT( "Changes not saved. Save?" ) );
		if( userAnswer == IDYES )
		{
			if ( m_folderTree->SaveOrShowFaileMessage( m_folderTree->GetRootItem(), false ) )
			{
				return;
			}
		}
		else if( userAnswer == IDCANCEL )
		{
			return;
		}
	}
	event.Skip();
}

Bool CEdCharacterDBEditor::IsRootDirectoryDefined()
{
	RED_FATAL_ASSERT( 0, "Not supported!" );
	//RED_FATAL_ASSERT( GGame, "Can't access game resource as there's no game" );
	//CR6GameResource* gameResource = Cast< CR6GameResource >( GGame->GetGameResource() );

	//if( gameResource )
	//{
	//	const String& basePath = gameResource->GetCharacterDBPath();

	//	if( !basePath.Empty() )
	//	{
	//		return true;
	//	}
	//}

	return false;
}

void CEdCharacterDBEditor::OnNotebookPageChanged( wxCommandEvent& event )
{
	Int32 i = m_notebook->GetSelection();
	m_viewChoice->Select( i );
	UpdateCharacterList( ( EViewChoice ) i );
}

void CEdCharacterDBEditor::UpdateCharacterList( EViewChoice choice )
{
	m_visibleCharacters.Clear();

	switch ( choice )
	{
	case VC_Folder:
		{
			m_folderTree->GatherCharactersForSelectedNode( m_visibleCharacters );
		}
		break;

	case VC_Inhritance:
		{
			m_inheritanceTree->GatherCharactersForSelectedNode( m_visibleCharacters );
		}
		break;
	}
	m_invalidData = true;
}


void CEdCharacterDBEditor::OnViewChanged( wxCommandEvent& event )
{
	Int32 i = event.GetInt();
	m_notebook->ChangeSelection( i );
	UpdateCharacterList( ( EViewChoice ) i );
}

void CEdCharacterDBEditor::OnDataChanged( wxEvent& event )
{
	m_invalidData = true;
}

void CEdCharacterDBEditor::OnFolderTreeNodeSelectionChanged( wxTreeEvent& event )
{
	if ( m_changingSelection )
	{
		return;
	}
	m_changingSelection = true;

	if ( !m_folderTree->SelectionChanged( event.GetItem() ) )
	{
		return;
	}

	CCharacter* character = m_folderTree->GetSelectedCharacter();

	m_inheritanceTree->SelectCharacterItem( character );

	m_visibleCharacters.Clear();
	m_folderTree->GatherCharactersForSelectedNode( m_visibleCharacters );
	m_invalidData = true;

	m_changingSelection = false;
}

void CEdCharacterDBEditor::OnInheritanceTreeNodeSelectionChanged( wxTreeEvent& event )
{
	if ( m_changingSelection )
	{
		return;
	}
	m_changingSelection = true;

	if ( !m_inheritanceTree->SelectionChanged( event.GetItem() ) )
	{
		return;
	}

	CCharacter* character = m_inheritanceTree->GetSelectedCharacter();
	RED_FATAL_ASSERT( character, "Selected character can't be null");

	m_folderTree->SelectCharacterItem( character );

	m_visibleCharacters.Clear();
	m_inheritanceTree->GatherCharactersForSelectedNode( m_visibleCharacters );
	m_invalidData = true;

	m_changingSelection = false;
}

void CEdCharacterDBEditor::SelectCharacter( CGUID guid )
{
	CCharacter* character = m_resourceContainer->FindCharacterByGUID( guid );

	if ( !character )
	{
		return;
	}
	m_inheritanceTree->SelectCharacterItem( character );
	m_folderTree->SelectCharacterItem( character );

	m_viewChoice->Select( VC_Folder );

	m_visibleCharacters.Clear();
	m_folderTree->GatherCharactersForSelectedNode( m_visibleCharacters );
	m_invalidData = true;
}

CDirectory* CEdCharacterDBEditor::GetRootDirectory()
{
	RED_FATAL_ASSERT( 0, "Not finished" );
	return NULL;
	//RED_FATAL_ASSERT( IsRootDirectoryDefined(), "Functions should check IsDirectoryDefined() before calling to request a directory" );

	//CR6GameResource* gameResource = Cast< CR6GameResource >( GGame->GetGameResource() );
	//const String& basePath = gameResource->GetCharacterDBPath();
	//CDirectory* baseDir = GDepot->CreatePath( basePath );
	//return baseDir;
}


void CEdCharacterDBEditor::SetGridObject()
{
	TDynArray< CObject* > & characters = CastArray< CObject, CCharacter >( m_visibleCharacters );
	m_characterGrid->SetObjects( characters, false );
}

void CEdCharacterDBEditor::OnInternalIdle()
{
	if ( m_invalidData )
	{
		SetGridObject();

		m_invalidData = false;
	}
}

void CEdCharacterDBEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT( "/Frames/CharacterDBEditor" ) );
}

void CEdCharacterDBEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT( "/Frames/CharacterDBEditor" ) );
}
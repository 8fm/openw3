/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "assetBrowser.h"
#include "animSetReportDlg.h"
#include "downscaleBiasDlg.h"
#include "changeLightChannels.h"
#include "changeDrawableFlags.h"
#include "changeMaterialFlags.h"
#include "resourceView.h"
#include "importDlg.h"
#include "commonDlgs.h"
#include "editorExternalResources.h"
#include "characterEntityEditor.h"
#include "../../common/core/clipboardBase.h"
#include "../../common/core/gatheredResource.h"

#include <wx/tokenzr.h>
#include <wx/dirdlg.h>

//RED_DEFINE_STATIC_NAME( VersionControlStatusChanged )
//RED_DEFINE_STATIC_NAME( FileEdited )

// Editors

#include "guiResourceEditor.h"
#include "steeringEditor.h"
#include "genericResourceEditor.h"
#include "materialEditor.h"
#include "textureViewer.h"
#include "swfViewer.h"
#include "meshEditor.h"
#include "lodAssigner.h"
#include "entityEditor.h"
#include "behaviorEditor.h"
#include "particleEditor.h"
#include "shortcutsEditor.h"
#include "spawnSetEditor.h"
#include "communityEditor.h"
#include "traitEditor.h"
#include "2dArrayEditor.h"
#include "dialogEditor.h"
#include "behTreeEditor.h"
#include "animBrowser.h"
#include "questEditor.h"
#include "animEventsEditor.h"
#include "jobTreeEditor.h"
#include "sceneDialogsetEditor.h"
#include "mimicFacesEditor.h"
#include "skeletonEditor.h"
#include "inventoryReport.h"
#include "textureArrayViewer.h"
#include "restartable.h"
#include "interactiveDialogEditor.h"
#include "idConnectorPackEditor.h"
#include "encounterEditor.h"
#include "simplexTreeEditor.h"
#include "gameResourceEditor.h"
#include "batchLODGenerator.h"
#include "floatValueEditor.h"
#include "attitudeSearchDialog.h"

#include "../../common/engine/simplexTreeResource.h"

#include "classHierarchyMapper.h"
#include "itemSelectionDialogs/componentSelectorDialog.h"
#include "itemSelectionDialogs/behaviorTreeScriptTaskSelector.h"
#include "itemSelectionDialogs/entityClassSelectorDialog.h"
#include "../../common/game/behTreeTask.h"
#include "../../common/game/behTree.h"
#include "../../common/game/behTreeNode.h"

#include <Uxtheme.h>
#include <shellapi.h>
#pragma comment(lib, "uxtheme")

#include "../../common/engine/swfResource.h"
#include "../../common/game/guiResource.h"
#include "../../common/game/movingAgentComponent.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneDialogset.h"
#include "../../common/game/jobTree.h"
#include "../../common/game/jobTreeNode.h"
#include "../../common/game/journalResource.h"
#include "../../common/game/journalPath.h"
#include "../../common/game/quest.h"
#include "../../common/game/inventoryDefinition.h"
#include "../../common/game/interactionArea.h"
#include "../../common/game/spawnTree.h"
#include "../../common/game/characterResource.h"
#include "../../games/r6/idResource.h"
#include "../../games/r6/idConnector.h"
#include "../../games/r6/r6GameResource.h"
#include "../../games/r6/traitData.h"

#include "../../common/core/factory.h"
#include "../../common/core/exporter.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/configFileManager.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/material.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/textureArray.h"
#include "../../common/engine/environmentDefinition.h"
#include "../../common/engine/characterEntityTemplate.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/cubeTexture.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/apexClothResource.h"
#include "meshStats.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventMimicsAnim.h"

#include "meshMaterialRemapper.h"
#include "utils.h"
#include "animationValidator.h"

#include "itemsThumbnailGenerator.h"

// HACK to make CDepot::FindResource work fine
#ifdef UNICODE
#	define FindResource  FindResourceW
#else
#	define FindResource  FindResourceA
#endif // !UNICODE

enum
{
	ID_CREATE_DIRECTORY				= 2000,
	ID_RESCAN_DIRECTORY				,
	ID_SYNC_DIRECTORY				,
	ID_CHECK_OUT_DIRECTORY			,
	ID_SUBMIT_DIRECTORY				,
	ID_REVERT_DIRECTORY				,
	ID_ADD_DIRECTORY				,
	ID_NEW_TAB_DIRECTORY			,
	ID_SEARCH_ENTITY_CLASS			,
	ID_DELETE_ASSET_AB				,
	ID_COPY_ASSET_AB				,
	ID_CUT_ASSET_AB					,
	ID_PASTE_ASSET_AB				,
	ID_PASTE_AS_ASSET_AB			,
	ID_RENAME_ASSET_AB				,
	ID_SHOW_IN_EXPLORER				,
	ID_USE_PATH_ASSET_AB			,
	ID_GENERATE_ITEMS_ICONS			,
	ID_CONVERT_SENSES				,
	ID_SHOW_IN_TREE					,
	ID_SEARCH_ATTITUDE_GROUP		,
	ID_SEARCH_COMPONENT_ABSENCE		,
	ID_SEARCH_COMPONENT_PRESENCE	,
	ID_SEARCH_BEHTREE_SCRIPTTASK	,
	ID_SEARCH_NPC_LEVEL				,
	ID_RESAVE_RESOURCES				,
	ID_CREATE_INVENTORIES_REPORT	,
	ID_SET_AUTO_HIDE_DISTANCE		,
	ID_SET_DOWNSCALE_BIAS			,
	ID_EXTRACT_SOURCE_TEXTURE		,
	ID_VALIDATE_ANIMATIONS			,
	ID_FIX_SOURCE_ANIM_DATA			,
	ID_VIEW_ANIM_SET_REPORT		,
	ID_ASSIGN_LODS					,
	ID_GENERATE_LODS				,
	ID_CHANGLE_LIGHT_CHANNELS		,
	ID_CHANGLE_DRAWABLE_FLAGS		,
	ID_CHANGLE_MATERIAL_FLAGS		,
	ID_REMOVE_UNUSED_MATERIALS		,
	ID_SET_STREAMING_LOD			,
	ID_CALCULATE_SHADOW_PRIORITIES	,
	ID_SIMPLIFY_MATERIALS			,
	ID_REMOVE_UNUSED_ANIMS			,
	ID_CALCULATE_TEXEL_DENSITY		,
	ID_SET_MESH_PROPERTIES			,
	ID_APPEARANCES_TEXTURE_COST		,
	ID_RESAVE_TEMPLATES				,
	ID_DUMP_ANIMS					,
	ID_BATCH_TEXTURE_GROUP_CHANGE	,
	ID_ASSIGN_MATERIALS_TO_MESHES	,
	ID_CALCULATE_COMPONENTS			,
	ID_REMOVE_FORCED_APPEARANCES	,
	ID_DUMP_DIALOGANIMS				,
	ID_ENTITIES_DEP_COUNT			,

	ID_DYNAPATH_FIRST_DIRECTORY		,
	ID_DYNAPATH_LAST_DIRECTORY		= ID_DYNAPATH_FIRST_DIRECTORY + 500,

	ID_BOOKMARKS_FIRST				,
	ID_BOOKMARKS_LAST				= ID_BOOKMARKS_FIRST + 500,

	ID_CLOSE_TAB					,
	ID_LOCK_TAB						,
	ID_FLAT_TAB						,

	ID_GENERATE_DEBUG_INFO_MESHES						,
	ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES				,
	ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES_EFFECTS		,
};

namespace Config
{
	TConfigVar<String> cvSkippedSearchPaths( "Editor", "SkippedSearchPaths", TXT(""), eConsoleVarFlag_ReadOnly );
}

class CPageWrapper : public wxObject 
{ 
public:  
	Int32 m_page;
	CPageWrapper( Int32 page ) : m_page( page ) { }
	virtual ~CPageWrapper() { }
};

#define MAX_SEARCH 20000
#define QUESTION_DISPLAY_SEARCH "More than 20000 files were found. It may take a while to display them. Do you wish to proceed?"

BEGIN_EVENT_TABLE( CEdAssetBrowser, wxPanel )
	EVT_SHOW( CEdAssetBrowser::OnShow )
	EVT_CLOSE( CEdAssetBrowser::OnClose )
	EVT_TREE_ITEM_COLLAPSED( XRCID( "DepotTree"), CEdAssetBrowser::OnDirectoryOpened )
	EVT_TREE_ITEM_EXPANDED( XRCID( "DepotTree"), CEdAssetBrowser::OnDirectoryOpened )
	EVT_TREE_SEL_CHANGED( XRCID( "DepotTree"), CEdAssetBrowser::OnDirectoryChanged )
	EVT_TREE_ITEM_RIGHT_CLICK( XRCID( "DepotTree"), CEdAssetBrowser::OnDirectoryContextMenu )
	EVT_CHOICE( XRCID( "ClassFilter" ), CEdAssetBrowser::OnClassFilterChanged )
	EVT_BUTTON( XRCID( "BtnTree" ), CEdAssetBrowser::OnTreeButton )
	EVT_SPLITTER_UNSPLIT( XRCID( "Splitter" ), CEdAssetBrowser::OnSplitterUnsplit )
	EVT_SPLITTER_DCLICK( XRCID( "Splitter" ), CEdAssetBrowser::OnSplitterDoubleClick )
	EVT_BUTTON( XRCID( "BtnNewFolder" ), CEdAssetBrowser::OnNewFolderButton )
	EVT_CHOICE ( XRCID( "ViewType" ), CEdAssetBrowser::OnChangeView )
	EVT_MENU( ID_DELETE_ASSET_AB, CEdAssetBrowser::OnDeleteAsset )
	EVT_MENU( ID_COPY_ASSET_AB, CEdAssetBrowser::OnCopyAsset )
	EVT_MENU( ID_CUT_ASSET_AB, CEdAssetBrowser::OnCutAsset )
	EVT_MENU( ID_PASTE_ASSET_AB, CEdAssetBrowser::OnPasteAsset )
	EVT_MENU( ID_PASTE_AS_ASSET_AB, CEdAssetBrowser::OnPasteAsAsset )
	EVT_MENU( ID_RENAME_ASSET_AB, CEdAssetBrowser::OnRenameAsset )
	EVT_MENU( ID_USE_PATH_ASSET_AB, CEdAssetBrowser::OnUsePathFromClipboard )
	EVT_MENU( XRCID( "recentFiles" ), CEdAssetBrowser::OnRecentFilesButton )
	EVT_MENU( XRCID( "pendingFiles" ), CEdAssetBrowser::OnCheckedOutButton )
	EVT_MENU( XRCID( "savePerSession" ), CEdAssetBrowser::OnSavePerSessionButton )
	EVT_MENU( XRCID( "editCopyPath" ), CEdAssetBrowser::OnEditCopyPath )
	EVT_MENU( XRCID( "editPastePath" ), CEdAssetBrowser::OnEditPastePath )
	EVT_MENU( XRCID( "toggleBookmark" ), CEdAssetBrowser::OnToggleBookmark )
	EVT_BUTTON( XRCID( "BtnBookmark" ), CEdAssetBrowser::OnToggleBookmark )
END_EVENT_TABLE()

CWxAuiNotebook::CWxAuiNotebook( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
: wxAuiNotebook( parent, id, pos, size, style )
{

}

void CWxAuiNotebook::GetOrder( TDynArray< wxWindow * > &target )
{
	/*
	for( Uint32 i=0; i<GetPageCount(); i++ )
	{
		target.PushBack( m_tabs.GetWindowFromIdx( i ) );
	}
	*/

	size_t page_count = m_tabs.GetPageCount();
	for (size_t p = 0; p < page_count; ++p)
	{
		wxAuiNotebookPage& page = m_tabs.GetPage(p);
		const size_t page_idx = m_tabs.GetIdxFromWindow(page.window); 

		target.PushBack( m_tabs.GetWindowFromIdx( page_idx ) );
	}
}

CDirectory* wxTreeItemDataDir::GetDirectory()
{
	if ( !m_directory )
	{
		m_directory = GDepot->FindPath( m_depotPath.AsChar() );
		wxTheFrame->GetAssetBrowser()->m_dirMapping.Insert( m_directory, GetId() );
	}
	return m_directory;
}

static Bool FindDepotTreeChildItemByPath( wxTreeCtrl* tree, wxTreeItemId parent, const String& depotPath, wxTreeItemId& id )
{
	wxTreeItemIdValue cookie;
	for ( wxTreeItemId childId=tree->GetFirstChild( parent, cookie ); childId.IsOk(); childId=tree->GetNextChild( parent, cookie ) )
	{
		wxTreeItemDataDir* data = static_cast<wxTreeItemDataDir*>( tree->GetItemData( childId ) );
		if ( data && data->GetDepotPath() == depotPath )
		{
			id = data->GetId();
			return true;
		}
		if ( FindDepotTreeChildItemByPath( tree, childId, depotPath, id ) )
		{
			return true;
		}
	}

	return false;
}

static Bool FindDepotTreeItemByPath( wxTreeCtrl* tree, const String& depotPath, wxTreeItemId& id )
{
	return FindDepotTreeChildItemByPath( tree, tree->GetRootItem(), depotPath, id );
}

BEGIN_EVENT_TABLE( CDynaPath, wxPanel )
	EVT_PAINT( CDynaPath::OnPaint )
	EVT_ERASE_BACKGROUND( CDynaPath::OnEraseBackground )
	EVT_MOUSE_EVENTS( CDynaPath::OnMouse )
END_EVENT_TABLE()

CDynaPath::CDynaPath( wxWindow* parent, CEdAssetBrowser* owner )
:	wxPanel( parent ),
	m_owner( owner )
	
{
	m_hover = NULL;
	m_hoverPart = -1;
	m_bitmapRoot = SEdResources::GetInstance().LoadBitmap( TXT("IMG_LOCAL") );
	//m_bitmapOpened = SEdResources::GetInstance().LoadBitmap( TXT("IMG_DIR_OPENED") );
	m_bitmapOpened = SEdResources::GetInstance().LoadBitmap( TXT("IMG_ARROW_RIGHT") );
	//m_bitmapArrow = SEdResources::GetInstance().LoadBitmap( TXT("IMG_ARROW_RIGHT") );
	m_bitmapClosed = SEdResources::GetInstance().LoadBitmap( TXT("IMG_DIR_CLOSED") );
	
	m_textEdit = new wxTextCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_LEFT );
	m_textEdit->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CDynaPath::OnTextEditKillFocus ), NULL, this );
	m_textEdit->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CDynaPath::OnTextEditEnter ), NULL, this );
	m_textEdit->SetMinSize( wxSize( -1, 17 ) );
	m_textEdit->Hide();

	wxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );
	sizer->Add( m_textEdit, 0, wxEXPAND, 0 );

	m_timer = new CEdTimer( this );
	m_font = *wxNORMAL_FONT;
	m_font.SetPointSize( m_font.GetPointSize() + 1 );
	Connect( wxEVT_TIMER, wxCommandEventHandler( CDynaPath::OnTimeout ), 0, this );
	SetMinSize( wxSize( -1, 17 ) );
	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
}

void CDynaPath::OnTimeout( wxCommandEvent &event )
{
	if( m_hover )
	{
		m_hoverInset = false;
		if( m_hoverPart == 1 )
			m_hover = NULL;
		Refresh( true );
	}
}

void CDynaPath::OnMouse( wxMouseEvent &event )
{
	wxPoint p = event.GetPosition();
	CDirectory *hover = m_hover;
	Int32 hoverPart = m_hoverPart;
	m_hover = NULL;
	m_hoverPart = -1;
	for( THashMap< CDirectory*, wxRect >::iterator it=m_rectDir.Begin(); it!=m_rectDir.End(); ++it )
	{
		if( it->m_second.Contains( p ) )
		{
			m_hover = it->m_first;

			if ( m_hover && m_hover->GetDirectories().Size() )
			{
				if( p.x < it->m_second.GetX() + it->m_second.GetWidth() - m_bitmapOpened.GetWidth() - 2 )
				{
					m_hoverPart = 0;
				}
				else
				{
					m_hoverPart = 1;
				}
			}
			else
			{
				m_hoverPart = 0;
			}

			break;
		}
	}

	if( event.RightDown() )
	{
		if( m_hover != NULL && !m_textEdit->IsShown() )
		{
			Refresh( true );
			if( m_hoverPart == 0 )
			{
				wxMenu menu;

				TDynArray< CDirectory* > dirs;
				dirs.PushBack( m_hover );
				m_owner->CreateDirectoryContextMenu( menu, dirs );

				// Show menu
				PopupMenu( &menu );
			}
		}
	}
	else
	if( event.LeftDown() )
	{
		if( m_hover != NULL && !m_textEdit->IsShown() )
		{
			Refresh( true );
			if( m_hoverPart == 0 )
			{
				m_owner->SelectDirectory( m_hover );
			}
			else
			{
				TSortedArray< String > directories;
				GetSortedDirectories( directories );

				if ( directories.Size() > 0 )
				{
					wxMenu menu;

					Int32 i = 0;
					for ( const String& dir: directories )
					{
						wxMenuItem *item = new wxMenuItem( &menu, ID_DYNAPATH_FIRST_DIRECTORY+i, dir.AsChar() );
						item->SetBitmap( m_bitmapClosed );
						menu.Append( item );
						i ++;
					}
					menu.Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CDynaPath::OnSubfolder ), 0, this );

					wxRect r;
					m_rectDir.Find( m_hover, r );
					r.x += r.width - m_bitmapOpened.GetWidth() - 6;
					r.y += r.height - 1;
					m_owner->PopupMenu( &menu, m_owner->ScreenToClient( ClientToScreen( r.GetLeftTop() ) ) );
				}
			}
		}
		else if ( CDirectory *dir = m_owner->GetActiveDirectory() )
		{
			m_textEdit->SetValue( dir->GetDepotPath().AsChar() );
			m_textEdit->SetSize( GetClientSize() );
			m_textEdit->SetSelection( -1, -1 );
			m_textEdit->Show();
			m_textEdit->SetFocus();
		}
	}
	else
	if( m_hover != hover || hoverPart != m_hoverPart || event.Leaving() )
	{
		m_hoverInset = false;
		Refresh( true );
	}
}

void CDynaPath::OnSubfolder( wxCommandEvent &event )
{
	Int32 id = event.GetId() - ID_DYNAPATH_FIRST_DIRECTORY;

	TSortedArray< String > directories;
	GetSortedDirectories( directories );
	String dirName = directories[ id ];

	if ( m_hover && directories.Size() > 0 )
	{
		for ( CDirectory* childDir : m_hover->GetDirectories() )
		{
			if( dirName == childDir->GetName() )
			{
				m_owner->SelectDirectory( childDir );
				break;
			}
		}
	}

	m_hover = NULL;
	Refresh( true );
}

void CDynaPath::OnTextEditKillFocus( wxFocusEvent& event )
{
	m_textEdit->Hide();
}

void CDynaPath::OnTextEditEnter( wxCommandEvent& event )
{
	String path = m_textEdit->GetValue().c_str();
	CDirectory* dir = GDepot->FindPath( path.AsChar() );
	if ( !dir )
	{
		wxMessageBox( wxT("Path doesn't exist"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		return;
	}

	// Go there
	m_owner->SelectDirectory( dir );
	m_owner->SelectFile( path );
	m_textEdit->Hide();
}

void CDynaPath::GetSortedDirectories( TSortedArray< String >& directories )
{
	directories.Clear();

	if ( m_hover && m_hover->GetDirectories().Size() > 0 )
	{
		directories.Reserve( m_hover->GetDirectories().Size() );

		for ( CDirectory* childDir : m_hover->GetDirectories() )
		{
			directories.PushBack( childDir->GetName() );
		}
		directories.Sort();
	}
}

void CDynaPath::OnPaint( wxPaintEvent &event )
{
	wxPaintDC realDC( this );
	wxBufferedDC dc( &realDC, GetClientSize() );

	dc.SetFont( m_font );

	// Clear background
	Int32 width, height;
	GetClientSize( &width, &height );
	dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) ) );
	dc.DrawRectangle( -1, -1, width + 2, height + 2 );

	if ( m_textEdit->IsShown() )
	{
		m_textEdit->Refresh();
		return;
	}

	m_rectDir.Clear();
	wxWindow *panel = m_owner->GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab *tab = m_owner->m_tabs.FindPtr( panel );
		if ( tab )
		{
			wxString caption;
			if ( tab->m_type == CEdAssetBrowser::ETT_CheckedOut )
			{
				caption = TXT("All checked out files");
			}
			else if ( tab->m_type == CEdAssetBrowser::ETT_RecentFiles )
			{
				caption = TXT("Recent files");
			}

			if( !tab->m_searchPhrase.Empty() )
			{
				caption = TXT("Search results: ");
				caption += tab->m_searchPhrase.AsChar();
			}
			if( !caption.IsEmpty() )
			{
				int w, h;
				dc.GetTextExtent( caption, &w, &h );
				Int32 y = ( height - h ) / 2;
				dc.DrawText( caption, 2, y );
				return;
			}
		}
	}
	else
	{
		return;
	}

	TDynArray< CDirectory* > dirs;

	Int32 x = 3;
	CDirectory *dir = m_owner->GetActiveDirectory();
	while( dir != NULL )
	{
		dirs.PushBack( dir );
		dir = dir->GetParent();
	}

	for( Int32 i=dirs.Size()-1; i>=0; i-- )
	{
		dir = dirs[ i ];

		Bool gotSubdirs = ( m_hover && m_hover->GetDirectories().Size() > 0 );

		{
			Int32 prevX = x;
			wxString caption;
			if ( dir->GetName().GetLength() )
			{
				caption = dir->GetName().AsChar();
			}
			else
			{
				caption = TXT( "Depot" );
			}

			int w, h;
			dc.GetTextExtent( caption, &w, &h );
			Int32 y = ( height - h ) / 2 - 1;

			wxRect r( x - 2, y - 2, 0, h + 5 );

			if( m_hover == dir && ( m_hoverPart == 0 || gotSubdirs ) && GetClientRect().Contains( ScreenToClient( wxGetMousePosition() ) ) )
			{
				if( m_hoverPart == 1 )
				{
					dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) ) );
					dc.DrawRectangle( x + w + 4, 1, m_bitmapOpened.GetWidth() + 6, height - 2 );
					dc.SetPen( *wxBLACK_PEN );
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
				}
				else
				{
					dc.SetPen( *wxTRANSPARENT_PEN );
					dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) ) );
					dc.DrawRectangle( r.x, 1, w + 8, height - 2 );
					dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) ) );
					dc.SetPen( *wxBLACK_PEN );
					dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
				}
			}
			else
			{
				dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
			}

			dc.DrawText( caption, x+2, y );

			x += w + 8;

			dc.DrawBitmap( m_bitmapOpened, x, ( height - m_bitmapOpened.GetHeight() ) / 2 + 1, true );
			x += m_bitmapOpened.GetWidth() + 2;

			r.SetWidth( x - prevX );

			m_rectDir.Set( dir, r );
		}
	}
}

void CDynaPath::OnEraseBackground( wxEraseEvent &event )
{
	// Do nothing, avoid flicker, be happy
}

void GetExpandedFromTree( wxTreeCtrl *m_sceneTree, String &expanded, String &selected, bool skipRoot )
{
	wxTreeItemId root = m_sceneTree->GetRootItem();
	if( root.IsOk() && m_sceneTree->IsExpanded( root ) )
	{
		TDynArray< TPair< wxTreeItemId, String > > stack;
		stack.PushBack( TPair< wxTreeItemId, String >( root, String( m_sceneTree->GetItemText( root ).wc_str() ) + TXT("/") ) );
		while( !stack.Empty() )
		{
			TPair< wxTreeItemId, String > b = stack.PopBack();
			if( !skipRoot || b.m_first != root )
				expanded += b.m_second + TXT(";");
			wxTreeItemIdValue cookie;
			wxTreeItemId i = m_sceneTree->GetFirstChild( b.m_first, cookie );
			while( i.IsOk() )
			{
				String id = b.m_second + m_sceneTree->GetItemText( i ).wc_str() +  TXT( "/" );
				if( m_sceneTree->IsExpanded( i ) )
					stack.PushBack( TPair< wxTreeItemId, String >( i, id ) );
				if( m_sceneTree->IsSelected( i ) )
					selected = id;
				i = m_sceneTree->GetNextChild( b.m_first, cookie );
			}
		}
	}
}

void SetExpandedToTree( wxTreeCtrl *m_sceneTree, String expandedStr, String selected, bool skipRoot )
{
	if( expandedStr.Empty() && selected.Empty() )
		return;

	TSet< String > expanded;
	expandedStr.Slice( expanded, TXT( ";" ) );

	wxTreeItemId root = m_sceneTree->GetRootItem();
	//m_sceneTree->UnselectAll();

	TDynArray< TPair< wxTreeItemId, String > > stack;
	stack.PushBack( TPair< wxTreeItemId, String >( root, String( m_sceneTree->GetItemText( root ).wc_str() ) + TXT("/") ) );
	while( !stack.Empty() )
	{
		TPair< wxTreeItemId, String > b = stack.PopBack();

		if( b.m_first == root || expanded.Find( b.m_second ) != expanded.End() )
		{
			if( !skipRoot || b.m_first != root )
				m_sceneTree->Expand( b.m_first );
			wxTreeItemIdValue cookie;
			wxTreeItemId i = m_sceneTree->GetFirstChild( b.m_first, cookie );
			while( i.IsOk() )
			{
				stack.PushBack( TPair< wxTreeItemId, String >( i, b.m_second + m_sceneTree->GetItemText( i ).wc_str() +  TXT( "/" ) ) );
				i = m_sceneTree->GetNextChild( b.m_first, cookie );
			}
		}
		else
		if( !skipRoot || b.m_first != root )
			m_sceneTree->Collapse( b.m_first );
		
		if( b.m_second == selected && ( !skipRoot || b.m_first != root ) )
			m_sceneTree->SelectItem( b.m_first );
	}

}

CEdAssetBrowser::CEdAssetBrowser( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("AssetBrowser"), true )
    , m_depotTree( nullptr )
	, m_classFilterBox( nullptr )
	, m_lockTree( false )
	, m_savePerSession( false )
	, m_resToRestartEditorWith( nullptr )
	, m_scanningDepot( false )
	, m_searchDirectory ( nullptr )
	, m_repopulateDirOnActivation( false )
	, m_autoUpdateIcons( false )
{
	// Load designed frame from resource
	//wxXmlResource::Get()->LoadPanel( this, parent, TXT("AssetBrowserPanelNew") );

    // Set icon
    wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_DEPOT") ) );
	SetIcon( iconSmall );

	// Set default size
	//SetSize( 800, 600 );
	//Center();

	m_search = new wxSearchCtrl( XRCCTRL( *this, "SearchPanel", wxPanel ), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_search->SetWindowStyleFlag( wxTE_PROCESS_ENTER );

	m_searchInCurrentDir.AddValue( false, SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_DEPOT" ) ) );
	m_searchInCurrentDir.AddValue( true, SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_DIR_CLOSED" ) ) );
	m_searchInCurrentDir.AddButton( XRCCTRL( *this, "BtnSearchDirectory", wxBitmapButton ) );
	m_searchInCurrentDir.SetValue( false );

	String skippedPathsStr = Config::cvSkippedSearchPaths.Get();
	if ( skippedPathsStr.Empty() == false )
	{
		m_skippedDirs = skippedPathsStr.Split( TXT(";") );
	}

	// Grab pointers to dialog items
	m_depotTree = XRCCTRL( *this, "DepotTree", wxTreeCtrl );
	m_depotTree->SetWindowStyle( wxTR_DEFAULT_STYLE | wxTR_SINGLE | wxNO_BORDER );

	// Set drop target
	m_depotTreeDropTarget = new CDropTargetTreeDepot(m_depotTree, this);

    m_depotTree->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdAssetBrowser::OnTreeKey ), NULL, this );
	if ( ::IsThemeActive() )
	{
		XRCCTRL( *this, "LeftPanel", wxPanel )->SetWindowStyle( wxDOUBLE_BORDER | wxTAB_TRAVERSAL );
	}

	m_classFilterBox = XRCCTRL( *this, "ClassFilter", wxChoice );

	// Set images for tree view
	wxImageList* images = new wxImageList( 16, 16, true, 2 );
	m_imgDepotId	= images->Add( SEdResources::GetInstance().LoadBitmap( TXT("IMG_DEPOT") ) );
	m_imgOpenId		= images->Add( SEdResources::GetInstance().LoadBitmap( TXT("IMG_DIR_OPENED") ) );
	m_imgClosedId	= images->Add( SEdResources::GetInstance().LoadBitmap( TXT("IMG_DIR_CLOSED") ) );
	m_imgChangedId	= images->Add( SEdResources::GetInstance().LoadBitmap( TXT("IMG_DIR_CHANGED") ) );
	m_depotTree->AssignImageList( images );

	m_lockedTabBitmap = SEdResources::GetInstance().LoadBitmap( _T("IMG_CHECK_IN") );


	m_splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );

	wxPanel* dynaPathPanel = XRCCTRL( *this, "DynaPathPanel", wxPanel );
	m_dynaPath = new CDynaPath( dynaPathPanel, this );
	dynaPathPanel->GetSizer()->Add( m_dynaPath, 0, wxEXPAND, 0 );

	// setting noteBook and its pages

	wxPanel *innerPanel = XRCCTRL( *this, "InnerPanel", wxPanel );
	m_noteBook = new CWxAuiNotebook( innerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON );
	innerPanel->GetSizer()->Add( m_noteBook, 1, wxEXPAND, 5 );

	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabChange ), NULL, this );
	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabDragDone ), NULL, this );
	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_BEGIN_DRAG, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabDragBegin ), NULL, this );
	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabClose ), NULL, this );
	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_DOWN, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabContextMenu ), NULL, this );
	m_noteBook->Connect( wxEVT_COMMAND_AUINOTEBOOK_BG_DCLICK, wxAuiNotebookEventHandler( CEdAssetBrowser::OnTabHeaderDblClick ), NULL, this );

	m_timer.Connect( wxEVT_TIMER, wxCommandEventHandler( CEdAssetBrowser::OnRefreshTab ), NULL, this );

	String p4Path = TXT("z:");
	String npath = TXT("");

	if( !SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path ) )
	{
		// By the default aritst should have z: setup like that
		SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path );
	}

	if ( SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path ) )
	{
		if( p4Path != TXT("") )
		{
			npath = p4Path + TXT("\\W3_Assets\\");
			// Setup source assets file watcher
			m_sourceAssetDirWatcher = 
				new CDirectoryWatcher( 
				npath, this, FAF_Removed | FAF_Modified
				);
		}
	}

	if( !SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("AutoUpdateIconInAssetBrowser"), m_autoUpdateIcons ) )
	{
		SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("AutoUpdateIconInAssetBrowser"), m_autoUpdateIcons );
	}

	m_resourceEditors.Clear();
	
	SEvents::GetInstance().RegisterListener( CNAME( FileEdited ), this );
	SEvents::GetInstance().RegisterListener( CNAME( VersionControlStatusChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ResourcePreSave ), this );

	GetOriginalFrame()->Bind( wxEVT_ACTIVATE, &CEdAssetBrowser::OnActivate, this );

	// Update and finalize layout
	LayoutRecursively( this );

	// Update filter
	UpdateClassFilter();
	UpdateDepotTree();
	UpdateImportClasses();

	SetConnections();
	RefreshIcons();
}

void CEdAssetBrowser::OnTabDragBegin( wxAuiNotebookEvent &event )
{
	m_dragBegin = event.GetSelection();
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdAssetBrowser::OnTabDragDone( wxAuiNotebookEvent &event )
{
	Int32 target = event.GetSelection();
	if( m_dragBegin == target )
	{
		return;
	}

	THashMap< Int32, wxWindow* > oldOrder;
	for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
	{
		assert( !oldOrder.KeyExist( it->m_second.m_order ) );
		oldOrder.Insert( it->m_second.m_order, it->m_first );
	}

	if( target < m_dragBegin )
	{
		for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
		{
			if( it->m_second.m_order == m_dragBegin )
			{
				it->m_second.m_order = target;
			}
			else
			if( it->m_second.m_order >= target && it->m_second.m_order < m_dragBegin )
			{
				it->m_second.m_order ++;
			}
		}
	}
	else
	{
		for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
		{
			if( it->m_second.m_order == m_dragBegin )
			{
				it->m_second.m_order = target;
			}
			else
			if( it->m_second.m_order > m_dragBegin && it->m_second.m_order <= target )
			{
				it->m_second.m_order --;
			}
		}
	}

	THashMap< Int32, wxWindow* > order;
	for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
	{
		assert( !order.KeyExist( it->m_second.m_order ) );
		order.Insert( it->m_second.m_order, it->m_first );
	}
}

void CEdAssetBrowser::OnTreeKey( wxKeyEvent& event )
{
	switch ( event.GetKeyCode() )
	{
	case WXK_TAB:
		{
			wxWindow *current = GetCurrentPage();
			if( current )
			{
				CEdAssetBrowserTab* tabData = m_tabs.FindPtr( current );
				if ( tabData )
				{
					tabData->m_view->CheckActive();
					tabData->m_view->SetFocus();
				}
			}
			return;
		}
	case 'B':
		{
			if( event.ControlDown() )
			{
				OnFlatDirectory( wxCommandEvent() );
				return;
			}
		}
	}
	event.Skip( true );
}

void CEdAssetBrowser::OnSearch()
{
	m_search->SetSelection( 0, 999 );
	m_search->SetFocus();
}

void CEdAssetBrowser::OnNewFolderButton( wxCommandEvent &event )
{
	wxWindow *page = GetCurrentPage();

	if( page )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if ( tabData )
		{
			Bool checkActive = ( tabData->m_view->GetSize() == 0 );

			UpdateResourceList();

			CDirectory *directory = tabData->m_directory;

			if ( directory )
			{
				String dirName;
				// Ask for directory name
				if ( InputBox( this, TXT("Add directory"), TXT("Enter name of new directory:"), dirName ) )
				{
					if ( ( !dirName.ContainsCharacter( TXT( ' ' ) ) ) && (dirName.ToLower() == dirName ) ) 
					{
						// Create directory
						CDirectory* dir = directory->CreateNewDirectory( dirName );

						// Create directory on disk
						Bool success = dir->CreateOnDisk();
						if( success == false )
						{
							GFeedback->ShowWarn( TXT( "Failed to create directory %s." ), dir->GetAbsolutePath().AsChar() );
						}

						m_addedDirectories.Insert( dir );

						// Reset filter
						m_classFilterBox->SetSelection( 0 );
						OnClassFilterChanged( event );

						// Change filter to show the new directory
						UpdateDepotTree();

						// Redraw shit
						SelectDirectory( dir );
					}
					else
					{
						wxMessageDialog dialog( 0, wxT("Cannot create dir with whitespaces or large letters"), wxT("Error"), wxOK | wxICON_ERROR );
						dialog.ShowModal();
					}
				}
				return;
			}
		}
	}

	wxMessageBox( wxT("Please select a folder view first"), wxT("New folder"), wxCENTRE|wxOK, this );
}

void CEdAssetBrowser::OnTreeButton( wxCommandEvent &event )
{
	if( m_splitter->GetSashPosition() < 10 )
	{
		SetTreeVisible( true );
	}
	else
	{
		SetTreeVisible( false );
	}
}

void CEdAssetBrowser::OnSplitterUnsplit( wxSplitterEvent &event )
{
	SetTreeVisible( false );
}

void CEdAssetBrowser::OnSplitterDoubleClick( wxSplitterEvent &event )
{
	SetTreeVisible( false );
	event.Veto();
}

void CEdAssetBrowser::SetTreeVisible( Bool visible, Int32 sash )
{
	wxBitmapButton *b = XRCCTRL( *this, "BtnTree", wxBitmapButton );
	if( visible )
	{
		if( sash == -1 )
			sash = GetScreenRect().GetWidth() / 3;
		if( !m_splitter->IsSplit() )
		{
			wxPanel *leftPanel = XRCCTRL( *this, "LeftPanel", wxPanel );
			wxPanel *rightPanel = XRCCTRL( *this, "InnerPanel", wxPanel );
			m_splitter->SplitVertically( leftPanel, rightPanel, sash );
		}
		else
			m_splitter->SetSashPosition( sash );
		b->SetBitmapLabel( SEdResources::GetInstance().LoadBitmap( TXT("IMG_ARROW_LEFT") ) );
        b->SetToolTip( SEdShortcut::AppendToolTipShortcut( TXT("Hide tree"),
                       SEdShortcut::GetToolTipShortcut(b->GetToolTip()->GetTip()) ));
	}
	else
	{
		if( m_splitter->IsSplit() )
			m_splitter->SetSashPosition( 1 );
		b->SetBitmapLabel( SEdResources::GetInstance().LoadBitmap( TXT("IMG_ARROW_RIGHT") ) );
		b->SetToolTip( SEdShortcut::AppendToolTipShortcut( TXT("Show tree"),
                       SEdShortcut::GetToolTipShortcut(b->GetToolTip()->GetTip()) ));
	}
}

wxWindow* CEdAssetBrowser::EditAsset( CResource* res, TDynArray< SDataError >* dataErrors /*= nullptr*/ )
{
	if( res->GetFile() ) SEvents::GetInstance().DispatchEvent( CNAME( FileEdited ), CreateEventData< String >( res->GetFile()->GetDepotPath() ) );

	wxWindow* editor = NULL;
	if ( !m_resourceEditors.Find( res, editor ) )
	{
		// Grab the latest file status from VC
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		CDiskFile* resourceFile = res->GetFile();
		if ( resourceFile )
		{
			resourceFile->GetStatus();
		}
#endif

		// Create new editor
		if ( CSwfResource* swfRes= Cast< CSwfResource >( res ) )
		{
			editor = new CEdSwfViewer( nullptr, swfRes );
		}
		else if ( IGuiResource* guiRes = Cast< IGuiResource >( res ) )
		{
			editor = new CEdGuiResourceEditor( nullptr, guiRes );
		}
		else if ( CMaterialGraph* matGraphRes = Cast< CMaterialGraph >( res ) )
		{		
			// Create material editor
			editor = new CEdMaterialEditor( nullptr, matGraphRes );
		}
		//special case...
		else if ( res->IsA< CBitmapTexture >() || res->IsA< CCubeTexture >() )
		{		
			// Create material editor
			editor = new CEdTextureViewer( 0, res );
		}
		else if ( CMeshTypeResource* meshRes = Cast< CMeshTypeResource >( res ) )
		{
			// Create mesh editor
			CEdMeshEditor* meshEditor = new CEdMeshEditor( nullptr, meshRes );

			// Set information about errors that occurred during loading resource
			if( dataErrors != nullptr )
			{
				meshEditor->AddDataErrors( *dataErrors );
			}

			editor = meshEditor;
		}
		else if ( CCharacterEntityTemplate* charEntRes = Cast< CCharacterEntityTemplate >( res ) )
		{
			if ( GGame->IsActive() )
			{
				wxMessageBox( TXT("Cannot edit character entity while game is active.") );
				return NULL;
			}
			// Create entity template editor
			editor = new CEdCharacterEntityEditor( nullptr, charEntRes );
		}
		else if ( CEntityTemplate* entRes = Cast< CEntityTemplate >( res ) )
		{
			// Create entity template editor
			CEdEntityEditor* entityEditor = new CEdEntityEditor( nullptr, entRes );

			// Set information about errors that occurred during loading resource
			if( dataErrors != nullptr )
			{
				entityEditor->AddDataErrors( *dataErrors );
			}
			editor = entityEditor;
		}
		else if ( CBehaviorGraph* behRes = Cast< CBehaviorGraph >( res ) )
		{		
			// Create behavior graph editor
			editor = new CEdBehaviorEditor( nullptr, behRes );
		}
		else if ( CParticleSystem* particleRes = Cast< CParticleSystem >( res ) )
		{		
			// Create particle system editor
			editor = new CEdParticleEditor( nullptr, particleRes );
		}
		else if ( CCommunity* commRes = Cast< CCommunity >( res ) )
		{
			editor = new CEdCommunityEditor( nullptr, commRes );
		}
		else if ( C2dArray* twoDimArrRes = Cast< C2dArray >( res ) )
		{
			// Create 2d array editor
			editor = new CEd2dArrayEditor( nullptr, twoDimArrRes );
		}
		else if ( CCutsceneTemplate* cutRes = Cast< CCutsceneTemplate >( res ) )
		{
			editor = new CEdCutsceneEditor( nullptr, cutRes );
		}
		else if ( CStorySceneDialogset* storyRes = Cast< CStorySceneDialogset >( res ) )
		{
			editor = new CEdSceneDialogsetEditor( nullptr, storyRes );
		}
		else if ( CSkeletalAnimationSet* animsetRes = Cast< CSkeletalAnimationSet >( res ) )
		{
			editor = new CEdAnimBrowser( nullptr, animsetRes );
		}
		else if ( CStoryScene* storyRes = Cast< CStoryScene >( res ) )
		{
			editor = new CEdSceneEditor( nullptr, storyRes);
		}
		else if( CBehTree* behTreeRes = Cast< CBehTree >( res ) )
		{
			editor = new CEdBehTreeEditor( nullptr, new CBehTreeEditedResource( behTreeRes ) );
		}		
		else if ( CQuestPhase* questPhaseRes = Cast< CQuestPhase >( res ) )
		{
			editor = new CEdQuestEditor( nullptr, questPhaseRes );
		}
		else if ( CWorld* worldRes = Cast< CWorld >( res ) )
		{
			wxTheFrame->OpenWorld( worldRes->GetFile()->GetDepotPath() );
			return nullptr;
		}
		else if ( CJournalResource* journalRes = Cast< CJournalResource >( res ) )
		{
			THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( journalRes->Get(), journalRes );
			wxTheFrame->OpenJournalEditor( path );
			return nullptr;
		}
		else if ( CCharacterResource* charRes = Cast< CCharacterResource >( res ) )
		{
			wxTheFrame->OpenCharacterDBEditor( charRes );
			return nullptr;
		}
		else if ( CExtAnimEventsFile* animEventRes = Cast< CExtAnimEventsFile >( res ) )
		{
			CEdAnimEventsEditor* e = new CEdAnimEventsEditor( nullptr, animEventRes );
			if( !e->InitWidget() )
			{
				e->Destroy();
				return nullptr;
			}
			else
			{
				editor = e;
			}
		}
		else if ( CJobTree* jobRes = Cast< CJobTree >( res ) )
		{
			editor = new CEdJobTreeEditor( nullptr, jobRes );
		}
		else if( CMoveSteeringBehavior* steeringRes = Cast< CMoveSteeringBehavior >( res ) )
		{
			editor = new CEdSteeringEditor( nullptr, steeringRes);
		}
		else if ( CSkeleton* skelRes = Cast< CSkeleton >( res ) )
		{
			editor = new CEdSkeletonEditor( nullptr, skelRes );
		}
		else if ( CTextureArray* textArrRes = Cast< CTextureArray >( res ) )
		{
			editor = new CEdTextureArrayViewer( nullptr, textArrRes );
		}
		// another special case...
		else if ( res->IsA< CEnvironmentDefinition >() )
		{
			wxTheFrame->OpenWorldEnvironmentEditor();
			wxTheFrame->EditEnvironmentParams( res );
			return nullptr;
		}
		else if ( CSpawnTree* spawnTreeRes = Cast< CSpawnTree >( res ) )
		{
			editor = new CEdEncounterEditor( nullptr, spawnTreeRes );
		}
		else if ( CResourceSimplexTree* simplexRes = Cast< CResourceSimplexTree >( res ) )
		{
			editor = new CEdSimplexTreeEditor( nullptr, simplexRes );
		}
		else if ( CGameResource* gameRes = Cast< CGameResource >( res ) )
		{
			editor = new CGameResourceEditor( nullptr, gameRes );
		}
		else
		{
			// Default case - show generic editor (properties)
			editor = new CEdGenericResourceEditor( 0, res );
		}

		// Listen when the window is closed
		editor->Bind( wxEVT_CLOSE_WINDOW, &CEdAssetBrowser::OnEditorClosed, this );

		// Listen when the window is destroyed
		editor->Bind( wxEVT_DESTROY, &CEdAssetBrowser::OnEditorDestroyed, this );
	}

	// Show the editor
	editor->Show();
	editor->SetFocus();
	editor->Raise();

	// Bind editor to the resource
	m_resourceEditors.Insert( res, editor );
	return editor;
}

Bool CEdAssetBrowser::IsResourceOpenedInEditor( CResource* res )
{
	// custom check for opened quest phases
	if ( res && res->IsA< CQuestPhase >() )
	{
		for ( auto it = m_resourceEditors.Begin(); it != m_resourceEditors.End(); ++it )
		{
			if ( CQuestPhase* openedQuestPhase = Cast< CQuestPhase >( it->m_first.Get() ) )
			{
				if ( IsQuestPhaseOpenedInEditor( res, openedQuestPhase ) )
				{
					return true;
				}
			}
		}
	}

	// check for all other resources
	return m_resourceEditors.FindPtr( res ) != NULL;
}

Bool CEdAssetBrowser::IsQuestPhaseOpenedInEditor( CResource* res, CQuestPhase* phase )
{
	TDynArray< CResource* > usedResources;

	if ( res == phase )
	{
		// this phase is opened in quest editor, return true
		return true;
	}
	phase->CollectUsedResources( usedResources );
	for ( Uint32 i = 0; i < usedResources.Size(); i++ )
	{
		if ( usedResources[ i ]->IsA< CQuestPhase >() )
		{
			CQuestPhase* openedQuestPhase = Cast< CQuestPhase >( usedResources[ i ] );
			if ( IsQuestPhaseOpenedInEditor( res, openedQuestPhase ) )
			{
			    return true;
			}
		}
	}
	return false;
}

/*
Returns whether instance of Entity Template Editor is open.
*/
Bool CEdAssetBrowser::IsEntityTemplateEditorOpen() const
{
	for ( auto it = m_resourceEditors.Begin(), end = m_resourceEditors.End(); it != end; ++it )
	{
		THandle< CResource > res = it->m_first;
		if ( res && res->IsA< CEntityTemplate >() )
		{
			return true;
		}
	}

	return false;
}

void CEdAssetBrowser::OnEditorClosed( wxCloseEvent& event )
{
	wxWindow* window = static_cast< wxWindow* >( event.GetEventObject() );
	auto it = m_resourceEditors.FindByValue( window );
	ASSERT ( it != m_resourceEditors.End() );

	// Check if the window being closed is restartable and wants to be restarted
	// NOTE: this whole checking cannot be done OnEditorDestroyed, because that handler is called from the destructor
	//       where the type info is not preserved
	if ( IRestartable* restartableWnd = dynamic_cast< IRestartable* >( it->m_second ) )
	{
		if ( restartableWnd->ShouldBeRestarted() )
		{
			// Store for handling in OnEditorDestroyed
			m_resToRestartEditorWith = it->m_first;
		}
	}

	// Continue evaluating this event at higher level
	event.Skip();
}

void CEdAssetBrowser::OnEditorDestroyed( wxWindowDestroyEvent& event )
{
	// Reset editor->resource binding
	wxWindow* window = (wxWindow* )event.GetEventObject();
	m_resourceEditors.EraseByValue( window );

	// Restart the editor if requested
	if ( m_resToRestartEditorWith )
	{
		EditAsset( m_resToRestartEditorWith.Get() );
		m_resToRestartEditorWith = nullptr;
	}

	// Continue evaluating this event at higher level
	event.Skip();
}

void CEdAssetBrowser::OnEditorReload(CResource* res, wxWindow* editor )
{
	auto resMapCopy = m_resourceEditors; // copy to not invalidate the iterator while erasing from the map
	for( auto it=resMapCopy.Begin(); it!=resMapCopy.End(); ++it )
	{
		if(it->m_second == editor)
		{
			wxSmartLayoutPanel* smartPanel = dynamic_cast< wxSmartLayoutPanel* >( *editor->GetChildren().begin() );
			if (smartPanel)
				m_resourceEditors.EraseByValue( smartPanel );
			else
				m_resourceEditors.EraseByValue( editor );

			m_resourceEditors.Insert( res, editor );
		}
	}
}

void CEdAssetBrowser::CloseAllEditors()
{
	auto eds = m_resourceEditors; // make copy!
	for ( auto it=eds.Begin(); it!=eds.End(); ++it )
	{
		it->m_second->Close();
	}
}

void CEdAssetBrowser::OnRefreshTab( wxCommandEvent& event )
{
	wxWindow *page = GetCurrentPage();
	if( page )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if ( tabData )
		{
			Bool checkActive = ( tabData->m_view->GetSize() == 0 );

			UpdateResourceList();

			//CDirectory *directory = tabData->m_directory;
			//if( directory )
			//{
			//	wxTreeItemId itemId;
			//	if ( m_dirMapping.Find( directory, itemId ) )
			//	{
			//		m_depotTree->SelectItem( itemId, true );
			//	}
			//}

			if( checkActive && tabData->m_view->GetSize() )
			{
				tabData->m_view->CheckActive();
			}
			tabData->m_view->SetFocus();
		}
	}
	m_dynaPath->Refresh( true );
	UpdateBookmarkButton();
}

void CEdAssetBrowser::OnAddTabButton( wxCommandEvent &event )
{
	EEditorResourceViewMode mode = ERVM_Big;
	CDirectory *directory = GDepot;
	wxWindow *current = GetCurrentPage();
	if ( current )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( current );
		if ( tabData )
		{
			mode = tabData->m_view->GetViewType();
			directory = tabData->m_directory;
		}
	}
	else
	{
		return;
	}

	if ( directory == NULL )
	{
		directory = GDepot;
	}
	
	wxWindow* newTab = AddTab( directory );

	CEdResourceView *ed = m_tabs.GetRef( newTab ).m_view;

	ed->SetViewType( mode );
}

void CEdAssetBrowser::OnShowInDirectoriesTree( wxCommandEvent& event )
{
	CPageWrapper* pw = (CPageWrapper*) event.GetClientData();
	wxWindow *page = ( pw != nullptr ) ? m_noteBook->GetPage( pw->m_page ) : GetCurrentPage();
	if( page != nullptr )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if( tabData != nullptr )
		{
			String tabPath = String::EMPTY;
			wxTreeItemId id;

			tabData->m_directory->GetDepotPath( tabPath );
			FindDepotTreeItemByPath( m_depotTree, tabPath, id );
			m_depotTree->SelectItem( id, true );
		}
	}
}

CDirectory*	CEdAssetBrowser::GetActiveDirectory()
{
	wxWindow *current = GetCurrentPage();
	if ( current )
	{
		CEdAssetBrowserTab* tabPtr = m_tabs.FindPtr( current );
		if ( tabPtr )
		{
			return tabPtr->m_directory;
		}
	}
	return NULL;
}

void CEdAssetBrowser::SaveOptionsToConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	String favClassesStr = String::Join( m_favClasses, TXT(";") );
	config.Write( TXT("/Frames/AssetBrowser/FavoriteClasses"), favClassesStr );

	config.Write( TXT("/Frames/AssetBrowser/Sash"), m_splitter->GetSashPosition() );

	for ( Uint32 i = 0; i < m_recentFiles.Size(); ++i )
	{
		if( !m_recentFiles[i] ) continue;
		config.Write( TXT("/Frames/AssetBrowser/RecentFiles/" + ToString( i ) ), m_recentFiles[i]->GetDepotPath() );
	}

	for ( Uint32 i = 0; i < m_bookmarks.Size(); ++i )
	{
		config.Write( TXT("/Frames/AssetBrowser/Bookmarks/Bookmark") + ToString( i ), m_bookmarks[i] );
	}

	String resources;
	for ( auto it=m_resourceEditors.Begin(); it!=m_resourceEditors.End(); ++it )
	{
		THandle< CResource > r = it->m_first;
		wxWindow *w = it->m_second;

		if ( !r || !r->GetFile() )
		{
			continue;
		}

		Bool              docked = false;
		wxTopLevelWindow* tw     = NULL;

		wxSmartLayoutPanel* smartPanel = dynamic_cast< wxSmartLayoutPanel* >( w );
		if (smartPanel)
		{
			tw = static_cast<wxTopLevelWindow*>( smartPanel->GetOriginalFrame() );
			docked = smartPanel->IsDocked();
		}
		else
		{
			tw = static_cast<wxTopLevelWindow*>( wxGetTopLevelParent(w) );
		}

		if ( !tw )
		{
			HALT( "No top window" );
			return;
		}

		wxRect geometry = tw->GetRect();

		String pathToResource = r->GetFile()->GetDepotPath();

		// This will skip the resource in the case where the path is invalid
		if ( !GDepot->FindFile( pathToResource ) )
		{
			continue;
		}

		resources += pathToResource + TXT(",") +
			ToString( geometry.GetLeft() ) + TXT(",") +
			ToString( geometry.GetTop() ) + TXT(",") +
			ToString( geometry.GetWidth() ) + TXT(",") +
			ToString( geometry.GetHeight() ) + TXT(",") +
			ToString( (int)tw->IsMaximized() ) + TXT(",") +
			ToString( (int)docked ) +
			TXT(";");

		ISavableToConfig *savable = dynamic_cast< ISavableToConfig* >( w );
		if( savable )
			savable->SaveSession( config );
	}
	config.Write( TXT("/Frames/AssetBrowser/Editors"), resources );
	config.Write( TXT("/Frames/AssetBrowser/SavePerSession"), m_savePerSession ? 1 : 0 );

	SaveLayout(TXT("/Frames/AssetBrowser"));

	if ( !m_savePerSession )
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/AssetBrowser") );
		SaveSessionInfo( config );
	}
}

void CEdAssetBrowser::LoadOptionsFromConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	String favClassesStr = config.Read( TXT("/Frames/AssetBrowser/FavoriteClasses"), String::EMPTY );
	m_favClasses = favClassesStr.Split( TXT(";") );

	Int32 sash = config.Read( TXT("/Frames/AssetBrowser/Sash"), 0 );
	if( sash >= 0 )
	{
		if( sash < 10 )
		{
			SetTreeVisible( false );
		}
		else
		{
			SetTreeVisible( true, sash );
		}
	}

	m_recentFiles.Clear();
	for ( Uint32 i=0; true; ++i )
	{
		String path = config.Read( TXT("/Frames/AssetBrowser/RecentFiles/") + ToString(i), String::EMPTY );
		if( path.Empty() )
			break;

		CDiskFile* file = GDepot->FindFile( path );
		if( file )
			m_recentFiles.PushBack( file );
	}

	m_bookmarks.Clear();
	for ( Uint32 i=0; true; ++i )
	{
		String path = config.Read( TXT("/Frames/AssetBrowser/Bookmarks/Bookmark") + ToString( i ), String::EMPTY );
		if ( path.Empty() )
		{
			break;
		}

		if ( GDepot->FindPath( path.AsChar() ) )
		{
			m_bookmarks.PushBackUnique( path );
		}
	}
	UpdateBookmarksMenu();

	CloseAllEditors();

	String eds = config.Read( TXT("/Frames/AssetBrowser/Editors"), String::EMPTY );
	if( !eds.Empty() )
	{
		TDynArray< String > editors;
		TDynArray< String > options;
		eds.Slice( editors, TXT(";") );
		for( TDynArray< String >::iterator it=editors.Begin(); it!=editors.End(); it++ )
		{
			options.Clear();
			( *it ).Slice( options, TXT(",") );

			CDiskFile* diskFile = GDepot->FindFile( options[0] );
			if( diskFile )
			{
				// Load resource
				diskFile->Load();

				// Check that resource was loaded
				if ( diskFile->IsLoaded() )
				{
					// Try to open existing resource editor
					CResource* res = diskFile->GetResource();

					wxWindow *wnd = EditAsset( res );
					if( wnd )
					{
						wxSmartLayoutPanel* smartPanel = dynamic_cast< wxSmartLayoutPanel* >( wnd );
						if (smartPanel)
						{
							Int32 docked = 0;
							if ( options.Size() > 6 && FromString( options[6], docked ) )
							{
								if (docked)
									smartPanel->Dock();
								else
									smartPanel->Undock();
							}
							else
								smartPanel->Undock();
						}

						Int32 x, y, w, h;
						if( FromString( options[1], x ) && FromString( options[2], y ) && FromString( options[3], w ) && FromString( options[4], h ) )
							if( w > 0 && h > 0 )
								if (smartPanel)
									smartPanel->GetOriginalFrame()->SmartSetSize( x, y, w, h );
								else
									wnd->SetSize( x, y, w, h );

						ISavableToConfig *savable = dynamic_cast< ISavableToConfig* >( wnd );
						if( savable )
							savable->RestoreSession( config );
					}
				}
			}

		}
	}

    CEdShortcutsEditor::Load(*this->GetMenuBar(), TXT("Asset Browser"), wxString(), true, false);
    CEdShortcutsEditor::Load(*this, *GetAccelerators(), TXT("Asset Browser"), false, true);

	LoadLayout(TXT("/Frames/AssetBrowser"));

	Int32 savePerSession = 0;
	config.Read( TXT("/Frames/AssetBrowser/SavePerSession"), savePerSession );
	m_savePerSession = ( savePerSession == 1 );
	GetMenuBar()->FindItem( XRCID( "savePerSession" ) )->Check( m_savePerSession );

	if ( !m_savePerSession )
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/AssetBrowser") );
		RestoreSessionInfo( config );
	}

	UpdateBookmarkButton();
}

void CEdAssetBrowser::SaveSession( CConfigurationManager &config )
{
	if ( m_savePerSession )
		SaveSessionInfo( config );
}

void CEdAssetBrowser::RestoreSession( CConfigurationManager &config )
{
	if ( m_savePerSession )
		RestoreSessionInfo( config );

	// Update layout of top window, so the size of resource view window will be correct
	LayoutRecursively( this );
	UpdateBookmarkButton();
}

void CEdAssetBrowser::SaveSessionInfo( CConfigurationManager &config )
{
    String expanded, selected;
	//GetExpandedFromTree( m_depotTree, expanded, selected, true );

	config.Write( TXT("Frames/AssetBrowser/Expanded"), expanded );
	//config.Write( TXT("Frames/AssetBrowser/Selected"), selected );

	config.DeleteDirectory( TXT("Frames/AssetBrowser/Tabs") );

	THashMap< Int32, wxWindow* > order;
	for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
	{
		assert( !order.KeyExist( it->m_second.m_order ) );
		order.Insert( it->m_second.m_order, it->m_first );
	}

	wxWindow *current = GetCurrentPage();
	Int32 activeSelection = 0;
	Int32 i=0;
	for( THashMap< Int32, wxWindow* >::iterator it=order.Begin(); it!=order.End(); ++it )
	{
		if( m_tabs.KeyExist( it->m_second ) )
		{
			if( current == it->m_second )
			{
				activeSelection = i;
			}

			CEdAssetBrowserTab *tabPtr = m_tabs.FindPtr( it->m_second );
			if ( !tabPtr )
			{
				continue;
			}

			CEdAssetBrowserTab &tab = *tabPtr;

			TSet< CDiskFile* > activeSet;
			String active;
			tab.m_view->GetActive( activeSet );
			if( !activeSet.Empty() )
			{
				for( TSet< CDiskFile* >::iterator it=activeSet.Begin(); it!=activeSet.End(); it++ )
					active += ( *it )->GetDepotPath() + TXT(";");
			}

			String cp = TXT("Frames/AssetBrowser/Tabs/") + ToString( i );
			config.Write( cp + TXT("/Path"), tab.m_directory ? tab.m_directory->GetDepotPath() : String::EMPTY );
			config.Write( cp + TXT("/SearchPhrase"), tab.m_searchPhrase );
			if ( tab.m_searchFilter != NULL )
			{
				config.Write( cp + TXT("/SearchFilter"), tab.m_searchFilter->GetName().AsString() );
			}
			config.Write( cp + TXT("/Type"), tab.m_type );
			config.Write( cp + TXT("/Active"), active );
			config.Write( cp + TXT("/ListType"), tab.m_view->GetViewType() );
			config.Write( cp + TXT("/Scroll"), tab.m_view->GetScroll() );
			config.Write( cp + TXT("/Lock"), tab.m_lock ? 1 : 0 );
			config.Write( cp + TXT("/Flat"), tab.m_flat ? 1 : 0 );

			if( tab.m_view->GetActive() != NULL )
				config.Write( cp + TXT("/Current"), tab.m_view->GetActive()->GetDepotPath() );

			i ++;
		}
	}

	config.Write( TXT("Frames/AssetBrowser/Tabs/Active"), activeSelection );
}

void CEdAssetBrowser::RestoreSessionInfo( CConfigurationManager &config )
{
	//m_noteBook->Freeze();
	for( Int32 p=m_noteBook->GetPageCount()-1; p>=0; p-- )
	{
		m_noteBook->DeletePage( p );
	}
	m_tabs.Clear();
	//m_noteBook->Thaw();
	Int32 i=-1;
	while( 1 )
	{
        i ++; // moved to beginning, to avoid missing increments on "continue" command.
		if( i > 10 )
			break;
		String cp = TXT("Frames/AssetBrowser/Tabs/") + ToString( i );
		String path;

		Int32 type = config.Read( cp + TXT("/Type"), -1 );
		if( type == -1 )
		{
			// No more tabs
			break;
		}
		else if( type == ETT_Folder )
		{
			if( !config.Read( cp + TXT("/Path"), &path ) )
				break;
		}
		wxWindow *tab;
        CDirectory *directory = NULL;
		String searchPhrase;
		CClass* searchFilter = NULL;

		if( type == ETT_Folder )
		{
			if( !path.Empty() )
			{
				directory = GDepot->FindPath( path.AsChar() );
				if( !directory )
                    continue;
			}
			else
			{
				directory = GDepot;
			}

			searchPhrase = config.Read( cp + TXT("/SearchPhrase"), searchPhrase );
			if( !searchPhrase.Empty() )
			{
				String className = config.Read( cp + TXT("/SearchFilter"), TXT("") );
				if ( ! className.Empty() )
				{
					searchFilter = SRTTI::GetInstance().FindClass( CName( className ) );
				}
			}
		}
#if 1 // we'll ignore searches for now, they slow down the on-demand filesystem a lot
		if ( !searchPhrase.Empty() )
		{
			continue;
		}
#endif

		tab = AddTab( directory, ETabType(type), searchPhrase, searchFilter );

		Int32 viewType = config.Read( cp + TXT("/ListType"), 0 );
		m_tabs.GetRef( tab ).m_view->SetViewType( viewType == ERVM_List ? ERVM_List : ( viewType == ERVM_Small ? ERVM_Small : ERVM_Big ) );

#if 0 // too slow
		if( !searchPhrase.Empty() )
		{
			TDynArray< CDiskFile* > result;
			if ( directory )
			{
				directory->Search( searchPhrase, result );
			}
			else
			{
				GDepot->Search( searchPhrase, result );
			}

			if ( searchFilter )
			{
				// Filter by class
				CResource* res = searchFilter->GetDefaultObject< CResource >();
				if ( res )
				{
					for ( Int32 i = (Int32)result.Size()-1; i >= 0; --i )
					{
						CFilePath path( result[i]->GetFileName() );
						if ( ! path.GetExtension().EqualsNC( res->GetExtension() ) )
						{
							result.Erase( result.Begin() + i );
						}
					}
				}
			}

			m_tabs.GetRef( tab ).m_view->ListFiles( result );
		}
#endif
		Int32 lock = config.Read( cp + TXT("/Lock"), 0 );
		Int32 flat = config.Read( cp + TXT("/Flat"), 0 );
		m_tabs.GetRef( tab ).m_lock = lock==1;
		m_tabs.GetRef( tab ).m_flat = flat==1;

		SetTabTitle( m_noteBook->GetPageCount() - 1 );

		UpdateResourceList( m_tabs.GetRef( tab ) );

		String active = config.Read( cp + TXT("/Active"), String::EMPTY );
		String current = config.Read( cp + TXT("/Current"), String::EMPTY );
		if( !active.Empty() )
		{
			CDiskFile *curr = NULL;
			TSet< CDiskFile* > activeSet;
			TDynArray< String  > a;
			active.Slice( a, TXT(";") );

			for( TDynArray< String  >::iterator it=a.Begin(); it!=a.End(); it++ )
			{
				CDiskFile* df = GDepot->FindFile( *it );
				if( df )
				{
					activeSet.Insert( df );
					if( *it == current )
						curr = df;
				}
			}
			m_tabs.GetRef( tab ).m_view->SetActive( activeSet, curr );
		}

		Int32 scroll = config.Read( cp + TXT("/Scroll"), -1 );
		if( scroll != -1 )
		{
			m_tabs.GetRef( tab ).m_view->Scroll( scroll );
		}
	}

	String selected = String::EMPTY; //config.Read( TXT("Frames/AssetBrowser/Selected"), String::EMPTY );
	String expanded = config.Read( TXT("Frames/AssetBrowser/Expanded"), String::EMPTY );
	m_lockTree = true;
	//SetExpandedToTree( m_depotTree, expanded, selected, true );
	m_lockTree = false;

	Uint32 active = config.Read( TXT("Frames/AssetBrowser/Tabs/Active"), 0 );

	if( m_noteBook->GetPageCount() == 0 )
	{
		AddTab( GDepot, ETT_Folder, String::EMPTY );
	}
	else
	{
		if( m_noteBook->GetPageCount() > active )
			m_noteBook->SetSelection( active );
	}

	m_dynaPath->Refresh( true );
	UpdateBookmarkButton();
}


TEdShortcutArray *CEdAssetBrowser::GetAccelerators()
{
    if (m_shortcuts.Empty())
    {
        struct ControlShortcutAdder
        {
            static bool Add(TEdShortcutArray &shortcuts, wxWindow &parent, Int32 xrcid, const wxString &id = wxString())
            {
                wxWindowBase *ctrl = parent.FindWindow(xrcid);
                if (!ctrl) return false;
                
                if (!id.empty())
                    shortcuts.PushBack(SEdShortcut(id, *ctrl));
                else
                if (!ctrl->GetLabel().empty())
                    shortcuts.PushBack(SEdShortcut(TXT("Toolbar\\") + ctrl->GetLabel(), *ctrl));
                else
                if (!ctrl->GetToolTip()->GetTip().empty())
                {
                    shortcuts.PushBack(SEdShortcut(TXT("Toolbar\\") + SEdShortcut::StripToolTipShortcut(ctrl->GetToolTip()->GetTip()), *ctrl));
                }
                else
                    return false;
                return true;
            }
        };

        ControlShortcutAdder::Add(m_shortcuts, *this, XRCID("BtnTree"));
        
        m_shortcuts.PushBack(SEdShortcut(TXT("Context Menu\\Delete"), wxAcceleratorEntry(0, WXK_DELETE, ID_DELETE_ASSET_AB ) ) );
		m_shortcuts.PushBack(SEdShortcut(TXT("Context Menu\\Copy"), wxAcceleratorEntry(wxACCEL_CTRL, 'C', ID_COPY_ASSET_AB ) ) );
		m_shortcuts.PushBack(SEdShortcut(TXT("Context Menu\\Paste"), wxAcceleratorEntry(wxACCEL_CTRL, 'V', ID_PASTE_ASSET_AB ) ) );
		m_shortcuts.PushBack(SEdShortcut(TXT("Context Menu\\Rename"), wxAcceleratorEntry(wxACCEL_SHIFT, WXK_F6, ID_RENAME_ASSET_AB ) ) );
		m_shortcuts.PushBack(SEdShortcut(TXT("Use path from clipboard"), wxAcceleratorEntry(0, 0, ID_USE_PATH_ASSET_AB ) ) );
    }

    return &m_shortcuts;
}

void CEdAssetBrowser::SetConnections()
{
	Connect( m_search->GetId(), wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdAssetBrowser::OnSearch ) );
	Connect( m_search->GetId(), wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( CEdAssetBrowser::OnSearch ) );
}

CEdAssetBrowser::~CEdAssetBrowser()
{
	delete m_depotTreeDropTarget;
	m_searchDirectory = nullptr;

	SaveOptionsToConfig();

    CloseAllEditors();

	SEvents::GetInstance().UnregisterListener( this );
}

Bool CEdAssetBrowser::DoesDirExist( CDirectory* directory ) const
{
	return m_dirMapping.Find( directory ) != m_dirMapping.End();
}

void CEdAssetBrowser::UpdateDepotTree()
{
	struct ScannedDirectory
	{
		String name;
		String path;
		TDynArray<ScannedDirectory*> children;
		ScannedDirectory(){}
		~ScannedDirectory()
		{
			for ( auto it=children.Begin(); it != children.End(); ++it )
			{
				delete *it;
			}
		}
		ScannedDirectory* Add( const String& newName )
		{
			ScannedDirectory* dir = new ScannedDirectory();
			dir->name = newName;
			dir->path = path + newName + TXT("\\");
			children.PushBack( dir );
			return dir;
		}
	};

	// Scans the depot structure using Win32 API directly
	struct DepotScanTask : public CEdRunnable
	{
		ScannedDirectory* m_root;
		DepotScanTask( ScannedDirectory* root ) : m_root( root ){}
		Bool vista;

		void Scan( ScannedDirectory* dir, const String& path )
		{
			WIN32_FIND_DATA data;
			HANDLE h;
			PVOID oldValue = NULL;

			::Wow64DisableWow64FsRedirection( &oldValue );
			h = ::FindFirstFileEx( ( path + TXT("\\*") ).AsChar(), vista ? FindExInfoStandard : FindExInfoBasic, &data, FindExSearchLimitToDirectories, NULL, vista ? 0 : FIND_FIRST_EX_LARGE_FETCH );
			if ( h != INVALID_HANDLE_VALUE )
			{
				while ( h != INVALID_HANDLE_VALUE )
				{
					// Skip . and ..
					if ( data.cFileName[0] == TXT('.') && ( data.cFileName[1] == TXT('\0') || ( data.cFileName[1] == TXT('.') && data.cFileName[2] == TXT('\0') ) ) )
					{
						// Get next file, if any
						if ( ::FindNextFile( h, &data ) )
						{
							continue;
						}
						// No more files, stop
						break;
					}

					// Scan subdirectory
					if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					{
						ScannedDirectory* subdir = dir->Add( data.cFileName );
						Scan( subdir, path + TXT("\\") + data.cFileName );
					}

					// Get next file, if any
					if ( !::FindNextFile( h, &data ) )
					{
						break;
					}
				}
				::FindClose( h );
			}
			::Wow64RevertWow64FsRedirection( oldValue );
		}

		virtual void Run()
		{
			String path = GDepot->GetRootDataPath();
			path.PopBack(); // remove trailing \

			// Detect OS (vista or newer?)
			OSVERSIONINFO info;
			info.dwOSVersionInfoSize = sizeof(info);
			if ( ::GetVersionEx( &info ) )
			{
				vista = info.dwMajorVersion == 6 && info.dwMinorVersion == 0;
			}
			else
			{
				vista = false; // failsafe
			}

			// Scan root
			Scan( m_root, path );
		}
	};

	// Called when the task above is finished
	struct ScanFinishTask : public CEdRunnable
	{
		ScannedDirectory* m_root;
		ScanFinishTask( ScannedDirectory* root ) : m_root( root ){}

		void FillDirectory( wxTreeCtrl* depotTree, ScannedDirectory* dir, wxTreeItemId item )
		{
			for( auto it=dir->children.Begin(); it != dir->children.End(); ++it )
			{
				ScannedDirectory* subdir = *it;
				wxTreeItemId levelItem = depotTree->AppendItem( item, subdir->name.AsChar(), 2 );
				if ( levelItem )
				{
					depotTree->SetItemData( levelItem, new wxTreeItemDataDir( NULL, subdir->path ) ); // NULL will cause an on-demand fill
					FillDirectory( depotTree, subdir, levelItem );
				}
			}
		}

		virtual void Run()
		{
			// Schedule the refresh for later, if the asset browser is not visible
			if ( !wxTheFrame->GetAssetBrowser()->IsShownOnScreen() )
			{
				ScanFinishTask* delayedTask = new ScanFinishTask( m_root );
				delayedTask->TriggerAfter( 0.5 ); // retry after half a second
				RunLaterOnceEx( delayedTask );
				return;
			}

			// Fill depot tree
			wxTreeCtrl* depotTree = wxTheFrame->GetAssetBrowser()->m_depotTree;
			depotTree->DeleteAllItems();
			depotTree->Freeze();

			// Fill root
			wxTreeItemId root = depotTree->AddRoot( TXT("Depot"), 0 );
			depotTree->SetItemData( root, new wxTreeItemDataDir( GDepot, String::EMPTY ) );
			wxTheFrame->GetAssetBrowser()->m_dirMapping.Insert( GDepot, root );
			FillDirectory( depotTree, m_root, root );

			// Update tree
			depotTree->Thaw();
			depotTree->Expand( root );
			depotTree->Refresh();
			wxTheFrame->GetAssetBrowser()->m_lockTree = false;

			// Mark scan as finished
			wxTheFrame->GetAssetBrowser()->m_scanningDepot = false;
			delete m_root;
		}
	};

	// Cannot start a new scan while another is in progress
	if ( m_scanningDepot )
	{
		return;
	}
	m_scanningDepot = true;

	// Add temporary node to inform users
	m_depotTree->DeleteAllItems();
	m_depotTree->AddRoot( TXT("Please wait, scanning...") );

	// Clear mappings
	m_dirMapping.Clear();

	// Lock the tree
	m_lockTree = true;

	ScanFinishTask* finishTask = new ScanFinishTask( new ScannedDirectory() );
	RunParallelEx( new DepotScanTask( finishTask->m_root ), finishTask );
}

void CEdAssetBrowser::UpdateClassFilter()
{
	// Begin update
	m_classFilterBox->Freeze();
	m_classFilterBox->Clear();
	m_classFilterBox->Append( TXT("(all)") );

	// Fill class list
	TDynArray< CClass* > resourceClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< CResource >(), resourceClasses );
	for ( Uint32 i=0; i<resourceClasses.Size(); i++ )
	{
		CClass* resClass = resourceClasses[i];

		// Hide CWorld and CLayer classes
		// if ( resClass == ClassID<CWorld>() || resClass == ClassID<CLayer>() )
		// {
		// 	continue;
		// }	

		// Add to list
		m_resourceClasses.PushBack( resClass );
		m_classFilterBox->Append( resClass->GetName().AsString().AsChar(), (void*) resClass );
	}

	// Done
	m_classFilterBox->SetSelection( 0 );
	m_classFilterBox->Thaw();

	// Update class filter
	wxCommandEvent e;
	OnClassFilterChanged( e );
}

void CEdAssetBrowser::UpdateResourceList( Bool repopulate /*=false*/ )
{
	wxWindow *page = GetCurrentPage();

	CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
	if ( tab )
	{
		if ( page && tab->m_view )
		{
			XRCCTRL( *this, "ViewType", wxChoice )->SetSelection( tab->m_view->GetViewType() );
		}

		Bool enabledLock = tab->m_searchPhrase.Empty() && tab->m_type == ETT_Folder;

		if( page )
		{
			if ( repopulate && tab->m_directory )
			{
				tab->m_directory->ForceRepopulate();
			}

			UpdateResourceList( *tab );
		}
	}
}

void CEdAssetBrowser::OnFlatDirectory( wxCommandEvent &event )
{
	CPageWrapper* pw = (CPageWrapper*) event.GetClientData();
	wxWindow *page = pw ? m_noteBook->GetPage( pw->m_page ) : GetCurrentPage();

	CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
	if ( tab )
	{
		if( page && tab && tab->m_searchPhrase.Empty() && tab->m_type == ETT_Folder )
		{
			tab->m_flat = !tab->m_flat;
			UpdateResourceList( *tab );
			tab->m_view->SetFocus();
			tab->m_view->CheckActive();
			SetTabTitle( m_noteBook->GetSelection() );
		}
	}
}

void CEdAssetBrowser::OnLockTab( wxCommandEvent &event )
{
	CPageWrapper* pw = (CPageWrapper*) event.GetClientData();
	wxWindow *page = pw ? m_noteBook->GetPage( pw->m_page ) : GetCurrentPage();

	CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
	if ( tab )
	{
		if( page && tab && tab->m_searchPhrase.Empty() && tab->m_type==ETT_Folder )
		{
			tab->m_lock = !tab->m_lock;
			tab->m_view->SetFocus();
			tab->m_view->CheckActive();

			SetTabTitle( m_noteBook->GetSelection() );
		}
	}
}

void CEdAssetBrowser::OnChangeView( wxCommandEvent &event )
{
	wxWindow *page = GetCurrentPage();
	if( page )
	{
		int val = XRCCTRL( *this, "ViewType", wxChoice )->GetCurrentSelection();

		CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
		if ( tab )
		{
			tab->m_view->SetFocus();
			tab->m_view->CheckActive();

			tab->m_view->SetViewType( (EEditorResourceViewMode) val );
		}
	}
}

void CEdAssetBrowser::OnSavePerSessionButton( wxCommandEvent &event )
{
	m_savePerSession = !m_savePerSession;
	GetMenuBar()->FindItem( XRCID( "savePerSession" ) )->Check( m_savePerSession );
}

void CEdAssetBrowser::OnEditCopyPath( wxCommandEvent &event )
{
	wxWindow *page = GetCurrentPage();
	if ( page )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if ( tabData )
		{
			CDirectory* directory = tabData->m_directory;
			if ( directory )
			{
				if ( !GClipboard->Copy( directory->GetDepotPath() ) )
				{
					wxMessageBox( wxT("Failed to copy the path to the clipboard"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
				}
			}
			else
			{
				wxMessageBox( wxT("This tab does not represent a directory in the depot"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
			}
		}
		else
		{
			wxMessageBox( wxT("No data for this tab"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		}
	}
	else
	{
		wxMessageBox( wxT("Failed to get the current page"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
	}
}

void CEdAssetBrowser::OnEditPastePath( wxCommandEvent &event )
{
	// Obtain path
	String path;
	if ( !GClipboard->Paste( path ) )
	{
		wxMessageBox( wxT("Failed to paste the path from the clipboard"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		return;
	}

	// Search for the directory
	CDirectory* dir = GDepot->FindPath( path.AsChar() );
	if ( !dir )
	{
		wxMessageBox( wxT("The clipboard does not contain a valid path"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		return;
	}

	// Go there
	SelectDirectory( dir );
	SelectFile( path );
}

void CEdAssetBrowser::OnToggleBookmark( wxCommandEvent &event )
{
	wxWindow *page = GetCurrentPage();
	if ( page )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if ( tabData )
		{
			CDirectory* directory = tabData->m_directory;
			if ( directory )
			{
				String path = directory->GetDepotPath();
				if ( !m_bookmarks.Exist( path ) )
				{
					m_bookmarks.PushBack( path );
				}
				else
				{
					m_bookmarks.Remove( path );
				}
				UpdateBookmarksMenu();
				UpdateBookmarkButton();
			}
			else
			{
				wxMessageBox( wxT("This tab does not represent a directory in the depot"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
			}
		}
		else
		{
			wxMessageBox( wxT("No data for this tab"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		}
	}
	else
	{
		wxMessageBox( wxT("Failed to get the current page"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
	}
}

void CEdAssetBrowser::OnBookmark( wxCommandEvent &event )
{
	Uint32 index = static_cast< Uint32 >( event.GetId() ) - ID_BOOKMARKS_FIRST;
	ASSERT( index >= 0 && index < m_bookmarks.Size(), TXT("Invalid bookmark menu id") );

	CDirectory* directory = GDepot->FindPath( m_bookmarks[index].AsChar() );
	if ( directory )
	{
		SelectDirectory( directory );
	}
	else
	{
		wxMessageBox( wxT("Failed to find the bookmarked directory"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
	}
}

void CEdAssetBrowser::OnCheckedOutButton( wxCommandEvent &event )
{
	for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
	{
		if( it->m_second.m_type == ETT_CheckedOut )
		{
			Int32 idx = m_noteBook->GetPageIndex( it->m_first );
			if( idx == m_noteBook->GetSelection() )
			{
				UpdateResourceList( it->m_second );
			}
			else
			{
				m_noteBook->SetSelection( idx );
			}

			return;
		}
	}

	AddTab( NULL, ETT_CheckedOut, String::EMPTY );
}

void CEdAssetBrowser::OnRecentFilesButton( wxCommandEvent &event )
{
	for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
	{
		if( it->m_second.m_type == ETT_RecentFiles )
		{
			Int32 idx = m_noteBook->GetPageIndex( it->m_first );
			if( idx == m_noteBook->GetSelection() )
			{
				UpdateResourceList( it->m_second );
			}
			else
			{
				m_noteBook->SetSelection( idx );
			}

			return;
		}
	}

	AddTab( NULL, ETT_RecentFiles, String::EMPTY );
}

bool CEdAssetBrowser::UpdateTextureGroup( CDiskFile * inFile, const TextureGroup & inGroup )
{
	static Uint32 filesProcessed = 0;
	const String & fileName = inFile->GetFileName();
	if ( inFile->Load() )
	{
		CBitmapTexture * bm = SafeCast <CBitmapTexture>(inFile->GetResource());
		ASSERT( bm );
		if ( bm )
		{
			LOG_EDITOR( TEXT( "Settng texture group %ls at file %ls " ), inGroup.m_groupName.AsString().AsChar(), fileName.AsChar() );
			bm->SetTextureGroup( inGroup.m_groupName );
			bm->Save();	
		}

		// Process GC once a while
		if ( (++filesProcessed %10 ) == 9 )
		{
			SGarbageCollector::GetInstance().CollectNow();
		}
	}
	else
	{
		WARN_EDITOR( TEXT( "Failed to load file: %ls" ), fileName );
	}

	// Done
	return true;
}

bool CEdAssetBrowser::SetNormalTextureGroup(CDiskFile * inFile, const TextureGroup & inGroup)
{
	const String & fileName = inFile->GetFileName();
	if ( fileName.EndsWith( TEXT( "_n.xbm" ) ) ) 
	{
		UpdateTextureGroup( inFile, inGroup );
	}
	return true;
}

bool CEdAssetBrowser::SetDiffuseTextureGroup(CDiskFile * inFile, const TextureGroup & inGroup)
{
	const String & fileName = inFile->GetFileName();
	if ( fileName.EndsWith( TEXT( ".xbm" ) ) && !fileName.EndsWith( TEXT( "_n.xbm" ) ) && !fileName.EndsWith( TEXT( "_s.xbm" ) ) ) 
	{
		UpdateTextureGroup( inFile, inGroup );
	}
	return true;
}

bool CEdAssetBrowser::SetTextureGroup(CDirectory * inDir, const TextureGroup & inGroup, AnsiChar inType )
{
	const TFiles & fileMap = inDir->GetFiles();
	for ( TFiles::const_iterator fileIter = fileMap.Begin(); fileIter!= fileMap.End(); ++fileIter)
	{
		if ( inType == 'n')
		{
			SetNormalTextureGroup( (*fileIter), inGroup );
		}
		else if ( inType == 'd' )
		{
			SetDiffuseTextureGroup( (*fileIter), inGroup );
		}
	}	
	return true;
}

void CEdAssetBrowser::OnDeleteAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnDeleteAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnCopyAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnCopyAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnCutAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnCutAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnPasteAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnPasteAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnPasteAsAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnPasteAsAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnRenameAsset( wxCommandEvent &event )
{
	wxWindow *panel = GetCurrentPage();

	if( panel )
	{
		CEdAssetBrowser::CEdAssetBrowserTab* tab = m_tabs.FindPtr( panel );
		if ( tab )
		{
			tab->m_view->OnRenameAsset( wxCommandEvent() );
		}
	}
}

void CEdAssetBrowser::OnUsePathFromClipboard( wxCommandEvent &event )
{
	if ( wxTheClipboard->Open() )
	{
		if ( wxTheClipboard->IsSupported( wxDF_TEXT ) )
		{
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			String clipboardText( data.GetText() );

			CDirectory *dir;
			dir = GDepot->FindPath( clipboardText.AsChar() );
			if ( dir )
			{
				SelectDirectory( dir, false );
			}

		}  
		wxTheClipboard->Close();
	}
}

void CEdAssetBrowser::SetTabTitle( Int32 idx )
{
	wxWindow *page = m_noteBook->GetPage( idx );
	if ( page )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
		if ( tab )
		{
			String caption;
			if ( !tab->m_searchPhrase.Empty() )
			{
				caption = TXT( "Search: " );
				caption += tab->m_searchPhrase.AsChar();
			}
			else if ( tab->m_type == ETT_CheckedOut )
			{
				caption = TXT( "Checked out" );
			}
			else if ( tab->m_type == ETT_RecentFiles )
			{
				caption = TXT( "Recent files" );
			}
			else if ( tab->m_directory )
			{
				caption = ( tab->m_directory == GDepot ? TXT("Depot") : tab->m_directory->GetName() ); 
			}

			m_noteBook->SetPageBitmap( idx, wxNullBitmap );

			if ( tab->m_lock )
			{
				m_noteBook->SetPageBitmap( idx, m_lockedTabBitmap );
			}
			if ( tab->m_flat )
			{
				caption = caption + TXT(" [flat]");
			}

			m_noteBook->SetPageText( idx, caption.AsChar() );
		}
	}
}

Bool CEdAssetBrowser::IsCurrentTabFlat()
{
	wxWindow *page = GetCurrentPage();
	if( page )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
		if ( tab )
		{
			return tab->m_flat;
		}
	}
	return false;
}

void CEdAssetBrowser::UpdateResourceList( CEdAssetBrowserTab &tab )
{
	if( !tab.m_searchPhrase.Empty() )
		return;

	if( tab.m_type == ETT_CheckedOut )
	{
		TDynArray< CDiskFile* > checkedOutFiles;

		GVersionControl->Opened( checkedOutFiles );
		tab.m_view->ListFiles( checkedOutFiles );

		return;
	}
	else if( tab.m_type == ETT_RecentFiles )
	{
		tab.m_view->ListFiles( m_recentFiles, NULL, false );
		return;
	}

	TDynArray< TPair< CDirectory*, String > > dirs;
	TDynArray< CDiskFile* > allFiles;

	if( !tab.m_directory )
		tab.m_directory = GDepot;

	//dirs.PushBack( TPair< CDirectory*, String >( tab.m_directory, TXT( "[.]" ) ) );
	if( tab.m_directory != GDepot )
		dirs.PushBack( TPair< CDirectory*, String >( tab.m_directory->GetParent(), TXT( "[..]" ) ) );

	if(	tab.m_directory )
	{
		if( tab.m_flat )
		{
			TDynArray< CDirectory* > stack;
			stack.PushBack( tab.m_directory );
			Bool skipWarning = false;
			while( !stack.Empty() )
			{
				CDirectory *d = stack.PopBack();
				FillDirectory( d, allFiles );
				if( !skipWarning && allFiles.Size() > MAX_SEARCH )
				{
					wxMessageDialog dialog( 0, wxT(QUESTION_DISPLAY_SEARCH), wxT("Question"), 
						wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
					if ( dialog.ShowModal() == wxID_NO )
					{
						tab.m_flat = false;
						UpdateResourceList( tab );
						return;
					}
					else
						skipWarning = true;
				}

				for ( CDirectory* childDir : d->GetDirectories() )
				{
					stack.PushBack( childDir );
				}
			}
		}
		else
		{
			FillDirectory( tab.m_directory, allFiles );

			for ( CDirectory* childDir : tab.m_directory->GetDirectories() )
			{
				dirs.PushBack( TPair< CDirectory*, String >( childDir, TXT("[" ) + childDir->GetName() + TXT("]" ) ) );
			}
		}
	}

	// Update resource views
	tab.m_view->ListFiles( allFiles, &dirs );
}

Bool CEdAssetBrowser::FillDirectory( CDirectory *directory, TDynArray< CDiskFile* > &allFiles )
{
	if ( directory )
	{
		const TFiles & files = directory->GetFiles();
		for ( TFiles::const_iterator i = files.Begin(); i != files.End(); ++i )
		{
			if ( this->CanShowFile( *i ) && !(*i)->IsDeleted() )
			{
				allFiles.PushBack( *i );
			}
		}
	}

	return true;
}

void CEdAssetBrowser::UpdateImportClasses()
{

}

CDirectory* CEdAssetBrowser::GetCurrentDirectory()
{
	wxWindow *current = GetCurrentPage();
	if ( current )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( current );
		if ( tab )
		{
			return tab->m_directory;
		}
	}

	return NULL;
}

Bool CEdAssetBrowser::SelectDirectory( CDirectory* directory, Bool allowNewTab )
{
	wxWindow *page = GetCurrentPage();

	CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
	if ( tab )
	{
		if( tab->m_directory != directory && tab->m_lock )
		{
			AddTab( directory, ETT_Folder, String::EMPTY );
			UpdateBookmarkButton();
			return true;
		}

		// Find item
		if( !page || tab->m_type!=ETT_Folder )
		{
			page = NULL;
			for( Uint32 i=0; i<m_noteBook->GetPageCount(); i++ )
			{
				wxWindow *p = m_noteBook->GetPage( i );
				if( m_tabs.KeyExist( p ) )
				{
					CEdAssetBrowserTab *tab2 = m_tabs.FindPtr( p );
					if ( tab2 )
					{
						if( !tab2->m_lock && tab2->m_type==ETT_Folder && tab2->m_searchPhrase.Empty() )
						{
							page = p;
							break;
						}
					}
				}
			}
			if( !page )
			{
				if( !allowNewTab )
				{
					return false;
				}
				AddTab( directory, ETT_Folder, String::EMPTY );
				UpdateBookmarkButton();
				return true;
			}
		}

		Bool ret = false;
		wxTreeItemId itemId;
		if ( m_dirMapping.Find( directory, itemId ) )
		{
			ret = true;
			if( !m_depotTree->IsSelected( itemId ) )
			{
				m_depotTree->SelectItem( itemId, true );
			}
		}

		Int32 idx = m_noteBook->GetPageIndex( page );

		if( tab->m_directory != directory )
		{
			tab->m_flat = false;
			tab->m_directory = directory;
			tab->m_searchPhrase = String::EMPTY;
			tab->m_type = ETT_Folder;

			directory->GetStatus();

			UpdateResourceList( *tab );

			tab->m_view->CheckActive();

			SetTabTitle( idx );

			m_dynaPath->Refresh( true );
		}

		if( idx != m_noteBook->GetSelection() )
		{
			m_noteBook->SetSelection( idx );
		}

		UpdateBookmarkButton();
		return ret;
	}

	UpdateBookmarkButton();
	return false;
}

Bool CEdAssetBrowser::FillDirectory( CDirectory* dir, wxTreeItemId item )
{
	// Cleanup
	m_depotTree->Collapse( item );
	m_depotTree->DeleteChildren( item );

	// Remove directory from mapping
	m_dirMapping.Erase( dir );

	// Add sub directories
	Int32 numMatchingSubdirs = 0;
	for ( CDirectory * pDir : dir->GetDirectories() )
	{
		Int32 iconValue = pDir->IsCheckedOut() ? 3 : 2;
		wxTreeItemId levelItem = m_depotTree->AppendItem( item, pDir->GetName().AsChar(), iconValue );
		if ( levelItem )
		{
			m_depotTree->SetItemData( levelItem, new wxTreeItemDataDir( pDir, pDir->GetDepotPath() ) );
			if ( FillDirectory( pDir, levelItem ) )
			{
				// Directory contains some matched data
				numMatchingSubdirs++;
			}
		}
	}

	// Add directory to mapping
	m_dirMapping.Insert( dir, item );
	return true;
}

Bool CEdAssetBrowser::CanShowFile( CDiskFile* so, Bool inSearchMode )
{
	CFilePath path( so->GetDepotPath() );

	// Check class filter
	Bool canShow = m_allowedExtensions.Exist( path.GetExtension().ToLower() );

	if ( canShow && inSearchMode )
	{
		String pathStr = path.ToString();
		// check skipped directories
		for ( auto dirIt = m_skippedDirs.Begin(); dirIt < m_skippedDirs.End(); ++dirIt )
		{
			String dir = *dirIt;
			if ( pathStr.BeginsWith( dir ) )
			{
				return false;
			}
		}
	}

	return canShow;
}

Bool CEdAssetBrowser::SelectResource( CResource* resource )
{
	(void)resource;
	return false;
}

void CEdAssetBrowser::RepopulateDirectory()
{
	CDirectory* dir = GetActiveDirectory();
	if ( dir )
	{
		// Update selected directory
		dir->Repopulate( false );
		dir->GetStatus();
	}
}

void CEdAssetBrowser::OnShow( wxShowEvent& event )
{
	LayoutRecursively( this );
}

void CEdAssetBrowser::OnActivate( wxActivateEvent& event )
{
	if ( event.GetActive() && m_repopulateDirOnActivation )
	{
		RepopulateDirectory();
		m_repopulateDirOnActivation = false;
	}
}


void CEdAssetBrowser::OnClose( wxCloseEvent& event )
{
	// Just hide the window
	Hide();
}

void CEdAssetBrowser::OnDirectoryOpened( wxTreeEvent& event )
{
	if( m_lockTree )
	{
		return;
	}
	// Get item
	wxTreeItemId itemId = event.GetItem();
	wxTreeItemDataDir* data = static_cast<wxTreeItemDataDir*>( m_depotTree->GetItemData( itemId ) );
}

void CEdAssetBrowser::OnDirectoryChanged( wxTreeEvent& event )
{
	if( m_lockTree || m_scanningDepot )
		return;

	// Get item
	wxTreeItemId itemId = event.GetItem();
	wxTreeItemDataDir* data = static_cast<wxTreeItemDataDir*>( m_depotTree->GetItemData( itemId ) );

	CDirectory* dir = data->GetDirectory();
	if ( dir )
	{
		SelectDirectory( dir );
	}
}

void CEdAssetBrowser::OnDirectoryContextMenu( wxTreeEvent& event )
{
	if ( m_scanningDepot )
	{
		return;
	}

	// Get item
	wxTreeItemId itemId = event.GetItem();
	wxTreeItemDataDir* data = static_cast<wxTreeItemDataDir*>( m_depotTree->GetItemData( itemId ) );

	// Assemble context menu
	wxMenu menu;

	TDynArray< CDirectory* > dirs;
	if( data->GetDirectory() )
	{
		dirs.PushBack( data->GetDirectory() );
	}
	CreateDirectoryContextMenu( menu, dirs );

	// Show menu
	PopupMenu( &menu );
}

void CEdAssetBrowser::CreateDirectoryContextMenu( wxMenu &menu, const TDynArray< CDirectory* > &dirs )
{
	menu.Append( ID_NEW_TAB_DIRECTORY, TXT("Open in new tab"), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnNewTabDirectory, this, ID_NEW_TAB_DIRECTORY, wxID_ANY, new CContextMenuDir( dirs ) );
	
	menu.AppendSeparator();
	
	if( dirs.Size() == 1 )
	{
		String msg = TXT("Add directory to ") + dirs[0]->GetName();
		menu.Append( ID_CREATE_DIRECTORY, wxString(msg.AsChar()), wxEmptyString );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCreateDirectory, this, ID_CREATE_DIRECTORY, wxID_ANY, new CContextMenuDir( dirs ) );
	}
	menu.Append( ID_RESCAN_DIRECTORY, TXT("Rescan content"), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnRescanDirectory, this, ID_RESCAN_DIRECTORY, wxID_ANY, new CContextMenuDir( dirs ) );

	// Search menu group
	menu.AppendSeparator();

	wxMenu* searchMenu = new wxMenu;
	menu.AppendSubMenu( searchMenu, TXT( "Search" ) );

	searchMenu->Append( ID_SEARCH_ATTITUDE_GROUP, TXT("By attitude group..."), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchAttitudeGroup, this, ID_SEARCH_ATTITUDE_GROUP, wxID_ANY, new CContextMenuDir( dirs ) );

	searchMenu->Append( ID_SEARCH_ENTITY_CLASS, TXT("By entity class..."), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchEntityClass, this, ID_SEARCH_ENTITY_CLASS, wxID_ANY, new CContextMenuDir( dirs ) );

	searchMenu->Append( ID_SEARCH_NPC_LEVEL, TXT("By NPC level..."), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchNPCLevel, this, ID_SEARCH_NPC_LEVEL, wxID_ANY, new CContextMenuDir( dirs ) );

	wxMenu* componentMenu = new wxMenu;
	searchMenu->AppendSubMenu( componentMenu, TXT( "By component..." ) );

	componentMenu->Append( ID_SEARCH_COMPONENT_ABSENCE, TXT("Absence"), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchComponentAbsence, this, ID_SEARCH_COMPONENT_ABSENCE, wxID_ANY, new CContextMenuDir( dirs ) );

	componentMenu->Append( ID_SEARCH_COMPONENT_PRESENCE, TXT("Presence"), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchComponentPresence, this, ID_SEARCH_COMPONENT_PRESENCE, wxID_ANY, new CContextMenuDir( dirs ) );

	wxMenu* behTreeMenu = new wxMenu;
	searchMenu->AppendSubMenu( behTreeMenu, TXT( "By behavior tree..." ) );

	behTreeMenu->Append( ID_SEARCH_BEHTREE_SCRIPTTASK, TXT("Script task"), wxEmptyString );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSearchBehTreeScriptTask, this, ID_SEARCH_BEHTREE_SCRIPTTASK, wxID_ANY, new CContextMenuDir( dirs ) );

	menu.AppendSeparator();

	menu.Append( ID_SYNC_DIRECTORY, TXT("Get latest revision"), wxEmptyString );
	menu.Append( ID_CHECK_OUT_DIRECTORY, TXT("Check out") );
	menu.Append( ID_SUBMIT_DIRECTORY, TXT("Submit") );
	menu.Append( ID_REVERT_DIRECTORY, TXT("Revert") );
	
	if( dirs.Size() == 1 )
	{
		menu.Append( ID_ADD_DIRECTORY, TXT("Add") );
		menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnAddDirectory, this, ID_ADD_DIRECTORY, wxID_ANY, new CContextMenuDir( dirs ) );
	}

    menu.Append( ID_SHOW_IN_EXPLORER, TXT("Show in Explorer") );

	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSyncDirectory,           this, ID_SYNC_DIRECTORY,      wxID_ANY, new CContextMenuDir( dirs ) );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCheckOutDirectory,       this, ID_CHECK_OUT_DIRECTORY, wxID_ANY, new CContextMenuDir( dirs ) );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSubmitDirectory,         this, ID_SUBMIT_DIRECTORY,    wxID_ANY, new CContextMenuDir( dirs ) );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnRevertDirectory,         this, ID_REVERT_DIRECTORY,    wxID_ANY, new CContextMenuDir( dirs ) );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnOpenDirectoryInExplorer, this, ID_SHOW_IN_EXPLORER,    wxID_ANY, new CContextMenuDir( dirs ) );
}

void CEdAssetBrowser::CreateBatchersContextMenu( wxMenu &menu, const TDynArray< CDirectory* >& dirs, const TDynArray< CDiskFile* >& files )
{
	wxMenu* batchersMenu = new wxMenu;
	menu.AppendSubMenu( batchersMenu, TXT("Batch operations") );

	batchersMenu->Append( ID_REMOVE_UNUSED_ANIMS, TXT("Remove unused animations") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnRemoveUnusedAnimations, this, ID_REMOVE_UNUSED_ANIMS, wxID_ANY, 0 );

	batchersMenu->Append( ID_DUMP_ANIMS, TXT("Dump animations in selected animsets") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpAnimationNames, this, ID_DUMP_ANIMS, wxID_ANY, 0 );

	// remember to make a copy of contextMenuDir for every handler!

	CContextMenuDir contextMenuDir = !dirs.Empty() ? CContextMenuDir( dirs ) : CContextMenuDir( files );

	batchersMenu->AppendSeparator();

	batchersMenu->Append( ID_GENERATE_ITEMS_ICONS, TXT("Generate icons for items") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnGenerateItemsIcons, this, ID_GENERATE_ITEMS_ICONS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	batchersMenu->Append( ID_CONVERT_SENSES, TXT("Convert AI senses") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnConvertAISenses, this, ID_CONVERT_SENSES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	
	batchersMenu->Append( ID_CREATE_INVENTORIES_REPORT, TXT("Create inventories report") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCreateInventoriesReport, this, ID_CREATE_INVENTORIES_REPORT, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	
	batchersMenu->AppendSeparator();

	batchersMenu->Append( ID_RESAVE_TEMPLATES, TXT("Resave entity templates") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnResaveEntityTemplates, this, ID_RESAVE_TEMPLATES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	batchersMenu->Append( ID_RESAVE_RESOURCES, TXT("Resave resources") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnResaveResources, this, ID_RESAVE_RESOURCES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	
	batchersMenu->AppendSeparator();

	// Texture Menu Group

	wxMenu* texMenu = new wxMenu;
	batchersMenu->AppendSubMenu( texMenu, TXT( "Textures" ) );

	texMenu->Append( ID_SET_DOWNSCALE_BIAS, TXT("Set Downscale Bias") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSetDownscaleBias, this, ID_SET_DOWNSCALE_BIAS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	texMenu->Append( ID_SET_DOWNSCALE_BIAS, TXT("Extract source texture") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnExtractSourceTexture, this, ID_SET_DOWNSCALE_BIAS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	texMenu->Append( ID_BATCH_TEXTURE_GROUP_CHANGE, TXT("Batch texture group change") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnBatchTextureGroupChange, this, ID_BATCH_TEXTURE_GROUP_CHANGE, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	
	// Materials Menu Group

	wxMenu* materialMenu = new wxMenu;
	batchersMenu->AppendSubMenu( materialMenu, TXT( "Materials" ) );

	materialMenu->Append( ID_CHANGLE_MATERIAL_FLAGS, TXT("Change material flags") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnChangeMaterialFlags, this, ID_CHANGLE_MATERIAL_FLAGS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	materialMenu->Append( ID_REMOVE_UNUSED_MATERIALS, TXT("Remove unused materials") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnRemoveUnusedMaterials, this, ID_REMOVE_UNUSED_MATERIALS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	materialMenu->Append( ID_ASSIGN_MATERIALS_TO_MESHES, TXT("Assign materials to meshes") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnAssingMaterialToMeshes, this, ID_ASSIGN_MATERIALS_TO_MESHES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	// Meshes Menu Group

	wxMenu* meshMenu = new wxMenu;
	batchersMenu->AppendSubMenu( meshMenu, TXT( "Meshes" ) );

	meshMenu->Append( ID_SET_AUTO_HIDE_DISTANCE, TXT("Set Auto Hide Distance") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSetAutoHideDistance, this, ID_SET_AUTO_HIDE_DISTANCE, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_GENERATE_LODS, TXT("Generate LODs") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnGenerateLODs, this, ID_GENERATE_LODS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_CHANGLE_LIGHT_CHANNELS, TXT("Change light channels") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnChangeLightChannels, this, ID_CHANGLE_LIGHT_CHANNELS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_CHANGLE_DRAWABLE_FLAGS, TXT("Change drawable flags") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnChangeDrawableFlags, this, ID_CHANGLE_DRAWABLE_FLAGS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_SET_STREAMING_LOD, TXT("Update streaming LOD values") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSetStreamingLODs, this, ID_SET_STREAMING_LOD, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_CALCULATE_SHADOW_PRIORITIES, TXT("Calculate Shadow Priorities") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCalculateShadowPriorities, this, ID_CALCULATE_SHADOW_PRIORITIES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_CALCULATE_TEXEL_DENSITY, TXT("Calculate Texel Density Factors") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCalculateTexelDensity, this, ID_CALCULATE_TEXEL_DENSITY, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	
	meshMenu->Append( ID_SET_MESH_PROPERTIES, TXT("Set Mesh Properties") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSetMeshProperties, this, ID_SET_MESH_PROPERTIES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );


	meshMenu->Append( ID_SIMPLIFY_MATERIALS, TXT("Simplify Materials") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnSimplifyMaterials, this, ID_SIMPLIFY_MATERIALS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	meshMenu->Append( ID_APPEARANCES_TEXTURE_COST, TXT("Appearances Texture Costs") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCalculateAppearancesTextureCost, this, ID_APPEARANCES_TEXTURE_COST, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	// components menu

	wxMenu* componentsMenu = new wxMenu;
	batchersMenu->AppendSubMenu( componentsMenu, TXT( "Components" ) );
	
	componentsMenu->Append( ID_CALCULATE_COMPONENTS, TXT("Calculate Components") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnCalculateComponentsInEntities, this, ID_CALCULATE_COMPONENTS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	componentsMenu->Append( ID_REMOVE_FORCED_APPEARANCES, TXT("Remove forced appearances") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnRemoveForcedAppearances, this, ID_REMOVE_FORCED_APPEARANCES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	// components menu

	wxMenu* entityMenu = new wxMenu;
	batchersMenu->AppendSubMenu( entityMenu, TXT( "Entities" ) );

	entityMenu->Append( ID_ENTITIES_DEP_COUNT, TXT("Dump entities dependency count") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpEntitiesDepCount, this, ID_ENTITIES_DEP_COUNT, wxID_ANY, new CContextMenuDir( contextMenuDir ) );

	// debug dump info

	wxMenu* debugMenu = new wxMenu();
	batchersMenu->AppendSubMenu( debugMenu, TXT("Debug") );

	debugMenu->Append( ID_GENERATE_DEBUG_INFO_MESHES, TXT("Dump debug info for Meshes") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpDebugInfoMeshes, this, ID_GENERATE_DEBUG_INFO_MESHES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	debugMenu->Append( ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES, TXT("Dump debug info for Entity Templates") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpDebugInfoEntityTemplates, this, ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	debugMenu->Append( ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES_EFFECTS, TXT("Dump debug info for Entity Templates with effects") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpDebugInfoEntityTemplatesEffects, this, ID_GENERATE_DEBUG_INFO_ENTITY_TEMPLATES_EFFECTS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	//  Animations menu

	wxMenu* animationsMenu = new wxMenu;
	batchersMenu->AppendSubMenu( animationsMenu, TXT( "Animations" ) );

	animationsMenu->Append( ID_DUMP_DIALOGANIMS, TXT("Dump Animations in Dialogs") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnDumpDialogAnimationNames, this, ID_DUMP_DIALOGANIMS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	animationsMenu->Append( ID_VALIDATE_ANIMATIONS, TXT("Validate Animations") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnValidateAnimations, this, ID_VALIDATE_ANIMATIONS, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	animationsMenu->Append( ID_FIX_SOURCE_ANIM_DATA, TXT("Fix corrupted source anim data") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnFixSourceAnimData, this, ID_FIX_SOURCE_ANIM_DATA, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
	animationsMenu->Append( ID_VIEW_ANIM_SET_REPORT, TXT("View anim set report") );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdAssetBrowser::OnViewAnimSetReport, this, ID_VIEW_ANIM_SET_REPORT, wxID_ANY, new CContextMenuDir( contextMenuDir ) );
}

void CEdAssetBrowser::OnNewTabDirectory( wxCommandEvent &event )
{
	EEditorResourceViewMode mode = ERVM_Big;
	wxWindow *current = GetCurrentPage();
	if ( current )
	{
		mode = m_tabs.GetRef( current ).m_view->GetViewType();
	}

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory *currentDirectory = contextMenuDir->GetDirs()[ i ];

		CEdResourceView *ed = m_tabs.GetRef( AddTab( currentDirectory ) ).m_view;

		ed->SetViewType( mode );
	}

	m_timer.Start( 50, true );
}

void CEdAssetBrowser::OnTabContextMenu( wxAuiNotebookEvent &event )
{
	int page = event.GetSelection();
	wxWindow *wpage = m_noteBook->GetPage( page );

	wxMenu menu;

	menu.AppendCheckItem( ID_LOCK_TAB, TXT("Lock tab"), wxEmptyString )->Check( m_tabs.GetRef( wpage ).m_lock );
	menu.Connect( ID_LOCK_TAB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnLockTab ), new CPageWrapper( page ), this );
	menu.AppendCheckItem( ID_FLAT_TAB, TXT("Flat view (show all subfolders)"), wxEmptyString )->Check( m_tabs.GetRef( wpage ).m_flat );
	menu.Connect( ID_FLAT_TAB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnFlatDirectory ), new CPageWrapper( page ), this );
	menu.AppendSeparator();
	menu.Append( ID_NEW_TAB_DIRECTORY, TXT("New tab"), wxEmptyString );
	menu.Connect( ID_NEW_TAB_DIRECTORY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnAddTabButton ), new CPageWrapper( page ), this );
	menu.AppendSeparator();
	menu.Append( ID_SHOW_IN_TREE, TXT("Show in directories tree"), wxEmptyString );
	menu.Connect( ID_SHOW_IN_TREE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnShowInDirectoriesTree ), new CPageWrapper( page ), this );
	menu.AppendSeparator();
	menu.Append( ID_CLOSE_TAB, TXT("Close tab"), wxEmptyString );
	menu.Connect( ID_CLOSE_TAB, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnTabClose ), new CPageWrapper( page ), this );

	// Show menu
	PopupMenu( &menu );	
}

void CEdAssetBrowser::OnTabHeaderDblClick( wxAuiNotebookEvent &event )
{
	AddTab( GDepot );
}

void CEdAssetBrowser::OnTabClose( wxCommandEvent &event )
{
	CPageWrapper* pw = (CPageWrapper*) event.GetClientData();
	wxAuiNotebookEvent e;
	e.SetSelection( pw ? pw->m_page : m_noteBook->GetSelection() );
	OnTabClose( e );
}

void CEdAssetBrowser::OnTabClose( wxAuiNotebookEvent &event )
{
	event.Veto();	// Let's just do it myself

	if( m_noteBook->GetPageCount() <= 1 )
	{
		wxMessageBox( wxT("You cannot close the last tab"), wxT("The last tab"), wxCENTRE|wxOK|wxICON_ERROR, this );
		return;
	}

	wxWindow *page = m_noteBook->GetPage( event.GetSelection() );
	if( page )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
		if ( tab )
		{
			// Confirm to close a locked tab
			if ( tab->m_lock )
			{
				if ( wxMessageBox( wxT("The tab has been marked as locked.\n\nAre you sure that you want to close it?"), wxT("Close locked tab"), wxICON_QUESTION|wxYES_NO|wxCENTRE, this ) != wxYES )
				{
					return;
				}
			}

			Int32 order = tab->m_order;
			m_tabs.Erase( page );
			for( THashMap< wxWindow*, CEdAssetBrowserTab >::iterator it=m_tabs.Begin(); it!=m_tabs.End(); ++it )
			{
				if( it->m_second.m_order > order )
				{
					it->m_second.m_order --;
				}
			}
		}

		m_noteBook->DeletePage( m_noteBook->GetPageIndex( page ) );
	}
}

wxWindow* CEdAssetBrowser::GetCurrentPage()
{
	if( m_noteBook->GetSelection() == -1 )
		return NULL;
	wxWindow *page = m_noteBook->GetPage( m_noteBook->GetSelection() );
	if( m_tabs.KeyExist( page ) )
		return page;
	return NULL;
}

Bool CEdAssetBrowser::IsCurrentPageSearchType()
{
	wxWindow *current = GetCurrentPage();
	if( current )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( current );
		if ( tab )
		{
			return tab->m_searchPhrase != String::EMPTY;
		}
	}

	return false;
}

void CEdAssetBrowser::NextTab()
{
	Uint32 sel = m_noteBook->GetSelection();
	sel ++;
	if( sel > m_noteBook->GetPageCount()-1 )
		sel = 0;
	m_noteBook->SetSelection( sel );
}

wxPanel* CEdAssetBrowser::AddTab( CDirectory *dir, ETabType type, String searchPhrase, const CClass * searchFilter )
{
	wxPanel *panel = new wxPanel( m_noteBook );
	wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );
	wxPanel *subPanel = new wxPanel( panel, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxTAB_TRAVERSAL );
	wxScrollBar *scroll = new wxScrollBar( panel, -1, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL );

	CEdResourceView* ed = new CEdResourceView( subPanel, this, scroll );

	sizer->Add( subPanel, 1, wxALL | wxEXPAND, 5 );
	sizer->Add( scroll, 0, wxALIGN_RIGHT | wxEXPAND );
	panel->SetSizer( sizer );
	panel->Layout();

	wxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );
	sizer2->Add( ed, 1, wxEXPAND | wxALL, 0 );
	subPanel->SetSizer( sizer2 );
	subPanel->Layout();

	CEdAssetBrowserTab& tab = m_tabs.GetRef( panel );

	tab.m_view = ed;
	if(	dir )
	{
		dir->GetStatus();
	}
	tab.m_directory = dir;
	tab.m_searchPhrase = searchPhrase;
	tab.m_searchFilter = searchFilter;
	tab.m_type = type;
	tab.m_order = m_noteBook->GetPageCount();
	
	m_noteBook->AddPage( panel, wxEmptyString, true );

	SetTabTitle( m_noteBook->GetPageCount() - 1 );

	return panel;
}

void CEdAssetBrowser::OnClassFilterChanged( wxCommandEvent& event )
{
	// Get selected class
	Int32 selected = m_classFilterBox->GetSelection();

	// Get list of allowed classes
	TDynArray< CClass* > allowedClasses;
	if ( !selected )
	{
		// Show all valid resources
		allowedClasses = m_resourceClasses;
	}
	else
	{
		// Show only selected resource type
		CClass* resourceClass = (CClass* )m_classFilterBox->GetClientData( selected );
		allowedClasses.PushBack( resourceClass );
	}

	// Create list of allowable extensions
	m_allowedExtensions.Clear();
	for ( Uint32 i=0; i<allowedClasses.Size(); i++ )
	{
		CResource* res = allowedClasses[i]->GetDefaultObject< CResource >();
		m_allowedExtensions.Insert( res->GetExtension() );
	}

	// Update list
	UpdateResourceList();
}

void CEdAssetBrowser::OnCreateDirectory( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	if( contextMenuDir->GetDirs().Size() == 1 )
	{
		String dirName;
		// Ask for directory name
		if ( InputBox( this, TXT("Add directory"), TXT("Enter name of new directory:"), dirName ) )
		{
			if ( (!dirName.ContainsCharacter( TXT( ' ' ) ) ) && (dirName.ToLower() == dirName) ) 
			{
				// Create directory
				CDirectory* dir = contextMenuDir->GetDirs()[ 0 ]->CreateNewDirectory( dirName );

				// Create directory on disk
				Bool success = dir->CreateOnDisk();
				if( success == false )
				{
					GFeedback->ShowWarn( TXT( "Failed to create directory %s." ), dir->GetAbsolutePath().AsChar() );
				}

				m_addedDirectories.Insert( dir );

				// Reset filter
				m_classFilterBox->SetSelection( 0 );
				OnClassFilterChanged( event );

				// Change filter to show the new directory
				m_lockTree = true;
				wxTreeItemId parentId;
				if ( m_dirMapping.Find( contextMenuDir->GetDirs()[0], parentId ) )
				{
					if ( parentId )
					{
						wxTreeItemId newId = m_depotTree->AppendItem( parentId, dirName.AsChar(), 2 );
						if ( newId )
						{
							m_depotTree->SetItemData( newId, new wxTreeItemDataDir(  dir, dir->GetDepotPath() ) );
							m_depotTree->SelectItem( newId );
						}
					}
				}
				m_lockTree = false;

				// Redraw shit
				SelectDirectory( dir );
			}
			else
			{
				wxMessageDialog dialog( 0, wxT("Cannot create dir with whitespaces or large letters"), wxT("Error"), wxOK | wxICON_ERROR );
				dialog.ShowModal();
			}
		}
	}
}

void CEdAssetBrowser::OnRescanDirectory( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	// Pause configuration saving timer
	wxTheFrame->PauseConfigTimer();

	// Cleanup list of freshly added directories
	m_addedDirectories.Clear();

	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		// Rescan disk structure
		contextMenuDir->GetDirs()[ i ]->Repopulate();
	}

	m_dynaPath->Refresh( true );

	// Update tree
	wxTreeItemId itemId;
	if ( m_dirMapping.Find( GetActiveDirectory(), itemId ) )
	{
		// Update directory tree
		FillDirectory( GetActiveDirectory(), itemId );

		// Reselect
		SelectDirectory( GetActiveDirectory() );
	}

	UpdateResourceList();

	// Resume configuration saving timer
	wxTheFrame->ResumeConfigTimer();
}

void CEdAssetBrowser::OnTabChange( wxAuiNotebookEvent &event )
{
	if( m_lockTree )
		return;

	m_timer.Start( 50, true );
}

void CEdAssetBrowser::OnContentChange( wxCommandEvent &event )
{
	RefreshIcons();
}

void CEdAssetBrowser::OnSearch( wxCommandEvent &event )
{
	String phrase = m_search->GetValue().wc_str();
	m_searchPhrase = phrase;

	if ( phrase != String::EMPTY )
	{
		ShowSearchResults( phrase );
	}

	m_dynaPath->Refresh( true );
	UpdateBookmarkButton();
}

void CEdAssetBrowser::OnSyncDirectory( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		contextMenuDir->GetDirs()[ i ]->Sync();
		contextMenuDir->GetDirs()[ i ]->Repopulate();
		contextMenuDir->GetDirs()[ i ]->Reload(true);
	}
	UpdateResourceList();
	RefreshCurrentView();
	m_dynaPath->Refresh( true );
}

void CEdAssetBrowser::OnCheckOutDirectory( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		contextMenuDir->GetDirs()[ i ]->CheckOut();
	}
	RefreshIcons();
	RefreshCurrentView();
	m_dynaPath->Refresh( true );
}

void CEdAssetBrowser::RefreshCurrentView()
{
	wxWindow *page = GetCurrentPage();
	if( page )
	{
		CEdAssetBrowserTab* tab = m_tabs.FindPtr( page );
		if ( tab )
		{
			tab->m_view->Refresh( true );
		}
	}
}

void CEdAssetBrowser::OnSubmitDirectory( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		contextMenuDir->GetDirs()[ i ]->Submit();
	}
	RefreshIcons();
	RefreshCurrentView();
	m_dynaPath->Refresh( true );
}

void CEdAssetBrowser::OnRevertDirectory( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		contextMenuDir->GetDirs()[ i ]->Revert();
	}
	RefreshIcons();
	RefreshCurrentView();
	m_dynaPath->Refresh( true );
}

void CEdAssetBrowser::OnOpenDirectoryInExplorer( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		ShellExecute( NULL, TXT("explore"),
			contextMenuDir->GetDirs()[i]->GetAbsolutePath().AsChar(),
			NULL, NULL, SW_SHOWNORMAL);
	}
}

void CEdAssetBrowser::OnAddDirectory( wxCommandEvent &event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	if( contextMenuDir->GetDirs().Size() == 1 )
	{
		contextMenuDir->GetDirs()[ 0 ]->Add();
		RefreshIcons();
		RefreshCurrentView();
		m_dynaPath->Refresh( true );
	}
}

static void ImportSenseParams( CEntityTemplate* templ, CInteractionAreaComponent* component, EAISenseType senseType )
{
	CAIProfile* profile = templ->FindParameter< CAIProfile >( false );
	if ( !profile )
	{
		// create a new profile if the template doesn't have one
		profile = CreateObject< CAIProfile >( templ );
		templ->AddParameterUnique( profile );
		templ->MarkModified();
	}

	ASSERT( profile );

	CAISenseParams* senseParams = profile->GetSenseParams( senseType );
	if( !senseParams )
	{
		senseParams = CreateObject< CAISenseParams >( profile ); 
		profile->SetSenseParams( senseType, senseParams );
		templ->MarkModified();
	}

	ASSERT( senseParams );	

	senseParams->m_enabled = component->IsEnabled();
	senseParams->m_rangeMax = component->GetRangeMax();
	senseParams->m_rangeMin = component->GetRangeMin();
	senseParams->m_rangeAngle = Float( component->GetRangeAngle() );
	senseParams->m_height = component->GetHeight();
	senseParams->m_testLOS = true;

	templ->MarkModified();
}

static void ConvertAISenses( CDirectory* dir )
{
	static const String EXT = TXT(".w2ent" );

	static const String SENSE_VISION = TXT("SENSE_VISION" );
	static const String SENSE_ABSOLUTE = TXT("SENSE_ABSOLUTE" );
	static const String SENSE_HEARING = TXT("SENSE_HEARING" );
	static const String SENSE_MEET = TXT("SENSE_MEET" );

	static String strDir;
	dir->GetDepotPath( strDir );
	LOG_EDITOR( TXT("ConvertAISenses dir %s"), strDir.AsChar() );

	const TFiles& files = dir->GetFiles();
	for( TFiles::const_iterator iter = files.Begin(); iter != files.End(); iter++ )
	{
		const CDiskFile* diskFile = (*iter);
		static String str;
		str = diskFile->GetDepotPath();
		if( str.ContainsSubstring( EXT ) )
		{
			CEntityTemplate* templ = LoadResource< CEntityTemplate >( str );
			if( templ )
			{
				CEntity* entity = templ->GetEntityObject();
				if( entity )
				{
					CInteractionAreaComponent* c = entity->FindComponent< CInteractionAreaComponent >( SENSE_VISION );
					if( c )
					{
						LOG_EDITOR( TXT("ConvertAISenses template %s, component %s"), str.AsChar(), c->GetName().AsChar() );
						ImportSenseParams( templ, c, AIST_Vision );
						entity->RemoveComponent( c );
						templ->MarkModified();
						templ->Save();
					}

					c = entity->FindComponent< CInteractionAreaComponent >( SENSE_ABSOLUTE );
					if( c )
					{
						LOG_EDITOR( TXT("ConvertAISenses template %s, component %s"), str.AsChar(), c->GetName().AsChar() );
						ImportSenseParams( templ, c, AIST_Absolute );
						entity->RemoveComponent( c );
						templ->MarkModified();
						templ->Save();
					}

					c = entity->FindComponent< CInteractionAreaComponent >( SENSE_HEARING );
					if( c )
					{
						LOG_EDITOR( TXT("ConvertAISenses template %s, component %s"), str.AsChar(), c->GetName().AsChar() );
						entity->RemoveComponent( c );
						templ->MarkModified();
						templ->Save();
					}

					c = entity->FindComponent< CInteractionAreaComponent >( SENSE_MEET );
					if( c )
					{	
						LOG_EDITOR( TXT("ConvertAISenses template %s, component %s"), str.AsChar(), c->GetName().AsChar() );
						entity->RemoveComponent( c );
						templ->MarkModified();
						templ->Save();
					}
				}
			}
		}
	}

	for ( CDirectory* subDir : dir->GetDirectories() )
	{
		ConvertAISenses( subDir );
	}
}

void CEdAssetBrowser::OnGenerateItemsIcons( wxCommandEvent &event )
{
	CContextMenuDir* contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	CEdItemsThumbnailGenerator( this, contextMenuDir ).Execute();
}

void CEdAssetBrowser::OnConvertAISenses( wxCommandEvent &event )
{
	//wxMessageBox( wxT("Disabled") );
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	LOG_EDITOR( TXT("ConvertAISenses start") );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		ConvertAISenses( dir );
	}
	LOG_EDITOR( TXT("ConvertAISenses end") );
}

void CEdAssetBrowser::OnGenerateLODs( wxCommandEvent& event )
{
	CContextMenuDir contextMenuDir = *wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	CEdBatchLodGenerator( this, contextMenuDir ).Execute();
}

void CEdAssetBrowser::OnChangeLightChannels( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	CEdChangeLightChannelsDlg* changeLightChannels = new CEdChangeLightChannelsDlg( this, contextMenuDir );
	changeLightChannels->Show();
}

void CEdAssetBrowser::OnChangeDrawableFlags( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	CEdChangeDrawableFlagsDlg* changeDrawableFlags = new CEdChangeDrawableFlagsDlg( this, contextMenuDir );
	changeDrawableFlags->Show();
}

void CEdAssetBrowser::OnChangeMaterialFlags( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	CEdChangeMaterialFlagsDlg* changeMaterialFlags = new CEdChangeMaterialFlagsDlg( this, contextMenuDir );
	changeMaterialFlags->Show();
}

#undef FindResource

void CEdAssetBrowser::ImportResources( CClass* resourceClass )
{
	ASSERT( resourceClass );

	// Enumerate importable formats
	TDynArray< CFileFormat > formats;
	IImporter::EnumImportFormats( resourceClass, formats );

	// Reset material autobinding
	m_materialAutoBindResult = ECR_Skip;

	// Define import formats
	m_importDlg.ClearFormats();
	m_importDlg.SetMultiselection( true );
	m_importDlg.AddFormats( formats );
	m_importDlg.SetIniTag(resourceClass->GetDefaultObject<CObject>()->GetFriendlyName());

	// Ask user to specify filename
	if ( m_importDlg.DoOpen( (HWND) GetHandle(), true )) 
	{
		TDynArray< CResource* > importedResources;
		Bool doAll = false;

		// Import each file
		TDynArray< String > files = m_importDlg.GetFiles();
		for ( Uint32 i=0; i<files.Size(); i++ )
		{
			CFilePath path( files[i] );

			// Get suitable importer for this file
			IImporter* importer = IImporter::FindImporter( resourceClass, path.GetExtension() );
			if ( !importer )
			{
				WARN_EDITOR( TXT("No valid importer for extension '%s'"), path.GetExtension().AsChar() );
				continue;
			}

			// Ask for options
			String fileName = path.GetFileName();
			if ( !doAll )
			{
				CEdImportDlg importDlg( this, resourceClass, GetActiveDirectory(), fileName, importer );
				EImportCreateReturn ret = importDlg.DoModal();

				if ( ret == ECR_Cancel )
				{
					// Whole import was canceled
					break;
				}
				else if ( ret == ECR_Skip )
				{
					// Skip this file
					continue;
				}
				else if ( ret == ECR_OKAll )
				{
					// OK for all files
					doAll = true;
				}

				// Get modified file name
				fileName = importDlg.GetFileName();
			}			

			// Make sure we do not overlap any existing resource
			CClass*    resourceClass = importer->GetSupportedResourceClass();
			CResource* resourceDef   = resourceClass->GetDefaultObject<CResource>();
			String     outFileName   = GetActiveDirectory()->GetDepotPath() + fileName + TXT(".") + resourceDef->GetExtension();
			
			// Do the import !
			IImporter::ImportOptions options;
			options.m_existingResource = GDepot->FindResource( outFileName );
			options.m_parentObject = NULL;
			options.m_sourceFilePath = files[i];

			if( !importer->PrepareForImport( path.GetFileName(), options ) )
			{
				if ( options.m_errorCode == IImporter::ImportOptions::EEC_FileToReimport )
				{
					GFeedback->ShowMsg( TXT("RE file warning."), TXT("This file is old but you can import it.\nPlease re-export this file with latest tech.") );
				}
				else
				{
					String errorLog;
					errorLog += TXT("Bad version of re file.");
					// Report error
					WARN_EDITOR( TXT("Unable to import '%s' from '%s'. Error log: '%s'."), 
						resourceClass->GetName().AsString().AsChar(), files[i].AsChar(), errorLog.AsChar() );

					GFeedback->ShowError( TXT("Import preparation failed.\nPlease export with latest tech.") );
					return;
				}
			}

			CResource* imported = importer->DoImport( options );
			if ( imported )
			{
				// Set import file 
				imported->SetImportFile( options.m_sourceFilePath );

				// Save this resource for the first time, this will also create thumbnail
				if ( imported->SaveAs( GetActiveDirectory(), fileName ) )
				{
					// Saved
					importedResources.PushBack( imported );

					// Process resource after import
					OnAfterImport( importer, imported );

					// Update thumbnail
					ASSERT( imported->GetFile() );
					imported->GetFile()->UpdateThumbnail();
					imported->Save();
				}
				else
				{
					// Report error
					WARN_EDITOR( TXT("Unable to save imported '%s'"), fileName.AsChar() );
				}
			}
			else
			{
				// Report error
				WARN_EDITOR( TXT("Unable to import '%s' from '%s'"), resourceClass->GetName().AsString().AsChar(), files[i].AsChar() );
			}
		}

		// Update interface after successful import
		if ( importedResources.Size() )
		{
			// Update resource list
			UpdateResourceList();

			// Select imported resource
			SelectResource( importedResources[0] );
		}

		// Assume some rendering resources were changed
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
}

void CEdAssetBrowser::OnResaveResources( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	for ( CResourceIteratorAdapter< CResource > res( *contextMenuDir, TXT("Resaving resources...") ); res; ++res )
	{
		// do nothing cuz adapter will handle load/save/unload of resources and correct implementation is handled in resources itselves
		// if you need to resave somehting on resource please override onPreSave() call in resource
	}
}

void CEdAssetBrowser::OnResaveEntityTemplates( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}


	TDynArray< CDiskFile* > resaved, failed;

	LOG_EDITOR( TXT("Resaving of entity templates starting...") );
	GFeedback->BeginTask( TXT("Resaving"), true );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		ResaveEntityTemplatesInDirectory( dir, resaved, failed );
	}
	LOG_EDITOR( TXT("Resaving ended") );

	GFeedback->UpdateTaskInfo( TXT("Creating report...") );
	// Build report
	String report = TXT("<h1>Resave Report</h1><hr><h2><font color='#008000'>Succeeded Resaves:</font></h2><ol>");
	for ( CDiskFile* file : resaved ) report += String::Printf( TXT("<li>%ls</li>"), file->GetDepotPath().AsChar() );
	report += TXT("</ol><hr><h2><font color='#900000'>Failed Resaves:</font></h2><ol>");
	for ( CDiskFile* file : failed ) report += String::Printf( TXT("<li>%ls</li>"), file->GetDepotPath().AsChar() );
	report += String::Printf( TXT("</ol><hr><big><b><font color='#008000'>%i</font></b> files resaved, <b><font color='#900000'>%i</font></b> files failed, <b>%i</b> total"),
		(int)resaved.Size(), (int)failed.Size(), (int)( resaved.Size() + failed.Size() ) );
	GFeedback->EndTask();

	// Show report
	HtmlBox( nullptr, TXT("Entity Template Resave Report"), report );
}

void CEdAssetBrowser::OnCreateInventoriesReport( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	CInventoryReport report;

	LOG_EDITOR( TXT("Inventory report generation starting...") );
	for( Uint32 i=0; i<contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		AddInventoriesReportFromDirectory( dir, &report );
	}
	report.WriteXML();
	LOG_EDITOR( TXT("Inventory report generation ended") );
}

void CEdAssetBrowser::OnSetDownscaleBias( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	CEdDownscaleBiasDlg* downscale = new CEdDownscaleBiasDlg( this, contextMenuDir );
	downscale->Show();
}

void CEdAssetBrowser::OnExtractSourceTexture( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	String dirName = wxDirSelector( wxT("Export to file"), wxT("") );

	// hardcoded by now
	// add fancy popup later on
	String extension = TXT("tga");

	IExporter* exporter = IExporter::FindExporter( ClassID< CBitmapTexture >(), extension );
	if ( !exporter )
	{
		WARN_EDITOR( TXT("No valid exporter for extension '%s' "), extension );
		return;		
	}

	if( !dirName.Empty() )
	{
		for ( CResourceIteratorAdapter< CBitmapTexture > bitmapTexture( *contextMenuDir, TXT("Checking out textures...") ); bitmapTexture; ++bitmapTexture )
		{
			String sourceTexturePath = bitmapTexture->GetImportFile();			

			IExporter::ExportOptions exportOptions;
			exportOptions.m_resource = bitmapTexture.Get();
			exportOptions.m_saveFileFormat = CFileFormat( extension, String::EMPTY );
			exportOptions.m_saveFilePath = dirName + TXT("\\") + (bitmapTexture->GetFile()->GetFileName()).StringBefore(TXT(".")) + TXT(".") + extension;

			LOG_EDITOR( TXT("Exporting resource %s as %s.")
				, exportOptions.m_resource->GetFriendlyName().AsChar()
				, exportOptions.m_saveFilePath.AsChar() );

			if( exporter->DoExport( exportOptions ) )
			{
				// make sure we save new path to source texture 
				// this happens when you generate texture inside the engine, ie. entity proxies
				if( sourceTexturePath.Empty() )
				{
					bitmapTexture->SetImportFile( exportOptions.m_saveFilePath );
				}
			}
		}
	}
}


void CEdAssetBrowser::OnSetAutoHideDistance( wxCommandEvent& event )
{
	// First we show the user an input float dialog.

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	Float forcedAutohideDistance = 20.0f;
	Bool shouldForceAutohideDistance = false;
	Int32 buttonSelected = 0;

	buttonSelected = FormattedDialogBox( this,
		wxT("Batch autohide change..."),
		wxT("V{H{X'Force autohide distance to: 'F''}|H{~B@'OK'B'Cancel'}}"), &shouldForceAutohideDistance, &forcedAutohideDistance
		 );

	if ( buttonSelected == 0 )
	{
		for ( CResourceIteratorAdapter< CMeshTypeResource > meshTypeRes( *contextMenuDir, TXT("Checking out meshes...") ); meshTypeRes; ++meshTypeRes )
		{
			meshTypeRes->GetFile()->LoadThumbnail(); // load thumbnails - otherwise they'll be removed during saving

			if( !shouldForceAutohideDistance )
			{
				Box bbox = meshTypeRes->GetBoundingBox();

				Float bbox_x = Abs( bbox.Max.X - bbox.Min.X );
				Float bbox_y = Abs( bbox.Max.Y - bbox.Min.Y );
				Float bbox_z = Abs( bbox.Max.Z - bbox.Min.Z );

				Float oldAutohide = meshTypeRes->GetAutoHideDistance();
				Float newAutohide = Min(70.0f * MSqrt( Max( Max( bbox_x * bbox_y, bbox_x * bbox_z), bbox_y * bbox_z )), 500.0f );
				if( newAutohide < meshTypeRes->GetAutoHideDistance() )
				{
					meshTypeRes->SetAutoHideDistance( newAutohide );
				}
			}
			else
			{
				if( meshTypeRes->GetAutoHideDistance() != forcedAutohideDistance )
				{
					meshTypeRes->SetAutoHideDistance( forcedAutohideDistance );
				}
			}
		}
	}
}

void CEdAssetBrowser::AddInventoriesReportFromDirectory( CDirectory* dir, CInventoryReport* report )
{
	for ( CDirectory* child : dir->GetDirectories() )
	{
		AddInventoriesReportFromDirectory( child, report );
	}

	for ( CDiskFile* diskFile : dir->GetFiles() )
	{
		const String path = diskFile->GetDepotPath();
		if( path.ContainsSubstring( TXT(".w2ent") ) )
		{
			CEntityTemplate* entTempl = LoadResource< CEntityTemplate >( path );
			if( entTempl )
			{
				LOG_EDITOR( TXT("File: %s"), entTempl->GetDepotPath().AsChar() );
				
				TDynArray< CInventoryDefinition* > invDefs;
				entTempl->GetAllParameters< CInventoryDefinition >( invDefs );

				for ( Uint32 i=0; i<invDefs.Size(); ++i )
				{
					report->CreateReport( invDefs[i], path );
				}				

				entTempl->Discard();
			}
		}
	}
}

void CEdAssetBrowser::ResaveResourcesInDirectory( CDirectory* dir, const String& ext )
{
	ASSERT( dir );

	for ( CDirectory* child : dir->GetDirectories() )
	{
		ResaveResourcesInDirectory( child, ext );
	}

	for ( CDiskFile* diskFile : dir->GetFiles() )
	{
		const String path = diskFile->GetDepotPath();
		if( path.ContainsSubstring( ext ) )
		{
			CJobTree* jobTree = LoadResource< CJobTree >( path );
			if( jobTree )
			{
				LOG_EDITOR( TXT("File: %s"), diskFile->GetDepotPath().AsChar() );
				if ( !diskFile->IsCheckedOut() )
				{
					diskFile->CheckOut();
				}
				if ( diskFile->IsCheckedOut() )
				{
					jobTree->Save();
					jobTree->Discard();
				}
			}
		}
	}
}

void CEdAssetBrowser::ResaveEntityTemplatesInDirectory( CDirectory* dir, TDynArray< CDiskFile* >& saved, TDynArray< CDiskFile* >& failed )
{
	ASSERT( dir );

	// Resave local files
	for ( CDiskFile* diskFile : dir->GetFiles() )
	{
		const String path = diskFile->GetDepotPath();

		// Cancel check
		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		if ( path.EndsWith( TXT(".w2ent") ) )
		{
			LOG_EDITOR( TXT("Attempt to resave template: %s"), path.AsChar() );
			GFeedback->UpdateTaskInfo( String::Printf( TXT("Resaving %ls"), path.AsChar() ).AsChar() );

			// Load and checkout template
			Bool wasLoaded = diskFile->IsLoaded();
			Bool success = false;
			CEntityTemplate* tpl = LoadResource< CEntityTemplate >( path );
			if ( tpl == nullptr ) 
			{
				LOG_EDITOR( TXT("   ...failed to load the template resource") );
				continue;
			}
			if ( !GVersionControl->IsSourceControlDisabled() )
			{
				if ( !diskFile->IsCheckedOut() )
				{
					if ( !diskFile->SilentCheckOut() )
					{
						failed.PushBack( diskFile );
						LOG_EDITOR( TXT("   ...failed to check out the template resource") );
						continue;
					}
				}
			}

			// Try to modify the template
			if ( tpl->MarkModified() )
			{
				// Recreate the flat data buffer
				tpl->CreateFullDataBuffer( nullptr, EntityTemplateInstancingInfo(), nullptr );

				// Create temporary entity for capturing
				CEntity* entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
				if ( entity != nullptr ) 
				{
					// Recreate and update streamed components
					entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
					entity->UpdateStreamedComponentDataBuffers();
					entity->PrepareEntityForTemplateSaving();
					entity->DetachTemplate();

					// Capture data (first pass) 
					tpl->CaptureData( entity );

					// Destroy temporary entity
					entity->Discard();

					// Second instance to convert any new properties
					entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
					if ( entity != nullptr )
					{
						entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
						entity->UpdateStreamedComponentDataBuffers();
						entity->Discard();
					}

					// Save the template
					success = diskFile->Save();
				}
			}

			// Unload the template if it wasn't loaded
			if ( !wasLoaded )
			{
				diskFile->Unload();
			}

			// Log
			if ( success )
			{
				saved.PushBack( diskFile );
				LOG_EDITOR( TXT("   ...SUCCESS!!") );
			}
			else
			{
				failed.PushBack( diskFile );
				LOG_EDITOR( TXT("   ...Failed to resave!") );
			}
		}
	}

	// Resave subdirectories
	for ( CDirectory* child : dir->GetDirectories() )
	{
		// Cancel check
		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}
		ResaveEntityTemplatesInDirectory( child, saved, failed );
	}
}

extern void BindMeshMaterials( wxWindow* parent, const MeshMaterialAutoAssignmentConfig& info, CMeshTypeResource* mesh );

void CEdAssetBrowser::OnAfterImport( IImporter* importer, CResource *imported )
{
	if ( imported->IsA< CMeshTypeResource >() )
	{
		CMeshTypeResource* mesh = SafeCast< CMeshTypeResource >( imported );

		// Ask for mesh material override params
		if ( m_materialAutoBindResult == ECR_Skip || m_materialAutoBindResult == ECR_OK )
		{
			CEdImportMeshDlg importDlg( this, mesh );
			m_materialAutoBindResult = importDlg.DoModal();
			m_materialAutoBindConfig = importDlg.GetConfig();
		}

		// Bind materials
		if ( m_materialAutoBindResult == ECR_OK || m_materialAutoBindResult == ECR_OKAll )
		{
			BindMeshMaterials( this, m_materialAutoBindConfig, mesh );
		}
	}
#if 0 // GFx 3
	if ( imported->IsA< CFlash >() )
	{
		SafeCast<CFlash>( imported )->RegisterFlashTextures();
	}
#endif
}

Bool CEdAssetBrowser::ReimportResource( CResource* resource )
{
	CFilePath path( resource->GetImportFile() );

	// Get suitable importer for this resource
	IImporter* importer = IImporter::FindImporter( resource->GetClass(), path.GetExtension() );
	if ( !importer )
	{
		// Custom handling for texture arrays reimport
		if( resource->IsA< CTextureArray >() )
		{
			CTextureArray* ta = Cast< CTextureArray >( resource );
			return BatchReimportTextureArrays( ta );
		}
		else
		{
			WARN_EDITOR( TXT("No valid importer for extension '%s'"), path.GetExtension().AsChar() );
			return false;
		}
	}

	// Define import data, make sure to specify existing resource
	IImporter::ImportOptions options;
	options.m_existingResource = resource;
	options.m_parentObject = resource->GetParent();
	options.m_sourceFilePath = resource->GetImportFile();

	// Special case for mesh: ask for options
	SMeshImporterParams meshImporterParams( false, false );
	if ( resource->IsA< CMesh >() )
	{
		static Int32 importType = 0;
		int button = FormattedDialogBox( wxT("Reimport Mesh"), wxT("R('Reimport mesh and collision ''Reimport mesh only''Reimport collision only''Reimport mesh and regenerate collision')|H{~B@'&OK'|B'&Cancel'}"), &importType );
		if ( button != 0 )
		{
			return false;
		}
		switch ( importType )
		{
		case 1:
			meshImporterParams.m_reuseVolumes = true;
			break;
		case 2:
			meshImporterParams.m_reuseMesh = true;
			break;
		case 3:
			meshImporterParams.m_regenerateVolumes = true;
			break;
		}
		options.m_params = &meshImporterParams;
	}

	// Do the (re)import !
	GFeedback->BeginTask( TXT("Reimporting"), false );
	CResource* imported = importer->DoImport( options );
	GFeedback->EndTask();

	if ( !imported )
	{
		// Report error
		WARN_EDITOR( TXT("Unable to reimport '%s' from '%s'"), resource->GetClass()->GetName().AsString().AsChar(), resource->GetImportFile().AsChar() );
		return false;
	}

	// Old resource and imported resource should point to the same location
	ASSERT( imported == resource );

	// Set import file 
	imported->SetImportFile( options.m_sourceFilePath );

	// Save this resource for the first time, this will also create thumbnail
	if ( !imported->Save() )
	{
		WARN_EDITOR( TXT("Unable to save reimported '%s'"), path.GetFileName().AsChar() );
		return false;
	}
	
	if ( m_autoUpdateIcons )
	{	
		ASSERT( imported->GetFile() );
		imported->GetFile()->UpdateThumbnail();
	}

	// Assume some rendering resources were changed
	CDrawableComponent::RecreateProxiesOfRenderableComponents();

	// Inform listeners that resource has been reimported. We can re-use the existing FileReload event data, but we'll
	// use a different event name, since the behavior is a bit different -- reload automatically removes all rootset
	// references to the old resource, but we don't do that here.
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().DispatchEvent( CNAME( FileReimport ), CreateEventData( CReloadFileInfo( resource, imported, String::EMPTY ) ) );
#endif

	// Well, we have imported something
	return true;
}

Bool CEdAssetBrowser::BatchReimportTextureArrays( CTextureArray* texArray )
{	
	if( GFeedback->AskYesNo( TXT("Reimporting Texture Array requires reimporting all the source textures! Are you sure?") ) )
	{
		TDynArray< CBitmapTexture* > bitmaps;
		texArray->GetTextures( bitmaps );

		Uint32 succesfullyReimported = 0;

		for( Uint32 i=0; i<bitmaps.Size(); i++ )
		{
			CResource* res = Cast< CResource >( bitmaps[i] );
			if( res != nullptr ) 
			{
				if(	ReimportResource( res ) )
				{
					succesfullyReimported++;
				}
			}
		}

		if( succesfullyReimported > 0 ) 
		{	
			texArray->SetDirty(true);
			texArray->MarkModified();
			texArray->Save(); //also recreating render proxies here					

			// Reload shaders
			for ( ObjectIterator<CMaterialGraph> it; it; ++it )
			{
				(*it)->ForceRecompilation();
			}
			CDrawableComponent::RecreateProxiesOfRenderableComponents();

			bitmaps.Clear();	
			GFeedback->ShowMsg( TXT("Success!"), TXT("Reimported textures: %d"), succesfullyReimported );
			return true;
		}
		else
		{
			GFeedback->ShowMsg( TXT("Reimport failed!"), TXT("No textures reimported! Check source paths.") );
			bitmaps.Clear();
			return false;
		}				
	}
	else
		return false;
}

void CEdAssetBrowser::CreateResourceFromClass( CClass* resourceClass )
{
	ASSERT( resourceClass );

	// Get suitable factory class for this resource
	IFactory* factory = IFactory::FindFactory( resourceClass );
	if ( !factory )
	{
		WARN_EDITOR( TXT("No valid factory for resource '%s'"), resourceClass->GetName().AsString().AsChar() );
		return;
	}

	CreateResourceFromFactory( factory );
}

void CEdAssetBrowser::CreateResourceFromFactory( IFactory* factory )
{
	ASSERT( factory != NULL );

	CClass* resourceClass = factory->GetResourceClass();
	ASSERT( resourceClass != NULL );
	ASSERT( resourceClass->IsBasedOn( ClassID< CResource >() ) );

	// Ask for name
	CEdImportDlg importDlg( this, resourceClass, GetActiveDirectory(), String::EMPTY, factory, &m_favClasses );
	if ( importDlg.DoModal() == ECR_OK )
	{
		String fileName = importDlg.GetFileName();
		bool fileNameOk = true;
		for ( Uint32 i=0; i<fileName.GetLength(); ++i )
			if ( fileName[i] < 32 || String(TXT("<>:\"/\\|?* ")).ContainsCharacter( fileName[i] ) )
			{
				fileNameOk = false;
				break;
			}
		if ( !fileNameOk )
		{
			wxMessageBox( wxT("Cannot create a file that contains a space or the characters <, >, :, \", /, \\, |, ? or *"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR, this );
			return;
		}

		if ( fileName.EndsWith( TXT(".") ) )
		{
			wxMessageBox( wxT("Cannot create a file that contains a trailing period"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR, this );
			return;
		}

		if ( fileName.EqualsNC( L"CON" ) ||
			 fileName.EqualsNC( L"PRN" ) ||
			 fileName.EqualsNC( L"AUX" ) ||
			 fileName.EqualsNC( L"NUL" ) ||
			 fileName.EqualsNC( L"COM1" ) ||
			 fileName.EqualsNC( L"COM2" ) ||
			 fileName.EqualsNC( L"COM3" ) ||
			 fileName.EqualsNC( L"COM4" ) ||
			 fileName.EqualsNC( L"COM5" ) ||
			 fileName.EqualsNC( L"COM6" ) ||
			 fileName.EqualsNC( L"COM7" ) ||
			 fileName.EqualsNC( L"COM8" ) ||
			 fileName.EqualsNC( L"COM9" ) ||
			 fileName.EqualsNC( L"LPT1" ) ||
			 fileName.EqualsNC( L"LPT2" ) ||
			 fileName.EqualsNC( L"LPT3" ) ||
			 fileName.EqualsNC( L"LPT4" ) ||
			 fileName.EqualsNC( L"LPT5" ) ||
			 fileName.EqualsNC( L"LPT6" ) ||
			 fileName.EqualsNC( L"LPT7" ) ||
			 fileName.EqualsNC( L"LPT8" ) ||
			 fileName.EqualsNC( L"LPT9" ) )
		{
			wxMessageBox( wxT("The given filename is an invalid reserved Windows name and cannot be used"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR, this );
			return;
		}

		if ( (!fileName.ContainsCharacter( TXT( ' ' ) ) ) && (fileName.ToLower() == fileName ) ) 
		{
			IFactory::FactoryOptions options;
			options.m_parentObject = NULL;

			// Create resource
			CResource* created = factory->DoCreate( options );
			if ( created )
			{
				// Save this resource for the first time, this will also create thumbnail
				if ( !created->SaveAs( GetActiveDirectory(), fileName ) )
				{
					// Report error
					WARN_EDITOR( TXT("Unable to save created '%s'"), fileName );
				}

				// Update resource list
				UpdateResourceList();

				// Select created resource
				SelectResource( created );
			}
			else
			{
				// Report error
				WARN_EDITOR( TXT("Unable to create '%s'"), resourceClass->GetName().AsString().AsChar() );
			}
		}
		else
		{
			wxMessageDialog dialog( 0, wxT("Cannot create a file with whitespaces or uppercase letters"), wxT("Error"), wxOK | wxICON_ERROR );
			dialog.ShowModal();
		}
	}
}

void CEdAssetBrowser::ExportResource( CResource* resource )
{
	ASSERT( resource );

	// Get resource class
	CClass* resourceClass = resource->GetClass();

	// Enumerate exportable formats
	TDynArray< CFileFormat > formats;
	IExporter::EnumExportFormats( resourceClass, formats );

	if ( formats.Size() == 0 )
	{
		WARN_EDITOR( TXT("No valid exporter for %s '%s'"), resourceClass->GetName().AsString().AsChar(), resource->GetFriendlyName().AsChar() );
		return;
	}

	// Decompose to file path
	CFilePath importFilePath( resource->GetFile()->GetAbsolutePath() );

	String defaultDir = resource->GetFile()->GetAbsolutePath();
	String defaultFile = importFilePath.GetFileName();
	String wildCard;
	CFilePath savePath;

	for ( Uint32 i=0; i<formats.Size(); i++ )
	{
		wildCard += String::Printf( TXT("%s (*.%s)|*.%s")
			, formats[i].GetDescription().AsChar()
			, formats[i].GetExtension().AsChar() 
			, formats[i].GetExtension().AsChar() );

		if ( i < formats.Size() - 1 )
		{
			wildCard += TXT("|");
		}
	}

	String fileDescription = String::EMPTY;
	wxFileDialog saveFileDialog( this, TXT("Export as"), defaultDir.AsChar(), defaultFile.AsChar(), wildCard.AsChar(), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		savePath = saveFileDialog.GetPath().wc_str();

		if ( saveFileDialog.GetFilterIndex() >= 0 && saveFileDialog.GetFilterIndex() < (Int32)formats.Size() )
		{
			savePath.SetExtension( formats[ saveFileDialog.GetFilterIndex() ].GetExtension() );
			fileDescription = formats[ saveFileDialog.GetFilterIndex() ].GetDescription();
		}
	}
	else 
	{
		return;
	}

	IExporter* exporter = IExporter::FindExporter( resourceClass, savePath.GetExtension() );

	if ( !exporter )
	{
		WARN_EDITOR( TXT("No valid exporter for extension '%s' for %s")
			, savePath.GetExtension().AsChar()
			, resource->GetFriendlyName().AsChar() );
		return;		
	}

	IExporter::ExportOptions exportOptions;
	exportOptions.m_resource = resource;
	exportOptions.m_saveFileFormat = CFileFormat( savePath.GetExtension(), fileDescription );
	exportOptions.m_saveFilePath = savePath.ToString();
	
	LOG_EDITOR( TXT("Exporting resource %s as %s.")
		, exportOptions.m_resource->GetFriendlyName().AsChar()
		, exportOptions.m_saveFilePath.AsChar() );

	exporter->DoExport( exportOptions );
}

void CEdAssetBrowser::HandleMissingDependencies( CResource* res ) const
{
	if ( res )
	{
		TDynArray< CResource::DependencyInfo > dependentResourcesPaths;
		res->GetDependentResourcesPaths( dependentResourcesPaths, TDynArray< String >(), false );

		TDynArray< String > missingFiles;
		for ( const CResource::DependencyInfo& info : dependentResourcesPaths )
		{
			if ( !GDepot->FileExist( info.m_path ) && !GDepot->FileExist( info.m_path + TXT(".link") ) )
			{
				missingFiles.PushBack( info.m_path + TXT(";") );
			}
		}

		Bool disableSaving = false;
		if ( !missingFiles.Empty() )
		{
			disableSaving = !wxTheFrame->DisplayErrorsList( 
				String::Printf( 
					TXT("Cannot find some dependencies for file: %ls. Please, make sure you have the latest revision of data, otherwise YOU MAY ERASE IMPORTANT DATA WHILE SAVING."), 
					res->GetDepotPath().AsChar() ), 
				TXT("Save anyway?"), 
				missingFiles, TDynArray< String >(), true );
		}
		res->DisableSaving( disableSaving );
	}
}

void CEdAssetBrowser::RefreshIcons()
{
	THashMap< CDirectory*, wxTreeItemId > :: iterator iter;
	for ( iter = m_dirMapping.Begin(); iter != m_dirMapping.End(); ++iter )
	{
		if ( iter->m_first != GDepot )
		{
			RefreshIcon( iter->m_first, iter->m_second );
		}
	};
}

void CEdAssetBrowser::RefreshIcon( CDirectory *dir, const wxTreeItemId id )
{
	if ( dir->IsCheckedOut() )
	{
		if ( m_depotTree->GetItemImage( id ) != m_imgChangedId )
			m_depotTree->SetItemImage( id, m_imgChangedId );
	}
	else 
	{
		Bool isExpanded = m_depotTree->IsExpanded( id );
		if ( isExpanded && ( m_depotTree->GetItemImage( id ) != m_imgOpenId ) )
			m_depotTree->SetItemImage( id, m_imgOpenId );
		if ( !isExpanded && ( m_depotTree->GetItemImage( id ) != m_imgClosedId ) )
			m_depotTree->SetItemImage( id, m_imgClosedId );
	}
}

void CEdAssetBrowser::OnSearchNPCLevel( wxCommandEvent& event )
{
	Uint8 levelSearch = 1;
	FormattedDialogBox( this, wxT("NPC Level Search"), wxT("H{'Level:'|I=32}H{~B@'Search'~}"), &levelSearch );
 
	TDynArray< String > paths;
	TDynArray< CDiskFile* > relevantFiles;

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	for ( CResourceIteratorAdapter<CEntityTemplate> entityTemplate( *contextMenuDir, TXT("Searching directory for entities..."), RIF_ReadOnly ); entityTemplate; ++entityTemplate )
	{
		if ( entityTemplate )
		{
			CEntity* entity = entityTemplate->GetEntityObject();

			//only search npc entities
			if ( entity && entity->IsA<CNewNPC>() )
			{
				Uint8 npcLevel;
				GetPropertyValue( entity, TXT("level"), npcLevel );				

				if ( npcLevel == levelSearch )
					relevantFiles.PushBack( entityTemplate->GetFile() );
			}
		}
	}

	String displayStr = String::Printf( TXT("NPC Level %d"), levelSearch );
	DisplaySpecialSearchResults( relevantFiles, displayStr );
}


void CEdAssetBrowser::OnSearchEntityClass( wxCommandEvent& event )
{
	//show entity class selector dialog and wait for selection
	CEdEntityClassSelectorDialog selector( this, NULL );
	if ( CClass* searchClass = selector.Execute() )
	{
		SearchEntityClass( event, searchClass );
	}
}

void CEdAssetBrowser::SearchEntityClass( wxCommandEvent& event, CClass* searchClass )
{
	TDynArray< String > paths;
	TDynArray< CDiskFile* > relevantFiles;

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	GFeedback->BeginTask( TXT("Searching directory for entities..."), false);
	GFeedback->UpdateTaskProgress( 0, 100 );

	//find resources (path) on all selected dirs
	for( Uint32 i = 0; i < contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		dir->FindResourcesByExtension( TXT("w2ent"), paths, true, true );
	}

	//check the paths of the entityTemplates for the search class
	for ( Uint32 i = 0; i < paths.Size(); i++ )
	{
		CResource* fileResource;
		CDiskFile* diskFile = GDepot->FindFile( paths[i] );

		Bool alreadyLoaded = diskFile->IsLoaded(); 

		//if the file was already loaded, just get the resource without loading it, and DON'T unload it later on
		if ( alreadyLoaded )
			fileResource = diskFile->GetResource();
		else
			fileResource = LoadResource<CResource>( paths[i] );

		//update feedback
		GFeedback->UpdateTaskProgress(i, paths.Size() );
		GFeedback->UpdateTaskInfo( diskFile->GetDepotPath().AsChar() );


		if ( fileResource )
		{
			CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( fileResource );

			if ( entityTemplate )
			{
				if ( entityTemplate->GetEntityClassName() == searchClass->GetName() )
				{
					relevantFiles.PushBack( diskFile );
				}

				//we have no use for the resource anymore so UNLOAD if previously NOT LOADED
				if ( !alreadyLoaded )
				{
					diskFile->Unload();
				}
			}
		}
	}

	GFeedback->EndTask();

	DisplaySpecialSearchResults( relevantFiles, searchClass->GetName().AsString() );
}


void CEdAssetBrowser::OnSearchBehTreeScriptTask( wxCommandEvent& event )
{
	CClassHierarchyMapper taskClasses;
	struct CTaskNames : public CClassHierarchyMapper::CClassNaming
	{
		void GetClassName( CClass* classId, String& outName ) const override
		{
			IBehTreeTaskDefinition::GetTaskNameStatic( classId, outName );
		}
	};
	CClassHierarchyMapper::MapHierarchy( ClassID< IBehTreeTaskDefinition >(), taskClasses, CTaskNames() );


	CEdBehaviorTreeScriptTaskSelectorDialog selector( this, taskClasses, NULL );	
	if ( CClass* searchTask = selector.Execute() )
	{
		SearchBehTrees( event, searchTask );
	}
}

void CEdAssetBrowser::SearchBehTrees( wxCommandEvent& event, CClass* searchClass )
{
	TDynArray< String > paths;
	TDynArray< CDiskFile* > relevantFiles;

	String searchString;
	IBehTreeTaskDefinition::GetTaskNameStatic( searchClass, searchString );

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	for ( CResourceIteratorAdapter<CBehTree> behTree( *contextMenuDir, TXT("Searching directory for behavior trees..."), RIF_ReadOnly ); behTree; ++behTree )
	{
		if ( behTree )
		{
			TDynArray< IBehTreeNodeDefinition* > nodes;
			IBehTreeNodeDefinition* root = behTree->GetRootNode();

			if ( root )
			{
				root->CollectNodes( nodes );

				//find all script type nodes then check the task name
				for ( IBehTreeNodeDefinition* node : nodes)	
				{
					if ( node && node->GetEditorNodeType() == IBehTreeNodeDefinition::NODETYPE_SCRIPTED )
					{
						IBehTreeTaskDefinition* task = node->GetTask();

						if ( task )
						{
							String taskName;
							task->GetTaskName( taskName );
					
							if ( taskName == searchString )
							{
								relevantFiles.PushBack( behTree->GetFile() );
								break; 
							}
						}
					}
				}
			}
		}
	}

	DisplaySpecialSearchResults( relevantFiles, searchString );
}

void CEdAssetBrowser::OnSearchComponentAbsence( wxCommandEvent& event )
{
	CEdComponentSelectorDialog selector( this );
	if ( CClass* searchComponent = selector.Execute() )
	{
		SearchComponents( event, searchComponent, false );
	}
}

void CEdAssetBrowser::OnSearchComponentPresence( wxCommandEvent& event )
{
	CEdComponentSelectorDialog selector( this );
	if ( CClass* searchComponent = selector.Execute() )
	{
		SearchComponents( event, searchComponent, true );
	}
}

void CEdAssetBrowser::SearchComponents( wxCommandEvent& event, const CClass* searchComponent, Bool presence )
{
	TDynArray< String > paths;
	TDynArray< CDiskFile* > relevantFiles;

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	GFeedback->BeginTask( TXT("Searching directory for entities..."), false );
	GFeedback->UpdateTaskProgress( 0, 100 );

	//find resources (path) on all selected dirs
	for( Uint32 i = 0; i < contextMenuDir->GetDirs().Size(); ++i )
	{
		const CDirectory* dir = contextMenuDir->GetDirs()[i];
		dir->FindResourcesByExtension( CEntityTemplate::GetFileExtension(), paths, true, true );
	}

	//check the paths of the entityTemplates for the search class
	for ( Uint32 i = 0; i < paths.Size(); ++i )
	{
		const CResource* fileResource( nullptr );
		CDiskFile* diskFile = GDepot->FindFile( paths[i] );

		//if the file was already loaded, just get the resource without loading it, and DON'T unload it later on
		const Bool alreadyLoaded = diskFile->IsLoaded(); 
		if ( alreadyLoaded )
		{
			fileResource = diskFile->GetResource();
		}
		else
		{
			fileResource = LoadResource< CResource >( paths[i] );
		}

		//update feedback
		GFeedback->UpdateTaskProgress( i, paths.Size() );
		GFeedback->UpdateTaskInfo( diskFile->GetDepotPath().AsChar() );

		if ( fileResource )
		{
			const CEntityTemplate* entityTemplate = Cast< const CEntityTemplate >( fileResource );
			if ( entityTemplate )
			{
				if ( const CEntity* ent = entityTemplate->GetEntityObject() ) 
				{
					const TDynArray< CComponent* >& componentsList = ent->GetComponents();
					Uint32 nComponentsChecked = 0;

					//search for the presence or absence of the component
					for ( CComponent* component : componentsList )
					{
						if ( component->GetClass()->GetName() == searchComponent->GetName() )
						{
							//we only care about having at least one component in the case of presence
							if ( presence )
							{
								relevantFiles.PushBack( diskFile );
								break;
							}
							else //in the case of absence, a present component breaks the loop
							{
								break; 
							}
						}
						else
						{
							nComponentsChecked++;
						}
					}

					//if we're looking for absence and we got to this point, means we have a hit
					if ( !presence && nComponentsChecked == componentsList.Size() )
					{
						relevantFiles.PushBack( diskFile );
					}

					//we have no use for the resource anymore so UNLOAD if previously NOT LOADED
					if ( !alreadyLoaded )
					{
						diskFile->Unload();
					}
				}
			}
		}
		
		if ( i % 5 == 0 )
		{
			SGarbageCollector::GetInstance().CollectNow();
		}
	}

	GFeedback->EndTask();

	DisplaySpecialSearchResults( relevantFiles, searchComponent->GetName().AsString() );
}


void CEdAssetBrowser::OnSearchAttitudeGroup( wxCommandEvent& event )
{
	CEdAttitudeSearchlDialog dialog( event );
}

void CEdAssetBrowser::SearchAttitudeGroup( wxCommandEvent& event, CName searchGroup )
{
	TDynArray< String > paths;
	TDynArray< CDiskFile* > relevantFiles;

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	GFeedback->BeginTask( TXT("Searching directory for entities..."), false);
	GFeedback->UpdateTaskProgress( 0, 100 );

	//find resources (path) on all selected dirs
	for( Uint32 i = 0; i < contextMenuDir->GetDirs().Size(); i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		dir->FindResourcesByExtension( TXT("w2ent"), paths, true, true );
	}

	//check the paths of the entityTemplates for the search class
	for ( Uint32 i = 0; i < paths.Size(); i++ )
	{
		CResource* fileResource;
		CDiskFile* diskFile = GDepot->FindFile( paths[i] );

		Bool alreadyLoaded = diskFile->IsLoaded(); 

		//if the file was already loaded, just get the resource without loading it, and DON'T unload it later on
		if ( alreadyLoaded )
			fileResource = diskFile->GetResource();
		else
			fileResource = LoadResource<CResource>( paths[i] );

		//update feedback
		GFeedback->UpdateTaskProgress(i, paths.Size() );
		GFeedback->UpdateTaskInfo( diskFile->GetDepotPath().AsChar() );


		if ( fileResource )
		{
			CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( fileResource );

			if ( entityTemplate )
			{
				//find out if we have an attitude group and whether it equates to our search
				CAIProfile* profile = entityTemplate->FindParameter< CAIProfile >( false );
				if ( profile )
				{
					if ( profile->GetAttitudeGroup() != CName::NONE && profile->GetAttitudeGroup() == searchGroup )
					{
						relevantFiles.PushBack( diskFile );
					}
				}

				//we have no use for the resource anymore so UNLOAD if previously NOT LOADED
				if ( !alreadyLoaded )
				{
					diskFile->Unload();
				}
			}
		}
	}

	GFeedback->EndTask();

	DisplaySpecialSearchResults( relevantFiles, searchGroup.AsString() );
}


void CEdAssetBrowser::DisplaySpecialSearchResults( TDynArray< CDiskFile* > relevantFiles, String searchPhrase )
{
	//create a new tab for our results
	CEdAssetBrowser* self = wxTheFrame->GetAssetBrowser();
	EEditorResourceViewMode mode = ERVM_Big;
	wxWindow *current = self->GetCurrentPage();

	if( current )
	{
		CEdAssetBrowserTab* tab = self->m_tabs.FindPtr( current );
		if ( tab )
		{
			mode = tab->m_view->GetViewType();
		}
	}

	//figure out searchFilter
	wxWindow *panel = self->AddTab( NULL, ETT_Folder, searchPhrase );

	CEdAssetBrowserTab* tab = self->m_tabs.FindPtr( panel );
	if ( tab )
	{
		tab->m_view->SetViewType( mode );
		tab->m_view->ListFiles( relevantFiles );
		tab->m_view->CheckActive();
	}

	// Refresh UI
	self->m_dynaPath->Refresh( false );
	self->m_search->Enable( true );
	self->UpdateBookmarkButton();
}


void CEdAssetBrowser::ShowSearchResults(const String &phrase)
{
	struct SearchInfo
	{
		String phrase;
		CDirectory* directory;
		TDynArray< CDiskFile* > result;
		SearchInfo(){}
	};
	
	// Performs the search
	struct SearchTask : public CEdRunnable
	{
		SearchInfo* m_info;
		SearchTask( SearchInfo* info ) : m_info( info ) {}
		void Run()
		{
			if ( m_info->directory )
			{
				m_info->directory->Search( m_info->phrase, m_info->result );
			}
			else
			{
				GDepot->Search( m_info->phrase, m_info->result );
			}
		}
	};

	// Called after the search is finished
	struct SearchFinish : public CEdRunnable
	{
		SearchInfo* m_info;
		SearchFinish( SearchInfo* info ) : m_info( info ) {}
		void Run()
		{
			CEdAssetBrowser* self = wxTheFrame->GetAssetBrowser();

			// Filter by class
			for ( Int32 i = (Int32)m_info->result.Size()-1; i >= 0; --i )
			{
				if ( ! self->CanShowFile( m_info->result[ i ], true ) )
				{
					m_info->result.Erase( m_info->result.Begin() + i );
				}
			}

			if ( m_info->result.Size() > MAX_SEARCH )
			{
				wxMessageDialog dialog( 0, wxT(QUESTION_DISPLAY_SEARCH), wxT("Question"), 
					wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
				if ( dialog.ShowModal() == wxID_NO )
				{
					self->m_search->Enable( true );
					return;
				}
			}

			// Create new tab with results
			self->SetCursor( *wxHOURGLASS_CURSOR );

			EEditorResourceViewMode mode = ERVM_Big;
			wxWindow *current = self->GetCurrentPage();
			if( current )
			{
				CEdAssetBrowserTab* tab = self->m_tabs.FindPtr( current );
				if ( tab )
				{
					mode = tab->m_view->GetViewType();
				}
			}

			CClass* searchFilter = self->m_classFilterBox->GetSelection() > -1
				? (CClass* )self->m_classFilterBox->GetClientData( self->m_classFilterBox->GetSelection() )
				: NULL;
			wxWindow *panel = self->AddTab( m_info->directory, ETT_Folder, m_info->phrase, searchFilter );

			CEdAssetBrowserTab* tab = self->m_tabs.FindPtr( panel );
			if ( tab )
			{
				tab->m_view->SetViewType( mode );
				tab->m_view->ListFiles( m_info->result );
				tab->m_view->CheckActive();
			}

			// Refresh UI
			self->SetCursor( *wxSTANDARD_CURSOR );
			self->m_dynaPath->Refresh( false );
			self->m_search->Enable( true );
			self->UpdateBookmarkButton();

			delete m_info;
		}
	};

	// Disable search box
	m_search->Enable( false );

	// Set search directory
	if ( m_searchInCurrentDir.GetValue() )
	{
		wxWindow *current = GetCurrentPage();
		if ( current )
		{
			CEdAssetBrowserTab* tabData = m_tabs.FindPtr( current );
			if ( tabData )
			{
				m_searchDirectory = tabData->m_directory;
			}
		}
	}
	else
	{
		m_searchDirectory = nullptr;
	}

	// Search in the background
	SearchInfo* info = new SearchInfo();
	info->phrase = phrase;
	info->directory = m_searchDirectory;

	RunParallelEx( new SearchTask( info ), new SearchFinish( info ) );
}

void CEdAssetBrowser::SelectFile( const String& filePath )
{
	// Find asset disk file
	CDiskFile* diskFile = GDepot->FindFile( filePath );
	if ( diskFile )
	{
		// Reset filter
		m_classFilterBox->SetSelection( 0 );
		OnClassFilterChanged( wxCommandEvent() );

		// Select directory
		wxTreeItemId id;
		if ( !m_scanningDepot )
		{
			if ( !m_dirMapping.KeyExist( diskFile->GetDirectory() ) )
			{
				if ( FindDepotTreeItemByPath( m_depotTree, diskFile->GetDirectory()->GetDepotPath(), id ) )
				{
					m_dirMapping.Insert( diskFile->GetDirectory(), id );
				}
			}
			if ( m_dirMapping.Find( diskFile->GetDirectory(), id ) )
			{
				// Select tree item
				m_depotTree->SelectItem( id, true );
				UpdateResourceList();
				SelectDirectory( diskFile->GetDirectory() );

				if ( GetCurrentPage() )
				{
					m_tabs.GetRef( GetCurrentPage() ).m_view->SelectFile( diskFile );
				}

				// Show this window
				Show();
				SetFocus();
			}
		}
	}
}

void CEdAssetBrowser::SelectAndOpenFile( const String& filePath )
{
	// Find asset disk file
	CDiskFile* diskFile = GDepot->FindFile( filePath );
	if ( diskFile )
	{
		// Reset filter
		m_classFilterBox->SetSelection( 0 );
		OnClassFilterChanged( wxCommandEvent() );

		// Select directory
		wxTreeItemId id;
		if ( !m_scanningDepot )
		{
			if ( !m_dirMapping.KeyExist( diskFile->GetDirectory() ) )
			{
				if ( FindDepotTreeItemByPath( m_depotTree, diskFile->GetDirectory()->GetDepotPath(), id ) )
				{
					m_dirMapping.Insert( diskFile->GetDirectory(), id );
				}
			}
			if ( m_dirMapping.Find( diskFile->GetDirectory(), id ) )
			{
				// Select tree item
				m_depotTree->SelectItem( id, true );
				UpdateResourceList();

				// Select file
				if( GetCurrentPage() )
				{
					m_tabs.GetRef( GetCurrentPage() ).m_view->SelectFile( diskFile );
					EditAsset( diskFile->GetResource() );
				}
			}
		}
	}
}

void CEdAssetBrowser::OpenFile( const String& filePath )
{
	// Find asset disk file
	CDiskFile* diskFile = GDepot->FindFile( filePath );
	if ( diskFile )
	{
		// Check if the directory exists
		wxTreeItemId id;
		if ( !m_scanningDepot )
		{
			if ( !m_dirMapping.KeyExist( diskFile->GetDirectory() ) )
			{
				if ( FindDepotTreeItemByPath( m_depotTree, diskFile->GetDirectory()->GetDepotPath(), id ) )
				{
					m_dirMapping.Insert( diskFile->GetDirectory(), id );
				}
			}
			if ( m_dirMapping.Find( diskFile->GetDirectory(), id ) )
			{
				// Open asset editor
				if( GetCurrentPage() )
				{
					EditAsset( diskFile->GetResource() );
				}
			}
		}
	}
}

void CEdAssetBrowser::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileEdited ) )
	{
		String path = GetEventData< String >( data );
		if ( CDiskFile* file = GDepot->FindFile( path ) )
		{
			m_recentFiles.Remove( file );
			m_recentFiles.Insert( 0, file );

			// Keep only 50 recents...
			while ( m_recentFiles.Size() > 50 )
			{
				m_recentFiles.PopBack();
			}
		}
	}
	else if ( name == CNAME( VersionControlStatusChanged ) )
	{
		if ( wxGetActiveWindow() == this )
		{
			RunLaterOnce( [ this ](){ RepopulateDirectory(); } );
		}
		else
		{
			m_repopulateDirOnActivation = true;
		}
	}
	else if ( name == CNAME( ResourcePreSave ) )
	{
		CResource* resource = GetEventData< CResource* >( data );
		if ( resource && !resource->IsA< CCharacterEntityTemplate >() )
		{
			HandleMissingDependencies( resource );
		}
	}
}

void CEdAssetBrowser::UpdateBookmarksMenuNow()
{
	// Find menu
	int menuId = GetMenuBar()->FindMenu( TXT("&Bookmarks") );
	ASSERT( menuId != -1, TXT("Invalid menu caption") );
	wxMenu* menu = GetMenuBar()->GetMenu( menuId );
	ASSERT( menu, TXT("Menu not found") );

	// Remove existing bookmarks (assumes that the menu has two permanent items: toggle and separator)
	while ( menu->GetMenuItemCount() > 2 )
	{
		menu->Destroy( menu->GetMenuItems()[2]->GetId() );
	}
	menu->UpdateUI( NULL );

	// Remove invalid bookmarks
	for ( Int32 i=m_bookmarks.Size() - 1; i >= 0; --i )
	{
		if ( !GDepot->FindPath( m_bookmarks[i].AsChar() ) )
		{
			m_bookmarks.RemoveAt( i );
		}
	}

	// Sort remaining bookmarks
	Sort( m_bookmarks.Begin(), m_bookmarks.End() );

	// Add remaining bookmarks to the menu
	for ( Int32 i=m_bookmarks.SizeInt() - 1; i >= 0; --i )
	{
		menu->Append( ID_BOOKMARKS_FIRST + i, m_bookmarks[i].AsChar(), wxT("Follow bookmark") )->GetId();
		menu->Connect( ID_BOOKMARKS_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAssetBrowser::OnBookmark ), 0, this );
	}
}

void CEdAssetBrowser::UpdateBookmarksMenu()
{
	RunLaterOnce( [](){ wxTheFrame->GetAssetBrowser()->UpdateBookmarksMenuNow(); } );
}

void CEdAssetBrowser::UpdateBookmarkButton()
{
	wxWindow *page = GetCurrentPage();
	if ( page )
	{
		CEdAssetBrowserTab* tabData = m_tabs.FindPtr( page );
		if ( tabData )
		{
			CDirectory* directory = tabData->m_directory;
			if ( directory )
			{
				String path = directory->GetDepotPath();
				if ( m_bookmarks.Exist( path ) )
				{
					XRCCTRL( *this, "BtnBookmark", wxBitmapButton )->SetBitmapLabel( SEdResources::GetInstance().LoadBitmap( TXT("IMG_STAR16_YELLOW") ) );
				}
				else
				{
					XRCCTRL( *this, "BtnBookmark", wxBitmapButton )->SetBitmapLabel( SEdResources::GetInstance().LoadBitmap( TXT("IMG_STAR16_GRAY") ) );
				}
				XRCCTRL( *this, "BtnBookmark", wxBitmapButton )->Enable();
				return;
			}
		}
	}

	XRCCTRL( *this, "BtnBookmark", wxBitmapButton )->SetBitmapLabel( SEdResources::GetInstance().LoadBitmap( TXT("IMG_GRAY16_GRAY") ) );
	XRCCTRL( *this, "BtnBookmark", wxBitmapButton )->Disable();
}

CResource* GetMeshTexture( wxWindow* parent, const String &name, String& notFoundTextures )
{
	// Search for files
	TDynArray< CDiskFile* > filesFound;
	TDynArray< String > textures;
	GDepot->Search( name, filesFound );
	for( TDynArray< CDiskFile* >::iterator it=filesFound.Begin(); it!=filesFound.End(); it++ )
	{
		if( ( *it )->GetFileName() == name )
		{
			textures.PushBack( ( *it )->GetDepotPath() );
		}
	}

	// No textures found
	if ( textures.Size() == 0 )
	{
		notFoundTextures += TXT( "\n  " ) + name;
		return 0;
	}

	// Ask for texture name
	String textureName;
	if( textures.Size() > 1 )
	{
		wxString caption = wxT("More than one texture found for material ");
		caption += name.AsChar();
		caption += wxT(".\nPlease select texture to connect:");
		CEdChooseFromListDlg dlg( parent, textures, textures[ 0 ], TXT("Select texture"), caption, TXT("OK"), TXT("Skip") );
		if( dlg.DoModal() == wxID_OK )
		{
			textureName = dlg.GetValue();
		}
		else
		{
			return 0;
		}
	}
	else
	{
		textureName = textures[ 0 ];
	}

	// Load texture
	CResource* textureRes = LoadResource< CResource >( textureName );
	if( !textureRes )
	{
		notFoundTextures += TXT( "\n  " ) + name;
		return 0;
	}

	// Return loaded texture
	return textureRes;
}

void BindMeshMaterials( wxWindow* parent, const MeshMaterialAutoAssignmentConfig& info, CMeshTypeResource* mesh )
{
	String textureErrorString;

	// Load the material to set
	IMaterial* baseMaterial = Cast< IMaterial >( GDepot->LoadResource( info.m_materialName ) );
	if ( baseMaterial )
	{
		Uint32 volumeMaterialsApplied = 0;

		const TDynArray< String > &materials = mesh->GetMaterialNames();
		for( Uint32 i=0; i<materials.Size(); i++ )
		{
			// Auto assign volume interior material
			if( Red::StringCompare( materials[i].AsChar(), TXT("volume") ) == 0 )
			{
				// Create instance of base material
				IMaterial* volMaterial = Cast< IMaterial >( GDepot->LoadResource( VOLUME_MATERIAL_PATH ) );

				if( volMaterial )
				{
					volumeMaterialsApplied++;
					CMaterialInstance* instance = new CMaterialInstance( mesh, volMaterial );
					mesh->SetMaterial( i, instance );
				}				
			}
			else
			{
				// Load texture to set
				THandle< ITexture > diffuseRes;
				THandle< ITexture > normalRes;
				THandle< ITexture > specularRes;
				THandle< ITexture > maskRes;				
				if ( info.m_setDiffuseParam )
				{
					diffuseRes = Cast< ITexture >( GetMeshTexture( parent, materials[ i ] + TXT(".xbm"), textureErrorString ) );
				}
				if ( info.m_setNormalParam )
				{
					normalRes = Cast< ITexture >( GetMeshTexture( parent, materials[ i ] + TXT("_n.xbm"), textureErrorString ) );
				}
				if ( info.m_setSpecularParam )
				{
					specularRes = Cast< ITexture >( GetMeshTexture( parent, materials[ i ] + TXT("_s.xbm"), textureErrorString ) );
				}
				if ( info.m_setMaskParam )
				{
					maskRes = Cast< ITexture >( GetMeshTexture( parent, materials[ i ] + TXT("_m.xbm"), textureErrorString ) );
				}
				// Nothing to set
				if( !diffuseRes && !normalRes && !specularRes && !maskRes )
				{
					mesh->SetMaterial( i, baseMaterial );
					continue;
				}

				// Create instance of base material
				CMaterialInstance* instance = new CMaterialInstance( mesh, baseMaterial );
				mesh->SetMaterial( i, instance );

				if ( diffuseRes )
				{
					CName parameter( info.m_diffuseParameterName );
					instance->WriteParameter( parameter, diffuseRes );
				}

				if ( normalRes )
				{
					CName parameter( info.m_normalParameterName );
					instance->WriteParameter( parameter, normalRes );
				}

				if ( specularRes )
				{
					CName parameter( info.m_specularParameterName );
					instance->WriteParameter( parameter, specularRes );
				}
				if ( maskRes )
				{
					CName parameter( info.m_maskParameterName );
					instance->WriteParameter( parameter, maskRes );
				}
			}			
		}

		// Show error message
		if ( !textureErrorString.Empty() )
		{
			String msg = TXT( "Cannot find texture(s):" ) + textureErrorString + TXT("\nNo instance(s) created for those material(s)!");
			wxMessageDialog dialog( 0, msg.AsChar(), wxT("Warning"), wxOK | wxICON_WARNING );
			dialog.ShowModal();
		}

		if( volumeMaterialsApplied > 0 ) GFeedback->ShowMsg( TXT("Volume Interior material found!"), TXT("Found: %d, volume materials."), volumeMaterialsApplied );
	}
	else
	{
		wxMessageDialog dialog( 0, wxT("Cannot load material"), wxT("Error"), wxOK | wxICON_ERROR );
		dialog.ShowModal();
	}
}

void CEdAssetBrowser::OnDirectoryChange( const TDynArray< ChangedFileData >& changes )
{
	// We are listening for the deletion of a file here for Photoshop and modified for MAX. 
	// Since Photoshop creates a .tmp file with a crazy name we look for which file was deleted.
	// We make sure that the file deleted exists on disk since Photoshop re-creates the deleted file. And that makes sure we don't reload a file that was deleted by the user in windows explorer.
	for( Uint32 i =0; i<changes.Size(); i++ )
	{
		// Creating a wxString so that we can check if file exists or not
		wxString pathString( changes[i].m_path.AsChar(), wxConvUTF8 );
		if( wxFileExists( pathString ) )
		{
			// Map with extensions. If we find the key we use the value
			THashMap< String, String > map;
			map.Insert( TXT("tga"), TXT("xbm") );
			map.Insert( TXT("re"), TXT("w2mesh") );

			CFilePath filePath( changes[i].m_path );
			//filePath.Normalize();
			String ext = filePath.GetExtension();
			String path = filePath.GetPathString();
			String fileName = filePath.GetFileName();
			
			// Slice the path out
			TDynArray< String > parts;
			path.Slice( parts, TXT("/w3_assets/") );

			// Making sure that we actually split something otherwise we have an invalid path
			if( parts.Size() > 1 )
			{
				if ( String* extRes = map.FindPtr( ext ) )
				{
					// Get all the files with the correct extension
					// Iterate over them and query their import file. If it is the one we just changed we re-import that file					
					String nPath = parts[1] + TXT("\\");
					nPath.ReplaceAll( TXT("/"), TXT("\\") );
					
					// Need to check here if fDir is found or not.
					if ( CDirectory *fDir = GDepot->FindPath( nPath.AsChar() ) )
					{
						for ( CDiskFile* file : fDir->GetFiles() )
						{
							if ( file->GetFileName().EndsWith( extRes->AsChar() ) )
							{
								THandle< CResource > res = file->Load();
								if ( res )
								{
									// If the import source is the same as the one we just changed we re import the asset
									String importFile = res->GetImportFile();
									if ( importFile.ToLower() == pathString )
									{
										wxTheFrame->GetAssetBrowser()->ReimportResource( res );
										file->UpdateThumbnail();
									}
								}
							}
						}
					} // if ( CDirectory* ...
				}
			}
		}
	}
}

Bool PerformSilentCheckOutOnResource( CDiskFile* file )
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( file )
	{
		file->GetStatus(); // force to update the p4 status

		if ( file->IsLocal() || file->IsCheckedOut() )
		{
			return file->MarkModified();
		}
		else
		{
			if ( file->SilentCheckOut() )
			{
				return file->MarkModified();
			}
			else
			{
				return false;
			}
		}
	}
	else
#endif
	{
		return false;
	}
}

static int SuggestLODFromDistance( Float autoHideDistance )
{ 
	if ( autoHideDistance <= 128.0f ) return 0;
	else if ( autoHideDistance <= 256.0f ) return 1;
	else if ( autoHideDistance <= 512.0f ) return 2;
	else return -1;
}

void CEdAssetBrowser::OnSetStreamingLODs( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	CWorld*	m_world = nullptr;

	const Uint32 size = contextMenuDir->GetDirs().Size();
	CDirectory* dir;

	Uint32 totalEntitiesCount = 0;
	Uint32 processedEntitiesCount = 0;

	// gathering meshes
	for( Uint32 i=0; i<size; i++ )
	{
		dir = contextMenuDir->GetDirs()[i];
		GetAllResourceFilesInDirectory< CEntityTemplate>( dir, diskFiles );

		const Uint32 diskFileSize = diskFiles.Size();

		if( diskFileSize > 0 )
		{
			GFeedback->BeginTask( TXT("Updating streaming LOD values..." ), false );
			CResource::FactoryInfo< C2dArray > info;
			C2dArray* debugDumpInfo = info.CreateResource();
			debugDumpInfo->AddColumn( TXT("Name"), TXT("") );
			debugDumpInfo->AddColumn( TXT("Path"), TXT("") );

			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				if( diskFiles[ f ]->IsLoaded() || diskFiles[ f ]->Load() )
				{
					totalEntitiesCount++;
					
					if ( CEntityTemplate* tpl = Cast< CEntityTemplate >( diskFiles[ f ]->GetResource() ) )
					{					
						CEntity* entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
						if ( entity == nullptr ) continue;

						entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

						//ForceStreamingSetupToEntity( entity );
						//////////////
						// TODO swap with proper resave
						entity->ForceFinishAsyncResourceLoads();

						const TDynArray< CComponent* >& components = entity->GetComponents();

						// Check if we should force the entity to be streamed
						Bool shouldForceStreaming = true;
						for ( auto it=components.Begin(); it != components.End(); ++it )
						{
							CMeshTypeComponent* meshTypeComponent = Cast< CMeshTypeComponent >( *it );
							if ( meshTypeComponent == nullptr )
							{
								continue;
							}
							/*
							// If a mesh has attachments, it might be needed by other stuff, so dont force it
							if ( !meshTypeComponent->GetParentAttachments().Empty() ||
								!meshTypeComponent->GetChildAttachments().Empty() )
							{
								shouldForceStreaming = false;
								break;
							}
							*/
						}
						if ( shouldForceStreaming )
						{
							entity->SetStreamed( true );
						}

						// If the entity isn't streamed, don't bother
						if ( !entity->ShouldBeStreamed() )
						{
							continue;
						}

						TDynArray< String > rowData;
						rowData.PushBack( diskFiles[ f ] ->GetFileName() );
						rowData.PushBack( diskFiles[ f ] ->GetDepotPath() );
						processedEntitiesCount++;
						debugDumpInfo->AddRow( rowData );		

						entity->UpdateStreamedComponentDataBuffers();
						entity->PrepareEntityForTemplateSaving();
						entity->DetachTemplate();
						tpl->CaptureData( entity );

						// Destroy instance
						entity->Discard();

						// Second instance to convert any new properties
						entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
						if ( entity != nullptr )
						{
							entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
							entity->UpdateStreamedComponentDataBuffers();
							entity->Discard();
						}

						PerformSilentCheckOutOnResource( diskFiles[ f ] );						
						diskFiles[f]->Save();				
					}	

					diskFiles[ f ]->Unload();
				}
				GFeedback->UpdateTaskProgress( f, diskFileSize );
			}
			GFeedback->EndTask();

			if( GFeedback->AskYesNo( String::Printf( TXT("Entities checked: %d\nEntities updated: %d\nDump summary to CSV?"), totalEntitiesCount, processedEntitiesCount ).AsChar() ) )
			{
				CDirectory* savePath = contextMenuDir->GetDirs()[i]->GetParent();
				String temp;
				savePath->GetDepotPath( temp );
				temp = String::Printf( TXT("%s\\%s"), temp.AsChar(), contextMenuDir->GetDirs()[i]->GetName().AsChar() );

				if ( CDiskFile *diskFile = GDepot->FindFile( temp ) )
				{
					String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), temp.AsChar() );
					if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
					{
						return;
					}
				}

				debugDumpInfo->SaveAs( savePath, CFilePath( temp ).GetFileName(), true );
				ShellExecute( NULL, TXT("explore"),
					savePath->GetAbsolutePath().AsChar(),
					NULL, NULL, SW_SHOWNORMAL);
				debugDumpInfo->Discard();
			}

		}
	}
	OnRefreshTab( event );
}

void CEdAssetBrowser::OnRemoveUnusedMaterials( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	Uint32 unusedMaterialsCount = 0;

	for ( CResourceIteratorAdapter< CMesh > meshRes( *contextMenuDir, TXT("Removing unused materials") ); meshRes; ++meshRes )
	{
		meshRes->GetFile()->LoadThumbnail(); // load thumbnails - otherwise they'll be removed during saving
		unusedMaterialsCount += meshRes->RemoveUnusedMaterials();
	}

	GFeedback->ShowMsg( TXT("Unused material removal"), String::Printf( TXT("Unused materials: %d"), unusedMaterialsCount ).AsChar() );
	OnRefreshTab( event );
}


void CEdAssetBrowser::OnCalculateShadowPriorities( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	/*Bool force = false;
	Int32 selection = 3;
	Float qualityValue = 1.0f;

	CEnum* meshShadowImportanceEnum = SRTTI::GetInstance().FindEnum(CName(TXT("EMeshShadowImportance")));
	TDynArray<CName> enumNames = meshShadowImportanceEnum->GetOptions();
	TDynArray<Int32> enumValues;
	Int32 enumValue = 0;
	String enumOptions = TXT("");
	
	for( CName enumName : enumNames )
	{
		meshShadowImportanceEnum->FindValue( enumName, enumValue );
		enumValues.PushBack(enumValue);
		enumOptions += TXT("'") + enumName.AsString() + TXT("'");
	}
	wxT("H{X'Force to'=100C('Dupa')=100}");
	String dialogContent = TXT("H{X'Force to'=100C(") + enumOptions + TXT(")=150}H{'ShadowFactor'=100F=150}H{B@'OK'=125|B'Cancel'=125}");

	Int32 buttonSelection = FormattedDialogBox( this, wxT("Calculate Shadow Priorities"), dialogContent.AsChar(), &force, &selection, &qualityValue );
	if( buttonSelection == 0 )
	{
		for ( CResourceIteratorAdapter< CMesh > meshRes( *contextMenuDir, TXT("Calculating Shadow Priorities") ); meshRes; ++meshRes )
		{
			meshRes->GetFile()->LoadThumbnail(); // load thumbnails - otherwise they'll be removed during saving

			if ( force )
			{
				meshRes->SetShadowImportance( ( EMeshShadowImportance ) enumValues[selection] );
			}
			else
			{
				meshRes->CalculateShadowInfo( qualityValue );
			}
		}

		OnRefreshTab( event );
	}*/
}

// Temporary function for debug resave, will be changed to proper soon
void CEdAssetBrowser::OnBatchTextureGroupChange( wxCommandEvent& event )
{
	TDynArray< CName > groups;
	SRenderSettingsManager::GetInstance().GetTextureGroups().GetKeys( groups );
	Sort( groups.Begin(), groups.End(), []( CName a, CName b ) { return Red::System::StringCompareNoCase( a.AsChar(), b.AsChar() ) < 0; } );

	wxString comboStr = wxT("C(");
	for ( CName group : groups )
	{
		comboStr += wxT("'") + wxString( group.AsChar() ) + wxT("'");
	}
	comboStr += wxT(")");

	Int32 fromSelection = 0;
	Int32 toSelection = 1;
	FormattedDialogBox( this,
		wxT("Batch texture group change..."),
		wxT("T{'from: '") + comboStr + wxT("'to: '") + comboStr + wxT("};|H{~B@'OK'}"),
		&fromSelection, &toSelection );
	ASSERT ( fromSelection < groups.SizeInt() );
	ASSERT ( toSelection < groups.SizeInt() );

	if ( fromSelection == toSelection )
	{
		return; // nothing to do
	}

	String excludeListPath = TXT("characters\\exclude_character_textures.csv");
	FormattedDialogBox( this, wxT("Fixing character textures..."), wxT("H{'Excluded list:  'S^=400}H{~B@'OK'~}"), &excludeListPath );
	CDiskFile* excludeListFile = GDepot->FindFile( excludeListPath );

	TDynArray< String > excludedPaths;

	if( excludeListFile == nullptr )
	{
		FormattedDialogBox( this, wxT("Batch texture group change..."), wxT("H{'Unable to find excluded list file'}"));		
	}
	else
	{
		excludeListFile->Load();
		C2dArray* excludeList = Cast< C2dArray >( excludeListFile->GetResource() );

		if( excludeList == nullptr )
		{
			FormattedDialogBox( this, wxT("Batch texture group change..."), wxT("H{'Unable to load excluded list file'}"));
			return;
		}

		excludedPaths = excludeList->GetColumn< String >( TXT( "Excluded" ) );
		excludeListFile->Unload();
	}

	CContextMenuDir* contextMenu = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	TDynArray< CDiskFile* > texFiles;
	for ( CResourceIteratorAdapter< CBitmapTexture > texture( *contextMenu, TXT("Gathering textures" ), RIF_ReadOnly ); texture; ++texture )
	{
		Bool isExcluded = false;
		String depotPath = texture->GetDepotPath();
		for ( const String& excludedPath : excludedPaths )
		{
			if( depotPath.ContainsSubstring( excludedPath ) )
			{
				isExcluded = true;
				break;
			}
		}

		if ( !isExcluded && texture->GetTextureGroupName() == groups[fromSelection] )
		{
			texFiles.PushBack( texture->GetFile() );
		}
	}

	if ( texFiles.Empty() )
	{
		GFeedback->ShowMsg( TXT("No textures"), TXT("No textures matching selected source group found") );
	}
	else
	{
		for ( CResourceIterator< CBitmapTexture > texture( texFiles, TXT("Changing texture group" ) ); texture; ++texture )
		{
			texture->SetTextureGroup( groups[toSelection] );
		}
	}
}

void CEdAssetBrowser::OnSimplifyMaterials( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	const Uint32 size = contextMenuDir->GetDirs().Size();
	CDirectory* dir;

	GFeedback->BeginTask( TXT("Simplifying Materials" ), false );

	CDiskFile* PbrStdMaterial = GDepot->FindFile( TXT("engine\\materials\\graphs\\pbr_std.w2mg") );
	PbrStdMaterial->Load();
	IMaterial* newMaterial = Cast< IMaterial >( PbrStdMaterial->GetResource() );
	ASSERT(newMaterial);

	for( Uint32 d=0; d<size; d++ )
	{
		dir = contextMenuDir->GetDirs()[d];
		GetAllResourceFilesInDirectory< CMesh>( dir, diskFiles );

		const Uint32 diskFileSize = diskFiles.Size();

		if( diskFileSize > 0 )
		{
			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				Bool wasLoaded = diskFiles[ f ]->IsLoaded();

				if( diskFiles[ f ]->Load() )
				{
					if ( CMesh* meshRes = Cast< CMesh >( diskFiles[ f ]->GetResource() ) )
					{
						CMeshTypeResource::TMaterials& mats =  meshRes->GetMaterials();

						for( Uint32 i=0; i<mats.Size(); ++i )
						{
							IMaterial* resourceMaterial = Cast< IMaterial > ( mats[i] );
							

							if (resourceMaterial)
							{
								CMaterialInstance* instance = Cast< CMaterialInstance >( resourceMaterial );

								if (instance)
								{
									String detTexture = TXT("");

									THandle< ITexture > det; 
									THandle< ITexture > basedet; 

									Bool detfound = instance->ReadParameter( CName( TXT("DetailNormal") ), det );
									Bool basedetFound = false;
									if ( resourceMaterial )
									{
										basedetFound = resourceMaterial->ReadParameter( CName( TXT("DetailNormal") ), basedet );
									}
									if(detfound && det && basedetFound && basedet)
									{ 
										detTexture = det->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") );
										if (detTexture == TXT("\"engine\\textures\\editor\\normal.xbm\""))
										{
											diskFiles[f]->SilentCheckOut();
											
											if( diskFiles[f]->MarkModified() )
											{
												instance->SetBaseMaterial( newMaterial );
												meshRes->Save();
											}
										}
									}
								}
							}
						}
					}

					if ( !wasLoaded )
					{
						diskFiles[ f ]->Unload();
					}
				}
				GFeedback->UpdateTaskProgress( f, diskFileSize );
			}
		}
	}
	GFeedback->EndTask();
	OnRefreshTab( event );
}

void CEdAssetBrowser::OnCalculateComponentsInEntities( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	TSortedMap< String, Uint32 > componentMap;

	const Uint32 size = contextMenuDir->GetDirs().Size();
	CDirectory* dir;

	for( Uint32 d=0; d<size; d++ )
	{
		dir = contextMenuDir->GetDirs()[d];
		GetAllResourceFilesInDirectory< CEntityTemplate >( dir, diskFiles );
		
		const Uint32 diskFileSize = diskFiles.Size();
		GFeedback->BeginTask( TXT("Checking components"), false);
		GFeedback->UpdateTaskProgress( 0, diskFileSize );
		if( diskFileSize > 0 )
		{
			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				Bool wasLoaded = diskFiles[ f ]->IsLoaded();

				if( diskFiles[ f ]->Load() )
				{
					const String path = diskFiles[ f ]->GetDepotPath();

					CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( path );
					if( entityTemplate )
					{
						CEntity* ent = entityTemplate->GetEntityObject();
						if(ent)
						{
							ent->CreateStreamedComponents( SWN_DoNotNotifyWorld );
							ent->ForceFinishAsyncResourceLoads();

							Uint32 compCount = ent->GetComponents().Size();
							componentMap.Insert(path, compCount);
						}
					}					
				}
				if(!wasLoaded) diskFiles[ f ]->Unload();	
				GFeedback->UpdateTaskProgress( f, diskFileSize );

				if ( f % 30 == 0 )
				{
					SGarbageCollector::GetInstance().CollectNow();
				}
			}
		}
		GFeedback->EndTask();
	}

	// Component info
	CResource::FactoryInfo< C2dArray > info;
	C2dArray* dumpInfo = info.CreateResource();

	dumpInfo->AddColumn( TXT("Path"), TXT("") );
	dumpInfo->AddColumn( TXT("Count"), TXT("") );

	Uint32 rowNr = 0;
	TDynArray< String > rowData;

	for ( auto elem : componentMap )
	{
		dumpInfo->AddRow( rowData );
		dumpInfo->SetValue(  elem.m_first,  String ( TXT( "Path" ) ), rowNr ) ;
		dumpInfo->SetValue(  ToString( elem.m_second ),  String ( TXT( "Count" ) ), rowNr ) ;
		rowNr +=1;
	}

	CDirectory* savePath = contextMenuDir->GetDirs()[0]->GetParent();
	String temp;
	savePath->GetDepotPath( temp );
	temp = String::Printf( TXT("%s\\%s"), temp.AsChar(), TXT("ComponentsCount.csv") );

	dumpInfo->SaveAs( savePath, CFilePath( temp ).GetFileName(), true );
	dumpInfo->Discard();
}

void CEdAssetBrowser::OnRemoveForcedAppearances( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	const Uint32 size = contextMenuDir->GetDirs().Size();

	// gathering files
	for( Uint32 i=0; i<size; i++ )
	{
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		GetAllResourceFilesInDirectory< CEntityTemplate>( dir, diskFiles );

		const Uint32 diskFileSize = diskFiles.Size();

		if( diskFileSize > 0 )
		{
			GFeedback->BeginTask( TXT("Removing forced appearances..." ), false );

			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				Bool wasLoaded = diskFiles[f]->IsLoaded();
				GFeedback->UpdateTaskInfo( TXT("%ls"), diskFiles[f]->GetDepotPath().AsChar() );
				if ( diskFiles[ f ]->IsLoaded() || diskFiles[ f ]->Load() )
				{					
					if ( CEntityTemplate* tpl = Cast< CEntityTemplate >( diskFiles[ f ]->GetResource() ) )
					{					
						CEntity* entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
						if ( entity == nullptr ) continue;

						entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
						entity->ForceFinishAsyncResourceLoads();

						const TDynArray< CComponent* >& components = entity->GetComponents();
						
						CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );

						if ( !appearanceComponent->IsIncludedFromTemplate() )
						{
							// Save
							appearanceComponent->SetForcedAppearance( CName::NONE );

							// Capture
							entity->UpdateStreamedComponentDataBuffers();
							entity->PrepareEntityForTemplateSaving();
							entity->DetachTemplate();
							tpl->CaptureData( entity );

							// Destroy instance
							entity->Discard();
							
							// Second instance to convert any new properties
							entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
							if ( entity != nullptr )
							{
								entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
								entity->UpdateStreamedComponentDataBuffers();
								entity->Discard();
							}

							PerformSilentCheckOutOnResource( diskFiles[ f ] );						
							diskFiles[f]->Save();				
						}
						else
						{
							entity->Discard();
						}
					}	

					if ( !wasLoaded )
					{
						diskFiles[ f ]->Unload();
					}
				}
				GFeedback->UpdateTaskProgress( f, diskFileSize );
			}
			GFeedback->EndTask();
		}
	}
	OnRefreshTab( event );
}

void CEdAssetBrowser::OnCalculateAppearancesTextureCost( wxCommandEvent& event )
{
	struct SAppearance
	{
		Float size;
		String appearance;
		Float normalmapCost;
		Float diffuseCost;
		Float ambienceCost;

		String entityName;
		SAppearance()
		{
			normalmapCost = 0.0f;
			diffuseCost = 0.0f;
		}

		Bool operator <( const SAppearance& other ) const
		{
			// Sort in reverse order, so biggest is first.
			return size > other.size;
		}
	};

	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}	
	
	CWorld* world = new CWorld();
	world->AddToRootSet();

	WorldInitInfo initInfo;	
	initInfo.m_previewWorld = true;
	world->Init( initInfo );

	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	const Uint32 size = contextMenuDir->GetDirs().Size();
	CDirectory* dir;

	// TODO : not a map, like appearanceList
	TSortedMap< Float, String > entityMap;
	TSortedArray< SAppearance > appearanceList;
	
	for( Uint32 d=0; d<size; d++ )
	{
		dir = contextMenuDir->GetDirs()[d];
		GetAllResourceFilesInDirectory< CEntityTemplate >( dir, diskFiles );

		Uint32 tdata = 0;
		TDynArray< CBitmapTexture* > usedTextures;

		const Uint32 diskFileSize = diskFiles.Size();
		TDynArray< Uint32 > textureData;
		GFeedback->BeginTask( TXT("Checking the appearances..."), false);
		GFeedback->UpdateTaskProgress( 0, diskFileSize );
		if( diskFileSize > 0 )
		{
			
			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				Bool wasLoaded = diskFiles[ f ]->IsLoaded();

				if( diskFiles[ f ]->Load() )
				{
					const String path = diskFiles[ f ]->GetDepotPath();
					
					CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( path );
					if( entityTemplate )
					{
						EntitySpawnInfo info;
						info.m_template = entityTemplate;
						CEntity* ent = world->GetDynamicLayer()->CreateEntitySync( info );

						TDynArray< const CEntityAppearance* > appList;
						entityTemplate->GetAllEnabledAppearances( appList );
						CName appearanceName;
						Uint32 usedAppTextureData;
						Uint32 usedAppNormalmapTextureData;
						Uint32 usedAppDiffusemapTextureData;
						Uint32 usedAppAmbiencemapTextureData;
						CAppearanceComponent* appComp = ent->FindComponent<CAppearanceComponent>();
						if ( appComp )
						{

							for ( Uint32 j = 0; j < appList.Size(); ++j )
							{
								appearanceName = appList[j]->GetName();


								appComp->ApplyAppearance( appearanceName );

								TDynArray< CBitmapTexture* > usedTextures;

								for ( ComponentIterator< CMeshTypeComponent > it( ent ); it; ++it )
								{
									CMeshTypeComponent* mtc = *it;
									CMeshTypeResource* meshResource = mtc ? mtc->GetMeshTypeResource() : nullptr;
									if ( meshResource )
									{
										ent->CreateStreamedComponents( SWN_DoNotNotifyWorld );
										ent->ForceFinishAsyncResourceLoads();
										const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();

										for ( Uint32 m=0; m<materials.Size(); m++ )
										{
											IMaterial* material = materials[m].Get();
											if ( material )
											{
												TDynArray< MeshTextureInfo* > usedTexturesInfo;
												MeshStatsNamespace::GatherTexturesUsedByMaterial( material, m, usedTexturesInfo );
												for ( Uint32 tex = 0; tex < usedTexturesInfo.Size(); ++tex )
												{
													// Uniquely push back the textures in this texture array
													usedTextures.PushBackUnique( usedTexturesInfo[tex]->m_texture );
												}
											}
										}
									}
								}

								usedAppTextureData = 0;
								usedAppNormalmapTextureData = 0;
								usedAppDiffusemapTextureData = 0;
								usedAppAmbiencemapTextureData = 0;
								for ( Uint32 it = 0; it < usedTextures.Size(); ++it )
								{
									// Get the total cost
									usedAppTextureData += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
									// Get the separate costs for the different textures
									String textureName = usedTextures[it]->GetFriendlyName();
									// Split the string to its parts
									TDynArray< String > parts = textureName.Split( TXT("_"), true );
									String lastInput = parts[parts.Size()-1];
									if( lastInput.BeginsWith(TXT("n")) )
									{
										usedAppNormalmapTextureData += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
									}
									else if( lastInput.BeginsWith(TXT("d")) )
									{
										usedAppDiffusemapTextureData += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
									}
									else if( lastInput.BeginsWith(TXT("a")) )
									{
										usedAppAmbiencemapTextureData += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
									}
								}
								//entityMap.Insert( usedTextureData / (1024.0f*1024.0f), depotPath  );

								SAppearance newStruct;
								newStruct.size = usedAppTextureData / (1024.0f*1024.0f);
								newStruct.normalmapCost = usedAppNormalmapTextureData / (1024.0f*1024.0f);
								newStruct.diffuseCost = usedAppDiffusemapTextureData / (1024.0f*1024.0f);
								newStruct.ambienceCost = usedAppAmbiencemapTextureData / (1024.0f*1024.0f);
								newStruct.appearance = appearanceName.AsString();
								newStruct.entityName = path;
								appearanceList.Insert( newStruct );
							}
						}

						ent->Destroy();
					}					
				}
				if(!wasLoaded) diskFiles[ f ]->Unload();	
				GFeedback->UpdateTaskProgress( f, diskFileSize );

				if ( f % 30 == 0 )
				{
					SGarbageCollector::GetInstance().CollectNow();
				}
			}
		}
		GFeedback->EndTask();
	}
	
	// Entity info
	CResource::FactoryInfo< C2dArray > info;
	C2dArray* dumpInfo = info.CreateResource();

	dumpInfo->AddColumn( TXT("TextureCost"), TXT("") );
	dumpInfo->AddColumn( TXT("Path"), TXT("") );

	Uint32 rowNr = 0;
	TDynArray< String > rowData;

	for ( auto elem : entityMap )
	{
		dumpInfo->AddRow( rowData );
		dumpInfo->SetValue(  ToString( elem.m_first ),  String ( TXT( "TextureCost" ) ), rowNr ) ;
		dumpInfo->SetValue(  elem.m_second,  String ( TXT( "Path" ) ), rowNr ) ;
		rowNr +=1;
	}
	
	CDirectory* savePath = contextMenuDir->GetDirs()[0]->GetParent();
	String temp;
	savePath->GetDepotPath( temp );
	temp = String::Printf( TXT("%s\\%s"), temp.AsChar(), TXT("EntityTextureCost.csv") );

	if ( CDiskFile *diskFile = GDepot->FindFile( temp ) )
	{
		String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), temp.AsChar() );
		if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
		{
			return;
		}
	}

	dumpInfo->SaveAs( savePath, CFilePath( temp ).GetFileName(), true );
	dumpInfo->Discard();

	// Write Appearance info to file
	CResource::FactoryInfo< C2dArray > appinfo;
	C2dArray* dumpInfoApp = appinfo.CreateResource();

	dumpInfoApp->AddColumn( TXT("TextureCost"), TXT("") );
	dumpInfoApp->AddColumn( TXT("TextureCostNormalmap"), TXT("") );
	dumpInfoApp->AddColumn( TXT("TextureCostDiffusemap"), TXT("") );
	dumpInfoApp->AddColumn( TXT("TextureCostAmbiencemap"), TXT("") );
	dumpInfoApp->AddColumn( TXT("Appearance"), TXT("") );
	dumpInfoApp->AddColumn( TXT("Entity"), TXT("") );

	Uint32 rowNrApp = 0;
	TDynArray< String > rowDataApp;

	for ( auto elem : appearanceList )
	{
		dumpInfoApp->AddRow( rowDataApp );
		dumpInfoApp->SetValue(  ToString( elem.size ),  String ( TXT( "TextureCost" ) ), rowNrApp ) ;
		dumpInfoApp->SetValue(  ToString( elem.normalmapCost ),  String ( TXT( "TextureCostNormalmap" ) ), rowNrApp ) ;
		dumpInfoApp->SetValue(  ToString( elem.diffuseCost ),  String ( TXT( "TextureCostDiffusemap" ) ), rowNrApp ) ;
		dumpInfoApp->SetValue(  ToString( elem.ambienceCost ),  String ( TXT( "TextureCostAmbiencemap" ) ), rowNrApp ) ;
		dumpInfoApp->SetValue(  elem.appearance,  String ( TXT( "Appearance" ) ), rowNrApp ) ;
		dumpInfoApp->SetValue(  elem.entityName,  String ( TXT( "Entity" ) ), rowNrApp ) ;
		rowNrApp +=1;
	}

	CDirectory* savePathApp = contextMenuDir->GetDirs()[0]->GetParent();
	String tempApp;
	savePathApp->GetDepotPath( tempApp );
	tempApp = String::Printf( TXT("%s\\%s"), tempApp.AsChar(), TXT("AppearanceTextureCost.csv") );

	if ( CDiskFile *diskFile = GDepot->FindFile( tempApp ) )
	{
		String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), tempApp.AsChar() );
		if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
		{
			return;
		}
	}

	dumpInfoApp->SaveAs( savePathApp, CFilePath( tempApp ).GetFileName(), true );
	dumpInfoApp->Discard();

	world->RemoveFromRootSet();
	world->Discard();
}

void CEdAssetBrowser::OnRemoveUnusedAnimations( wxCommandEvent& event )
{
	bool keep_list_anims = true;
	if ( wxMessageBox( TXT("Choose 'Yes' for keeping animations from list or 'No' for removing animations from list."), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
	{
		keep_list_anims = false;
	}
	String defaultDir = wxEmptyString;
	String wildCard = TXT("All files (*.*)|*.*|CSV files (*.csv)|*.csv");
	wxFileDialog loadFileDialog( this, wxT("Load csv"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = loadFileDialog.GetPath().wc_str();
		THashMap<String,THashSet<CName>> data;
		FILE* f = fopen( UNICODE_TO_ANSI( loadPath.AsChar() ), "r" );
		if( f )
		{
			char buf[1024];
			while( fgets(buf,1024,f) )
			{
				char* temp1 = strtok( buf,  " \n,;" );
				char* temp2 = strtok( NULL, " \n,;" );
				if( temp1 && temp2 )
				{
					String k = String( ANSI_TO_UNICODE( temp1 ) );
					String v = String( ANSI_TO_UNICODE( temp2 ) );
					data[k].Insert( CName(v) );
				}
			}
			fclose( f );
		}
		GFeedback->BeginTask( TXT("Removing unused animations." ), false );
		const TDynArray<String> & files = GetActiveResources();
		const Int32 numf = files.Size();
		Int32 i;
		for( i=0;i<numf;++i )
		{
			Int32 removed = 0;
			THashMap<String,THashSet<CName>>::iterator itarr = data.Find( files[i] );
			if( itarr==data.End() ){ continue; }
			CDiskFile* fil = GDepot->FindFile( files[i] );
			if( !fil ){ continue; }
			Bool wasLoaded = fil->IsLoaded(); 
			fil->Load();
			CResource* res = fil->GetResource();
			if( !res ){ continue; }
			CSkeletalAnimationSet* animSet = Cast< CSkeletalAnimationSet >( res );
			if( !animSet ){ continue; }
			THashSet<CName> & names = (*itarr).m_second;
			const Int32 numa = animSet->GetAnimations().Size();
			Int32 j;
			for( j=numa-1;j>=0;j-- )
			{
				CSkeletalAnimation* anim = animSet->GetAnimations()[j]->GetAnimation();
				CName animName = anim->GetName();
				THashSet<CName>::iterator it = names.Find( animName );
				if( keep_list_anims )
				{
					if( it==names.End() )
					{
						animSet->RemoveAnimation( anim );
						removed++;
					}
				}
				else
				{
					if( it!=names.End() )
					{
						animSet->RemoveAnimation( anim );
						removed++;
					}
				}
			}
			animSet->Save();
			if(!wasLoaded) fil->Unload();
			{
				char buf[256];
				sprintf( buf, "Removed %d animations", removed );
				wxMessageBox( buf, files[i].AsChar() );
			}
		}
		GFeedback->EndTask();
		OnRefreshTab( event );
	}
}

void CEdAssetBrowser::OnDumpAnimationNames( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString;
	String wildCard = TXT("All files (*.*)|*.*|TXT files (*.txt)|*.txt");
	wxFileDialog saveFileDialog( this, wxT("Save txt"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		GFeedback->BeginTask( TXT("Dump animations in animset." ), false );
		String loadPath = saveFileDialog.GetPath().wc_str();
		FILE* f = fopen( UNICODE_TO_ANSI( loadPath.AsChar() ), "w+" );
		if( f )
		{
			const TDynArray<String> & files = GetActiveResources();
			const Int32 numf = files.Size();
			Int32 i;
			for( i=0;i<numf;++i )
			{
				CDiskFile* fil = GDepot->FindFile( files[i] );
				if( !fil ){ continue; }
				Bool wasLoaded = fil->IsLoaded(); 
				fil->Load();
				CResource* res = fil->GetResource();
				if( !res ){ continue; }
				CSkeletalAnimationSet* animSet = Cast< CSkeletalAnimationSet >( res );
				if( !animSet ){ continue; }
				const Int32 numa = animSet->GetAnimations().Size();
				Int32 j;
				for( j=numa-1;j>=0;j-- )
				{
					CSkeletalAnimation* anim = animSet->GetAnimations()[j]->GetAnimation();
					fprintf( f, "%s, %s\n", UNICODE_TO_ANSI( files[i].AsChar() ), anim->GetName().AsAnsiChar() );
				}
				if(!wasLoaded) fil->Unload();
			}
			fclose( f );
		}
		GFeedback->EndTask();
	}
	OnRefreshTab( event );
}


void CEdAssetBrowser::OnCalculateTexelDensity( wxCommandEvent& event )
{	
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}	
	
	TDynArray< CDiskFile* > diskFiles;
	diskFiles.ClearFast();

	const Uint32 size = contextMenuDir->GetDirs().Size();
	CDirectory* dir;

	GFeedback->BeginTask( TXT("Calculating Texel Densities" ), false );

	CResource::FactoryInfo< C2dArray > info;
	C2dArray* debugDumpInfo = info.CreateResource();
	debugDumpInfo->AddColumn( TXT("Mesh depot path"), TXT("") );
	debugDumpInfo->AddColumn( TXT("Texture depot path"), TXT("") );
	debugDumpInfo->AddColumn( TXT("Texture Size"), TXT("") );
	debugDumpInfo->AddColumn( TXT("Area covered"), TXT("") );
	debugDumpInfo->AddColumn( TXT("Texel factor"), TXT("") );	

	const Float targetTexelDensitySqr = 400.0f*400.0f;

	Int32 ind = 0;
	char buf[256];

	for( Uint32 d=0; d<size; d++ )
	{
		dir = contextMenuDir->GetDirs()[d];
		GetAllResourceFilesInDirectory< CMesh>( dir, diskFiles );

		const Uint32 diskFileSize = diskFiles.Size();

		if( diskFileSize > 0 )
		{
			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				Bool wasLoaded = diskFiles[ f ]->IsLoaded();

				if( diskFiles[ f ]->Load() )
				{
					if ( CMesh* meshRes = Cast< CMesh >( diskFiles[ f ]->GetResource() ) )
					{
						const auto& chunks = meshRes->GetChunks();
						
						TDynArray<Float> summedAreaPerTexture;
						TDynArray<CBitmapTexture*> meshTextures;

						for ( Uint32 i=0; i< chunks.Size(); i++ )
						{
							CMeshTypeResource::TMaterials& mats =  meshRes->GetMaterials();
							Float ar = meshRes->GetChunkSurfaceArea( i );
							Int32 mid = chunks[i].m_materialID;

							Uint32 currentTexIndex = 0;
							{
								IMaterial* resourceMaterial = Cast< IMaterial > ( mats[mid] );
								if (resourceMaterial)
								{
									CMaterialInstance* instance = Cast< CMaterialInstance >( resourceMaterial );
									if (instance)
									{
										TMaterialParameters params = instance->GetParameters();
										const Int32 nump = params.Size();
										for( Int32 j=0;j<nump;++j )
										{
											THandle<ITexture> map = 0;
											// type check is broken due to material instance not storing rtti type 
											if( params[j].GetName() == TXT("Diffuse") || params[j].GetName() == TXT("Normal") )
											{
												instance->ReadParameter( params[j].GetName(), map );
												CBitmapTexture* tex = Cast< CBitmapTexture >( map );
												if( tex )
												{													
													Bool newTextureAdded = true;
													for( Uint32 t=0; t<meshTextures.Size(); ++t )
													{
														if( meshTextures[t]->GetImportFile() == tex->GetImportFile() ) 
														{
															currentTexIndex = t;
															newTextureAdded = false;
															break;
														}
													}

													if( newTextureAdded ) 
													{ 
														meshTextures.PushBack( tex );
														currentTexIndex = meshTextures.Size()-1;
														summedAreaPerTexture.PushBack( 0.0f );
													}
													
													summedAreaPerTexture[ currentTexIndex ] += ar;

												}
											}											
										}
									}
								}
							}
						}

						for( Uint32 t=0; t<meshTextures.Size(); ++t )
						{
							Int32 w = meshTextures[t]->GetWidth();
							Int32 h = meshTextures[t]->GetHeight();
							CDiskFile*  pat = meshTextures[t]->GetFile();
							
							Float factor = 0.0f;
							Float numpixels = Float(w)*Float(h);
							factor = numpixels / summedAreaPerTexture[t];
														
							TDynArray< String > rowData;
							rowData.PushBack( (diskFiles[ f ]->GetAbsolutePath()) );
							rowData.PushBack( (pat->GetAbsolutePath()) );
							
							sprintf( buf, "%d x %d", w, h );
							rowData.PushBack( ANSI_TO_UNICODE(buf) );
							
							sprintf( buf, "%.3f", summedAreaPerTexture[t] );
							rowData.PushBack( ANSI_TO_UNICODE(buf) );							

							sprintf( buf, "%.3f", factor/targetTexelDensitySqr );
							rowData.PushBack( ANSI_TO_UNICODE(buf));

							debugDumpInfo->AddRow( rowData );
						}

						meshTextures.ClearFast();
						summedAreaPerTexture.ClearFast();
					}
				}

				if ( !wasLoaded )
				{
					diskFiles[ f ]->Unload();
				}
				GFeedback->UpdateTaskProgress( f, diskFileSize );
			}
		}
	}

	//fclose( fil );
	if( contextMenuDir->GetDirs().Size()>0 )
	{
		CDirectory* savePath = dir->GetParent() ;
		String temp;
		savePath->GetDepotPath( temp );
		temp = String::Printf( TXT("%stexel_area_factors"), temp.AsChar() );

		if ( CDiskFile *diskFile = GDepot->FindFile( temp ) )
		{
			String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), temp.AsChar() );
			if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
			{
				return;
			}
		}
		debugDumpInfo->SaveAs( savePath, CFilePath( temp ).GetFileName(), true );
		ShellExecute( NULL, TXT("explore"),
			savePath->GetAbsolutePath().AsChar(),
			NULL, NULL, SW_SHOWNORMAL);
		debugDumpInfo->Discard();
	}
	GFeedback->EndTask();
	OnRefreshTab( event );
}

void CEdAssetBrowser::OnAssingMaterialToMeshes( wxCommandEvent& event )
{
	CContextMenuDir* contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( !contextMenuDir->Empty() )
	{
		CEdMeshMaterialRemapper remapper( this );
		if ( !contextMenuDir->GetDirs().Empty() )
		{
			remapper.Execute( contextMenuDir->GetDirs() );
		}
		else
		{
			remapper.Execute( contextMenuDir->GetFiles() );
		}
	}
}

void CEdAssetBrowser::OnValidateAnimations( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	if( contextMenuDir->GetDirs().Size()>0 )
	{
		CAnimationValidator validator;
		Uint32 numProblemsFound = 0;
		validator.Execute( contextMenuDir->GetDirs(), numProblemsFound );

		if( numProblemsFound > 0 )
		{
			CDirectory* savePath = contextMenuDir->GetDirs()[0];			
			validator.DumpErrorsToFile( TXT("animationCheckLog.csv"), savePath );

			GFeedback->ShowMsg( TXT("Animation Validator says:"), TXT("Found %d problems..."), numProblemsFound );

			ShellExecute( NULL, TXT("explore"), savePath->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );			
		}
		else
		{
			GFeedback->ShowMsg( TXT("Animation Validator says:"), TXT("Yay! No problems detected.") );
		}

		SGarbageCollector::GetInstance().CollectNow();		
	}
}

void CEdAssetBrowser::OnFixSourceAnimData( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	if( contextMenuDir->GetDirs().Size()>0 )
	{
		CAnimationValidator validator( true );
		Uint32 numProblemsFound = 0;
		validator.Execute( contextMenuDir->GetDirs(), numProblemsFound );

		if( numProblemsFound > 0 )
		{
			CDirectory* savePath = contextMenuDir->GetDirs()[0];
			validator.DumpErrorsToFile( TXT("animationCheckLog.csv"), savePath );

			GFeedback->ShowMsg( TXT("Animation Validator says:"), TXT("Fixed %d problems..."), numProblemsFound );

			ShellExecute( NULL, TXT("explore"), savePath->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );
		}
		else
			GFeedback->ShowMsg( TXT("Animation Validator says:"), TXT("No need to fix - 0 problems detected...") );

		SGarbageCollector::GetInstance().CollectNow();		
	}
}

void CEdAssetBrowser::OnViewAnimSetReport( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	CEdAnimSetReportDlg* dlg = new CEdAnimSetReportDlg( this, contextMenuDir->GetDirs() );
	dlg->Show();
	dlg->Raise();
}

void CEdAssetBrowser::OnDumpDialogAnimationNames( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString;
	String wildCard = TXT("All files (*.*)|*.*|TXT files (*.txt)|*.txt");
	wxFileDialog saveFileDialog( this, wxT("Save txt"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = saveFileDialog.GetPath().wc_str();
		FILE* f = fopen( UNICODE_TO_ANSI( loadPath.AsChar() ), "w+" );
		if( f )
		{
			CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

			for ( CResourceIteratorAdapter< CStoryScene > scene( *contextMenuDir, TXT("Dumping Dialogs Animations" ), RIF_ReadOnly ); scene; ++scene )
			{
				const String & str = scene->GetFile()->GetFileName();
				fprintf( f, "%s\n", UNICODE_TO_ANSI( str.AsChar() ) );

				const Int32 numsecs = scene->GetNumberOfSections();
				Int32 i;
				Int32 j;
				for( i=0;i<numsecs;++i )
				{
					CStorySceneSection* sec = scene->GetSection( i );
					const TDynArray< CStorySceneEvent* > events = sec->GetEventsFromAllVariants();
					Int32 numel = events.Size();
					for( j=0;j<numel;++j )
					{
						CStorySceneEvent* el = events[j];
						if( el->GetClass()->IsA< CStorySceneEventAnimation >() )
						{
							CStorySceneEventAnimClip* animEvt = static_cast< CStorySceneEventAnimClip* >( el );
							String animname = animEvt->GetAnimationName().AsString();
							fprintf( f, "\t%s\n", UNICODE_TO_ANSI( animname.AsChar() ) );
						}
						if( el->GetClass()->IsA< CStorySceneEventMimicsAnim >() )
						{
							CStorySceneEventMimicsAnim* animEvt = static_cast< CStorySceneEventMimicsAnim* >( el );
							String animname = animEvt->GetAnimationName().AsString();
							fprintf( f, "\t%s\n", UNICODE_TO_ANSI( animname.AsChar() ) );
						}
					}
				}
			}

			OnRefreshTab( event );
			fclose(f);
		}
	}
}
void CEdAssetBrowser::OnDumpEntitiesDepCount(wxCommandEvent& event)
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	if( !contextMenuDir->GetDirs().Empty() )
	{
		TDynArray< CDiskFile* > diskFiles;
		diskFiles.ClearFast();

		const Uint32 size = contextMenuDir->GetDirs().Size();
		CDirectory* dir;

		GFeedback->BeginTask( TXT("Collecting entities dependecies" ), false );

		// gathering meshes
		for( Uint32 i=0; i<size; i++ )
		{
			dir = contextMenuDir->GetDirs()[i];
			GetAllResourceFilesInDirectory< CEntityTemplate >( dir, diskFiles );
			String dirName;
			dirName = dir->GetName();

			const Uint32 diskFileSize = diskFiles.Size();

			if( diskFileSize > 0 )
			{
				String infoToSave;
				infoToSave += TXT("Entity template;");
				infoToSave += TXT("Dependencies count;");
				infoToSave += TXT("\n");
				for( Uint32 f = 0; f < diskFileSize; ++f )
				{
					if( diskFiles[ f ]->Load() )
					{
						if ( CEntityTemplate* entTemplate = Cast< CEntityTemplate >( diskFiles[ f ]->GetResource() ) )
						{
							TDynArray< CResource::DependencyInfo > temp;
							temp.ClearFast();
							entTemplate->GetDependentResourcesPaths( temp, TDynArray< String >() );
							Uint32 depSize = temp.Size();

							infoToSave += entTemplate->GetDepotPath();
							infoToSave += TXT(";");
							infoToSave += ToString(depSize);
							infoToSave += TXT(";");
							infoToSave += TXT("\n");
							for (Uint32 i = 0; i < depSize; ++i)
							{
								infoToSave += temp[i].m_path;
								infoToSave += TXT(";");
								infoToSave += TXT("\n");
							}
							//2 new lines intentionally left
							infoToSave += TXT("\n");
							infoToSave += TXT("\n");
						}
					}
					GFeedback->UpdateTaskProgress( f, diskFileSize );
				
					// Process GC once a while
					if ( ( f % 3 ) == 0 )
					{
						SGarbageCollector::GetInstance().CollectNow();
					}
				}

				CDirectory* savePath = contextMenuDir->GetDirs()[i]->GetParent();
				String temp;
				savePath->GetAbsolutePath( temp );
				String ext = TXT(".csv");

				temp += dirName;
				temp += ext;

				GFileManager->SaveStringToFile( temp, infoToSave );
			}
		}

		GFeedback->EndTask();

		OnRefreshTab( event );

		::ShellExecute( NULL, TXT("explore"), dir->GetParent()->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );
	}
}



void CEdAssetBrowser::OnSetMeshProperties( wxCommandEvent& event )
{	
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	Bool use1 = false;
	Bool use2 = false;
	Bool use3 = false;
	Bool use4 = false;
	Bool use5 = false;
	Bool use6 = false;
	Bool use7 = false;
	Bool use8 = false;
	Bool use9 = false;

	Bool useextrastreams = false;
	String soundtyp = TXT("fff");
	String soundsiz = TXT("fff");
	Float  dissolve = 0.25f;
	Int32 lodBias = 1;
	Bool forcnoshadowlod = false;
	Bool isoccluder = false;
	Bool isstatic = false;
	Bool entityproxy = false;

	Int32 buttonSelected = 0;

	buttonSelected = FormattedDialogBox( this,
		wxT("Left tickbox marks properties to change"),
		wxT("H{X|'       Use Extra Stream: '|X}H{X|'       Sound Type Identification: '|S}H{X|'       Sound Size Identification: '|S}H{X|'       LOD Dissolve Transition Time: '|F}H{X|'       Shadow LOD Bias: '|I}H{X|'       Force No Shadow LOD: '|X}H{X|'       Is Occluder: '|X}H{X|'       Is Static: '|X}H{X|'       Entity Proxy: '|X}H{~B'OK'~}"),

		&use1,
		&useextrastreams,

		&use2,
		&soundtyp,

		&use3,
		&soundsiz,

		&use4,
		&dissolve,

		&use5,
		&lodBias,

		&use6,
		&forcnoshadowlod,

		&use7,
		&isoccluder,

		&use8,
		&isstatic,

		&use9,
		&entityproxy
		);

	if( use1 || use2 || use3 || use4 || use5 || use6 || use7 || use8 || use9 )
	{
		for ( CResourceIteratorAdapter< CMesh > meshRes( *contextMenuDir, TXT("Mesh properties batcher..." ) ); meshRes; ++meshRes )
		{		
			if( use1 ){ meshRes->SetUseExtractStreams( useextrastreams ); }
			if( use2 )
			{
				SMeshSoundInfo* msi = meshRes->GetMeshSoundInfo();
				if( msi )
				{
					msi->m_soundTypeIdentification = CName(soundtyp); 
				}
			}
			if( use3 )
			{ 
				SMeshSoundInfo* msi = meshRes->GetMeshSoundInfo();
				if( msi )
				{
					msi->m_soundSizeIdentification = CName(soundsiz); 
				}
			}
			//if( use5 ){ meshRes->SetShadowLodBias( lodBias ); } // not used any more
			//if( use6 ){ meshRes->SetForceNoShadowLOD( forcnoshadowlod ); } // not used any more
			if( use7 ){ meshRes->SetIsOccluder( isoccluder ); }
			if( use8 ){ meshRes->SetIsStatic( isstatic ); }
			if( use9 ){ meshRes->SetEntityProxy( entityproxy ); }	
		}	

		OnRefreshTab( event );
	}	
	else 
		GFeedback->ShowMsg( TXT("Batch set mesh properties"), TXT("Nothing changed!") );
}

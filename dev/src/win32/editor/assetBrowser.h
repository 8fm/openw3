/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Shit
class CEdResourceView;
class CEdAssetBrowser;
class CInventoryReport;
struct SDataError;

#include "importDlg.h"
#include "resourceIterator.h"
#include "../../common/core/directory.h"
#include "multiValueBitmapButtonControl.h"

class CDynaPath : public wxPanel
{
	DECLARE_EVENT_TABLE();
public:
	CDynaPath( wxWindow* parent, CEdAssetBrowser* owner );
protected:
	CEdAssetBrowser*				m_owner;
	wxBitmap						m_bitmapRoot;
	wxBitmap						m_bitmapOpened;
	wxBitmap						m_bitmapClosed;
	wxBitmap						m_bitmapArrow;
	wxTextCtrl*						m_textEdit;

	THashMap< CDirectory*, wxRect >	m_rectDir;
	CDirectory*						m_hover;
	Int32							m_hoverPart;
	Bool							m_hoverInset;
	CEdTimer*						m_timer;
	Bool							m_gotFontHeight;
	wxFont							m_font;
	Int32							m_menu;
	CDirectory*						m_menuDirectory;
protected:
	void OnPaint( wxPaintEvent &event );
	void OnEraseBackground( wxEraseEvent &event );
	void OnMouse( wxMouseEvent &event );
	void OnTimeout( wxCommandEvent &event );
	void OnSubfolder( wxCommandEvent &event );
	void OnTextEditKillFocus( wxFocusEvent& event );
	void OnTextEditEnter( wxCommandEvent& event );
private:
	void GetSortedDirectories( TSortedArray< String >& directories );
};

//! Carries around a set of files or dirs (but not both). Meant to be passed as an event user object, hence wxObject
class CContextMenuDir : public wxObject
{
public:
	CContextMenuDir() {}

	CContextMenuDir( const TDynArray< CDirectory* >& dirs ) 
		: m_directories( dirs )
	{};

	CContextMenuDir( const TDynArray< CDiskFile* >& files ) 
		: m_files( files )
	{};

	Bool Empty() const
	{
		return m_directories.Empty() && m_files.Empty();
	}

	const TDynArray< CDirectory* >& GetDirs() const 
	{ 
		return m_directories;
	}
	
	const TDynArray< CDiskFile* >& GetFiles() const
	{
		return m_files;
	}

private:
	TDynArray< CDirectory* > m_directories;
	TDynArray< CDiskFile* >  m_files;
};

//! Resource iterator adapted to take CContextMenuDir (using either files or dirs)
template < typename ResourceT >
class CResourceIteratorAdapter
{
public:
	CResourceIteratorAdapter( const CContextMenuDir& dir, const String& taskName = String::EMPTY, EResourceIteratorFlags flags = RIF_Default )
	{
		if ( !dir.GetDirs().Empty() )
		{
			m_it.Reset( new CResourceIterator< ResourceT >( dir.GetDirs(), taskName, flags ) );
		}
		else
		{
			m_it.Reset( new CResourceIterator< ResourceT >( dir.GetFiles(), taskName, flags ) );
		}
	}

	operator Bool() const { return m_it->operator Bool(); }
	ResourceT& operator * () { return m_it->operator *(); }
	ResourceT* operator -> () { return m_it->operator ->(); }
	ResourceT* Get() { return m_it->Get(); }
	void operator ++ () { m_it->operator ++(); }

private:
	Red::TUniquePtr< CResourceIterator< ResourceT > > m_it;
};


class CWxAuiNotebook : public wxAuiNotebook
{
public:
	CWxAuiNotebook( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxAUI_NB_DEFAULT_STYLE );
	void GetOrder( TDynArray< wxWindow* > &target );
};

// Data holder for CDirectory
class wxTreeItemDataDir : public wxTreeItemData
{
private:
	CDirectory*		m_directory;
	String			m_depotPath;

public:
	wxTreeItemDataDir( CDirectory* dir, String depotPath ) 
		: m_directory( dir )
		, m_depotPath( depotPath )
	{};

	// Get the directory for this tree item.  This will build the directory
	// path for this branch if the directory isn't set.
	CDirectory* GetDirectory();

	// Get the directory for this tree item, if there is any set.
	RED_INLINE CDirectory* PeekDirectory() const { return m_directory; }

	// Get the depot path of the directory
	RED_INLINE const String& GetDepotPath() const { return m_depotPath; }
};

/// Asset browser
class CEdAssetBrowser : public wxSmartLayoutPanel, public IEdEventListener, public IDirectoryChangeListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	DECLARE_EVENT_TABLE();
	friend class CEdResourceView;
	friend class CEdCurrentResourceView;
	friend class CEdCheckedOutResourceView;
	friend class CDynaPath;
	friend class wxTreeItemDataDir;

protected:
	// ids for tree view images
	Int32					m_imgDepotId;
	Int32					m_imgClosedId;
	Int32					m_imgOpenId;
	Int32					m_imgChangedId;

protected:
	//CDirectory*			m_activeDirectory;

	enum ETabType
	{
		ETT_Folder,
		ETT_CheckedOut,
		ETT_RecentFiles,
	};

	class CEdAssetBrowserTab
	{
	public:
		CDirectory*			m_directory;
		CEdResourceView*	m_view;
		Bool				m_flat;
		Bool				m_lock;
		String				m_searchPhrase;
		const CClass*		m_searchFilter;
		ETabType			m_type;
		Int32					m_order;

		CEdAssetBrowserTab()
			: m_directory( NULL )
			, m_view( NULL )
			, m_flat( false )
			, m_lock( false )
			, m_searchPhrase()
			, m_searchFilter( NULL )
			, m_type( ETT_Folder )
			, m_order( 0 )
		{
		}
	};

	wxTreeCtrl*									m_depotTree;
	CDropTarget*								m_depotTreeDropTarget;
	CWxAuiNotebook*								m_noteBook;
	wxSplitterWindow*							m_splitter;
	THashMap< wxWindow*, CEdAssetBrowserTab >	m_tabs;
	String										m_searchPhrase;
	CDirectory*									m_searchDirectory;
	wxChoice*									m_classFilterBox;
	CEdFileDialog								m_importDlg;
	String										m_importPath;
	wxSearchCtrl*								m_search;
	CEdMultiValueBitmapButtonControl< Bool >	m_searchInCurrentDir;
	CDynaPath*									m_dynaPath;
	CEdTimer									m_timer;
	Int32										m_dragBegin;
	Bool										m_lockTree;
	Bool										m_savePerSession;
	TDynArray< String >							m_favClasses;
	Bool										m_autoUpdateIcons;

	THashMap< THandle< CResource >, wxWindow* > m_resourceEditors;

protected:
	Bool									m_scanningDepot;
	THashMap< CDirectory*, wxTreeItemId >		m_dirMapping;
	TSortedSet< String >					m_allowedExtensions;
	TDynArray< CClass* >					m_resourceClasses;
	TSortedSet< CDirectory* >				m_addedDirectories;
    
    TEdShortcutArray                        m_shortcuts;
	wxBitmap								m_lockedTabBitmap;

	TDynArray< CDiskFile* >					m_recentFiles;
	TDynArray< String >						m_skippedDirs;
	MeshMaterialAutoAssignmentConfig		m_materialAutoBindConfig;
	EImportCreateReturn						m_materialAutoBindResult;

	TDynArray< String >						m_bookmarks;

	CDirectoryWatcher*						m_sourceAssetDirWatcher;
	Bool									m_repopulateDirOnActivation;

public:
	CEdAssetBrowser( wxWindow* parent );
	~CEdAssetBrowser();

	void SelectFile( const String& filePath );
	void SelectAndOpenFile( const String& filePath );
	void OpenFile( const String& filePath );
	void CreateDirectoryContextMenu( wxMenu &menu, const TDynArray< CDirectory* > &dirs );
	void CreateBatchersContextMenu( wxMenu &menu, const TDynArray< CDirectory* >& dirs, const TDynArray< CDiskFile* >& files );

    TEdShortcutArray *GetAccelerators();

	void UpdateResourceList( Bool repopulate = false );
	void UpdateDepotTree();

	void OnEditorReload( CResource* res, wxWindow* editor );

	Bool DoesDirExist( CDirectory* directory ) const;


protected:
	// connects to child objects
	void SetConnections();
	
	CDirectory*	GetActiveDirectory();
	void UpdateClassFilter();
	void UpdateResourceList( CEdAssetBrowserTab &tab );
	void UpdateImportClasses();
	Bool SelectDirectory( CDirectory* directory, Bool allowNewTab = true );
	CDirectory* GetCurrentDirectory();
	Bool FillDirectory( CDirectory* dir, wxTreeItemId item );
	Bool CanShowFile( CDiskFile* so, Bool inSearchMode = false );
	Bool SelectResource( CResource* resource );
	void ImportResources( CClass* resourceClass );
	
	Bool BatchReimportTextureArrays( CTextureArray* texArray );
	void CreateResourceFromClass( CClass* resoruceClass );
	void CreateResourceFromFactory( IFactory* factory );
	void ExportResource( CResource* resource );
	void HandleMissingDependencies( CResource* res ) const;

public:
	Bool ReimportResource( CResource* resource );
	// refresh icons for all visible directories
	void RefreshIcons();
	
	// refresh icon for a given directory
	void RefreshIcon( CDirectory *dir, const wxTreeItemId id );
	
	// shows search results resource view in the notebook
	void ShowSearchResults( const String &phrase );

	// finds all entities of script class type
	void SearchEntityClass( wxCommandEvent& event, CClass* searchClass );

	//finds all entities (npc's) having the specified AI attitude
	void SearchAttitudeGroup( wxCommandEvent& event, CName searchGroup );

	//finds all entities that have/haven't the chosen component
	void SearchComponents( wxCommandEvent& event, const CClass* searchComponent, Bool presence );

	//finds all behavior trees containing the chosen node
	void SearchBehTrees( wxCommandEvent& event, CClass* searchClass );

	//finds all NPC's of the specified level
	void SearchNPCLevel( wxCommandEvent& event, CClass* searchClass );

	//this is used by searching for entity classes, attitude groups, and components
	void DisplaySpecialSearchResults( TDynArray< CDiskFile* > relevantFiles, String searchPhrase );

public:
	// refresh directory
	void RepopulateDirectory();

	void RefreshCurrentView();
public:
	// Save / Load options from config file
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();
	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );
	void SaveSessionInfo( CConfigurationManager &config );
	void RestoreSessionInfo( CConfigurationManager &config );
public:
	wxWindow* EditAsset( CResource* res, TDynArray< SDataError >* errorArray = nullptr );
	Bool IsResourceOpenedInEditor( CResource* res );
	Bool IsQuestPhaseOpenedInEditor( CResource* res, CQuestPhase* phase );
	Bool IsEntityTemplateEditorOpen() const;

protected:
	void OnShow( wxShowEvent& event );
	void OnActivate( wxActivateEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnDirectoryOpened( wxTreeEvent& event );
	void OnDirectoryChanged( wxTreeEvent& event );
	void OnDirectoryContextMenu( wxTreeEvent& event );
	void OnClassFilterChanged( wxCommandEvent& event );
	void OnCreateDirectory( wxCommandEvent& event );
	void OnRescanDirectory( wxCommandEvent& event );
	void OnTabChange( wxAuiNotebookEvent &event);
	void OnTabDragDone( wxAuiNotebookEvent &event);
	void OnTabDragBegin( wxAuiNotebookEvent &event);
	void OnTabClose( wxAuiNotebookEvent &event );
	void OnTabClose( wxCommandEvent &event );
	void OnTabContextMenu( wxAuiNotebookEvent &event );
	void OnTabHeaderDblClick( wxAuiNotebookEvent &event );
	void OnContentChange( wxCommandEvent &event );
	void OnSearch( wxCommandEvent &event );
	void OnSearchEntityClass( wxCommandEvent& event );
	void OnSearchAttitudeGroup( wxCommandEvent& event );
	void OnSearchComponentAbsence( wxCommandEvent& event );
	void OnSearchComponentPresence( wxCommandEvent& event );
	void OnSearchBehTreeScriptTask( wxCommandEvent& event );
	void OnSearchNPCLevel( wxCommandEvent& event );

	void OnSyncDirectory( wxCommandEvent &event );
	void OnCheckOutDirectory( wxCommandEvent &event );
	void OnSubmitDirectory( wxCommandEvent &event );
	void OnRevertDirectory( wxCommandEvent &event );
    void OnOpenDirectoryInExplorer( wxCommandEvent &event );
	void OnAddDirectory( wxCommandEvent &event );
	void OnNewTabDirectory( wxCommandEvent &event );
	wxPanel* AddTab( CDirectory *dir, ETabType type=ETT_Folder,  String searchPhrase = String::EMPTY, const CClass * searchFilter = NULL );
	void OnTreeButton( wxCommandEvent &event );
	void OnNewFolderButton( wxCommandEvent &event );
	void OnAddTabButton( wxCommandEvent &event );
	void OnShowInDirectoriesTree( wxCommandEvent& event );
	void OnRefreshTab( wxCommandEvent &event );
	void OnFlatDirectory( wxCommandEvent &event );
	void OnLockTab( wxCommandEvent &event );
	void OnSavePerSessionButton( wxCommandEvent &event );
	void OnEditCopyPath( wxCommandEvent &event );
	void OnEditPastePath( wxCommandEvent &event );
	void OnToggleBookmark( wxCommandEvent &event );
	void OnBookmark( wxCommandEvent &event );
	void OnChangeView( wxCommandEvent &event );
	void OnCheckedOutButton( wxCommandEvent &event );
	void OnRecentFilesButton( wxCommandEvent &event );
	void OnGenerateItemsIcons( wxCommandEvent &event );
	void OnConvertAISenses( wxCommandEvent &event );
	void OnResaveResources( wxCommandEvent& event );
	void OnResaveEntityTemplates( wxCommandEvent& event );
	void OnCreateInventoriesReport( wxCommandEvent& event );
	void OnSetAutoHideDistance( wxCommandEvent& event );
	void OnSetDownscaleBias( wxCommandEvent& event );
	void OnExtractSourceTexture( wxCommandEvent& event );

	void OnGenerateLODs( wxCommandEvent& event );

	void OnChangeLightChannels( wxCommandEvent& event );
	void OnChangeDrawableFlags( wxCommandEvent& event );
	void OnChangeMaterialFlags( wxCommandEvent& event );
	void OnSetStreamingLODs( wxCommandEvent& event );
	void OnSimplifyMaterials( wxCommandEvent& event );
	void OnBatchTextureGroupChange( wxCommandEvent& event );
	void OnCalculateAppearancesTextureCost( wxCommandEvent& event );
	void OnCalculateComponentsInEntities( wxCommandEvent& event );
	void OnRemoveForcedAppearances( wxCommandEvent& event );
	void OnSetMeshProperties( wxCommandEvent& event );

	void OnDumpEntitiesDepCount( wxCommandEvent& event );
	void OnDumpDebugInfoMeshes( wxCommandEvent& event );
	void OnDumpDebugInfoEntityTemplates( wxCommandEvent& event );
	void OnDumpDebugInfoEntityTemplatesEffects( wxCommandEvent& event );
	void OnRemoveUnusedMaterials( wxCommandEvent& event );
	void OnCalculateShadowPriorities( wxCommandEvent& event );
	void OnRemoveUnusedAnimations( wxCommandEvent& event );
	void OnDumpAnimationNames( wxCommandEvent& event );
	void OnDumpDialogAnimationNames( wxCommandEvent& event );
	void OnValidateAnimations( wxCommandEvent& event );
	void OnFixSourceAnimData( wxCommandEvent& event );
	void OnViewAnimSetReport( wxCommandEvent& event );
	void OnCalculateTexelDensity( wxCommandEvent& event );
	void OnAssingMaterialToMeshes( wxCommandEvent& event );

	void OnDeleteAsset( wxCommandEvent &event );
	void OnCopyAsset( wxCommandEvent &event );
	void OnCutAsset( wxCommandEvent &event );
	void OnPasteAsset( wxCommandEvent &event );
	void OnPasteAsAsset( wxCommandEvent &event );
	void OnRenameAsset( wxCommandEvent &event );
	void OnUsePathFromClipboard( wxCommandEvent &event );

	void SetTreeVisible( Bool visible, Int32 sash = -1 );
	void OnSplitterUnsplit( wxSplitterEvent &event );
	void OnSplitterDoubleClick( wxSplitterEvent &event );
	void NextTab();
	//void OnTabKeyDown( wxKeyEvent& event );

	Bool FillDirectory( CDirectory *directory, TDynArray< CDiskFile* > &allFiles );
	Bool IsCurrentTabFlat();
	void OnTreeKey( wxKeyEvent& event );
	void OnSearch();
	void SetTabTitle( Int32 idx );

	void OnAfterImport( IImporter* importer, CResource *imported );

	wxWindow* GetCurrentPage();
	Bool IsCurrentPageSearchType();
	void CloseAllEditors();
	void OnEditorClosed( wxCloseEvent& event );
	void OnEditorDestroyed( wxWindowDestroyEvent& event );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void UpdateBookmarksMenuNow();
	void UpdateBookmarksMenu();
	void UpdateBookmarkButton();

	// Force texture group setting
	bool UpdateTextureGroup( CDiskFile * inFile, const TextureGroup & inGroup );

	// Methods for batch texture group setting
	bool SetDiffuseTextureGroup(CDiskFile * inFile, const TextureGroup & inGroup);
	bool SetNormalTextureGroup(CDiskFile * inFile, const TextureGroup & inGroup);
	bool SetTextureGroup(CDirectory * inDir, const TextureGroup & inGroup, AnsiChar inType);

	void ResaveResourcesInDirectory( CDirectory* dir, const String& ext );
	void ResaveEntityTemplatesInDirectory( CDirectory* dir, TDynArray< CDiskFile* >& saved, TDynArray< CDiskFile* >& failed );
	
	void AddInventoriesReportFromDirectory( CDirectory* dir, CInventoryReport* report );

	void onResfDialogImport( wxCommandEvent& event );
	void onResfDialogCancel( wxCommandEvent& event );

	virtual void OnDirectoryChange( const TDynArray< ChangedFileData >& changes ) override;
	void HandleSourceFileChange( const ChangedFileData& change );
	void HandleResourceChange( const ChangedFileData& change );

	THandle< CResource > m_resToRestartEditorWith;
};

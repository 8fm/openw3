/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/dataError.h"

class CEdAssetBrowser;
class CThumbnailData;
class CVersionControlIconsPainter;
class CDiskFile;
struct SThumbnailSettings;
enum EEditorResourceViewMode
{
	ERVM_Big = 0,
	ERVM_Small = 1,
	ERVM_List = 2,
};

DECLARE_EVENT_TYPE(wxEVT_CHECK_OUT_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_SUBMIT_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_REVERT_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_ADD_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_DELETE_EVENT, -1)
DECLARE_EVENT_TYPE(wxEVT_SYNC_EVENT, -1)


/// Resource view panel
class CEdResourceView	: public CEdCanvas
{
	DECLARE_EVENT_TABLE();

protected:
	/// Info about disk file
	struct DiskFileInfo
	{
		Uint32				m_index;
		CDiskFile*			m_file;
		CClass*				m_class;
		Uint32              m_thumbnailIdx;
		String				m_caption;
		TDynArray< String >	m_info;
		wxPoint				m_offset;
		wxPoint				m_size;
		wxPoint				m_total;
		wxPoint				m_text;
		Bool				m_isDirectory;
		CDirectory*			m_directory;

		RED_INLINE DiskFileInfo()
			: m_index( 0 )
			, m_file( NULL )
			, m_thumbnailIdx( 0 )
			, m_class( NULL )
			, m_isDirectory( false )
			, m_directory( NULL )
		{};

		RED_INLINE DiskFileInfo( Uint32 _index, CDiskFile* file, CClass* resourceClass, const String& caption )
			: m_index( _index )
			, m_file( file )
			, m_caption( caption )
			, m_class( resourceClass )
			, m_isDirectory( false )
			, m_directory( NULL )
		{};

		RED_INLINE DiskFileInfo( Uint32 _index, CDirectory* dir, const String& caption )
			: m_index( _index )
			, m_file( NULL )
			, m_caption( caption )
			, m_class( NULL )
			, m_isDirectory( true )
			, m_directory( dir )
		{};

		// Get item rectangle
		RED_INLINE wxRect GetRect() const { return wxRect( m_offset.x, m_offset.y, m_total.x, m_total.y ); }
		
		static Bool Compare( const DiskFileInfo& p1, const DiskFileInfo& p2 )
		{
			if ( p1.m_isDirectory == p2.m_isDirectory )
			{
				return Red::System::StringCompareNoCase( p1.m_caption.AsChar(), p2.m_caption.AsChar() ) < 0;
			}
			return p1.m_isDirectory;
		}
	};


protected:
	CEdAssetBrowser*						m_browser;
	TDynArray< DiskFileInfo >				m_files;
	TSet< Uint32 >							m_active;
	Uint32									m_current;
	Int32									m_scrollOffset;
	Int32									m_maxHeight;
	Int32									m_minCaptionWidth;
	Int32									m_minClassNameWidth;
	Int32									m_minInfoWidth;
	Float									m_thumbnailScale;
	TDynArray< CClass* >					m_importClasses;
	TDynArray< CClass* >					m_factoryClasses;
	TDynArray< String >						m_favClasses;
	THashMap< String, CClass* >				m_resourceClasses;
	THashMap< String, Gdiplus::Bitmap* >	m_resourceThumbnails;
	wxScrollBar*							m_scrollBar;
	
	CVersionControlIconsPainter*			m_vciPainter;
	Gdiplus::Bitmap*						m_folderIcon;
	Gdiplus::Bitmap*						m_folderSmallIcon;

	EEditorResourceViewMode					m_viewType;
	bool									m_selecting;
	bool									m_dragAndDrop;
	wxPoint									m_selectionStartPoint;
	wxPoint									m_selectionEndPoint;

    struct SDragCursors
    {
        wxCursor m_copy;
        wxCursor m_move;

        SDragCursors()
        {}
        SDragCursors(wxCursor &copy, wxCursor &move)
            : m_copy(copy), m_move(move)
        {}
    };
	THashMap< Gdiplus::Bitmap*, SDragCursors >	m_cursorsCache;
	String									m_searchString;
	Gdiplus::Font*							m_searchFont;
	Bool									m_searching;

public:
	CEdResourceView( wxWindow* parent, CEdAssetBrowser* browser, wxScrollBar* scrollBar );
	~CEdResourceView();

	void ListFiles( TDynArray< CDiskFile* > &files, TDynArray< TPair< CDirectory*, String > > *dirs = NULL, Bool sortAlphabetically = true );
	Bool SelectFile( CDiskFile* file );
	Bool IsFileVisible( CDiskFile* file ) const;

	CDiskFile* GetActive();
	void GetActive( TSet< CDiskFile* > &a );
	void ChangeActiveResource( Int32 _index );
	void SetActive( TSet< CDiskFile* > &a, CDiskFile *current = NULL );
	void CheckActive();
	void RefreshGlobalActiveResources();

	EEditorResourceViewMode GetViewType(){ return m_viewType; }
	void SetViewType( EEditorResourceViewMode mode );
	void Scroll( Int32 offset );
	Int32 GetScroll(){ return m_scrollOffset; }

	Uint32 GetSize(){ return m_files.Size(); }

	void OnDeleteAsset( wxCommandEvent &event );
	void OnCopyAsset( wxCommandEvent &event );
	void OnCutAsset( wxCommandEvent &event );
	void OnPasteAsset( wxCommandEvent &event );
	void OnPasteAsAsset( wxCommandEvent &event );
	void OnRenameAsset( wxCommandEvent &event );
	void OnReloadAsset( wxCommandEvent &event );
	void OnOpenSource( wxCommandEvent &event );
	void OnOpenResource( wxCommandEvent &event );
	void OnRemapMaterials( wxCommandEvent &event );
	Bool SyncAndOpenFile( const String &path );
	
protected:
	DiskFileInfo* GetItemAtPoint( const wxPoint& point );
	void CalculateLayout();
	void PaintCanvas( Int32 width, Int32 height );
	String PaintThumbnail( CDiskFile* so, Uint32 thumbnailIdx, const wxRect& rect );
	void PaintNoThumbnail( const wxRect& rect );
	void PaintThumbnailImage( const wxRect& rect, Gdiplus::Bitmap* image );
	void UpdateScrollbar();
	void SetScrollOffset( Int32 offset );
	void ContentChange( const wxEventType &type );
	void ApplyPasteOnFiles(  CDirectory* dir, const TDynArray< String >& names  );
	void FillMenu( DiskFileInfo* clicked, wxMenu &menu );

protected:
	void OnContextMenu( wxMouseEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnChar( wxKeyEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnLoadAsset( wxCommandEvent& event );
	void OnSaveAsset( wxCommandEvent& event );
	void OnSaveAsAsset( wxCommandEvent &event );
	void OnCheckoutAsset( wxCommandEvent &event );
	void OnSubmitAsset( wxCommandEvent &event );
	void OnRevertAsset( wxCommandEvent &event );
	void OnEditAsset( wxCommandEvent &event );
	void OnAddAsset( wxCommandEvent &event );
	void OnRefreshAsset( wxCommandEvent &event );
	void OnAssetHistory( wxCommandEvent &event );
	void OnSyncAsset( wxCommandEvent &event );
	void OnExportAsset( wxCommandEvent &event );
	void OnCopyPathToClipboard( wxCommandEvent &event );
	void OnGotoResource( wxCommandEvent &event );
	void OnImport( wxCommandEvent& event );
	void OnReimportAsset( wxCommandEvent& event );
	void OnCreate( wxCommandEvent& event );
	void OnCreateFavorite( wxCommandEvent& event );
	void OnCreateDirectory( wxCommandEvent& event );
	void OnUpdateThumbnail( wxCommandEvent& event );
	void OnDeleteThumbnail( wxCommandEvent& event );
	void OnScroll( wxScrollEvent& event );
    void OnToggleReadonlyFlag( wxCommandEvent &event );
	void OnFindInWorld( wxCommandEvent &event );
	void OnFindInWorldMesh( wxCommandEvent &event );
	void OnFindInWorldStatic( wxCommandEvent &event );
	void OnFindInWorldRigid( wxCommandEvent &event );
	void OnFindInWorldEncounter( wxCommandEvent &event );
	void OnGenerateGloss( wxCommandEvent &event );

	void OnMouseEvent(  wxMouseEvent& event );
	void OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) );
	
	// version control operations on the canvas
	void OnSubmit( wxCommandEvent &event );
	void OnRevert( wxCommandEvent &event );
	void OnDelete( wxCommandEvent &event );
	void OnAdd( wxCommandEvent &event );
	void OnCheckOut( wxCommandEvent &event );

	void OnRefresh( wxCommandEvent &event );

	void OnViewBig( wxCommandEvent& event );
	void OnViewSmall( wxCommandEvent& event );
	void OnViewList( wxCommandEvent& event );

	void ChangeDirectory( CDirectory *dir, CDirectory *selectAfter );
	void ChangeDirectory( CDirectory *dir, CDiskFile *selectAfter );
	void GotoNeighbourItem( Int32 dx, Int32 dy, bool fixed, bool shiftDown );
	Bool EnsureItemVisible( Uint32 i );

protected:
	void CopyActive( bool isCopy );
	String GetNameForPastedAsset( const String& origName, bool pasteAs ) const;	
	void Paste( bool pasteAs );
	void SortFiles();

private:
	void UpdateThumbnailInFile( CDiskFile* file, const SThumbnailSettings* settings );
};
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "assetBrowser.h"
#include "wxThumbnailImageLoader.h"
#include "resourceView.h"
#include "fileDlg.h"
#include "commonDlgs.h"
#include "versionControlIconsPainter.h"
#include "resourceFinder.h"
#include "dataError.h"
#include "../../common/engine/gameResource.h"
#include "editorPreviewCameraProvider.h"
#include "updateThumbnailDialog.h"
#include "../../common/core/factory.h"
#include "../../common/core/versionControl.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/apexResource.h"

#include "../../common/core/depot.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/game/encounter.h"
#include "meshMaterialRemapper.h"
#include "itemsThumbnailGenerator.h"

enum
{
	ID_EDIT_ASSET			= 1000,
	ID_LOAD_ASSET			,
	ID_SAVE_ASSET			,
	ID_REIMPORT_ASSET		,
	ID_CHECKOUT_ASSET		,
	ID_SUBMIT_ASSET			,
	ID_REVERT_ASSET			,
	ID_DELETE_ASSET			,
	ID_ADD_ASSET			,
	ID_ASSET_HISTORY		,
	ID_SYNC_ASSET			,
	ID_REFRESH_ASSET		,
	ID_EXPORT_ASSET			,
	ID_SAVE_AS_ASSET		,
	ID_COPY_ASSET			,
	ID_CUT_ASSET			,
	ID_PASTE_ASSET			,
	ID_PASTE_AS_ASSET		,
	ID_RENAME_ASSET			,
	ID_RELOAD_ASSET			,
	ID_PATH_TO_CLIPBOARD	,
	ID_OPEN_SOURCE_FILE		,
	ID_OPEN_RESOURCE_FILE	,

	ID_IMPORT_ASSET_FIRST	,
	ID_IMPORT_ASSET_LAST	= ID_IMPORT_ASSET_FIRST + 200,

	ID_CREATE_ASSET_FIRST	,
	ID_CREATE_ASSET_LAST	= ID_CREATE_ASSET_FIRST + 200,

	ID_UPDATE_THUMBNAIL		,
	ID_DELETE_THUMBNAIL		,
	ID_REFRESH				,

	ID_CREATE_FAV_FIRST		,
	ID_CREATE_FAV_LAST		= ID_CREATE_FAV_FIRST + 200,

	ID_VIEW_BIG				,
	ID_VIEW_SMALL			,
	ID_VIEW_LIST			,

	ID_TOGGLE_READONLY		,
	ID_GOTO_RESOURCE		,
	ID_GENERATE_GLOSS		,

	ID_FIND_IN_WORLD		,
	ID_FIND_IN_WORLD_MESH	,
	ID_FIND_IN_WORLD_STATIC	,
	ID_FIND_IN_WORLD_RIGID	,
	ID_FIND_IN_WORLD_ENCOUNTER,

	ID_CREATE_DIRECTORY_CTX	,
};

#define ERROR_REVERT "Failed to revert this file."
#define ERROR_NOT_IN_WORKSPACE "File is not in the depot."

DEFINE_EVENT_TYPE(wxEVT_CHECK_OUT_EVENT)
DEFINE_EVENT_TYPE(wxEVT_SUBMIT_EVENT)
DEFINE_EVENT_TYPE(wxEVT_REVERT_EVENT)
DEFINE_EVENT_TYPE(wxEVT_ADD_EVENT)
DEFINE_EVENT_TYPE(wxEVT_DELETE_EVENT)
DEFINE_EVENT_TYPE(wxEVT_SYNC_EVENT)
DEFINE_EVENT_TYPE(wxEVT_NEW_EVENT)

BEGIN_EVENT_TABLE( CEdResourceView, CEdCanvas )
	EVT_MOUSE_EVENTS( CEdResourceView::OnMouseEvent )	
	//EVT_RIGHT_UP( CEdResourceView::OnContextMenu )
	EVT_SIZE( CEdResourceView::OnSize )
	EVT_MOUSEWHEEL( CEdResourceView::OnMouseWheel )
	EVT_KEY_DOWN( CEdResourceView::OnKeyDown )
	EVT_CHAR( CEdResourceView::OnChar )
	EVT_KEY_UP( CEdResourceView::OnKeyUp )
	EVT_MENU( ID_LOAD_ASSET, CEdResourceView::OnLoadAsset )
	EVT_MENU( ID_SAVE_ASSET, CEdResourceView::OnSaveAsset )
	EVT_MENU( ID_SAVE_AS_ASSET, CEdResourceView::OnSaveAsAsset )
	EVT_MENU( ID_COPY_ASSET, CEdResourceView::OnCopyAsset )
	EVT_MENU( ID_CUT_ASSET, CEdResourceView::OnCutAsset )
	EVT_MENU( ID_PASTE_ASSET, CEdResourceView::OnPasteAsset )
	EVT_MENU( ID_PASTE_AS_ASSET, CEdResourceView::OnPasteAsAsset )
	EVT_MENU( ID_RENAME_ASSET, CEdResourceView::OnRenameAsset )
	EVT_MENU( ID_DELETE_ASSET, CEdResourceView::OnDeleteAsset )
	EVT_MENU( ID_EDIT_ASSET, CEdResourceView::OnEditAsset )
	EVT_MENU( ID_RELOAD_ASSET, CEdResourceView::OnReloadAsset )
	EVT_MENU( ID_OPEN_SOURCE_FILE, CEdResourceView::OnOpenSource )
	EVT_MENU( ID_OPEN_RESOURCE_FILE, CEdResourceView::OnOpenResource )
	EVT_MENU( ID_REIMPORT_ASSET, CEdResourceView::OnReimportAsset )
	EVT_MENU( ID_EXPORT_ASSET, CEdResourceView::OnExportAsset )
	EVT_MENU( ID_PATH_TO_CLIPBOARD, CEdResourceView::OnCopyPathToClipboard )
	EVT_MENU( ID_GOTO_RESOURCE, CEdResourceView::OnGotoResource )
	EVT_MENU( ID_CHECKOUT_ASSET, CEdResourceView::OnCheckoutAsset )
	EVT_MENU( ID_SUBMIT_ASSET, CEdResourceView::OnSubmitAsset )
	EVT_MENU( ID_REVERT_ASSET, CEdResourceView::OnRevertAsset )
	EVT_MENU( ID_ADD_ASSET, CEdResourceView::OnAddAsset )
	EVT_MENU( ID_ASSET_HISTORY, CEdResourceView::OnAssetHistory )
	EVT_MENU( ID_REFRESH_ASSET, CEdResourceView::OnRefreshAsset )
	EVT_MENU( ID_SYNC_ASSET, CEdResourceView::OnSyncAsset )
	EVT_MENU( ID_REFRESH, CEdResourceView::OnRefresh )
    EVT_MENU( ID_TOGGLE_READONLY, CEdResourceView::OnToggleReadonlyFlag )
	EVT_MENU( ID_UPDATE_THUMBNAIL, CEdResourceView::OnUpdateThumbnail )
	EVT_MENU( ID_DELETE_THUMBNAIL, CEdResourceView::OnDeleteThumbnail )
	EVT_MENU_RANGE( ID_IMPORT_ASSET_FIRST, ID_IMPORT_ASSET_LAST, CEdResourceView::OnImport )
	EVT_MENU_RANGE( ID_CREATE_ASSET_FIRST, ID_CREATE_ASSET_LAST, CEdResourceView::OnCreate )
	EVT_MENU_RANGE( ID_CREATE_FAV_FIRST, ID_CREATE_FAV_LAST, CEdResourceView::OnCreateFavorite )
	EVT_MENU( ID_VIEW_BIG, CEdResourceView::OnViewBig )
	EVT_MENU( ID_VIEW_SMALL, CEdResourceView::OnViewSmall )
	EVT_MENU( ID_VIEW_LIST, CEdResourceView::OnViewList )
	EVT_MENU( ID_GENERATE_GLOSS, CEdResourceView::OnGenerateGloss )
	EVT_MENU( ID_FIND_IN_WORLD, CEdResourceView::OnFindInWorld )
	EVT_MENU( ID_FIND_IN_WORLD_MESH, CEdResourceView::OnFindInWorldMesh )
	EVT_MENU( ID_FIND_IN_WORLD_STATIC, CEdResourceView::OnFindInWorldStatic )
	EVT_MENU( ID_FIND_IN_WORLD_RIGID, CEdResourceView::OnFindInWorldRigid )
	EVT_MENU( ID_FIND_IN_WORLD_ENCOUNTER, CEdResourceView::OnFindInWorldEncounter )
	EVT_MENU( ID_CREATE_DIRECTORY_CTX, CEdResourceView::OnCreateDirectory )
	EVT_MOUSE_CAPTURE_LOST( CEdResourceView::OnMouseCaptureLost )
END_EVENT_TABLE()

CEdResourceView::CEdResourceView( wxWindow* parent, CEdAssetBrowser* browser, wxScrollBar* scrollBar )
	: CEdCanvas( parent, wxWANTS_CHARS )
	, m_browser( browser )
	, m_scrollOffset( 0 )
	, m_thumbnailScale( 1.0f )
	, m_scrollBar( scrollBar )
	, m_current( -1 )
	, m_dragAndDrop( false )
	, m_searching( false )
	, m_vciPainter( NULL )
{
	m_vciPainter = new CVersionControlIconsPainter( *this );

	// Get all resource classes
	TDynArray< CClass* > resourceClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< CResource >(), resourceClasses );

	// Create extension to resource class mapping
	for ( Uint32 i=0; i<resourceClasses.Size(); i++ )
	{
		// Get resource type extension
		CResource* defaultResource = resourceClasses[i]->GetDefaultObject<CResource>();
		String extension = String( defaultResource->GetExtension() ).ToLower();

		// Add to extension map
		m_resourceClasses.Insert( extension, resourceClasses[i] );
	}

	// Get icon directory
	String iconDirectory;
	GDepot->GetAbsolutePath( iconDirectory );
	iconDirectory += TXT("engine\\icons\\");

	// Scan icon directory
	TDynArray< String > iconFiles;
	GFileManager->FindFiles( iconDirectory, TXT("*.png"), iconFiles, false );
	for ( Uint32 i=0; i<iconFiles.Size(); i++ )
	{
		String absoluteFilePath = iconFiles[i];
		CFilePath path( absoluteFilePath );

		// Load PNG
		Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap( absoluteFilePath.AsChar(), false );
		if ( bmp )
		{
			// Add to per class thumbnail list
			m_resourceThumbnails.Insert( path.GetFileName().ToLower(), bmp );
		}
	}

	// Attach events
	m_scrollBar->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( CEdResourceView::OnScroll ), NULL, this );
	m_scrollBar->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CEdResourceView::OnScroll ), NULL, this );

	// Set version control icons
	m_folderIcon = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_FOLDER")));
	m_folderSmallIcon = ConvertToGDI( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_FOLDER_SMALL")));

	Gdiplus::FontFamily fontFamily;
	m_drawFont->GetFamily ( &fontFamily );
	m_searchFont = new Gdiplus::Font( &fontFamily, 1.3f * m_drawFont->GetSize(), m_drawFont->GetStyle(), m_drawFont->GetUnit() );

	m_viewType = ERVM_Big;
	m_selecting = false;
}

CEdResourceView::~CEdResourceView()
{
	delete m_vciPainter; m_vciPainter = NULL;
	delete m_searchFont;
	// Delete per-class thumbnail bitmaps
//	m_resourceThumbnails.ClearPtr();
}

void CEdResourceView::SetActive( TSet< CDiskFile* > &a, CDiskFile *current )
{
	TDynArray<String> activePaths;
	m_active.Clear();
	for( Uint32 i=0; i<m_files.Size(); i++ )
	{
		if( !m_files[ i ].m_isDirectory && a.Find( m_files[ i ].m_file ) != a.End() )
		{
			m_active.Insert( i );
			activePaths.PushBack( m_files[ i ].m_file->GetDepotPath() );
			if( m_files[ i ].m_file == current )
			{
				m_current = i;
			}
		}
	}
	SetActiveResources( activePaths );
	CalculateLayout();
	Repaint();
}

void CEdResourceView::CheckActive()
{
	if( m_current == -1 && m_files.Size() )
	{
		ChangeActiveResource( 0 );
		Refresh();
	}
}

void CEdResourceView::RefreshGlobalActiveResources()
{
	TDynArray< String > activePaths;
	for ( auto it=m_active.Begin(); it != m_active.End(); ++it )
	{
		if ( !m_files[ *it ].m_isDirectory )
		{
			activePaths.PushBack( m_files[ *it ].m_file->GetDepotPath() );
		}
	}
	SetActiveResources( activePaths );
}

CDiskFile* CEdResourceView::GetActive()
{ 
	if( m_current != -1 && !m_files[ m_current ].m_isDirectory )
		return m_files[ m_current ].m_file;

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
			return m_files[ *it ].m_file;
	return NULL;
}

void CEdResourceView::GetActive( TSet< CDiskFile* > &a )
{ 
	a.Clear();
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
			a.Insert( m_files[ *it ].m_file );
}

void CEdResourceView::CalculateLayout()
{
	// Get max width
	Int32 width=0, height=0;
	GetClientSize( &width, &height );

	// Setup stuff
	const Int32 sideMargin = 10;
	const Int32 hMargin = 15;
	Int32 vMargin = 15;

	// Start at top
	m_maxHeight = 0;
	Int32 textHeight = 16;
	Int32 x = sideMargin;
	Int32 y = sideMargin;      
	Int32 rowCount = 0;
	Int32 rowHeightMax = 0;

	// Snap scale to 1.0 for better viewing
	Float snappedImageScale = m_thumbnailScale;
	if ( abs( snappedImageScale - 1.0f ) < 0.15f )
	{
		snappedImageScale = 1.0f;
	}

	// Drawing filter
	const Bool drawCaption = snappedImageScale > 0.5f;
	const Bool drawInfo = snappedImageScale > 0.75f;

	Int32 thumbWidth = 128, thumbHeight = 128;

	if( m_viewType == ERVM_Small )
		thumbWidth = thumbHeight = 32;
	else
	if( m_viewType == ERVM_List )
	{
		thumbWidth = thumbHeight = textHeight;
		vMargin -= 5;
	}

	// Every resource in list
	m_minCaptionWidth = m_minClassNameWidth = m_minInfoWidth = 0;
	for ( Uint32 i=0; i<m_files.Size(); i++ )
	{
		DiskFileInfo &res = m_files[i];
		if ( res.m_file || res.m_isDirectory )
		{
			// Clear resource info
			res.m_info.Clear();

			// Determine size of the thumbnail
			res.m_size.x = thumbWidth * snappedImageScale;
			res.m_size.y = thumbHeight * snappedImageScale;

			// Calculate size of the caption & info lines
			Int32 classText = 0;
			Int32 numTextLines = 0;
			Int32 textWidth = 0;
			if ( drawCaption )
			{
				// Get caption width
				textWidth = TextExtents( GetGdiBoldFont(), res.m_caption ).x;
				numTextLines++;

				m_minCaptionWidth = Max( m_minCaptionWidth, textWidth );

				// Class name text
				Int32 classNameWidth;
				if( res.m_isDirectory )
				{
					classNameWidth = TextExtents( GetGdiDrawFont(), TXT("Directory") ).x;
				}
				else
				{
					CResource* defaultRes = res.m_class->GetDefaultObject<CResource>();
					classNameWidth = TextExtents( GetGdiDrawFont(), defaultRes->GetFriendlyDescription() ).x;
				}
				textWidth = Max< Int32 >( textWidth, classNameWidth );

				m_minClassNameWidth = Max( m_minClassNameWidth, classNameWidth );

				numTextLines++;

				// Additional resource info
				Bool hasAdditionalInfo = false;
				if( !res.m_isDirectory )
				{
					if ( res.m_file->GetResource() )
					{
						// Get additional resource info
						TDynArray< String > additionalInfo;
						res.m_file->GetResource()->GetAdditionalInfo( additionalInfo );

						// Add it
						for ( Uint32 i=0; i<additionalInfo.Size(); i++ )
						{
							if ( !additionalInfo[i].Empty() )
							{
								res.m_info.PushBack( additionalInfo[i] );
								hasAdditionalInfo = true;
							}
						}
					}

					// Standard resource info
					if ( !hasAdditionalInfo )
					{
						// Get info about file size
						String info;
						if ( res.m_file->GetResource() )
						{
							info = TXT("Loaded, ");
						}
						else
						{
							info = TXT("Not loaded, ");
						}

						// Add file size info
						Uint32 fileSize = GFileManager->GetFileSize( res.m_file->GetAbsolutePath() );
						if ( fileSize == ~0 )
						{
							info += TXT("Invalid");
						}
						else if ( fileSize < 1024*1024 )
						{
							info += String::Printf( TXT("%1.2f KB"), fileSize / 1024.0f );
						}
						else
						{
							info += String::Printf( TXT("%1.2f MB"), fileSize / (1024.0f*1024.0f) );
						}

						res.m_info.PushBack( info );
					}
				}

				// Max with extra text width
				if ( drawInfo )
				{
					Int32 w = 0;

					if ( res.m_file )
					{
						Bool found = false;
						TDynArray< CThumbnail* > thumbs = res.m_file->GetThumbnails();
						for ( Uint32 i = 0; i < thumbs.Size(); ++i )
							if ( ! thumbs[i]->GetName().Empty() )
							{
								found = true;
								Int32 localWidth = TextExtents( GetGdiDrawFont(), TXT("Appearance: ") + thumbs[i]->GetName() ).x;
								textWidth = Max( textWidth, localWidth );
								m_minInfoWidth = Max( m_minInfoWidth, localWidth );
							}

						if ( found )
							numTextLines++;
					}

					for ( Uint32 i=0; i<res.m_info.Size(); i++ )
					{
						Int32 localWidth = TextExtents( GetGdiDrawFont(), res.m_info[i] ).x;
						w += localWidth + 2;
						textWidth = Max( textWidth, localWidth );
						numTextLines++;
					}
					m_minInfoWidth = Max( m_minInfoWidth, w );
				}
			}

			// Accumulate size
			Int32 separationOffset = 0;
			Int32 sizeX, sizeY;
			if( m_viewType == ERVM_List )
			{
				sizeX = width - 2 * sideMargin - 2;
				//separationOffset = (res.m_size.y && numTextLines) ? 2 : 0;
				sizeY = Max( res.m_size.y, textHeight );
			}
			else
			{
				sizeX = Max( res.m_size.x, textWidth+10 );
				separationOffset = (res.m_size.y && numTextLines) ? 10 : 0;
				sizeY = res.m_size.y + separationOffset + textHeight * numTextLines;
			}

			// Calculate draw size
			res.m_total.x = sizeX;
			res.m_total.y = sizeY;

			// Check if resource fits in this row
			if ( rowCount && (x + sizeX > width) )
			{
				// Begin new row
				rowCount = 0; 
				y += rowHeightMax + vMargin;
				x = sideMargin;   
			}

			// New row
			if ( rowCount == 0 )
			{
				rowHeightMax = sizeY;
			}
			else
			{
				rowHeightMax = Max( sizeY, rowHeightMax );
			}

			// Place
			res.m_offset.x = x;
			res.m_offset.y = y;

			// Remember max height
			m_maxHeight = Max( m_maxHeight, res.m_offset.y + res.m_total.y + vMargin );  

			// Text pos
			res.m_text.x = res.m_offset.x;
			res.m_text.y = res.m_offset.y + res.m_size.y + separationOffset;  

			// Add resource to row
			rowCount++;  

			// Offset
			x += sizeX + hMargin;
		}
	}

	// Update scroll bar
	UpdateScrollbar();
}

void CEdResourceView::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	static wxColour back( 80, 80, 80 );
	static wxColour text( 255, 255, 255 );
	static wxColour texth( 255, 255, 0 );
	static wxColour highlight( 0, 0, 80 );

	// Clear background
	Clear( back );

	// Font size (hacked :P)
	const Int32 fontHeight = 16;

	// Snap scale to 1.0 for better viewing
	Float snappedImageScale = m_thumbnailScale;
	if ( abs( snappedImageScale - 1.0f ) < 0.15f )
	{
		snappedImageScale = 1.0f;
	}

	// Drawing filter
	const Bool drawCaption = snappedImageScale > 0.5f;
	const Bool drawInfo = snappedImageScale > 0.75f;

	Int32 captionX, classNameX, infoX;

	if( m_viewType == ERVM_List && m_files.Size() )
	{
		DiskFileInfo &res = m_files[0];
		Int32 totalW = res.m_total.x;
		Int32 left = totalW - 6 - /*2 **/ ( res.m_size.x + 2 ) - m_minCaptionWidth - m_minClassNameWidth - m_minInfoWidth - 4;
		if( left < 0 )
			left = 0;
		captionX = res.m_offset.x + 6 + /*2 **/ ( res.m_size.x + 2 );
		classNameX = captionX + m_minCaptionWidth + 2 + left / 2;
		infoX = classNameX + m_minClassNameWidth + 2 + left / 2;
	}

	// Draw resources

	for ( Uint32 i=0; i<=m_files.Size(); i++ )
	{
		Bool _break = false;
		Bool isActive = false;
		if( i == m_files.Size() )
		{
			if( m_current != -1 )
			{
				i = m_current;
				_break = true;
				isActive = true;
			}
			else
				break;
		}
		else
		{
			if( m_current == i )
				continue;
		}

		DiskFileInfo &res = m_files[i];

		const Bool isSelected = ( m_active.Find( i ) != m_active.End() );

		// Not visible yet
		if ( (res.m_offset.y + res.m_total.y ) < m_scrollOffset )
		{
			if( i == m_current )
				break;
			continue;
		}

		// Not visible already
		if ( (res.m_offset.y - m_scrollOffset ) > height )
		{
			if( i == m_current )
				break;
			i = m_files.Size() - 1; //paint current as last
			continue;
		}

		// Get item rect
		wxRect itemRect( res.m_offset, wxSize( res.m_total.x, res.m_total.y ) );

		// Draw background for selected resources
		if ( isActive )
		{ 
			// Draw highlight rect
			wxRect highlightRect = itemRect;
			highlightRect.Inflate( 5, 5 );

			// Draw hight
			FillRect( highlightRect, highlight );
			DrawRect( highlightRect, text );
		}
		else
		if ( isSelected )
		{ 
			// Draw highlight rect
			wxRect highlightRect = itemRect;
			highlightRect.Inflate( 5, 5 );

			// Draw hight
			FillRect( highlightRect, highlight );
		}

		String thumbName;
		
		// Draw thumbnail
		if ( res.m_size.x && res.m_size.y )
		{
			// Calculate thumb region
			wxRect thumbRect = itemRect;
			thumbRect.width = res.m_size.x;
			thumbRect.height = res.m_size.y;

			// Draw thumbnail
			SetClip( thumbRect );
			if( res.m_isDirectory )
			{
				if( m_viewType == ERVM_List )
					PaintThumbnailImage( thumbRect, m_folderSmallIcon );
				else
					PaintThumbnailImage( thumbRect, m_folderIcon );
			}
			else
			if( m_viewType != ERVM_List )
			{
				thumbName = PaintThumbnail( res.m_file, res.m_thumbnailIdx, thumbRect );
			}
			ResetClip();
		}

		SetClip( itemRect );
		// Draw tile
		if ( drawCaption )
		{
			const wxColour textColor = isSelected ? texth : text;

			// Class name
			String className;
			if( !res.m_isDirectory )
			{
				CResource* defaultRes = res.m_class->GetDefaultObject<CResource>();
				className = String::Printf( TXT("%s"), defaultRes->GetFriendlyDescription() );
			}
			else
			{
				className = TXT( "Directory" );
			}

			if( m_viewType == ERVM_List )
			{
				wxPoint p = res.m_offset;
				p.x = captionX;
				DrawText( p, GetGdiBoldFont(), res.m_caption, textColor );
				p.x = classNameX;
				DrawText( p, GetGdiDrawFont(), className, textColor );
				p.x = infoX;
				// Draw extra info
				if ( drawInfo && res.m_info.Size() )
				{
					String text = res.m_info[0];
					for ( Uint32 j=1; j<res.m_info.Size(); j++ )
						text += TXT(" ") + res.m_info[j];
					DrawText( p, GetGdiDrawFont(), text, textColor );
				}
			}
			else
			{
				// Caption
				wxPoint drawPoint = res.m_text;
				DrawText( drawPoint, GetGdiBoldFont(), res.m_caption, textColor );
				drawPoint.y += fontHeight;
				DrawText( drawPoint, GetGdiDrawFont(), className, textColor );

				// Draw extra info
				if ( drawInfo )
				{
					if ( !thumbName.Empty() )
					{
						drawPoint.y += fontHeight;
						DrawText( drawPoint, GetGdiDrawFont(), TXT("Appearance: ") + thumbName, textColor );
					}

					for ( Uint32 j=0; j<res.m_info.Size(); j++ )
					{
						drawPoint.y += fontHeight;
						DrawText( drawPoint, GetGdiDrawFont(), res.m_info[j], textColor );
					}
				}
			}
		}
		
		ResetClip();
		
		if( !res.m_isDirectory )
		{
			m_vciPainter->PaintVersionControlIcon( res.m_file, itemRect );
		}

		if( _break )
			break;
	}

	if( m_selecting && m_selectionStartPoint != m_selectionEndPoint )
	{
		DrawRect( wxRect( m_selectionStartPoint, m_selectionEndPoint ), text );
		//DrawText( m_selectionStartPoint, GetGdiDrawFont(), String::Printf( TXT( "%i, %i - %i, %i" ), m_selectionStartPoint.x, m_selectionStartPoint.y, m_selectionEndPoint.x, m_selectionEndPoint.y ), text );
	}

	if( m_searching )
	{
		wxPoint sz = TextExtents( *m_searchFont, m_searchString.Empty() ? TXT(" ") : m_searchString );
		//sz.x = 97 * sz.x / 100;
		if( sz.x < width / 2 - 24 )
		{
			sz.x = width / 2 - 24;
		}
		wxPoint sp( width-sz.x-20, m_scrollOffset+height-sz.y-20 );
		wxRect rect( sp.x, sp.y, sz.x, sz.y );
		rect.Inflate( 4, 4 );
		FillRect( rect, highlight );
		DrawRect( rect, text );
		DrawText( sp, *m_searchFont, m_searchString, texth );
	}
}

void CEdResourceView::SetViewType( EEditorResourceViewMode mode )
{
	if( m_viewType == mode )
		return;
	m_viewType = mode;
	CalculateLayout();
	Repaint();
}

void CEdResourceView::OnViewBig( wxCommandEvent& event )
{
	SetViewType( ERVM_Big );
	if ( m_browser )
		m_browser->UpdateResourceList();
}

void CEdResourceView::OnViewSmall( wxCommandEvent& event )
{
	SetViewType( ERVM_Small );
	if ( m_browser )
		m_browser->UpdateResourceList();
}

void CEdResourceView::OnViewList( wxCommandEvent& event )
{
	SetViewType( ERVM_List );
	if ( m_browser )
		m_browser->UpdateResourceList();
}

void CEdResourceView::ListFiles( TDynArray< CDiskFile* > &files, TDynArray< TPair< CDirectory*, String > > *dirs, Bool sortAlphabetically /* = true */ )
{
	// Remember active
	TDynArray< CDiskFile* > activeFiles;
	TDynArray< CDirectory* > activeDirs;
	Int32 current = m_current;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
			activeFiles.PushBack( m_files[ *it ].m_file );
		else
			activeDirs.PushBack( m_files[ *it ].m_directory );
	Int32 scroll = m_scrollOffset;

	// Reset
	SetScrollOffset( 0 );
	m_files.Clear();
	ChangeActiveResource( -1 );

	if( dirs )
	{
		for ( TDynArray< TPair< CDirectory*, String > >::iterator it=dirs->Begin(); it!=dirs->End(); it++ )
		{
			Uint32 idx = m_files.Size();
			m_files.PushBack( DiskFileInfo( idx, (*it).m_first, (*it).m_second ) );
			if( activeDirs.Exist( (*it).m_first ) )
				m_active.Insert( idx );
		}
	}

	struct AlphaFile
	{
		String m_path;
		TDynArray< TPair< CDiskFile*, CClass* > >* m_pairs;
	};

	TDynArray< AlphaFile > alphaFiles;
	TDynArray< TPair< CDiskFile*, CClass* > > stuff;
	TDynArray< TDynArray< TPair< CDiskFile*, CClass* > >* > deleteLater;

	for ( Uint32 i=0; i<files.Size(); i++ )
	{
		CFilePath path( files[i]->GetFileName() );
		// Display only files, that aren't layers and worlds
		// if ( ( path.GetExtension() != TXT("w2l") ) && ( path.GetExtension() != TXT("w2w") ) )
		{
			// Find resource class in list of allowed extensions
			CClass* resourceClass = NULL;
			if ( m_resourceClasses.Find( path.GetExtension().ToLower(), resourceClass ) )
			{
				String lowercasePath = path.GetFileName().ToLower();

				TDynArray< TPair< CDiskFile*, CClass* > >* pairs = nullptr;

				for ( auto it = alphaFiles.Begin(); it != alphaFiles.End(); ++it )
				{
					if ( it->m_path == lowercasePath )
					{
						pairs = it->m_pairs;
						break;
					}
				}


				if ( !pairs )
				{
					pairs = new TDynArray< TPair< CDiskFile*, CClass* > >();

					AlphaFile alphaFile;
					alphaFile.m_path = lowercasePath;
					alphaFile.m_pairs = pairs;

					alphaFiles.PushBack( alphaFile );
				}
				pairs->PushBack( TPair< CDiskFile*, CClass* >( files[i], resourceClass ) );
			}
		}
	}

	for( auto it=alphaFiles.Begin(); it!=alphaFiles.End(); ++it )
	{
		for( auto it2=it->m_pairs->Begin(); it2!=it->m_pairs->End(); ++it2 )
		{
			it2->m_first->LoadThumbnail();
			Uint32 idx = m_files.Size();

			m_files.PushBack( DiskFileInfo( idx, it2->m_first, it2->m_second, it->m_path ) );

			if ( activeFiles.Exist( it2->m_first ) )
			{
				m_active.Insert( idx );
			}
		}
	}
	
	if ( sortAlphabetically )
	{
		SortFiles();
	}
/*
	// Fill stubs
	for ( Uint32 i=0; i<files.Size(); i++ )
	{
		// Decompose path
		CFilePath path( files[i]->GetFileName() );
		// Display only files, that aren't layers and worlds
		if ( ( path.GetExtension() != TXT("w2l") ) && ( path.GetExtension() != TXT("w2w") ) )
		{
			// Find resource class in list of allowed extensions
			CClass* resourceClass = NULL;
			if ( m_resourceClasses.Find( path.m_extension.ToLower(), resourceClass ) )
			{
				// Load thumbnail
				files[i]->LoadThumbnail();

				// Assemble file caption
				String caption = path.m_fileName;
				Uint32 idx = m_files.Size();
				m_files.PushBack( DiskFileInfo( idx, files[i], resourceClass, caption ) );
				if( activeFiles.Exist( files[i] ) )
					m_active.Insert( idx );
			}
		}
	}
*/
	if( m_active.Find( current ) != m_active.End() )
	{
		m_current = current;
		if( m_files[ m_current ].m_isDirectory )
		{
			SetActiveDirectory( m_files[ m_current ].m_directory->GetDepotPath() );
		}
		else
		{
			SetActiveDirectory( m_files[ m_current ].m_file->GetDirectory()->GetDepotPath() );
		}
	}

	// Refresh active resource paths
	RefreshGlobalActiveResources();

	// Delete objects
	for ( auto it = alphaFiles.Begin(); it != alphaFiles.End(); ++it )
	{
		delete it->m_pairs;
	}

	// Update
	CalculateLayout();
	Scroll( scroll );
	Repaint();
}

void CEdResourceView::SortFiles()
{
	TSet< Uint32 > tmpActive = m_active;
	m_active.Clear();

	Sort( m_files.Begin(), m_files.End(), DiskFileInfo::Compare );
	for ( Uint32 i = 0; i < m_files.Size(); ++i )
	{
		if ( tmpActive.Exist( m_files[i].m_index ) )
		{
			m_active.Insert( i );
		}
		m_files[i].m_index = i;
	}
}

Bool CEdResourceView::IsFileVisible( CDiskFile* file ) const
{
	// Try to find among items
	for ( Uint32 i=0; i<m_files.Size(); i++ )
	{
		const DiskFileInfo& item = m_files[i];
		if ( item.m_file == file )
		{
			Int32 height = GetClientSize().y;
			wxRect itemRect = CanvasToClient( item.GetRect() );
			return GetClientRect().Intersects( itemRect );
		}
	}

	// Not visible in current view
	return false;
}

Bool CEdResourceView::SelectFile( CDiskFile* file )
{
	// Try to find among items
	for ( Uint32 i=0; i<m_files.Size(); i++ )
	{
		const DiskFileInfo& item = m_files[i];
		if ( item.m_file == file )
		{
			// Select file
			ChangeActiveResource( i );

			// Ensure visibility
			if ( !IsFileVisible( file ) )
			{
				SetScrollOffset( Max( 0, item.m_offset.y - 10 ) );
				Repaint();
			}

			// Found
			return true;
		}
	}

	// Not found
	return false;
}

CEdResourceView::DiskFileInfo* CEdResourceView::GetItemAtPoint( const wxPoint& point )
{
	// Linear search
	wxPoint mousePos = ClientToCanvas( point );
	for ( Uint32 i=0; i<m_files.Size(); i++ )
	{
		if ( m_files[i].GetRect().Inflate( 5, 5 ).Contains( mousePos ) )
		{
			// Found !
			return &m_files[i];
		}
	}

	// Not found
	return NULL;
}

String CEdResourceView::PaintThumbnail( CDiskFile* so, Uint32 thumbnailIdx, const wxRect& rect )
{
	// Use thumbnail if resource has one
	TDynArray<CThumbnail*> thumbs = so->GetThumbnails();
	if ( thumbs.Size() > 0 )
	{
		thumbnailIdx = Min( thumbs.Size()-1, thumbnailIdx );
		CWXThumbnailImage* image = (CWXThumbnailImage*)( thumbs[thumbnailIdx]->GetImage() );
		if ( image )
		{
			PaintThumbnailImage( rect, image->GetBitmap() );
			return thumbs[thumbnailIdx]->GetName();
		}
	}

	// Use per-class thumbnail if available
	Gdiplus::Bitmap* bitmap = NULL;
	String ext = so->GetDepotPath().StringAfter( TXT("."), true ).ToLower();
	if ( m_resourceThumbnails.Find( ext, bitmap ) )
	{
		PaintThumbnailImage( rect, bitmap );		
		return String::EMPTY;
	}

	// Default - no thumbnail
	PaintNoThumbnail( rect );
	return String::EMPTY;
}

void CEdResourceView::PaintNoThumbnail( const wxRect& rect )
{
	static wxColour back( 120, 120, 120 );
	static wxColour highlight( 255, 0, 0 );
	static wxColour red( 120, 0, 0 );
	static wxColour text( 255, 128, 64 );

	// Gray background
	FillRect( rect, back );

	// Lines
	DrawLine( rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom(), red, 3.0f );
	DrawLine( rect.GetRight(), rect.GetTop(), rect.GetLeft(), rect.GetBottom(), red, 3.0f );

	// Center text inside the window and clip it
	String caption = TEXT("No Thumbnail");
	wxRect drawRect = TextRect( GetGdiBoldFont(), caption ).CenterIn( rect );
	DrawText( drawRect.GetTopLeft(), GetGdiBoldFont(), caption, text );
}

void CEdResourceView::PaintThumbnailImage( const wxRect& rect, Gdiplus::Bitmap* image )
{
	DrawImage( image, rect );
}

void CEdResourceView::UpdateScrollbar()
{	
	// Update scroll position
	Int32 maxScroll = Max( 0, m_maxHeight - GetClientSize().y );
	if ( m_scrollOffset > maxScroll )
	{
		SetScrollOffset( maxScroll );
	}

	// Update scrollbar
	if ( m_maxHeight < GetClientSize().y )
	{
		// hiding scrollbar and resizing the window
		if ( m_scrollBar->IsShown() )
		{
			m_scrollBar->Hide();
			m_scrollBar->Enable( false );
			m_scrollBar->GetContainingSizer()->Layout();

			m_scrollBar->SetRange( 0 );
			m_scrollBar->SetThumbSize( 0 );
			m_scrollBar->SetThumbPosition( 0 );
		}
	}
	else
	{
		// showing scrollbar and resizing the window
		m_scrollBar->Enable( true );
		if ( !m_scrollBar->IsShown() )
		{
			m_scrollBar->Show();
			m_scrollBar->GetContainingSizer()->Layout();
		}

		m_scrollBar->SetRange( m_maxHeight );
		m_scrollBar->SetThumbSize( GetClientSize().y );
		m_scrollBar->SetThumbPosition( m_scrollOffset );
		
		// setting proper page size, so the view will be scrolled comfortably
		Uint32 i = 0;
		while ( ( i < m_files.Size() ) && ( m_files[i].m_offset.y < m_scrollOffset + GetClientSize().y ) )
			i++;
		if( i > 0 )
			m_scrollBar->SetPageSize( GetClientSize().y - m_files[i - 1].m_total.y );
	}
}

void CEdResourceView::ChangeActiveResource( Int32 _index )
{
	// Select resource
	m_active.Clear();
	m_current = -1;
	
	// Set active resource
	if ( _index != -1 )
	{
		m_current = _index;
		m_active.Insert( _index );
		if( m_files[ _index ].m_isDirectory )
		{
			SetActiveDirectory( m_files[ _index ].m_directory->GetDepotPath() );
		}
		else
		{
			SetActiveDirectory( m_files[ _index ].m_file->GetDirectory()->GetDepotPath() );
		}
		EnsureItemVisible( m_current );
	}
	else if ( m_browser->GetActiveDirectory() )
	{
		SetActiveDirectory( m_browser->GetActiveDirectory()->GetDepotPath() );
	}

	RefreshGlobalActiveResources();
}

void CEdResourceView::SetScrollOffset( Int32 offset )
{
	m_scrollOffset = offset;
	SetOffset( wxPoint( 0, -offset ) );
}

void CEdResourceView::Scroll( Int32 offset )
{
	if ( offset != m_scrollOffset )
	{
		// Set clamped scroll offset
		Int32 maxScroll = Max( 0, m_maxHeight - GetClientSize().y );
		SetScrollOffset( Clamp( offset, 0, maxScroll ) );

		// Update
		Repaint();
		UpdateScrollbar();
	}
}

void CEdResourceView::ContentChange( const wxEventType &type )
{
	if ( m_browser->GetActiveDirectory() )
	{
		m_browser->GetActiveDirectory()->Repopulate();
	}

	m_browser->UpdateResourceList();
	Repaint( true );

	wxCommandEvent submitEvent( type, GetId() );
	submitEvent.SetEventObject( this );
	GetEventHandler()->ProcessEvent( submitEvent );
}

void CEdResourceView::OnScroll( wxScrollEvent& event )
{
	if ( m_scrollBar->IsEnabled() )
	{
		Scroll( m_scrollBar->GetThumbPosition() );
	}
}

void CEdResourceView::OnToggleReadonlyFlag( wxCommandEvent &event )
{
    Bool readonly = false;

    for( TSet< Uint32 >::iterator it=m_active.Begin(); !readonly && it!=m_active.End(); it++ )
    {
        if ( m_files[ *it ].m_isDirectory ) continue;
        Uint32 attribs = GetFileAttributes( m_files[ *it ].m_file->GetAbsolutePath().AsChar() );
        readonly = readonly || (attribs & FILE_ATTRIBUTE_READONLY);
    }

    for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
    {
        if ( m_files[ *it ].m_isDirectory ) continue;
        const WCHAR *filename = m_files[ *it ].m_file->GetAbsolutePath().AsChar();
        Uint32 attribs = GetFileAttributes( filename );
        attribs = readonly
            ? attribs & ~FILE_ATTRIBUTE_READONLY
            : attribs |  FILE_ATTRIBUTE_READONLY;
        SetFileAttributes( filename, attribs );
    }
}

void CEdResourceView::OnMouseEvent( wxMouseEvent& event )
{
	if( event.LeftUp() )
	{
		if( m_selecting )
		{
			m_selecting = false;
			wxWindow::ReleaseMouse();
			Repaint();
		}

		m_dragAndDrop = false;
	}

	wxPoint scroll = wxPoint( 0, m_scrollBar->GetThumbPosition() );

	if( m_selecting )
	{
		wxPoint curPos = event.GetPosition() + scroll;
		if( m_dragAndDrop )
		{
			if ( MAbs(curPos.x - m_selectionEndPoint.x) > 5 || MAbs(curPos.y - m_selectionEndPoint.y) > 5 )
			{
				m_selecting = false;
				m_dragAndDrop = false;
				wxWindow::ReleaseMouse();

				String drop = TXT("Resources:");

				CDiskFile *thumbSrc = NULL;
				Int32 files = 0;
		
				for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
					if( !m_files[ *it ].m_isDirectory )
					{
						thumbSrc = m_files[ *it ].m_file;
						drop += thumbSrc->GetDepotPath() + TXT( ";" );
						files ++;
					}
					
				Gdiplus::Bitmap* bitmap = NULL;
				if( files == 1 )
				{
					TDynArray< CThumbnail* > thumbs = thumbSrc->GetThumbnails();
					if ( thumbs.Size() )
					{
						CWXThumbnailImage* image = (CWXThumbnailImage*)(  thumbs[0]->GetImage() );
						if ( image )
						{
							bitmap = image->GetBitmap();
						}
					}

					if( !bitmap )
					{
						String ext = thumbSrc->GetDepotPath().StringAfter( TXT("."), true ).ToLower();
						if( !m_resourceThumbnails.Find( ext, bitmap ) )
						{
							bitmap = NULL;
						}
					}
				}

				wxCursor cursorCopy, cursorMove;

				if( bitmap )
				{
					if ( m_cursorsCache.KeyExist( bitmap ) )
					{
						SDragCursors* cursor = m_cursorsCache.FindPtr( bitmap );
						cursorCopy = cursor->m_copy;
						cursorMove = cursor->m_move;
					}
					else
					{
						if ( m_cursorsCache.Size() > 32 )
						{
							m_cursorsCache.Clear();
						}

						Uint32 w = bitmap->GetWidth();
						Uint32 h = bitmap->GetHeight();
						Uint32 destW = 32;
						Uint32 destH = 32;
						wxImage b( destW, destH );
						b.SetAlpha();

						b.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 3);
						b.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 5);

						Gdiplus::Color pixelColor;
						for( Uint32 y=1; y<destH-1; y++ )
						{
							Uint32 srcY = y * h / destH;
							for( Uint32 x=1; x<destW-1; x++ )
							{
								Uint32 srcX = x * w / destW;
								bitmap->GetPixel( srcX, srcY, &pixelColor );
								if( pixelColor.GetAlpha() < 90 )
								{
									b.SetRGB( x, y, 255, 255, 255 );
									b.SetAlpha( x, y, 90 );
								}
								else
								{
									b.SetRGB( x, y, pixelColor.GetRed(), pixelColor.GetGreen(), pixelColor.GetBlue() );
									b.SetAlpha( x, y, pixelColor.GetAlpha() );
								}
							}
						}
						for( Uint32 y=0; y<destH; y++ )
						{
							b.SetRGB( 0, y, 255, 255, 255 );
							b.SetRGB( destW-1, y, 255, 255, 255 );
						}
						for( Uint32 x=1; x<destW-1; x++ )
						{
							b.SetRGB( x, 0, 255, 255, 255 );
							b.SetRGB( x, destH-1, 255, 255, 255 );
						}

						// Add arrow
						{
							wxBitmap curBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW") );
							wxImage imgCur = curBitmap.ConvertToImage();

							w = imgCur.GetWidth();
							h = imgCur.GetHeight();

							for( Uint32 y=0; y<h; y++ )
							{
								for( Uint32 x=0; x<w; x++ )
								{
									if ( !imgCur.IsTransparent(x,y) )
									{
										b.SetRGB(x,y,imgCur.GetRed(x,y),imgCur.GetGreen(x,y),imgCur.GetBlue(x,y));
										b.SetAlpha(x,y,255);								
									}							
								}
							}
						}

						cursorMove = wxCursor( b );

						// Add +
						{
							wxBitmap curBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_ADD") );
							wxImage imgCur = curBitmap.ConvertToImage();

							w = imgCur.GetWidth();
							h = imgCur.GetHeight();

							Uint32 xMin = b.GetWidth()  - w;
							Uint32 yMin = b.GetHeight() - h;

							for( Uint32 y=0; y<h; y++ )
							{
								for( Uint32 x=0; x<w; x++ )
								{
									if ( !imgCur.IsTransparent(x,y) )
									{
										b.SetRGB(x+xMin,y+yMin,imgCur.GetRed(x,y),imgCur.GetGreen(x,y),imgCur.GetBlue(x,y));
										b.SetAlpha(x+xMin,y+yMin,255);								
									}							
								}
							}
						}

						cursorCopy = wxCursor( b );

						m_cursorsCache.Set( bitmap, SDragCursors(cursorCopy, cursorMove) );
					}
				}
				else
				{
					Uint32 destW = 32;
					Uint32 destH = 32;
					wxImage b( destW, destH );
					b.SetAlpha();

					b.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 3);
					b.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 5);

					wxBitmap dropBitmap;
					if ( files == 1 )
					{
						dropBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SELECT_SINGLE") );
					}
					else
					{
						dropBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SELECT_MULTI") );
					}
	 
					wxImage dropImg = dropBitmap.ConvertToImage();

					Uint32 w = dropImg.GetWidth();
					Uint32 h = dropImg.GetHeight();

					for( Uint32 y=0; y<destH; y++ )
					{
						Uint32 srcYD = y * h / destH;

						for( Uint32 x=0; x<destW; x++ )
						{
							Uint32 srcXD = x * w / destW;
							
							if (dropImg.IsTransparent(srcXD,srcYD))
							{
								b.SetAlpha(x,y,0);
							}
							else
							{
								b.SetRGB(x,y,dropImg.GetRed(srcXD,srcYD),dropImg.GetGreen(srcXD,srcYD),dropImg.GetBlue(srcXD,srcYD));
								b.SetAlpha(x,y,180);
							}
						}
					}

					// Add arrow
					{
						wxBitmap curBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ARROW") );
						wxImage imgCur = curBitmap.ConvertToImage();

						w = imgCur.GetWidth();
						h = imgCur.GetHeight();

						for( Uint32 y=0; y<h; y++ )
						{
							for( Uint32 x=0; x<w; x++ )
							{
								if ( !imgCur.IsTransparent(x,y) )
								{
									b.SetRGB(x,y,imgCur.GetRed(x,y),imgCur.GetGreen(x,y),imgCur.GetBlue(x,y));
									b.SetAlpha(x,y,255);								
								}							
							}
						}
					}

					cursorMove = wxCursor( b );

					// Add +
					{
						wxBitmap curBitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_ADD") );
						wxImage imgCur = curBitmap.ConvertToImage();

						w = imgCur.GetWidth();
						h = imgCur.GetHeight();

						Uint32 xMin = b.GetWidth()  - w;
						Uint32 yMin = b.GetHeight() - h;

						for( Uint32 y=0; y<h; y++ )
						{
							for( Uint32 x=0; x<w; x++ )
							{
								if ( !imgCur.IsTransparent(x,y) )
								{
									b.SetRGB(x+xMin,y+yMin,imgCur.GetRed(x,y),imgCur.GetGreen(x,y),imgCur.GetBlue(x,y));
									b.SetAlpha(x+xMin,y+yMin,255);								
								}							
							}
						}
					}

					cursorCopy = wxCursor( b );
				}

				wxString wxDrop = drop.AsChar();
				wxTextDataObject myData( wxDrop );

				wxDropSource dragSource( myData, this, cursorCopy, cursorMove);
				wxDragResult result = dragSource.DoDragDrop( wxDrag_DefaultMove );

				Refresh( true );
			}

			return;
		}

		m_selectionEndPoint = event.GetPosition() + scroll;

		wxRect r( m_selectionStartPoint, m_selectionEndPoint );

		m_active.Clear();
		m_current = -1;

		for ( Uint32 i=0; i<m_files.Size(); i++ )
		{
			wxRect ir = m_files[i].GetRect().Inflate( 5, 5 );
			if ( ir.Intersects( r ) )
			{
				m_active.Insert( i );
				if( ir.Contains( m_selectionEndPoint ) )
				{
					m_current = i;
					if( m_files[ m_current ].m_isDirectory )
					{
						SetActiveDirectory( m_files[ m_current ].m_directory->GetDepotPath() );
					}
					else
					{
						SetActiveDirectory( m_files[ m_current ].m_file->GetDirectory()->GetDepotPath() );
					}
				}
			}
		}

		// Refresh active resource paths
		RefreshGlobalActiveResources();
		Repaint();
	}

	if( event.GetWheelDelta() )
	{
		OnMouseWheel( event );
	}
	
	if( event.RightUp() )
		OnContextMenu( event );

	if( event.LeftDown() || event.LeftDClick() )
	{
		// Always focus on this window ( needed for mouse wheel )
		SetFocus();

		// Select resource
		DiskFileInfo* clicked = GetItemAtPoint( event.GetPosition() );

        Bool hasBeenSelectedBefore = false;
        
		if ( clicked )
		{
            hasBeenSelectedBefore = m_active.Find( clicked->m_index ) != m_active.End();

            if( event.ControlDown() )
			{
				if( m_active.Find( clicked->m_index ) != m_active.End() )
				{
					m_active.Erase( clicked->m_index );
					if( m_current == clicked->m_index )
						m_current = -1;
				}
				else
				{
					m_active.Insert( clicked->m_index );
					m_current = clicked->m_index;
					if( m_files[ m_current ].m_isDirectory )
					{
						SetActiveDirectory( m_files[ m_current ].m_directory->GetDepotPath() );
					}
					else
					{
						SetActiveDirectory( m_files[ m_current ].m_file->GetDirectory()->GetDepotPath() );
					}
				}
			}
			else
			if( event.ShiftDown() )
			{
				if( m_current == -1 )
					ChangeActiveResource( clicked->m_index );
				else
				{
					m_active.Clear();
					Uint32 max = Max( m_current, clicked->m_index ), min = Min( m_current, clicked->m_index );
					for( Uint32 i=min; i<=max; i++ )
						m_active.Insert( i );
				}
			}
			else
            if (!hasBeenSelectedBefore)
				ChangeActiveResource( clicked->m_index );

			RefreshGlobalActiveResources();
			Repaint();

			// Double click, edit
			if ( event.LeftDClick() )
			{
				wxCommandEvent fake;
				OnEditAsset( fake );
			}
		}
		else
		{
			ChangeActiveResource( -1 );
			Repaint();
		}

        if( event.LeftDown() )
		{
			m_selecting = true;
			wxWindow::CaptureMouse();
			wxPoint scroll = wxPoint( 0, m_scrollBar->GetThumbPosition() );
			
			if ( !m_dragAndDrop )
			{
				m_selectionEndPoint = m_selectionStartPoint = event.GetPosition() + scroll;
				m_dragAndDrop = clicked && ( hasBeenSelectedBefore || (!event.ShiftDown() && !event.ControlDown()));
			}
		}
	}
}

void CEdResourceView::OnMouseCaptureLost( wxMouseCaptureLostEvent& WXUNUSED(event) )
{
	// don't call event.Skip() here...
}

void CEdResourceView::OnContextMenu( wxMouseEvent& event )
{
	// Focus on this window
	SetFocus();

	// Select resource
	DiskFileInfo* clicked = GetItemAtPoint( event.GetPosition() );
	if ( clicked )
	{
		if( event.ControlDown() )
			m_active.Insert( clicked->m_index );
		else
		if( m_active.Find( clicked->m_index ) == m_active.End() )
			ChangeActiveResource( clicked->m_index );
		Repaint();
	}
	else
	{
		ChangeActiveResource( -1 );
		Repaint();
	}

	// Compose menu
	wxMenu menu;
	FillMenu( clicked, menu );

	// Import menu
	{
		// Enumerate import formats
		m_importClasses.Clear();
		IImporter::EnumImportClasses( m_importClasses );

		// Sort classes alphabetically by friendly name
		Sort( m_importClasses.Begin(), m_importClasses.End(),
			[]( CClass* c1, CClass* c2 ) {
				return String( c1->GetDefaultObject<CResource>()->GetFriendlyDescription() ) < String( c2->GetDefaultObject<CResource>()->GetFriendlyDescription() );
			});

		// Assemble menu
		wxMenu *importMenu = new wxMenu;
		for ( Uint32 i=0; i<m_importClasses.Size(); i++ )
		{
			CClass* importClass = m_importClasses[i];
			String className = (importClass->GetDefaultObject<CResource>())->GetFriendlyDescription();
			importMenu->Append( ID_IMPORT_ASSET_FIRST + i, className.AsChar() );
		}

		menu.Append( wxID_ANY, TXT("Import"), importMenu );
	}

	// Factory menu
	{
		// Enumerate import formats
		m_factoryClasses.Clear();
		IFactory::EnumFactoryClasses( m_factoryClasses );

		// Sort classes alphabetically by friendly name
		Sort( m_factoryClasses.Begin(), m_factoryClasses.End(),
			[]( CClass* c1, CClass* c2 ) {
				return c1->GetDefaultObject<IFactory>()->GetFriendlyName() < c2->GetDefaultObject<IFactory>()->GetFriendlyName();
			});

		// Assemble menu
		wxMenu *factoryMenu = new wxMenu;
		for ( Uint32 i=0; i<m_factoryClasses.Size(); i++ )
		{
			CClass* factoryClass = m_factoryClasses[i];
			String className = (factoryClass->GetDefaultObject<IFactory>())->GetFriendlyName();
			factoryMenu->Append( ID_CREATE_ASSET_FIRST + i, className.AsChar() );
		}

		menu.Append( wxID_ANY, TXT("Create"), factoryMenu );

		// Crate favorites menu
		if ( !m_browser->m_favClasses.Empty() )
		{
			m_favClasses = m_browser->m_favClasses;
			Sort( m_favClasses.Begin(), m_favClasses.End() );

			wxMenu *favsMenu = new wxMenu();
			for ( Uint32 i=0; i<m_favClasses.Size(); i++ )
			{
				if ( CClass* favClass = SRTTI::GetInstance().FindClass( CName( m_favClasses[i] ) ) )
				{
					if ( IFactory* factory = IFactory::FindFactory( favClass ) )
					{
						String className = factory->GetFriendlyName();
						favsMenu->Append( ID_CREATE_FAV_FIRST + i, className.AsChar() );
					}
				}
			}

			menu.Append( wxID_ANY, TXT("Create favorite"), favsMenu );
		}
	}

	menu.Append( ID_CREATE_DIRECTORY_CTX, TXT("Create Directory Here") );

	// Show menu
	wxMenuUtils::CleanUpSeparators( &menu );
	PopupMenu( &menu );
}

void CEdResourceView::OnSize( wxSizeEvent& event )
{
	CalculateLayout();
	CEdCanvas::OnSize( event );
}

void CEdResourceView::ChangeDirectory( CDirectory *dir, CDirectory *selectAfter )
{
	m_browser->SelectDirectory( dir );
	m_active.Clear();
	for ( Uint32 i=0; i<m_files.Size(); i++ )
		if( m_files[ i ].m_isDirectory && m_files[ i ].m_directory == selectAfter )
		{
			m_current = i;
			m_active.Insert( i );
			break;
		}
	Refresh( true );
}

void CEdResourceView::ChangeDirectory( CDirectory *dir, CDiskFile *selectAfter )
{
	m_browser->SelectDirectory( dir );
	m_active.Clear();
	for ( Uint32 i=0; i<m_files.Size(); i++ )
		if( !m_files[ i ].m_isDirectory && m_files[ i ].m_file == selectAfter )
		{
			ChangeActiveResource( i );
			break;
		}
	Refresh( true );
}

void CEdResourceView::GotoNeighbourItem( Int32 dx, Int32 dy, bool fixed, bool shiftDown )
{
	if( m_current == -1 )
	{
		if( m_files.Size() )
		{
			ChangeActiveResource( 0 );
			Scroll( 0 );
			Refresh( true );
		}
		return;
	}
	
	DiskFileInfo &res = m_files[ m_current ];

	// Get item rect
	wxRect itemRect( res.m_offset, wxSize( res.m_total.x, res.m_total.y ) );
	itemRect.Inflate( 5, 5 );

	if( fixed )
	{
		itemRect.SetX( itemRect.GetX() + dx );
		itemRect.SetY( itemRect.GetY() + dy );
	}
	else
	{
		itemRect.SetX( itemRect.GetX() + dx * itemRect.GetWidth() / 2 );
		itemRect.SetY( itemRect.GetY() + dy * itemRect.GetHeight() / 2 );
	}

	if( itemRect.GetTop() < 0 )
		itemRect.SetTop( 0 );
	else
	if( itemRect.GetBottom() > m_maxHeight )
		itemRect.SetTop( m_maxHeight - itemRect.GetHeight() );
	if( itemRect.GetLeft() < 0 )
		itemRect.SetLeft( 0 );

	for ( Uint32 i=0; i<m_files.Size(); i++ )
		if( i != m_current )
		{
			wxRect ir = m_files[i].GetRect().Inflate( 5, 5 );
			if ( ir.Intersects( itemRect ) )
			{
				if( !shiftDown )
				{
					ChangeActiveResource( i );
				}
				else
				{
					Uint32 min = Min( m_current, i );
					Uint32 max = Max( m_current, i );
					for( Uint32 j=min; j<=max; j++ )
						m_active.Insert( j );

					m_current = i;
					m_active.Insert( i );
					if( m_files[ m_current ].m_isDirectory )
					{
						SetActiveDirectory( m_files[ m_current ].m_directory->GetDepotPath() );
					}
					else
					{
						SetActiveDirectory( m_files[ m_current ].m_file->GetDirectory()->GetDepotPath() );
					}
					RefreshGlobalActiveResources();
				}

				if( !EnsureItemVisible( i ) )
					Refresh( true );
				break;
			}
		}
}

Bool CEdResourceView::EnsureItemVisible( Uint32 i )
{
	if( i >= m_files.Size() )
		return false;

	wxRect ir = m_files[ i ].GetRect().Inflate( 5, 5 );
	Int32 max = m_scrollOffset + GetClientSize().y;
	if( ir.GetBottom() + 5 > max )
		Scroll( ir.GetBottom() - GetClientSize().y + 5 );
	else
	if( ir.GetTop() - 5 < m_scrollOffset )
		Scroll( ir.GetTop() - 5 );
	else
		return false;

	return true;
}

void CEdResourceView::ApplyPasteOnFiles( CDirectory* dir, const TDynArray< String >& names )
{
	for( Uint32 j = 0; j < names.Size(); ++j )
	{
		String newFilePath = dir->GetDepotPath() + names[ j ];
		CResource* copiedResource = GDepot->LoadResource( newFilePath );
		if ( copiedResource != NULL )
		{
			copiedResource->OnPaste( true );
		}
	}
}

void CEdResourceView::OnKeyUp( wxKeyEvent& event )
{
	/*
	if( !event.AltDown() )
	{
		if( !m_searchString.Empty() )
		{
			m_searchString = String::EMPTY;
			Refresh();
		}
	}
	*/
}

void CEdResourceView::OnChar( wxKeyEvent& event )
{
	if( event.AltDown() || m_searching )
	{
		Int32 code = event.GetKeyCode();
		Bool find = false;
		m_searching = true;

		if( code == WXK_BACK )
		{
			if( !m_searchString.Empty() )
			{
				m_searchString = m_searchString.LeftString( m_searchString.GetLength()-1 );
				find = true;
			}
		}
		else
		if( code >= 32 && code <= 255 )
		{
			wxString s = m_searchString.AsChar();
			s += wxChar( code );
			m_searchString = String( s );
			m_searchString = m_searchString.ToLower();
			find = true;
		}
		size_t subStringLocation;
		if( find )
		{
			if( !m_searchString.Empty() )
			{
				String searchStringDir = TXT( "[" );
				searchStringDir += m_searchString;
				for( Uint32 i=0; i<m_files.Size(); i++ )
				{
					if( m_files[ i ].m_isDirectory )
					{
						Bool found = m_files[ i ].m_caption.FindSubstring( searchStringDir, subStringLocation );
						if(found && subStringLocation == 0 )
						{
							ChangeActiveResource( i );
							break;
						}
					}
					else
					{
						Bool found = m_files[ i ].m_caption.FindSubstring( m_searchString, subStringLocation ); 
						if(found && subStringLocation == 0 )
						{
							ChangeActiveResource( i );
							break;
						}
					}
				}
			}
			Refresh();
		}
	}
}

void CEdResourceView::OnKeyDown( wxKeyEvent& event )
{
	if( event.AltDown() || m_searching )
	{
		switch ( event.GetKeyCode() )
		{
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
		case WXK_ESCAPE:
			{
				m_searching = false;
				m_searchString = String::EMPTY;
				Refresh();
				return;
			}
		}

		event.Skip( true );
		return;
	}

	if( event.ControlDown() )
	{
		switch ( event.GetKeyCode() )
		{
		case WXK_F1:
			{
				SetViewType( ERVM_List );
				break;
			}
		case WXK_F2:
			{
				SetViewType( ERVM_Small );
				break;
			}
		case WXK_F3:
			{
				SetViewType( ERVM_Big );
				break;
			}
		case 'T':
			{
				m_browser->OnAddTabButton( wxCommandEvent() );
				break;
			}
		case 'W':
			{
				m_browser->OnTabClose( wxAuiNotebookEvent() );
				break;
			}
		case 'B':
			{
				m_browser->OnFlatDirectory( wxCommandEvent() );
				break;
			}
		case 'F':
			{
				m_browser->OnSearch();
				break;
			}
		case 'O':
			{
				m_browser->OnCheckedOutButton( wxCommandEvent() );
				break;
			}
		}
		
		return;
	}

	// Process key
	switch ( event.GetKeyCode() )
	{
	case WXK_BACK:
		{
			if( m_files.Size() >= 2 && m_files[ 1 ].m_isDirectory )
				ChangeDirectory( m_files[ 1 ].m_directory, m_browser->GetCurrentDirectory() );
			return;
		}
	
	case WXK_TAB:
		{
			m_browser->NextTab();
			return;
		}
	case WXK_RETURN:
	case WXK_NUMPAD_ENTER:
		{
			if( m_current == -1 )
				return;
			if( m_browser->IsCurrentTabFlat() )
			{
				CDiskFile *df = GetActive();
				if( df )
				{
					ChangeDirectory( df->GetDirectory(), df );
					return;
				}
			}
			if( m_files[ m_current ].m_isDirectory )
			{
				ChangeDirectory( m_files[ m_current ].m_directory, m_browser->GetCurrentDirectory() );
			}
			else
			{
				CDiskFile *f = m_files[ m_current ].m_file;
				if( !f->Load() )
					return;
				// Try to open existing resource editor
				CResource* res = f->GetResource();
				m_browser->EditAsset( res );
			}
			return;
		}
	case WXK_DOWN:
		{
			GotoNeighbourItem( 0, 1, false, event.ShiftDown() );
			return;
		}
	case WXK_UP:
		{
			GotoNeighbourItem( 0, -1, false, event.ShiftDown() );
			return;
		}
	case WXK_LEFT:
		{
			GotoNeighbourItem( -1, 0, false, event.ShiftDown() );
			return;
		}
	case WXK_RIGHT:
		{
			GotoNeighbourItem( 1, 0, false, event.ShiftDown() );
			return;
		}

	// Home
	case WXK_HOME:
		{
			if( m_current == -1 )
				Scroll( 0 );
			else
				GotoNeighbourItem( -10000, -m_maxHeight, true, event.ShiftDown() );
			return;
		}

	// End
	case WXK_END:
		{
			if( m_current == -1 )
				Scroll( m_maxHeight );
			else
				GotoNeighbourItem( -10000, m_maxHeight, true, event.ShiftDown() );
			return;
		}

	// Page down
	case WXK_PAGEDOWN:
		{
			if( m_current == -1 )
				Scroll( m_scrollOffset + GetClientSize().y );
			else
				GotoNeighbourItem( -10000, GetClientSize().y, true, event.ShiftDown() );
			return;
		}

	// Page up
	case WXK_PAGEUP:
		{
			if( m_current == -1 )
				Scroll( m_scrollOffset - GetClientSize().y );
			else
				GotoNeighbourItem( -10000, -GetClientSize().y, true, event.ShiftDown() );
			return;
		}
	}
}

void CEdResourceView::OnMouseWheel( wxMouseEvent& event )
{
	if ( event.ControlDown() )
	{
		// Scaling
		m_thumbnailScale *= 1.0f + 0.05f * ( event.GetWheelRotation() / (Float) event.GetWheelDelta() );
		m_thumbnailScale = Clamp( m_thumbnailScale, 0.20f, 5.0f );

		// Update everything
		CalculateLayout();
		Repaint( true );
	}
	else
	if ( event.ShiftDown() )
	{
		for ( Uint32 i = 0; i < m_files.Size(); ++i )
		//for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		{
			DiskFileInfo &file = m_files[ i/**it*/ ];

			if ( file.m_isDirectory )
				continue;

			TDynArray<CThumbnail*> thumbs = file.m_file->GetThumbnails();
			if ( thumbs.Size() == 0 )
				continue;
			
			file.m_thumbnailIdx = (file.m_thumbnailIdx + 1) % thumbs.Size();
		}
		Repaint( true );
	}
	else
	{
		// Normal scroll
		Scroll( m_scrollOffset - event.GetWheelRotation() / 2 );
	}
}

void CEdResourceView::OnLoadAsset( wxCommandEvent& event )
{
	ASSERT( !m_active.Empty() );

	bool ret = false;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->Load() )
				ret = true;
		}

	if ( ret )
	{
		CalculateLayout();
		Refresh( false );
	}
}

void CEdResourceView::OnSaveAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	TDynArray< String > failedItems;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsLoaded() )
			{
				if ( f->GetThumbnails().Size() > 0 )
				{
					f->UpdateThumbnail();
				}
				if( !f->Save() )
				{
					failedItems.PushBack( m_files[ *it ].m_file->GetDepotPath() );
				}
			}
		}

	if( !failedItems.Empty() )
	{
		if( failedItems.Size() == 1 )
		{
			wxString msg;
			msg.sprintf( TXT("File %s cannot be saved."), failedItems[ 0 ].AsChar() );
			wxMessageDialog dialog( this, msg, TXT("Save error"), wxOK | wxICON_INFORMATION );
			dialog.ShowModal();
		}
		else
		{
			CDetailsDlg dlg( this, TXT("Save error"), TXT("Some files cannot be saved."), String::Join( failedItems, TXT("\n") ).AsChar(), TXT("Ok"), wxEmptyString );
			dlg.DoModal();
		}
	}

	m_browser->RepopulateDirectory();
	m_browser->UpdateResourceList();
	Repaint( true );
}

void CEdResourceView::OnSaveAsAsset( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );
	
	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	String absPath = file->GetDirectory()->GetAbsolutePath();
	CFilePath path( file->GetFileName() );

	wxFileDialog dialog( 0, wxT("Choose a file"), absPath.AsChar(), file->GetFileName().AsChar(), 
		( String( TXT("*.") ) + path.GetExtension() ).AsChar(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

	if ( wxID_OK == dialog.ShowModal() )
	{
		const Char *string = dialog.GetPath().wc_str();
		m_files[ *(m_active.Begin()) ].m_file->Load();
		m_files[ *(m_active.Begin()) ].m_file->Save( string );

		ContentChange( wxEVT_NEW_EVENT );
	}
}

void CEdResourceView::CopyActive( bool isCopy )
{
	ASSERT( m_active.Size() >= 1 );

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		TSet< CDiskFile* > files;
		GetActive( files );

		TDynArray< String > fileNames;
		for( TSet< CDiskFile* >::iterator it=files.Begin(); it!=files.End(); it++ )
		{
			fileNames.PushBack( (*it)->GetAbsolutePath() );
		}

		// Serialize file names
		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		fileNames.Serialize( writer );

		CClipboardData* clipboardData = new CClipboardData( TXT("CopyPasteAsset"), buffer, isCopy );
		wxTheClipboard->SetData(clipboardData);
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
}

void CEdResourceView::OnCopyAsset( wxCommandEvent &event )
{
	CopyActive( true );
}

void CEdResourceView::OnCutAsset( wxCommandEvent &event )
{
	CopyActive( false );
}

String CEdResourceView::GetNameForPastedAsset( const String& origName, bool pasteAs ) const
{
	if ( origName.Empty() )
	{
		return String::EMPTY;
	}

	CFilePath path( origName );
	String result;

	if ( pasteAs )
	{
		result = path.GetFileName() + TXT("_copy");

		String msg = TXT("Write new file name for ") + String::Printf( TXT("%s"), result.AsChar() );

		if ( !InputBoxFileName( m_browser, TXT("Paste as"), msg, result, path.GetExtension() ) )
		{
			return String::EMPTY;
		}
	}
	else
	{
		result = path.GetFileNameWithExt();
	}

	return result;
}

void CEdResourceView::Paste( bool pasteAs )
{
	if ( !wxTheClipboard->Open() )
	{
		return;
	}

	// receive data from clipboard
	CClipboardData data( TXT("CopyPasteAsset") );
	if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
	{
		wxTheClipboard->GetData( data );

		CDirectory* dir = m_browser->GetCurrentDirectory();

		if ( dir )
		{
			// Deserialize file names
			CMemoryFileReader reader( data.GetData(), 0 );
			TDynArray< String > fileNames;
			fileNames.Serialize( reader );

			TDynArray< CResource* > res;
			TDynArray< String > names;

			for ( Uint32 i=0; i<fileNames.Size(); i++ )
			{
				String origName = fileNames[i];
				GDepot->ConvertToLocalPath( origName, origName );

				String pastedName = GetNameForPastedAsset( origName, pasteAs );
				if ( pastedName.Empty() )
				{
					continue;
				}

				String curDir;
				dir->GetDepotPath( curDir );

				if ( curDir + pastedName == origName )
				{ 
					GFeedback->ShowError( TXT("Cannot paste asset '%s' on itself"), origName.AsChar() );
					continue;
				}
				
				if ( CResource* r = GDepot->LoadResource( origName ) )
				{
					if ( m_browser->IsResourceOpenedInEditor( r ) && !data.IsCopy() )
					{
						GFeedback->ShowError( TXT("Cannot cut & paste asset '%s' as it's opened in editor.\nClose editor first."), origName.AsChar() );
						continue;
					}

					res.PushBack( r );
					names.PushBack( pastedName );
				}
			}
			
			if ( pasteAs )
			{
				SVersionControlWrapper::GetInstance().CopyAsFiles( dir, res, names );
			}
			else
			{
				SVersionControlWrapper::GetInstance().CopyFiles( dir, res );
			}

			if ( ! data.IsCopy() )
			{
				SVersionControlWrapper::GetInstance().CreateLinksForFiles( dir, res );
				SVersionControlWrapper::GetInstance().DeleteFiles( res );
			}
			else
			{
				ApplyPasteOnFiles( dir, names );
			}

			ContentChange( wxEVT_NEW_EVENT );
		}
		else
		{
			GFeedback->ShowError( TXT("Cannot paste files here. You need to paste them in a directory.") );
		}
	}

	wxTheClipboard->Close();
}

void CEdResourceView::OnPasteAsset( wxCommandEvent &event )
{
	Paste( false );
}

void CEdResourceView::OnPasteAsAsset( wxCommandEvent &event )
{
	Paste( true );
}

void CEdResourceView::OnRenameAsset( wxCommandEvent &event )
{
	TSet< CDiskFile* > files;
	GetActive( files );

	TDynArray< CResource* > res;
	TDynArray< String > names;

	for( TSet< CDiskFile* >::iterator it=files.Begin(); it!=files.End(); it++ )
	{
		CDiskFile* file = (*it);

		CFilePath path(file->GetFileName());
		String newFileName = path.GetFileName();

		if ( m_browser->IsResourceOpenedInEditor( file->GetResource() ) )
		{
			GFeedback->ShowError( TXT("Cannot rename asset '%s' as it's opened in editor.\nClose editor first."), file->GetFileName().AsChar() );
			continue;
		}

		if (!InputBoxFileName(m_browser, TXT("Rename"), TXT("Write new file name for ")+String::Printf(TXT("%s"), path.GetFileName().AsChar()), newFileName, path.GetExtension()))
		{
			// ERROR
			continue;
		}
		else if ( newFileName != path.GetFileName() )
		{
			String filePath = file->GetAbsolutePath();
			GDepot->ConvertToLocalPath(filePath, filePath);

			CResource* r = GDepot->LoadResource( filePath );
			if( r )
			{
				String oldFileName = path.GetFileName()+TXT(".")+path.GetExtension();
				
				if (newFileName == oldFileName) 
					continue;

				res.PushBack( r );
				names.PushBack(newFileName);
			}
		}
	}

	if ( res.Empty() == false )
	{
		// Check if the game resource is among the renamed files and get the index
		CResource* gameResource = GGame->GetGameResource() ? GGame->GetGameResource() : NULL;
		ptrdiff_t gameResIndex = res.GetIndex( gameResource );
		String gameResourcePath = gameResource ? gameResource->GetDepotPath() : String::EMPTY;

		// Do the rename
		SVersionControlWrapper::GetInstance().RenameFiles( res, names );

		// If the game resource was among the files, check if it was renamed
		if ( gameResIndex != -1 )
		{
			CFilePath newNamePath( names[gameResIndex] );
			CFilePath path( gameResourcePath );
			path.SetFileName( newNamePath.GetFileName() );
			CDiskFile* newFile = GDepot->FindFile( path.ToString() );

			// File changed, use new game
			if ( newFile )
			{
				GGame->SetupGameResourceFromFile( newFile->GetDepotPath() );
				wxTheFrame->UpdateGameResourceNameField();
			}
		}


		ContentChange( wxEVT_NEW_EVENT );
	}
	
}

void CEdResourceView::OnReloadAsset( wxCommandEvent& event )
{
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if (f->IsLoaded()) f->GetResource()->Reload(true);
		}
	}
}

Bool CEdResourceView::SyncAndOpenFile( const String &path )
{
	Uint32 start = 1;
	Uint32 end = 2;
	String value;
	TDynArray< String > parts;

	// Checking if the path starts with the perforce path // if it does we remove //Red_engine/Main.Lava/ and replaces that with z:/ maybe the perforce command "edit" could work here.
	if( path.BeginsWith( TXT("//") ) )
	{
		path.Slice( parts, TXT("Main.Lava") );
	}
	else
	{	
		path.Slice( parts, TXT(":") );
	}
	// Checking so that we actually have anything in this list. If not then it could break
	if( parts.Size() > 1 )
	{
		String npath = TXT("");
		String p4Path;
		if( SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path ) )
		{
			npath = p4Path + parts[1];
		}
		else
		{
			p4Path = TXT("z:");
			SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path );
			npath = p4Path + parts[1];
		}
		wxString pathString(npath.AsChar(), wxConvUTF8);
	
		if( GVersionControl->GetAttribute( npath.AsChar(), TXT("headAction"), value) && value!=TXT("delete") )
		{
			GFeedback->BeginTask( TXT("Getting the latest version..."), false);

			GFeedback->UpdateTaskProgress( start, end );
			if( GVersionControl->GetLatest( npath.AsChar() ) )
			{
				// Updating the progress
				GFeedback->UpdateTaskProgress(end,end);
			}
			GFeedback->UpdateTaskProgress(end,end);

			if( GFeedback->AskYesNo( TXT("Do you want to check out this file %s ?"), npath.AsChar() ) )
			{
				// Need to check out the asset here.
				if( !GVersionControl->CheckOut( npath.AsChar(), SChangelist::DEFAULT, true ) )
				{
					GFeedback->ShowMsg( TXT("Couldn't check out the source file."), npath.AsChar());
					GFeedback->EndTask();
					return false;
				}
				else
				{
					if( ! wxFileExists( pathString ) )
					{
						GFeedback->ShowMsg( TXT("Couldn't Open the source file."), String::Printf( TXT("%s \n\nMaybe the P4OpenResourcePath in the bin\\r4config\\User.ini file is wrong.\n\nBy default it is z: and your Perforce Workspace might point somewhere else?"), npath.AsChar() ).AsChar() );
						GFeedback->EndTask();
						return false;
					}
					else
					{
						OpenExternalFile( npath.AsChar() );
						GFeedback->EndTask();
						return true;
					}
				}
			}
			else
			{
				if( ! wxFileExists( pathString ) )
				{
					GFeedback->ShowMsg( TXT("Couldn't Open the source file."), String::Printf( TXT("%s \n\nMaybe the P4OpenResourcePath in the bin\\r4config\\User.ini file is wrong.\n\nBy default it is z: and your Perforce Workspace might point somewhere else?"), npath.AsChar() ).AsChar() );
					GFeedback->EndTask();
					return false;
				}
				else
				{
					OpenExternalFile( npath.AsChar() );
					GFeedback->EndTask();
					return true;
				}
			}
		}
		else
		{
			return false;
		}
	}
	else
	{	
		GFeedback->ShowMsg( TXT("Couldn't do anything with this path."), path.AsChar());
		return false;
	}
}

// Opens up the source file of selected asset
void CEdResourceView::OnOpenSource( wxCommandEvent& event )
{
	const DiskFileInfo& fileInfo = m_files[ *m_active.Begin() ];

	// First check the same folder for the same name .psd file and open that one
	// If that doesn't work then check the folder for PSD folder and then check the name if the same name exists of the CBitMapFile
	if( fileInfo.m_class->IsA< CBitmapTexture >() )
	{
		CDiskFile* f = m_files[ *m_active.Begin() ].m_file;
		f->Load();

		CBitmapTexture* bmp = Cast< CBitmapTexture >(f->GetResource());

		if( bmp != nullptr )
		{
			String npath = bmp->GetImportFile();
			String value;

			CFilePath fpath(npath);
			String assetName = fpath.GetFileName();
			String mpath = fpath.GetPathString();
						
			TDynArray< String > parts;
			npath.Slice( parts, TXT(".") );
			// Do some checking for the PSD file.
			String PSDPath = parts[0]+TXT(".psd");
			
			String absolutePSDPath = mpath+TXT("/psd");

			// First checking if the file exists in the root folder and then the psd folder. If it exists we open it.
			if( SyncAndOpenFile( PSDPath ) )
			{
				// Everything works fine
				return;
			}
			else
			{
				TDynArray< String > nparts;
				// Checking if the path starts with the perforce path // if it does we remove //Red_engine/Main.Lava/ and replaces that with z:/ maybe the perforce command "edit" could work here.
				if( absolutePSDPath.BeginsWith( TXT("//") ) )
				{
					absolutePSDPath.Slice( nparts, TXT("Main.Lava") );
				}
				else
				{	
					absolutePSDPath.Slice( nparts, TXT(":") );
				}
				// Checking so that we actually have anything in this list. If not then it could break
				if( parts.Size() > 1 )
				{
					String npath = TXT("");
					String p4Path;
					if( SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path ) )
					{
						npath = p4Path + nparts[1];
					}
					else
					{
						p4Path = TXT("z:");
						SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("P4OpenResourcePath"), p4Path );
						npath = p4Path + nparts[1];
					}

					TDynArray< String > files;
					if( GVersionControl->GetListOfFiles( npath+TXT("/*.psd"), files ) ) 
					{
						// If the list size is smaller than 2 it will have one path and we open that
						// Else bigger than 1 there are more .psd's in that folder and we do not open anyone of them
						if( files.Size() < 2 )
						{
							if( SyncAndOpenFile( files[0].AsChar() ) )
							{
								return;
							}
							else
							{
								GFeedback->ShowMsg( TXT("Couldn't Open the source file."), String::Printf( TXT("%s \n\nMaybe the P4OpenResourcePath in the bin\\r4config\\User.ini file is wrong.\n\nBy default it is z: and your Perforce Workspace might point somewhere else?"), PSDPath.AsChar() ).AsChar() );
							}
						}
						else
						{
							GFeedback->ShowMsg( TXT("Too many PSD's in the PSD folder "), npath.AsChar() );	
						}
					}
					else
					{
						GFeedback->ShowMsg( TXT("Couldn't Open the source file."), String::Printf( TXT("%s \n\nMaybe the P4OpenResourcePath in the bin\\r4config\\User.ini file is wrong.\n\nBy default it is z: and your Perforce Workspace might point somewhere else?"), PSDPath.AsChar() ).AsChar() );
					}
				}
			}
		}
	}

	// Need to add the functionality to open .re files 
	if( fileInfo.m_class->IsA< CMesh >() )
	{
		CDiskFile* f = m_files[ *m_active.Begin() ].m_file;
		f->Load();

		const CMesh* amesh = Cast< CMesh >(f->GetResource());

		if( amesh != nullptr)
		{
			// Getting the source file like .max .fbx etc..
			const String& npath = amesh->GetBaseResourceFilePath();
			// If we have nothing here we will not go in here.
			if( npath != TXT("") )
			{
				if( SyncAndOpenFile( npath ) )
				{
					// We found what we were looking for
				}	
			}
			else
			{
				GFeedback->ShowMsg( TXT("This asset has no info about source file (max,fbx,mb)."), amesh->GetFriendlyName().AsChar() );	
			}
		}
	}
}

// Opens up the .re file of selected asset or the TGA
void CEdResourceView::OnOpenResource( wxCommandEvent& event )
{
	const DiskFileInfo& fileInfo = m_files[ *m_active.Begin() ];

	// First check the same folder for the same name .psd file and open that one
	// If that doesn't work then check the folder for PSD folder and then check the name if the same name exists of the CBitMapFile
	if( fileInfo.m_class->IsA< CBitmapTexture >() )
	{
		CDiskFile* f = m_files[ *m_active.Begin() ].m_file;
		f->Load();

		CBitmapTexture* bmp = Cast< CBitmapTexture >(f->GetResource());

		if( bmp != nullptr )
		{
			String npath = bmp->GetImportFile();
			
			if( SyncAndOpenFile( npath ) )
			{
				// Opening the file
			}
			else
			{
				GFeedback->ShowMsg( TXT("Cant find this asset in Perforce."), npath.AsChar() );	
			}
		}
	}

	// Need to add the functionality to open .re files 
	if( fileInfo.m_class->IsA< CMesh >() )
	{
		CDiskFile* f = m_files[ *m_active.Begin() ].m_file;
		f->Load();

		const CMesh* amesh = Cast< CMesh >(f->GetResource());

		if( amesh != nullptr)
		{
			// Getting the source file like .max .fbx etc..
			String npath = amesh->GetImportFile();
			if( SyncAndOpenFile( npath ) )
			{
				// Opening the file	
			}
			else
			{
				GFeedback->ShowMsg( TXT("Cant find this asset in Perforce."), npath.AsChar() );	
			}
		}
	}
}

void CEdResourceView::OnRemapMaterials( wxCommandEvent &event )
{
	TDynArray< CDiskFile* > meshFiles;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if ( f )
			{
				CFilePath path( f->GetFileName() );
				String extension = path.GetExtension();

				if ( extension == TXT( "w2mesh" ) || extension == TXT( "redapex" ) || extension == TXT("redcloth") )
				{
					meshFiles.PushBack( f );
				}
			}
		}
	}

	if ( !meshFiles.Empty() )
	{
		CEdMeshMaterialRemapper remapper( m_browser );
		remapper.Execute( meshFiles );
	}


}

static Bool IsDeletableFromAssetBrowser( const String& filepath )
{
	CFilePath path( filepath );
	const String& ext = path.GetExtension();

	return !( ext == TXT("w2l")  ||
		ext == TXT("w2lg") );
}

void CEdResourceView::OnDeleteAsset( wxCommandEvent& event )
{
	if( m_active.Empty() )
    {
        return;
    }

    if ( ! GFeedback->AskYesNo( TXT("You are going to delete %d file(s)\n\nAre you sure you want to continue?\n"), m_active.Size() ) )
    {
        return;
    }

	for ( TSet< Uint32 >::iterator it = m_active.Begin(); it != m_active.End(); it++ )
	{
		if ( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;

			if ( !IsDeletableFromAssetBrowser( f->GetFileName() ) )
			{
				GFeedback->ShowError( TXT("Cannot delete '%s' using the Asset Browser"), f->GetFileName().AsChar() );
				continue;
			}
			
			if ( m_browser->IsResourceOpenedInEditor( f->GetResource() ) )
			{
				GFeedback->ShowError( TXT("Cannot delete asset '%s' as it's opened in editor.\nClose editor first."), f->GetFileName().AsChar() );
				continue;
			}
			
			if ( f->IsLocal() )
			{
				f->Delete(false);
			}
			else if ( f->IsCheckedOut() )
			{
				if ( f->Revert() && f->Delete(false, false) )
				{
					f->Submit();
				}
			}
			else if ( f->Delete(false, false) )
			{
				f->Submit();
			}
		}
	}
	//m_active->Delete();
	m_browser->UpdateResourceList();	
	Repaint( true );
}

void CEdResourceView::OnCheckoutAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );
	Uint32 start = 1;
	Uint32 end = 2;

	GFeedback->BeginTask( TXT("Checking Out..."), false);
	GFeedback->UpdateTaskProgress( start, end );
	bool ret = false;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->CheckOut() )
				ret = true;
		}
	if( ret )
	{
		ContentChange( wxEVT_CHECK_OUT_EVENT );
	}
	GFeedback->UpdateTaskProgress( end, end );
	GFeedback->EndTask();
};

void CEdResourceView::OnSubmitAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );
	
    TDynArray<CDiskFile *> filesToSubmit;

	for( TSet< Uint32 >::iterator it = m_active.Begin(); it != m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *file = m_files[ *it ].m_file;
			if( file->IsEdited() )
			{
				file->Save();
                filesToSubmit.PushBack( file );
			}
		}

    if ( GVersionControl->Submit( filesToSubmit ) )
	{
		ContentChange( wxEVT_SUBMIT_EVENT );
	}
};

void CEdResourceView::OnRevertAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );
	bool ret = true;
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsEdited() && !f->Revert() )
				ret = false;
			else
			{
				if ( f->IsLoaded() )
				{
					f->GetResource()->Reload(true);
					f->UpdateThumbnail();
				}
			}
		}

	if ( !ret )
	{
		wxMessageDialog dialog(0, wxT(ERROR_REVERT), wxT("Error"), wxOK | wxICON_ERROR);
		dialog.ShowModal();
	};

	ContentChange( wxEVT_REVERT_EVENT );
};

void CEdResourceView::OnEditAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );
	TSet< Uint32 > activeResources = m_active;

	if( m_browser->IsCurrentTabFlat() )
	{
		CDiskFile *df = GetActive();
		if( df )
		{
			ChangeDirectory( df->GetDirectory(), df );
			return;
		}
	}

	OnLoadAsset( event );

	if( activeResources.Size() == 1 && m_files[ *activeResources.Begin() ].m_isDirectory )
	{
		ChangeDirectory( m_files[ *activeResources.Begin() ].m_directory, m_browser->GetCurrentDirectory() );
		return;
	}
	
	for( TSet< Uint32 >::iterator it=activeResources.Begin(); it!=activeResources.End(); it++ )
	{
		Uint32 activeFileIdx = *it;
		if( !m_files[ activeFileIdx ].m_isDirectory )
		{
			CDiskFile *f = m_files[ activeFileIdx ].m_file;

			if ( !f->IsLoaded() )
			{
				// Unable to load
				continue;
			}

			// Try to open existing resource editor
			CResource* res = f->GetResource();

			m_browser->EditAsset( res );
		}
	}
}

void CEdResourceView::OnAddAsset( wxCommandEvent &event )
{
//	ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsLocal() )
				f->Add();
		}

	wxCommandEvent addEvent( wxEVT_ADD_EVENT, GetId() );
	addEvent.SetEventObject( this );
	GetEventHandler()->ProcessEvent( addEvent );
}

void CEdResourceView::OnRefreshAsset( wxCommandEvent &event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			f->GetStatus();
		}

	m_browser->UpdateResourceList();
	Repaint( true );
}

void CEdResourceView::OnAssetHistory( wxCommandEvent &event )
{
//	ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsEdited() )
			{
				TDynArray< THashMap< String, String > > history;
				GVersionControl->FileLog( *f, history );
			}
		}
}

void CEdResourceView::OnSyncAsset( wxCommandEvent &event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	Uint32 start = 1;
	Uint32 end = 2;

	GFeedback->BeginTask( TXT("Syncing..."), false);
	GFeedback->UpdateTaskProgress( start, end );
	
	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			
			f->Sync();

			if ( !GFileManager->GetFileSize( f->GetAbsolutePath() ) )
			{
				if (f->IsLoaded())
				{
					CResource* res = f->GetResource();
					f->Rebind(NULL);
					res->Discard();
				}

				if (f->GetDirectory())
				{
					f->GetDirectory()->DeleteFile(*f);
				}
			}
			else
			{	
				if (f->IsLoaded()) f->GetResource()->Reload(true);
				f->UpdateThumbnail();
			}
		}
	GFeedback->UpdateTaskProgress( end, end );
	GFeedback->EndTask();
	ContentChange( wxEVT_SYNC_EVENT );
}

void CEdResourceView::OnExportAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	OnLoadAsset( event );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsLoaded() )
			{
				CClass* importClass = f->GetResource()->GetClass();
				ASSERT( importClass );
				ASSERT( importClass->IsBasedOn( ClassID< CResource >() ) );

				// Ask browser to the the re import active resource
				m_browser->ExportResource( f->GetResource() );
			}
		}
}

void CEdResourceView::OnCopyPathToClipboard( wxCommandEvent &event )
{
	ASSERT( !m_active.Empty() );

	String resourcePath;

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			resourcePath += f->GetDepotPath() + TXT("\r\n");
		}
	}

	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData( new wxTextDataObject( resourcePath.AsChar() ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
}

void CEdResourceView::OnGotoResource( wxCommandEvent &event )
{
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			
			wxTheFrame->GetAssetBrowser()->SelectFile( f->GetDepotPath() );
			break;
		}
	}
}

void CEdResourceView::OnImport( wxCommandEvent& event )
{
	// Extract import class, TODO: find a better way

	CClass* importClass = m_importClasses[ event.GetId() - ID_IMPORT_ASSET_FIRST ];
	ASSERT( importClass );
	ASSERT( importClass->IsBasedOn( ClassID< CResource >() ) );

	// Ask browser to the the import job !
	m_browser->ImportResources( importClass );
}

void CEdResourceView::OnReimportAsset( wxCommandEvent& event )
{
	//ASSERT( m_active );
	ASSERT( !m_active.Empty() );

	OnLoadAsset( event );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;
			if( f->IsLoaded() )
			{
				CClass* importClass = f->GetResource()->GetClass();
				ASSERT( importClass );
				ASSERT( importClass->IsBasedOn( ClassID< CResource >() ) );

				// Ask browser to the the reimport active resource
				m_browser->ReimportResource( f->GetResource() );
			}
		}
}

void CEdResourceView::OnCreate( wxCommandEvent& event )
{
	// Extract factory class, TODO: find a better way
	CClass* factoryClass = m_factoryClasses[ event.GetId() - ID_CREATE_ASSET_FIRST ];
	ASSERT( factoryClass );

	// Ask browser to the the import job !
	m_browser->CreateResourceFromFactory( factoryClass->GetDefaultObject< IFactory >() );
}

void CEdResourceView::OnCreateFavorite( wxCommandEvent& event )
{
	String className = m_favClasses[ event.GetId() - ID_CREATE_FAV_FIRST ];
	if ( CClass* resClass = SRTTI::GetInstance().FindClass( CName( className ) ) )
	{
		if ( IFactory* factory = IFactory::FindFactory( resClass ) )
		{
			m_browser->CreateResourceFromFactory( factory->GetClass()->GetDefaultObject< IFactory >() );
		}
	}
}

void CEdResourceView::OnCreateDirectory( wxCommandEvent& event )
{
	TDynArray<CDirectory*> dirs;
	dirs.PushBack( m_browser->GetActiveDirectory() );
	CContextMenuDir* ctx = new CContextMenuDir( dirs );
	wxCommandEvent fakeevent = event;
	fakeevent.m_callbackUserData = ctx;
	wxTheFrame->GetAssetBrowser()->OnCreateDirectory( fakeevent );
	delete ctx;
}

void CEdResourceView::OnRefresh( wxCommandEvent &event )
{
	TDynArray< DiskFileInfo > :: iterator info;
	for (info = m_files.Begin(); info != m_files.End(); info++)
	{
		if ( !info->m_isDirectory )
			info->m_file->GetStatus();
	}
	m_browser->UpdateResourceList( true );
	Repaint( true );
}

void CEdResourceView::UpdateThumbnailInFile( CDiskFile* file, const SThumbnailSettings* settings )
{
	Bool wasLoaded = file->IsLoaded();

	if ( !wasLoaded )
	{
		if ( !file->Load() )
		{
			WARN_EDITOR( TXT("Error loading resource, thumbnail not updated") );
			return;
		}
	}

	if ( settings && ( settings->m_flags & TF_OutputIcon ) )
	{
		TDynArray< CThumbnail* > thumbnails;
		if ( !file->CreateThumbnail( thumbnails, settings ) )
		{
			WARN_EDITOR( TXT("Couldn't create thumbnail") );
		}

		String outputPath = String::Printf( TXT("%s_%ix%i.png"), ( settings->m_iconOutputPath + file->GetFileName() ).AsChar(), settings->m_width, settings->m_height );
		CEdItemsThumbnailGenerator::OutputToPNG( thumbnails, settings->m_width, settings->m_height, outputPath );
	}
	else if ( file->UpdateThumbnail( settings ) )
	{
		if ( !file->Save() )
		{
			WARN_EDITOR( TXT("Resource not saved, thumbnail not updated") );
		}
	}

	if ( !wasLoaded )
	{
		file->Unload();
	}
}

void CEdResourceView::OnUpdateThumbnail( wxCommandEvent& event )
{
	ASSERT( !m_active.Empty() );

	SThumbnailSettings settings;

	// default to empty settings - CDiskFile::UpdateThumbnail will use last used setting when available
	SThumbnailSettings* settingsTuUse = nullptr; 

	if ( m_active.Size() == 1 )
	{
		const DiskFileInfo& fileInfo = m_files[ *m_active.Begin() ];

		if ( fileInfo.m_class->IsA< CMeshTypeResource >() || fileInfo.m_class->IsA< CEntityTemplate >() )
		{
			// For single selection and resources that support this - show the dialog and pass the user-selected settings
			settingsTuUse = &settings;

			Bool enableEditorCamera   = false;
			Bool enableLastUsedCamera = !fileInfo.m_file->GetThumbnails().Empty();

			wxWindow* editor;
			IEditorPreviewCameraProvider::Info editorCamInfo;
			if ( m_browser->m_resourceEditors.Find( fileInfo.m_file->GetResource(), editor ) )
			{
				IEditorPreviewCameraProvider* camProvider = dynamic_cast< IEditorPreviewCameraProvider* >( editor );
				ASSERT ( camProvider ); // supported resource editors should inherit from IEditorPreviewCameraProvider
				enableEditorCamera = true;
				editorCamInfo = camProvider->GetPreviewCameraInfo();
			}

			const TDynArray< CThumbnail* >& existingThumbs = fileInfo.m_file->GetThumbnails();

			CEdUpdateThumbnailDialog dialog( this, enableEditorCamera, enableLastUsedCamera );
			CEdUpdateThumbnailDialog::CameraType camType;
			Int32 flags = existingThumbs.Empty() ? 0 : existingThumbs[0]->GetFlags();
			Uint32 iconSize;
			Color bgColor;

			if ( !dialog.Execute( camType, flags, iconSize ) )
			{
				return;
			}

			settings.m_flags = flags;
			if ( flags & TF_OutputIcon )
			{
				settings.m_item = true;
				settings.m_height = iconSize;
				settings.m_width = iconSize;
				if ( flags & TF_SetBackgroundColor )
				{
					settings.m_backgroundColor = Vector::ZEROS;
				}

				CEdItemsThumbnailGenerator::ChooseOutputDir( this, settings.m_iconOutputPath );
			}
			else
			{
				settings.m_item = false;
			}

			switch ( camType )
			{
			case CEdUpdateThumbnailDialog::CT_Auto:
				{
					settings.m_customSettings = TCS_None;
				}
				break;
			case CEdUpdateThumbnailDialog::CT_FromEditor:
				{
					settings.m_cameraPosition = editorCamInfo.m_cameraPostion;
					settings.m_cameraRotation = editorCamInfo.m_cameraRotation;
					settings.m_lightPosition  = editorCamInfo.m_lightPosition;
					settings.m_cameraFov      = editorCamInfo.m_cameraFov;
					if ( flags & TF_CopyEnvironment )
					{
						settings.m_environmentPath = editorCamInfo.m_envPath;
					}
					settings.m_customSettings = TCS_All;
				}
				break;
			case CEdUpdateThumbnailDialog::CT_LastUsed:
				{
					ASSERT( !existingThumbs.Empty() );
					settings.m_cameraPosition = existingThumbs[0]->GetCameraPosition();
					settings.m_cameraRotation = existingThumbs[0]->GetCameraRotation();
					settings.m_lightPosition  = existingThumbs[0]->GetSunRotation().Yaw;
					settings.m_cameraFov      = existingThumbs[0]->GetCameraFov();
					settings.m_customSettings = TCS_All;
				}
				break;
			}
		}
	}

	for ( auto it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		const DiskFileInfo& fileInfo = m_files[ *it ];
		if ( fileInfo.m_isDirectory )
		{
			continue;
		}

		UpdateThumbnailInFile( fileInfo.m_file, settingsTuUse );
	}

	CalculateLayout();
	Repaint( true );
}

void CEdResourceView::OnDeleteThumbnail( wxCommandEvent& event )
{
	ASSERT( !m_active.Empty() );

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;

			Bool wasLoaded = f->IsLoaded();

			if ( !wasLoaded )
			{
				if ( !f->Load() )
				{
					WARN_EDITOR( TXT("Error loading resource, thumbnail not updated") );
					continue;
				}
			}

			f->RemoveThumbnail();
			if ( !f->Save() )
			{
				WARN_EDITOR( TXT("Resource not saved, thumbnail not updated") );
			}

			if ( !wasLoaded )
			{
				f->Unload();
			}
		}

		Repaint( true );
}

void CEdResourceView::OnFindInWorld( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );
	String depotPath = file->GetDepotPath();

	if ( !file->IsLoaded() )
	{
		file->Load();
	}

	if ( CResource* resource = file->GetResource() )
	{
		CEdResourceFinder::ShowForResource( resource );
	}
}

void CEdResourceView::OnFindInWorldMesh( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );
	String depotPath = file->GetDepotPath();

	if ( !file->IsLoaded() )
	{
		file->Load();
	}

	if ( CResource* resource = file->GetResource() )
	{
		CEdResourceFinder::ShowForResource( resource, CNAME( CMeshComponent ) );
	}
}

void CEdResourceView::OnFindInWorldStatic( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );
	String depotPath = file->GetDepotPath();

	if ( !file->IsLoaded() )
	{
		file->Load();
	}

	if ( CResource* resource = file->GetResource() )
	{
		CEdResourceFinder::ShowForResource( resource, CNAME( CStaticMeshComponent ) );
	}
}

void CEdResourceView::OnFindInWorldRigid( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );
	String depotPath = file->GetDepotPath();

	if ( !file->IsLoaded() )
	{
		file->Load();
	}

	if ( CResource* resource = file->GetResource() )
	{
		CEdResourceFinder::ShowForResource( resource, CNAME( CRigidMeshComponent ) );
	}
}

void CEdResourceView::OnFindInWorldEncounter( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	if ( !GGame->GetActiveWorld().IsValid() )
	{
		wxMessageBox( wxT("There is no loaded world"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK );
		return;
	}

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );
	String depotPath = file->GetDepotPath();

	if ( !file->IsLoaded() )
	{
		file->Load();
	}

	ClearEntityList( TXT("Encounters") );
	Bool foundEncounters = false;

	CSpawnTree* spawnTree = Cast< CSpawnTree >( file->GetResource() );
	if ( spawnTree != nullptr )
	{
		TDynArray< CLayerInfo* > layers;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( layers, false, true );
		GFeedback->BeginTask( TXT("Scanning layers"), true );
		GFeedback->UpdateTaskInfo( TXT("Scanning layers...") );

		for ( Uint32 i=0; i < layers.Size(); ++i )
		{
			CLayerInfo* layerInfo = layers[i];
			GFeedback->UpdateTaskProgress( i, layers.Size() );

			Bool unload = !layerInfo->IsLoaded();
			if ( unload ) layerInfo->SyncLoad( LayerLoadingContext() );

			if ( layerInfo->GetLayer() != nullptr )
			{
				for ( CEntity* entity : layerInfo->GetLayer()->GetEntities() )
				{
					CEncounter* encounter = Cast< CEncounter >( entity );
					if ( encounter != nullptr )
					{
						TDynArray< IEdSpawnTreeNode* > stNodes;
						stNodes.PushBack( encounter );

						while ( !stNodes.Empty() )
						{
							IEdSpawnTreeNode* stNode = stNodes.Front();
							stNodes.RemoveAt( 0 );

							if ( stNode->AsCObject() )
							{
								if ( CSpawnTree* st = Cast< CSpawnTree >( stNode->AsCObject()->GetParent() ) )
								{
									if ( st->GetDepotPath() == spawnTree->GetDepotPath() )
									{
										AddEntityToEntityList( TXT("Encounters"), entity );
										unload = false;
										foundEncounters = true;
										break;
									}
								}
							}

							for ( Int32 i = 0; i < stNode->GetNumChildren(); ++i )
							{
								stNodes.PushBack( stNode->GetChild( i ) );
							}
						}
					}
					if ( GFeedback->IsTaskCanceled() )
					{
						break;
					}
				}
			}

			if ( unload ) layerInfo->SyncUnload();

			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}
		}
		GFeedback->EndTask();
	}

	if ( foundEncounters )
	{
		ShowEntityList( TXT("Encounters") ) ;
	}
	else
	{
		wxMessageBox( wxT("No encounters were found that use this entity template"), wxT("Search"), wxOK|wxCENTRE );
	}
}

void CEdResourceView::OnGenerateGloss( wxCommandEvent &event )
{
	ASSERT( m_active.Size() == 1 );

	CDiskFile *file = m_files[ *(m_active.Begin()) ].m_file;
	ASSERT( file );

	if ( !file->IsLoaded() )
	{
		file->Load();
	}
	CResource* resource = file->GetResource();
	if ( resource )
	{
		CBitmapTexture* bmp = Cast< CBitmapTexture >( resource );
		if ( bmp->GetSourceData() )
		{
			const_cast< CBitmapTexture::MipMap& >( bmp->GetMips()[0] ).m_data.Load();
			TDynArray<Uint8> dat;
			dat.Resize( bmp->GetHeight() * bmp->GetWidth() * 4 );
			bmp->GetSourceData()->FillBufferTrueColor( dat.TypedData(), bmp->GetWidth(), bmp->GetHeight() );
			Uint8* data = dat.TypedData();

			CBitmapTexture::MipMap targetMip( bmp->GetWidth(), bmp->GetHeight(), bmp->GetWidth() * 4 );
			CBitmapTexture::MipMap targetMip2( bmp->GetWidth(), bmp->GetHeight(), bmp->GetWidth() * 4 );

			Uint8* targetData = (Uint8*)targetMip.m_data.GetData();
			Uint8* targetData2 = (Uint8*)targetMip2.m_data.GetData();

			Int32 width = targetMip.m_width;
			Int32 height = targetMip.m_height;
			{
				for ( Int32 x = 0; x < width; x ++ )
				{
					for ( Int32 y = 0; y < height; y ++ )
					{
						Vector averageNormal = Vector::ZEROS;

						for ( Int32 dy = -1; dy < 2; dy ++ )
						{
							for ( Int32 dx = -1; dx < 2; dx ++ )
							{
								if ( ( x + dx >= 0 ) && ( y + dy >= 0 ) && ( x + dx < width ) && ( y + dy < height ) )
								{
									// "sample"
									Vector normal = Vector::ZEROS;
									normal.X = (((Float)(data[ (x + dx + (width * (y + dy ) ) ) * 4 + 2] - 127))/255.0f)*(((Float)(data[ (x + dx + (width * (y + dy ) ) ) * 4 + 3] - 127))/255.0f);
									normal.Y = ((Float)(data[ (x + dx + (width * (y + dy ) ) ) * 4 + 1] - 127))/255.0f;
									normal.Z = sqrtf( 1.0f - normal.X * normal.X - normal.Y * normal.Y );
									normal.Normalize3();
									normal.W = 1.0f;

									const Float sigma = 0.5f;

									Float v = 2.0f*sigma*sigma;
									Vector p = Vector( dx, dy, 0, 0 );
									Float dist = p.SquareMag2();
									Float gauss = expf(-dist/v)/(M_PI*v);

									averageNormal += normal * gauss;
								}
							}
						}

						averageNormal /= averageNormal.W;
						Float len = averageNormal.Mag3();
						// Toksvig Factor
						Float ft = (len/(len + 50.0f * (1.0f-len)))*255.0f;

						targetData[ (x + (width *y) ) * 4 ]		= Max<Uint8>((Uint8)ft, 1);
						targetData[ (x + (width *y) ) * 4 + 1]  = Max<Uint8>((Uint8)ft, 1);
						targetData[ (x + (width *y) ) * 4 + 2]  = Max<Uint8>((Uint8)ft, 1);
						targetData[ (x + (width *y) ) * 4 + 3]  = 255;
					}

				}
			}

			{
				for ( Int32 x = 0; x < width; x ++ )
				{
					for ( Int32 y = 0; y < height; y ++ )
					{
						Vector average = Vector::ZEROS;

						for ( Int32 dy = -4; dy < 5; dy ++ )
						{
							for ( Int32 dx = -4; dx < 5; dx ++ )
							{
								if ( ( x + dx >= 0 ) && ( y + dy >= 0 ) && ( x + dx < width ) && ( y + dy < height ) )
								{
									// "sample"
									Vector sample = Vector::ZEROS;
									sample.X = ((Float)(targetData[ (x + dx + (width * (y + dy ) ) ) * 4]))/255.0f;
									sample.W = 1.0f;

									const Float sigma = 2.0f;

									Float v = 2.0f*sigma*sigma;
									Vector p = Vector( dx, dy, 0, 0 );
									Float dist = p.SquareMag2();
									Float gauss = expf(-dist/v)/(M_PI*v);

									average += sample * gauss;
								}
							}
						}
						average /= average.W;
						Float ft = average.X * 255.0f;

						targetData2[ (x + (width *y) ) * 4 ]		= (Uint8)ft;
						targetData2[ (x + (width *y) ) * 4 + 1]  = (Uint8)ft;
						targetData2[ (x + (width *y) ) * 4 + 2]  = (Uint8)ft;
						targetData2[ (x + (width *y) ) * 4 + 3]  = 255;
					}

				}
			}


			{
				CSourceTexture* sourceTexture = CreateObject<CSourceTexture>( (CObject*)NULL );
				sourceTexture->Init( bmp->GetWidth(), bmp->GetHeight(), TRF_TrueColor );
				sourceTexture->CreateFromMip( targetMip2 );

				CBitmapTexture* texture = CreateObject<CBitmapTexture>( (CObject*)NULL );
				texture->InitFromSourceData( sourceTexture, CName( TEXT("WorldDiffuse") ) );

				CFilePath path( file->GetFileName() );
				path.SetFileName( path.GetFileName() + TXT("_g") );
				texture->SaveAs( file->GetDirectory(), path.GetFileNameWithExt() );
				if ( texture->GetFile() )
				{
					texture->GetFile()->UpdateThumbnail();
				}
			}

		}
	}
}

void CEdResourceView::FillMenu( DiskFileInfo *clicked, wxMenu &menu )
{
	if ( m_active.Empty() )
	{
		menu.Append( ID_REFRESH, TEXT("Refresh all") );
		menu.AppendSeparator();
		menu.Append( ID_PASTE_ASSET, TEXT("Paste") );
		menu.Append( ID_PASTE_AS_ASSET, TEXT("Paste as...") );
		menu.AppendSeparator();
		return;
	}

	Bool edited = false;
	Bool local = false;
	Bool notLoaded = false;
	Bool loaded = false;
	Bool normal = false;
	Bool canBeLoaded = true;
	Bool readonly = false;
	Bool isBitmapTexture = false; // TODO: move this to batchers menu

	TDynArray< CDiskFile* > files;
	TDynArray< CDirectory* > dirs;

	for( TSet< Uint32 >::iterator it=m_active.Begin(); it!=m_active.End(); it++ )
	{
		if( !m_files[ *it ].m_isDirectory )
		{
			CDiskFile *f = m_files[ *it ].m_file;

			if( f->IsEdited() )
				edited = true;
			else
			if( f->IsLocal() )
				local = true;
			else
				normal = true;
			if( !f->IsLoaded() )
				notLoaded = true;
			else
				loaded = true;
			CFilePath path( f->GetFileName() );
			if( path.GetExtension() == TXT("w2l") || path.GetExtension() == TXT("w2w") )
				canBeLoaded = false;

			if( path.GetExtension() == TXT("xbm") )
				isBitmapTexture = true;

            Uint32 attribs = GetFileAttributes( f->GetAbsolutePath().AsChar() );
            readonly = readonly || (attribs & FILE_ATTRIBUTE_READONLY);

			files.PushBack( m_files[ *it ].m_file );
		}
		else
		{
			dirs.PushBack( m_files[ *it ].m_directory );
		}
	}

	if ( !files.Empty() )
	{
		wxMenu *filesMenu;
		if( dirs.Empty() )
			filesMenu = &menu;
		else
		{
			filesMenu = new wxMenu();
			menu.AppendSubMenu( filesMenu, TXT("File") );
		}

		if ( edited )
		{
			filesMenu->Append( ID_REVERT_ASSET, TEXT("Revert") );
			filesMenu->Append( ID_SUBMIT_ASSET, TEXT("Submit") );
			filesMenu->Append( ID_ASSET_HISTORY, TEXT("File's history"));
		}
			
		if ( local )
		{
			filesMenu->Append( ID_ADD_ASSET, TEXT("Add") );			
		}
			
		// clicked->m_file->IsCheckedIn();
		if( normal )
		{
			filesMenu->Append( ID_SYNC_ASSET, TEXT("Sync") );
			filesMenu->Append( ID_CHECKOUT_ASSET, TEXT("Checkout") );
			filesMenu->Append( ID_ASSET_HISTORY, TEXT("File's history"));
		}

		filesMenu->Append( ID_DELETE_ASSET, TEXT("Delete") );
		filesMenu->Append( ID_REFRESH_ASSET, TEXT("Refresh") );

		// Unloaded resource, load it
		if ( notLoaded && canBeLoaded )
		{
			// Show load option
			filesMenu->Append( ID_LOAD_ASSET, TEXT("Load from disk") );
		}
			
		if( loaded && canBeLoaded )
		{
			// Resource options
			filesMenu->Append( ID_EDIT_ASSET, TEXT("Edit") );
			filesMenu->AppendSeparator();
			filesMenu->Append( ID_SAVE_ASSET, TEXT("Save") );
		}

		if ( canBeLoaded )
		{
			filesMenu->Append( ID_EXPORT_ASSET, TEXT("Export Asset") );
			filesMenu->AppendSeparator();
			filesMenu->Append( ID_UPDATE_THUMBNAIL, TEXT("Update thumbnail") );
			filesMenu->Append( ID_DELETE_THUMBNAIL, TEXT("Delete thumbnail") );
		}

		filesMenu->Append( ID_REIMPORT_ASSET, TEXT("Reimport resources") );
		filesMenu->Append( ID_PATH_TO_CLIPBOARD, TEXT("Copy path to clipboard") );

		if ( wxTheFrame->GetAssetBrowser()->IsCurrentPageSearchType() )
		{
			filesMenu->Append( ID_GOTO_RESOURCE, TEXT("Go to resource home") );
		}

		if ( 1 == m_active.Size() )
		{
			filesMenu->AppendSeparator();
			filesMenu->Append( ID_SAVE_AS_ASSET, TEXT("Save as...") );
			filesMenu->AppendSeparator();
			wxMenu *findSomethingInWorldMenu = new wxMenu();
			filesMenu->AppendSubMenu( findSomethingInWorldMenu, TEXT("Find in world..." ) );
			findSomethingInWorldMenu->Append( ID_FIND_IN_WORLD, TEXT("Find in world anything") );
			findSomethingInWorldMenu->Append( ID_FIND_IN_WORLD_MESH, TEXT("Find in world a mesh component") );
			findSomethingInWorldMenu->Append( ID_FIND_IN_WORLD_STATIC, TEXT("Find in world a static mesh component") );
			findSomethingInWorldMenu->Append( ID_FIND_IN_WORLD_RIGID, TEXT("Find in world a rigid mesh component") );
			if ( isBitmapTexture )
			{
				filesMenu->Append( ID_GENERATE_GLOSS, TEXT("Generate gloss texture") );
			}

			if ( isBitmapTexture )
			{
				filesMenu->Append( ID_GENERATE_GLOSS, TEXT("Generate gloss texture") );
			}
		}

		filesMenu->AppendSeparator();
		filesMenu->Append( ID_RELOAD_ASSET, TEXT("Reload") );
		if ( 1 == m_active.Size() ) filesMenu->Append( ID_RENAME_ASSET, TEXT("Rename") );
		filesMenu->Append( ID_COPY_ASSET, TEXT("Copy") );
		filesMenu->Append( ID_CUT_ASSET, TEXT("Cut") );
			
		// Opens up the source file of selected asset. Do this only for Mesh and Texture.
		const DiskFileInfo& fileInfo = m_files[ *m_active.Begin() ];

		if ( fileInfo.m_class )
		{
			if (fileInfo.m_class->IsA< CMesh >() || fileInfo.m_class->IsA< CBitmapTexture >())
			{
				filesMenu->AppendSeparator();
				filesMenu->Append( ID_OPEN_SOURCE_FILE, TEXT("Open Source (PSD, MAX)") );	
				filesMenu->Append( ID_OPEN_RESOURCE_FILE, TEXT("Open Resource (TGA, RE)") );
				filesMenu->AppendSeparator();
			}
			else if ( fileInfo.m_class->IsA< CSpawnTree >() && 1 == m_active.Size() )
			{
				filesMenu->AppendSeparator();
				filesMenu->Append( ID_FIND_IN_WORLD_ENCOUNTER, TEXT("Find encounters that use this") );
				filesMenu->AppendSeparator();
			}
		}

#ifdef DEBUG
        wxMenuItem *item = filesMenu->AppendCheckItem( ID_TOGGLE_READONLY, TEXT("Readonly"));
        item->Check( readonly );
#endif

		if( dirs.Empty() )
			menu.AppendSeparator();
	}

	if ( dirs.Size() )
	{
		wxMenu *dirsMenu;
		if( files.Empty() )
			dirsMenu = &menu;
		else
		{
			dirsMenu = new wxMenu();
			menu.AppendSubMenu( dirsMenu, TXT("Directory") );
		}

		m_browser->CreateDirectoryContextMenu( *dirsMenu, dirs );

		if( files.Empty() )
			menu.AppendSeparator();
	}

	m_browser->CreateBatchersContextMenu( menu, dirs, files );
}

void CEdResourceView::OnSubmit( wxCommandEvent &event )
{
	const TFiles & content = m_browser->GetActiveDirectory()->GetFiles();
	TDynArray< CDiskFile * > files;
	TSet< CDiskFile * > chosen;

	GVersionControl->Opened( files );
	TFiles::const_iterator i;
	for (i = content.Begin(); i != content.End(); i++)
	{
		chosen.Insert( *i );
	}
	GVersionControl->Submit( files, chosen );
	ContentChange( wxEVT_SUBMIT_EVENT );
}

void CEdResourceView::OnRevert( wxCommandEvent &event )
{
	TDynArray< CDiskFile * > files;
	TDynArray< DiskFileInfo > :: iterator i;
	for ( i = m_files.Begin(); i != m_files.End(); i++ )
	{
		i->m_file->GetStatus();
		if ( i->m_file->IsEdited() )
		{
			files.PushBack( i->m_file );
		}
	}
	GVersionControl->Revert( files );
	ContentChange( wxEVT_REVERT_EVENT );
}

void CEdResourceView::OnAdd( wxCommandEvent &event )
{
	TDynArray< DiskFileInfo > :: iterator i;
	for ( i = m_files.Begin(); i != m_files.End(); i++ )
	{
		i->m_file->GetStatus();
		if ( i->m_file->IsLocal() )
		{
			i->m_file->Add();
		}
	}
	ContentChange( wxEVT_ADD_EVENT );
}

void CEdResourceView::OnDelete( wxCommandEvent &event )
{
	TDynArray< DiskFileInfo > :: iterator i;
	for ( i = m_files.Begin(); i != m_files.End(); i++ )
	{
		i->m_file->GetStatus();
		if ( i->m_file->IsCheckedIn() )
		{
			i->m_file->Delete();
		}
	}
	ContentChange( wxEVT_DELETE_EVENT );
}

void CEdResourceView::OnCheckOut( wxCommandEvent &event )
{
	TDynArray< DiskFileInfo > :: iterator i;
	for ( i = m_files.Begin(); i != m_files.End(); i++ )
	{
		i->m_file->GetStatus();
		if ( i->m_file->IsCheckedIn() )
		{
			i->m_file->SilentCheckOut();
		}
	}
	ContentChange( wxEVT_CHECK_OUT_EVENT );
}

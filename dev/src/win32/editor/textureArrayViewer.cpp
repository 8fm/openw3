#include "build.h"
#include "textureArrayViewer.h"
#include "wxThumbnailImageLoader.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/textureArray.h"
#include "../../common/engine/material.h"
#include "../../common/core/versionControl.h"

CEdTextureArrayViewer::CEdTextureArrayViewer( wxWindow* parent, CTextureArray* textureArray )
	: wxSmartLayoutFrame( parent, TXT( "Texture Array Viewer" ) )
	, m_backupValid( false )
	, m_textureArray( textureArray )
{
	m_TAVPanel			= wxXmlResource::Get()->LoadPanel( this, "mTextureArrayViewerPanel");

	m_pcSizeList = XRCCTRL( *this, "pcDownscaleBiasChoice", wxChoice );
	m_xboneSizeList = XRCCTRL( *this, "xboneDownscaleBiasChoice", wxChoice );
	m_ps4SizeList = XRCCTRL( *this, "ps4DownscaleBiasChoice", wxChoice );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );
	sizer->Add( m_TAVPanel, 1, wxEXPAND );

	m_gridPanel = XRCCTRL( *this, "m_gridPanel", wxPanel );

	m_slotTxt = XRCCTRL( *this, "m_TASlotStaticTxt", wxStaticText );
	
	XRCCTRL( *this, "m_TAAddButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnAddTexture ), NULL, this );
	XRCCTRL( *this, "m_TACloseSaveButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnCloseAndSave ), NULL, this );
	XRCCTRL( *this, "m_TACloseButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnCloseSavePrompt ), NULL, this );
	XRCCTRL( *this, "pcDownscaleBiasChoice", wxChoice )->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTextureArrayViewer::OnChoiceListChanged ), NULL, this );
	XRCCTRL( *this, "xboneDownscaleBiasChoice", wxChoice )->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTextureArrayViewer::OnChoiceListChanged ), NULL, this );
	XRCCTRL( *this, "ps4DownscaleBiasChoice", wxChoice )->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTextureArrayViewer::OnChoiceListChanged ), NULL, this );

	m_searchButton = XRCCTRL( *this, "m_TASearchButton", wxButton );	
	m_searchButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnSearchTexture ), NULL, this );
	m_replaceButton = XRCCTRL( *this, "m_TAReplaceButton", wxButton );
	m_replaceButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnReplaceTexture ), NULL, this );
	m_deleteButton = XRCCTRL( *this, "m_TARemoveButton", wxButton );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnRemoveTexture ), NULL, this );
	m_shiftButton = XRCCTRL( *this, "m_TAShiftButton", wxButton );
	m_shiftButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTextureArrayViewer::OnShiftTextures ), NULL, this );

	m_grid = new CEdTextureArrayGrid( m_gridPanel, wxBORDER_SUNKEN );
	m_grid->SetHook( this );
	m_grid->SetAllowEdit( true );
	m_grid->SetMultiSelection( true );
	wxBoxSizer* gridSizer = static_cast<wxBoxSizer*>( m_gridPanel->GetSizer() );
	gridSizer->Add( m_grid, 1, wxEXPAND );
	if ( m_textureArray )
	{
		m_grid->SetTextureArray( m_textureArray );
		m_grid->Bind( wxEVT_LEFT_DCLICK  , &CEdTextureArrayViewer::OnDoubleClick, this );
	}

	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdTextureArrayViewer::OnClose ), 0, this );

	m_textureArray->AddToRootSet();

	UpdateDownsizeLists();
	RefreshWindow();
	m_backupValid = false;

	SetSize( 500, 400 );

	if ( m_textureArray && m_textureArray->GetFile() )
	{
		SetTitle( m_textureArray->GetFile()->GetDepotPath().AsChar() );
	}
}

CEdTextureArrayViewer::~CEdTextureArrayViewer()
{
}

void CEdTextureArrayViewer::CreateBackup()
{
	if ( !m_backupValid )
	{
		m_backupTextures.Clear();
		m_textureArray->GetTextures( m_backupTextures );
		m_backupValid = true;
	}
}

void CEdTextureArrayViewer::OnChoiceListChanged( wxCommandEvent& event )
{
	CreateBackup();
}

void CEdTextureArrayViewer::UpdateThumbnails()
{
	CreateBackup();
	m_textureArray->SetTextures( m_backupTextures );
	m_grid->UpdateEntriesFromTextureArray();
}

void CEdTextureArrayViewer::OnCloseAndSave(wxCommandEvent& event)
{
	if ( m_backupValid )
	{
		m_textureArray->SetDirty( true );

		Uint32 selPC = m_pcSizeList->GetSelection();
		Uint32 selXB = m_xboneSizeList->GetSelection();
		Uint32 selPS4 = m_ps4SizeList->GetSelection();

		if ( m_textureArray->IsA<CTextureArray>() )
		{
			m_textureArray->SetPCDownscaleBias( selPC+1 );
			m_textureArray->SetXBoneDownscaleBias( selXB+1 );
			m_textureArray->SetPS4DownscaleBias( selPS4+1 );

			if(m_textureArray->GetPCDownscaleBias() > m_textureArray->GetXBoneDownscaleBias())
			{
				m_textureArray->SetXBoneDownscaleBias( m_textureArray->GetPCDownscaleBias() );
			}
			if(m_textureArray->GetPCDownscaleBias() > m_textureArray->GetPS4DownscaleBias())
			{
				m_textureArray->SetPS4DownscaleBias( m_textureArray->GetPCDownscaleBias() );
			}
		}
		m_textureArray->Save();
		UpdateDownsizeLists();
		m_backupValid = false;
	}
	Close();
}

// Make this function only read the value from the bias and set the choice lists depending on the bias
void CEdTextureArrayViewer::UpdateDownsizeLists()
{
	if ( m_textureArray->IsA<CTextureArray>() )
	{
		m_pcSizeList->Freeze();
		m_pcSizeList->Clear();
		m_xboneSizeList->Freeze();
		m_xboneSizeList->Clear();
		m_ps4SizeList->Freeze();
		m_ps4SizeList->Clear();

		//Uint8 count = 1;
		Uint16 mipCount = m_textureArray->GetMipCount();
		UINT32 count = 1;
		while( count < mipCount )
		{
			m_pcSizeList->AppendString( String::Printf( TXT("%d"), count ).AsChar() );
			m_xboneSizeList->AppendString( String::Printf( TXT("%d"), count ).AsChar() );
			m_ps4SizeList->AppendString( String::Printf( TXT("%d"), count ).AsChar() );
			count ++;
		}

		m_pcSizeList->SetSelection( m_textureArray->GetPCDownscaleBias()-1 );
		m_xboneSizeList->SetSelection( m_textureArray->GetXBoneDownscaleBias()-1 );
		m_ps4SizeList->SetSelection( m_textureArray->GetPS4DownscaleBias()-1 );

		m_pcSizeList->Thaw();
		m_pcSizeList->Refresh();
		m_xboneSizeList->Thaw();
		m_xboneSizeList->Refresh();
		m_ps4SizeList->Thaw();
		m_ps4SizeList->Refresh();

		// Update local textures copy
		m_backupValid = false;
		CreateBackup();
	}
}

void CEdTextureArrayViewer::OnCloseSavePrompt(wxCommandEvent& event)
{
	// this will invoke OnClose() method, asking user to save
	Close();
}

void CEdTextureArrayViewer::OnClose(wxCloseEvent& event)
{
	m_textureArray->RemoveFromRootSet();
	if ( m_backupValid )
	{
		Bool save = GFeedback->AskYesNo( TXT("Texture array has been modified. Do you want to save before closing?") );
		if ( save )
		{
			m_textureArray->SetDirty( true );
			m_textureArray->Save();
		}
		else
		{
			m_textureArray->Reload( false );
			m_textureArray->SetDirty( false );
		}
	}
	Destroy();
}

void CEdTextureArrayViewer::OnAddTexture( wxCommandEvent& event )
{		
	CDiskFile* selectedFile = NULL;
	CResource* textureResource = NULL;
	String resPath;

	if ( GetActiveResource( resPath ) )
	{
		selectedFile = GDepot->FindFile( resPath );
		if ( selectedFile )
		{
			if ( !selectedFile->IsLoaded() )
			{
				selectedFile->Load();
			}

			textureResource = selectedFile->GetResource();
		}
	}

	if ( textureResource && textureResource->IsA< CBitmapTexture >() )
	{
		if ( m_textureArray )
		{
			CBitmapTexture* tex = SafeCast< CBitmapTexture >( textureResource );
			if ( tex )
			{
				if ( m_textureArray->Contains( tex ) )
				{
					if ( !GFeedback->AskYesNo( TXT("Texture '%s' is already present in this array. Are you sure you want to add another instance?"), tex->GetDepotPath().AsChar() ) )
					{
						return;
					}
				}

				String error;
				if ( !CTextureArray::IsTextureValidForArray( tex, m_textureArray, error ) )
				{
					GFeedback->ShowError( error.AsChar() );
					return;
				}

				CreateBackup();
				m_backupTextures.PushBack( tex );
			}
		}
		else
		{
			return;
		}

		RefreshWindow();
	}
	else
	{
		GFeedback->ShowError(TEXT("Select texture file!"));
	}
}

static int SortIndices( const void *a, const void* b )
{
	Uint32 indexA = *((Uint32*)a);
	Uint32 indexB = *((Uint32*)b);

	if ( indexA > indexB ) return 1;
	if ( indexA < indexB ) return -1;
	return 0;
}

void CEdTextureArrayViewer::OnRemoveTexture(wxCommandEvent& event)
{
	if ( !m_textureArray )
	{
		return;
	}
	CreateBackup();
	
	qsort( m_selectedTextureIndices.TypedData(), m_selectedTextureIndices.Size(), sizeof( Int32 ), &SortIndices );
	for ( Int32 i = m_selectedTextureIndices.Size() - 1; i >= 0 ; --i )
	{
		m_backupTextures.RemoveAt( m_selectedTextureIndices[ i ] );
	}
	m_selectedTextureIndices.ClearFast();

	RefreshWindow();
}

void CEdTextureArrayViewer::OnReplaceTexture(wxCommandEvent& event)
{
	if ( !m_textureArray )
	{
		return;
	}

	if ( m_selectedTextureIndices.Size() != 1 )
	{
		GFeedback->ShowError(TEXT("Select one texture to be replaced with the selected one."));
		return;
	}
	
	CDiskFile* selectedFile = NULL;
	CResource* textureResource = NULL;
	String resPath;

	if ( GetActiveResource( resPath ) )
	{
		selectedFile = GDepot->FindFile( resPath );
		if ( selectedFile )
		{
			if ( !selectedFile->IsLoaded() )
			{
				selectedFile->Load();
			}

			textureResource = selectedFile->GetResource();
		}
	}

	if ( textureResource && textureResource->IsA< CBitmapTexture >() )
	{
		if ( m_textureArray )
		{
			CBitmapTexture* tex = SafeCast< CBitmapTexture >( textureResource );
			if ( tex )
			{
				if ( m_textureArray->Contains( tex ) )
				{
					if ( !GFeedback->AskYesNo( TXT("Texture '%s' is already present in this array. Are you sure you want to add another instance?"), tex->GetDepotPath().AsChar() ) )
					{
						return;
					}
				}

				String error;
				if ( !CTextureArray::IsTextureValidForArray( tex, m_textureArray, error ) )
				{
					GFeedback->ShowError( error.AsChar() );
					return;
				}

				CreateBackup();
				m_backupTextures.RemoveAt( m_selectedTextureIndices[ 0 ] );
				m_backupTextures.Insert( m_selectedTextureIndices[ 0 ], tex );
				m_selectedTextureIndices.ClearFast();
			}
		}
		else
		{
			return;
		}

		RefreshWindow();
	}
	else
	{
		GFeedback->ShowError(TEXT("Select texture file!"));
	}
}

void CEdTextureArrayViewer::OnDoubleClick( wxMouseEvent & event )
{
	if ( !m_textureArray )
	{
		return;
	}

	if ( m_selectedTextureIndices.Size() != 1 )
	{
		return;
	}

	TDynArray< CBitmapTexture*> bitmaps;
	if ( m_backupValid )
	{
		bitmaps = m_backupTextures;
	}
	else
	{
		m_textureArray->GetTextures( bitmaps );
	}

	CBitmapTexture* selectedBitmap = bitmaps[ m_selectedTextureIndices[0] ];
	if ( selectedBitmap )
	{
		String path = selectedBitmap->GetDepotPath();
		if ( !path.Empty() )
		{
			// Find asset disk file
			CDiskFile* diskFile = GDepot->FindFile( path );
			wxTheFrame->GetAssetBrowser()->EditAsset( diskFile->GetResource() );
		}
	}
}

void CEdTextureArrayViewer::OnSearchTexture(wxCommandEvent& event)
{
	if ( !m_textureArray )
	{
		return;
	}

	if ( m_selectedTextureIndices.Size() != 1 )
	{
		return;
	}

	TDynArray< CBitmapTexture*> bitmaps;
	if ( m_backupValid )
	{
		bitmaps = m_backupTextures;
	}
	else
	{
		m_textureArray->GetTextures( bitmaps );
	}

	CBitmapTexture* selectedBitmap = bitmaps[ m_selectedTextureIndices[0] ];
	if ( selectedBitmap )
	{
		String path = selectedBitmap->GetDepotPath();
		if ( !path.Empty() )
		{
			wxTheFrame->GetAssetBrowser()->SelectFile( path );
		}
	}
}

static int SortIndicesForShift( const void *a, const void* b )
{
	return -SortIndices( a, b );
}

void CEdTextureArrayViewer::OnShiftTextures(wxCommandEvent& event)
{
	qsort( m_selectedTextureIndices.TypedData(), m_selectedTextureIndices.Size(), sizeof( Int32 ), &SortIndicesForShift );
	CreateBackup();

	CBitmapTexture* first = m_backupTextures[m_selectedTextureIndices[0]];
	for ( Int32 i = 0; i<m_selectedTextureIndices.SizeInt() - 1; ++i )
	{
		m_backupTextures[m_selectedTextureIndices[i]] = m_backupTextures[m_selectedTextureIndices[i + 1]];
	}
	m_backupTextures[m_selectedTextureIndices[m_selectedTextureIndices.SizeInt() - 1]] = first;

	UpdateThumbnails();
	m_grid->SetAllSelected( m_selectedTextureIndices );
}

void CEdTextureArrayViewer::RefreshWindow( Bool updateThumbnails )
{
	if ( updateThumbnails )
	{
		UpdateThumbnails();
	}

	m_replaceButton->Show( m_selectedTextureIndices.Size() == 1 );
	m_searchButton->Show( m_selectedTextureIndices.Size() == 1 );
	m_deleteButton->Show( m_selectedTextureIndices.Size() >= 1 );
	m_shiftButton->Show( m_selectedTextureIndices.Size() > 1 );
	LayoutRecursively( XRCCTRL( *this, "m_buttonGrid", wxPanel ), false );
	
	// fill info about used textures, if modified - backup count is used, else information from texture array
	Char tmp[ 8 ];
	Uint32 usedSlots = m_backupValid ? m_backupTextures.Size() : ( m_textureArray ? m_textureArray->GetTextureCount() : 0 );
	Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), TXT( "%i" ), usedSlots );
	m_slotTxt->SetLabel( tmp );
}

void CEdTextureArrayViewer::OnTextureArrayGridModified( class CEdTextureArrayGrid* grid )
{
	// Update counter
	Char tmp[ 8 ];
	Uint32 usedSlots = m_textureArray->GetTextureCount();
	Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), TXT( "%i" ), usedSlots );
	m_slotTxt->SetLabel( tmp );

	// Update local textures copy
	m_backupValid = false;
	CreateBackup();
}

void CEdTextureArrayViewer::OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary )
{
	m_selectedTextureIndices = grid->GetAllSelected();
	RefreshWindow( false );
}

/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/versionControl.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"

#include "swfViewer.h"
#include "swfImagePanel.h"
#include "assetBrowser.h"
#include "colorPicker.h"

#include <wx/mstream.h>

static String FormatSize( Uint64 size );
static String FormatTextureFormat( GpuApi::eTextureFormat format );

CEdSwfViewer::STextureData::STextureData()
	: m_format( 0 )
{}

CEdSwfViewer::STextureData::~STextureData()
{
}

// Event table
BEGIN_EVENT_TABLE( CEdSwfViewer, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "swfSave" ), CEdSwfViewer::OnSave )
	EVT_MENU( XRCID( "swfBackgroundColor" ), CEdSwfViewer::OnBackgroundColor )
END_EVENT_TABLE()

CEdSwfViewer::CEdSwfViewer( wxWindow* parent, CSwfResource* swfResource )
	: wxSmartLayoutPanel( parent, TXT("SwfViewer"), false )
	, m_swfResource( swfResource )
	, m_properties( nullptr )
	, m_preview( nullptr )
	, m_thumbnailList( nullptr )
	, m_colorPicker( nullptr )
{
	m_swfResource->AddToRootSet();

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), m_swfResource->GetFriendlyName().AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MATERIALS") ) );
	SetIcon( iconSmall );

	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_preview = new CEdSwfImagePanel( rp );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesPage( rp, settings, nullptr );
		m_properties->SetObject( m_swfResource );
//		m_properties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdSwfViewer::OnPropertiesChanged ), NULL, this );
//		m_properties->Connect( wxEVT_COMMAND_PROPERTY_SELECTED, wxCommandEventHandler( CEdSwfViewer::OnPropertySelected ), nullptr, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	{
		const TDynArray< THandle< CSwfTexture > >& swfTextures = m_swfResource->GetTextures();

		m_thumbnailList = XRCCTRL( *this, "ThumbnailList", wxListCtrl );
		m_thumbnailList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxCommandEventHandler( CEdSwfViewer::OnThumbnailSelected ), nullptr, this );

		wxImageList* imageList = new wxImageList( 256, 256 );
		for ( Uint32 i = 0; i < swfTextures.Size(); ++i )
		{
			CSwfTexture* swfTexture = swfTextures[ i ];
			STextureData textureData;
			wxImage image;
			if ( GetTextureData( swfTexture, textureData ) && CreateImage( textureData, image ) )
			{
				image.Rescale( 256, 256, wxIMAGE_QUALITY_NORMAL );
				imageList->Add( wxBitmap( image ) );
			}
			else
			{
				// Keep the index filled
				imageList->Add( wxBitmap() );
			}
		}
		m_thumbnailList->AssignImageList( imageList, wxIMAGE_LIST_NORMAL );

		for ( Uint32 i = 0; i < swfTextures.Size(); ++i )
		{
			CSwfTexture* swfTexture = swfTextures[ i ];
			const Uint32 byteSize = swfTexture->GetMipCount() > 0 ? swfTexture->GetMips()[0].m_data.GetSize() : 0;
			GpuApi::eTextureFormat format = swfTexture->GetPlatformSpecificCompression();
			
			wxListItem item;
			item.SetData( swfTexture );
			item.SetColumn( 0 );
			item.SetId( i );
			item.SetImage( i );
			item.SetText( String::Printf( TXT("%ls\n%ux%u - %ls - %ls"), swfTexture->GetLinkageName().AsChar(), swfTexture->GetWidth(), swfTexture->GetHeight(), 
																 FormatSize( byteSize ).AsChar(),
																 FormatTextureFormat( format ).AsChar() ).AsChar() );
			m_thumbnailList->InsertItem( item );
		}

		if ( m_thumbnailList->GetItemCount() > 0 )
		{
			m_thumbnailList->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
		}
	}

// 	// Checking to see if resource exists. If not then we Grey out the restore button 
// 	if( ! CheckIfSourceExists() )
// 	{
// 		XRCCTRL( *this, "restoreBTN", wxButton )->Disable();
// 	}

	m_colorPicker = new CEdColorPicker( this );
	m_colorPicker->Bind( wxEVT_COMMAND_SCROLLBAR_UPDATED, &CEdSwfViewer::OnColorPicked, this );

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	// Update and finalize layout
	GetParent()->Layout();
	Layout();
	LoadOptionsFromConfig();
	Show();
}

CEdSwfViewer::~CEdSwfViewer()
{
	SaveOptionsToConfig();

	m_swfResource->RemoveFromRootSet();

	if ( m_colorPicker )
	{
		delete m_colorPicker;
	}

	SEvents::GetInstance().UnregisterListener( this );
}

wxString CEdSwfViewer::GetShortTitle()
{
	return m_swfResource->GetFile()->GetFileName().AsChar() + wxString(TXT(" - SwfViewer"));
}

Bool CEdSwfViewer::CheckIfSourceExists()
{
// 	String path = m_texture->GetImportFile();
// 	return GVersionControl->DoesFileExist( path );
	return true;
}

void CEdSwfViewer::OnRestore( wxCommandEvent& event )
{
// 	GFeedback->BeginTask( TXT("Restoring..."), false );
// 	GFeedback->UpdateTaskProgress(1,2);

// 	if ( m_texture->IsA<CBitmapTexture>() )
// 	{
// 		IMaterial::RecompileMaterialsUsingTexture( Cast< CBitmapTexture >( m_texture ) );
// 		CDrawableComponent::RecreateProxiesOfRenderableComponents();
// 	}
	m_properties->RefreshValues();

// 	GFeedback->UpdateTaskProgress(2,2);
// 	GFeedback->EndTask();
}

void CEdSwfViewer::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/SwfViewer"));

	ISavableToConfig::SaveSession();
}

void CEdSwfViewer::LoadOptionsFromConfig()
{
	// WTF?
	//CEdShortcutsEditor::Load(*this->GetMenuBar(), GetOriginalLabel());
	// Load layout after the shortcuts (duplicate menu after the shortcuts loading)
	LoadLayout(TXT("/Frames/SwfViewer"));

	ISavableToConfig::RestoreSession();
}

void CEdSwfViewer::SaveSession( CConfigurationManager& config )
{
	config.Write( TXT("/Frames/SwfViewer/BackgroundColor"), (Int32)m_preview->GetBackgroundColour().GetRGB() );
}

void CEdSwfViewer::RestoreSession( CConfigurationManager& config )
{
	wxColor bgColor = wxColor( (Uint32)config.Read( TXT("/Frames/SwfViewer/BackgroundColor"), (Int32)wxCYAN->GetRGB() ) );
	m_preview->SetBackgroundColour( bgColor );
	m_preview->Refresh();
}

void CEdSwfViewer::OnPropertiesChanged( wxCommandEvent& event )
{
	//m_preview->SetTexture( m_texture, m_lodBias );
}

void CEdSwfViewer::OnPropertySelected( wxCommandEvent& event )
{
// 	CBasePropItem* ai = m_properties->GetActiveItem();
// 	CSwfTexture* swfTex = ( ai && ai->GetNumObjects() > 0 ) ? ai->GetParentObject( 0 ).As< CSwfTexture >() : nullptr;

//	SetCurrentImage( swfTex );
}

void CEdSwfViewer::OnThumbnailSelected( wxCommandEvent& event )
{
	Int32 sel = event.GetSelection();
	if ( sel < 0 )
	{
		return;
	}

	Uint32 selectedItem = m_thumbnailList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	CSwfTexture* swfTexture = reinterpret_cast< CSwfTexture* >( m_thumbnailList->GetItemData( selectedItem ) );
	RED_ASSERT( swfTexture );
	if ( ! swfTexture )
	{
		return;
	}
	STextureData textureData;
	if ( swfTexture && GetTextureData( swfTexture, textureData ) )
	{
		SetCurrentImage( textureData );
	}
}

void CEdSwfViewer::OnSave( wxCommandEvent& event )
{
	m_swfResource->Save();
	m_properties->RefreshValues();
}

void CEdSwfViewer::OnBackgroundColor( wxCommandEvent& event )
{
	m_colorPicker->Show( Color::CYAN );
}

void CEdSwfViewer::OnColorPicked( wxCommandEvent& event )
{
	if ( m_colorPicker )
	{
		Color bgColor = m_colorPicker->GetColor();
		m_preview->SetBackgroundColour( wxColour( bgColor.R, bgColor.G, bgColor.B ) );
		m_preview->Refresh();
	}
}

void CEdSwfViewer::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
// 	if ( name == CNAME( FileReloadConfirm ) )
// 	{
// 		CResource* res = GetEventData< CResource* >( data );
// 		if ( res == m_texture )
// 		{
// 			CReloadFileInfo* reloadInfo = new CReloadFileInfo( res, NULL, GetTitle().wc_str() );
// 			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( reloadInfo ) );
// 		}
// 	}
// 	else if ( name == CNAME( FileReload ) )
// 	{
// 		CReloadFileInfo* reloadInfo = GetEventData< CReloadFileInfo* >( data );
// 
// 		if ( reloadInfo->m_newResource->IsA<ITexture>() )
// 		{
// 			ITexture* oldTex = (ITexture*)(reloadInfo->m_oldResource);
// 			ITexture* newTex = (ITexture*)(reloadInfo->m_newResource);
// 			if ( oldTex == m_texture )
// 			{
// 				m_texture = newTex;
// 				m_texture->AddToRootSet();
// 
// 				m_preview->Reload();
// 
// 				wxTheFrame->GetAssetBrowser()->OnEditorReload( m_texture, this );
// 			}
// 		}
// 	}
}

// void CEdSwfViewer::ClearCurrentImage()
// {
// 	 m_preview->SetSourceImage( wxImage() );
// }

void CEdSwfViewer::SetCurrentImage( const STextureData& textureData )
{
	wxImage image;
	if ( CreateImage( textureData, image ) )
	{
		m_preview->SetSourceImage( image );
	}
	else
	{
		m_preview->SetSourceImage( wxImage() );
	}
}

void CEdSwfViewer::DumpTexturesToFile()
{
	const TDynArray< THandle< CSwfTexture > >& swfTextures = m_swfResource->GetTextures();
	for ( Uint32 i = 0; i < swfTextures.Size(); ++i )
	{
		CSwfTexture* swfTexture = swfTextures[ i ];
		STextureData textureData;
		if ( GetTextureData( swfTexture, textureData ) )
		{
			textureData.m_mipCopy.m_data.Load();
			Uint8* srcData = static_cast< Uint8* >( textureData.m_mipCopy.m_data.GetData() );
			const size_t srcDataSize = textureData.m_mipCopy.m_data.GetSize();
			const size_t width = textureData.m_mipCopy.m_width;
			const size_t height = textureData.m_mipCopy.m_height;
			const GpuApi::eTextureFormat format = static_cast< GpuApi::eTextureFormat >( textureData.m_format );
			const size_t ddsPitch = textureData.m_mipCopy.m_pitch; // Already converted for DDS

			GpuApi::eTextureSaveFormat saveFormat = GpuApi::SAVE_FORMAT_DDS;
			GpuApi::TextureDataDesc textureToSave;
			textureToSave.data = &srcData;
			textureToSave.slicePitch = srcDataSize;
			textureToSave.width = width;
			textureToSave.height = height;
			textureToSave.format = format;
			textureToSave.rowPitch = ddsPitch;

			String fileName = swfTexture->GetLinkageName();

			GpuApi::SaveTextureToFile( textureToSave, String::Printf(TXT("d:\\flashdump\\%ls"), fileName.AsChar() ).AsChar(), saveFormat );
		}
	}
}

Bool CEdSwfViewer::GetTextureData( const CSwfTexture* swfTexture, STextureData& outTextureData )
{
	if ( ! swfTexture || swfTexture->GetMipCount() < 1 )
	{
		return false;
	}

	GpuApi::eTextureFormat format = swfTexture->GetPlatformSpecificCompression();
	outTextureData.m_mipCopy = swfTexture->GetMips()[0];
	outTextureData.m_format = static_cast< Uint32 >( format );

	// Programming guide for DDS
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
	Uint32 blockSize = 0;
	switch (format)
	{
	case GpuApi::TEXFMT_BC1:
		blockSize = 8;
		break;
	case GpuApi::TEXFMT_BC3:
		blockSize = 16;
		break;
	default:
		RED_HALT( "Unsupported texture format %u", (Uint32)format );
		break;
	}

	if ( blockSize < 1 )
	{
		return false;
	}

	outTextureData.m_mipCopy.m_pitch = Max<Uint32>( 1, (outTextureData.m_mipCopy.m_width + 3 )/4 ) * blockSize;

	return true;
}

Bool CEdSwfViewer::CreateImage( const STextureData& textureData, wxImage& outImage )
{
	const_cast< STextureData& >( textureData ).m_mipCopy.m_data.Load();

	const Uint8* srcData = static_cast< const Uint8* >( textureData.m_mipCopy.m_data.GetData() );
	const size_t srcDataSize = textureData.m_mipCopy.m_data.GetSize();
	const size_t width = textureData.m_mipCopy.m_width;
	const size_t height = textureData.m_mipCopy.m_height;
	const GpuApi::eTextureFormat format = static_cast< GpuApi::eTextureFormat >( textureData.m_format );
	const size_t ddsPitch = textureData.m_mipCopy.m_pitch; // Already converted for DDS

	void* data = nullptr;
	size_t dataSize = 0;
	if ( GpuApi::SaveTextureToMemory( srcData, srcDataSize, width, height, format, ddsPitch, GpuApi::SAVE_FORMAT_PNG, &data, dataSize ) )
	{
		wxMemoryInputStream stream( data, dataSize );
		outImage.LoadFile( stream );
		GpuApi::FreeTextureData( data );

		return true;
	}

	return false;
}

static String FormatSize( Uint64 size )
{
	Float KB = 1024.f;
	Float MB = KB * KB;
	Float GB = KB * KB * KB;

	if ( size < KB )
	{
		return String::Printf(TXT("%llu bytes"), size );
	}
	else if ( size < MB )
	{
		return String::Printf(TXT("%.2f KB"), size / KB );
	}
	else if ( size < GB )
	{
		return String::Printf(TXT("%.2f MB"), size / MB );
	}

	return String::Printf(TXT("%.2f GB"), size / GB );
}

static String FormatTextureFormat( GpuApi::eTextureFormat format )
{
	String formatName = TXT("<Unsupported format>");

	switch( format )
	{
		case GpuApi::TEXFMT_BC1:
			formatName = TXT("DXT1");
			break;
		case GpuApi::TEXFMT_BC3:
			formatName = TXT("DXT5");
			break;
		default:
			break;
	}

	return formatName;
}

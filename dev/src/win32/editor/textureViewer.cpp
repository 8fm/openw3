/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "textureViewer.h"
#include "assetBrowser.h"
#include "texturePreviewPanel.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/material.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/cubeTexture.h"

// Event table
BEGIN_EVENT_TABLE( CEdTextureViewer, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "textureSave" ), CEdTextureViewer::OnSave )
	EVT_SLIDER( XRCID( "mipSlider"), CEdTextureViewer::OnMipChange )
	EVT_BUTTON( XRCID( "pcMinusBTN"), CEdTextureViewer::OnDownsize )
	EVT_BUTTON( XRCID( "xboneMinusBTN"), CEdTextureViewer::OnDownsize )
	EVT_BUTTON( XRCID( "ps4MinusBTN"), CEdTextureViewer::OnDownsize )
	EVT_BUTTON( XRCID( "restoreBTN"), CEdTextureViewer::OnRestore )
	EVT_BUTTON( XRCID( "saveBTN"), CEdTextureViewer::OnSave )
	EVT_UPDATE_UI( wxID_ANY, CEdTextureViewer::OnUpdateUI )
END_EVENT_TABLE()

CEdTextureViewer::CEdTextureViewer( wxWindow* parent, CResource* texture )
	: wxSmartLayoutPanel( parent, TXT("TextureViewer"), false )
	, m_texture( texture )
	, m_lodBias( 0.f )
{
	// Keep reference to resource as long as editor is opened
	m_texture->AddToRootSet();

	m_bitmapTexture = nullptr;
	m_pcSizeList = XRCCTRL( *this, "pcDownscaleBiasChoice", wxChoice );
	m_xboneSizeList = XRCCTRL( *this, "xboneDownscaleBiasChoice", wxChoice );
	m_ps4SizeList = XRCCTRL( *this, "ps4DownscaleBiasChoice", wxChoice );

	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), m_texture->GetFriendlyName().AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MATERIALS") ) );
	SetIcon( iconSmall );

	// Create rendering panel
	{
		wxPanel* rp = XRCCTRL( *this, "PreviewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_preview = new CEdTexturePreviewPanel( rp, m_texture, m_lodBias );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_properties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_properties->Get().SetObject( m_texture );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdTextureViewer::OnPropertiesChanged ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// set the range for the slider
	OnUpdateMipMapSlider();
	
	// Setting the lists of resolutions
	UpdateDownsizeLists();

	SEvents::GetInstance().RegisterListener( CNAME( FileReload ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );

	// Update and finalize layout
	GetParent()->Layout();
	Layout();
	LoadOptionsFromConfig();
	Show();
}

CEdTextureViewer::~CEdTextureViewer()
{
	SaveOptionsToConfig();

	m_texture->RemoveFromRootSet();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdTextureViewer::OnUpdateMipMapSlider()
// set the range for the slider
{
	wxSlider* slider = XRCCTRL( *this, "mipSlider", wxSlider );
	slider->SetMin( 0 );

	Uint32 sliderMax = 11;
	if ( m_texture->IsA<CBitmapTexture>() )
	{
		m_bitmapTexture = SafeCast< CBitmapTexture >( m_texture );
		sliderMax = m_bitmapTexture->GetMipCount();
	}
	else if ( m_texture->IsA<CCubeTexture>() )
	{
		CCubeTexture* texCube = SafeCast< CCubeTexture >( m_texture );
		sliderMax = texCube->GetMipCount();
	}

	slider->SetMax( sliderMax );
}


void CEdTextureViewer::OnMipChange( wxCommandEvent& event )
{
	wxSlider* mipSlider = (wxSlider*)event.GetEventObject();
	m_lodBias = (Float)mipSlider->GetValue();

	m_preview->SetTexture( m_texture, m_lodBias );
}

// Make this function only read the value from the bias and set the choice lists depending on the bias
void CEdTextureViewer::UpdateDownsizeLists()
{
	if ( m_texture->IsA<CBitmapTexture>() )
	{
		m_pcSizeList->Freeze();
		m_pcSizeList->Clear();
		m_xboneSizeList->Freeze();
		m_xboneSizeList->Clear();
		m_ps4SizeList->Freeze();
		m_ps4SizeList->Clear();

		// This is only PC
		Uint32 pcScaledHeightSize = m_bitmapTexture->GetHeight();
		Uint32 pcScaledWidthSize = m_bitmapTexture->GetWidth();

		// If they are different then by dividing the difference to the power of 2 is correct size
		UINT32 xboneScaledHeightSize = pcScaledHeightSize/pow( 2, ( m_bitmapTexture->GetXBoneDownscaleBias() - m_bitmapTexture->GetPCDownscaleBias() ) );
		UINT32 xboneScaledWidthSize = pcScaledWidthSize/pow( 2, ( m_bitmapTexture->GetXBoneDownscaleBias() - m_bitmapTexture->GetPCDownscaleBias() ) );
		UINT32 ps4ScaledHeightSize = pcScaledHeightSize/pow( 2, ( m_bitmapTexture->GetPS4DownscaleBias() - m_bitmapTexture->GetPCDownscaleBias() ) );
		UINT32 ps4ScaledWidthSize = pcScaledWidthSize/pow( 2, ( m_bitmapTexture->GetPS4DownscaleBias() - m_bitmapTexture->GetPCDownscaleBias() ) );

		Uint32 mipCount = m_bitmapTexture->GetMipCount();
		// IF it is not lower than 16x16 we can resize it
		Uint32 count = 1;
		while ( count < mipCount )
		{
			if( pcScaledWidthSize >=2 && pcScaledHeightSize >= 2 )
			{
				m_pcSizeList->AppendString( String::Printf( TXT("%dx%d"), pcScaledWidthSize, pcScaledHeightSize ).AsChar() );
				pcScaledHeightSize /= 2;
				pcScaledWidthSize /= 2;
			}
			if( xboneScaledWidthSize >=2 && xboneScaledHeightSize >= 2 )
			{
				m_xboneSizeList->AppendString( String::Printf( TXT("%dx%d"), xboneScaledWidthSize, xboneScaledHeightSize ).AsChar() );
				xboneScaledHeightSize /= 2;
				xboneScaledWidthSize /= 2;
			}
			if( ps4ScaledWidthSize >=2 && ps4ScaledHeightSize >= 2 )
			{
				m_ps4SizeList->AppendString( String::Printf( TXT("%dx%d"), ps4ScaledWidthSize, ps4ScaledHeightSize ).AsChar() );
				ps4ScaledHeightSize /= 2;
				ps4ScaledWidthSize /= 2;
			}
			count ++;
		}

		m_pcSizeList->SetSelection( 0 );
		m_xboneSizeList->SetSelection( 0 );
		m_ps4SizeList->SetSelection( 0 );

		m_pcSizeList->Thaw();
		m_pcSizeList->Refresh();
		m_xboneSizeList->Thaw();
		m_xboneSizeList->Refresh();
		m_ps4SizeList->Thaw();
		m_ps4SizeList->Refresh();
	}
}

Bool CEdTextureViewer::CheckIfSourceExists()
{
	if ( m_texture )
	{
		String path = m_texture->GetImportFile();
		return wxFileExists( path.AsChar() );
	}
	else
	{
		return false;
	}
}

void CEdTextureViewer::OnRestore( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Restoring..."), false );
	GFeedback->UpdateTaskProgress(1,2);
	
	if ( m_texture->IsA<CBitmapTexture>() )
	{
		m_bitmapTexture->SetPCDownscaleBias( 1 );
		m_bitmapTexture->SetXBoneDownscaleBias( 1 );
		m_bitmapTexture->SetPS4DownscaleBias( 1 );

		GVersionControl->GetLatest( m_texture->GetImportFile() );
		wxTheFrame->GetAssetBrowser()->ReimportResource( m_texture );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		IMaterial::RecompileMaterialsUsingTexture( Cast< CBitmapTexture >( m_texture ) );
#endif // NO_RUNTIME_MATERIAL_COMPILATION
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
	m_properties->Get().RefreshValues();
	
	UpdateDownsizeLists();
	OnUpdateMipMapSlider();

	GFeedback->UpdateTaskProgress(2,2);
	GFeedback->EndTask();
}

void CEdTextureViewer::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/TextureViewer"));
}

void CEdTextureViewer::LoadOptionsFromConfig()
{
	// WTF?
	//CEdShortcutsEditor::Load(*this->GetMenuBar(), GetOriginalLabel());
	// Load layout after the shortcuts (duplicate menu after the shortcuts loading)
	LoadLayout(TXT("/Frames/TextureViewer"));
}

void CEdTextureViewer::OnPropertiesChanged( wxCommandEvent& event )
{
	m_preview->SetTexture( m_texture, m_lodBias );
}

void CEdTextureViewer::OnSave( wxCommandEvent& event )
{
	Uint32 selPC = m_pcSizeList->GetSelection();
	Uint32 selXB = m_xboneSizeList->GetSelection();
	Uint32 selPS4 = m_ps4SizeList->GetSelection();

	// if selPC is not 0 it has changed.
	// Need to check if the selection of pc is higher than any other platform.
	// If it is than we force the other platform to be the pc selection
	if( selPC != 0 || selPS4 != 0 || selXB != 0 )
	{
		if ( m_texture->IsA<CBitmapTexture>() )
		{
			m_bitmapTexture->SetPCDownscaleBias( m_bitmapTexture->GetPCDownscaleBias() + selPC );
			m_bitmapTexture->SetXBoneDownscaleBias( m_bitmapTexture->GetXBoneDownscaleBias() + selXB );
			m_bitmapTexture->SetPS4DownscaleBias( m_bitmapTexture->GetPS4DownscaleBias() + selPS4 );

			if(m_bitmapTexture->GetPCDownscaleBias() > m_bitmapTexture->GetXBoneDownscaleBias())
			{
				m_bitmapTexture->SetXBoneDownscaleBias( m_bitmapTexture->GetPCDownscaleBias() );
			}
			if(m_bitmapTexture->GetPCDownscaleBias() > m_bitmapTexture->GetPS4DownscaleBias())
			{
				m_bitmapTexture->SetPS4DownscaleBias( m_bitmapTexture->GetPCDownscaleBias() );
			}

			if( selPC != 0  )
			{
				m_bitmapTexture->DropMipmaps(selPC);
				OnUpdateMipMapSlider();
			}
		}
	}
	
	m_texture->Save();
	m_properties->Get().RefreshValues();
	UpdateDownsizeLists();

	if ( m_texture->IsA<CBitmapTexture>() )
	{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		IMaterial::RecompileMaterialsUsingTexture( Cast< CBitmapTexture >( m_texture ) );
#endif
	}

	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

void CEdTextureViewer::OnDownsize( wxCommandEvent& event )
{
	if( event.GetId() == XRCID("pcMinusBTN") )
	{
		// If the length of the list allows lets change it
		Uint32 sel = m_pcSizeList->GetSelection();
		if( (sel + 1) <= m_pcSizeList->GetCount() )
		{
			m_pcSizeList->SetSelection(sel + 1);
		}
	}
	else if( event.GetId() == XRCID("xboneMinusBTN") )
	{
		// If the length of the list allows lets change it
		Uint32 sel = m_xboneSizeList->GetSelection();
		if( (sel + 1) <= m_xboneSizeList->GetCount() )
		{
			m_xboneSizeList->SetSelection(sel + 1);
		}
	}
	else if( event.GetId() == XRCID("ps4MinusBTN") )
	{
		// If the length of the list allows lets change it
		Uint32 sel = m_ps4SizeList->GetSelection();
		if( (sel + 1) <= m_ps4SizeList->GetCount() )
		{
			m_ps4SizeList->SetSelection(sel + 1);
		}
	}
}

void CEdTextureViewer::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		if ( res == m_texture )
		{
			SEvents::GetInstance().QueueEvent( CNAME( FileReloadToConfirm ), CreateEventData( CReloadFileInfo( res, NULL, GetTitle().wc_str() ) ) );
		}
	}
	else if ( name == CNAME( FileReload ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );

		if ( reloadInfo.m_newResource->IsA<ITexture>() )
		{
			ITexture* oldTex = (ITexture*)(reloadInfo.m_oldResource);
			ITexture* newTex = (ITexture*)(reloadInfo.m_newResource);
			if ( oldTex == m_texture )
			{
				m_texture = newTex;
				m_texture->AddToRootSet();

				m_preview->Reload();

				wxTheFrame->GetAssetBrowser()->OnEditorReload( m_texture, this );
			}
		}
	}
}

void CEdTextureViewer::OnUpdateUI( wxUpdateUIEvent& event )
{
	XRCCTRL( *this, "restoreBTN", wxButton )->Enable( CheckIfSourceExists() );
}

/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "downscaleBiasDlg.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/material.h"
#include "resourceIterator.h"
#include "assetBrowser.h"

BEGIN_EVENT_TABLE( CEdDownscaleBiasDlg, wxDialog )
	EVT_BUTTON( XRCID("batchFolderBTN"), CEdDownscaleBiasDlg::OnBatch )
END_EVENT_TABLE()

CEdDownscaleBiasDlg::CEdDownscaleBiasDlg( wxWindow* parent, CContextMenuDir* contextMenuDir )
	: m_contextMenuDir( *contextMenuDir )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("DownscaleBiasViewer") );
	
	m_pcChoice = XRCCTRL( *this, "pcChoice", wxChoice );
	m_xboneChoice = XRCCTRL( *this, "xboneChoice", wxChoice );
	m_ps4Choice = XRCCTRL( *this, "ps4Choice", wxChoice );
}

void CEdDownscaleBiasDlg::OnBatch( wxCommandEvent &event )
{
	if( m_pcChoice->GetSelection() <= m_xboneChoice->GetSelection() && m_pcChoice->GetSelection() <= m_ps4Choice->GetSelection() )
	{
		for ( CResourceIteratorAdapter< CBitmapTexture > atexture( m_contextMenuDir, TXT("Checking out Textures...") ); atexture; ++atexture )
		{
			Downscale( atexture.Get() );
		}
	}
	else
	{
		GFeedback->ShowMsg( TXT("Wrong input"), TXT("XBOX and PS4 Downscale bias needs to be lower or equal to PC") );
	}

	Close();
}

void CEdDownscaleBiasDlg::Downscale( CBitmapTexture* atexture )
{
	if( m_pcChoice->GetSelection() == 0 )
	{
		atexture->SetPCDownscaleBias(1);
		atexture->SetXBoneDownscaleBias(1);
		atexture->SetPS4DownscaleBias(1);
		wxTheFrame->GetAssetBrowser()->ReimportResource( atexture );
	}

	Uint32 pcSelected = m_pcChoice->GetSelection()+1;
	Uint32 xboneSelected = m_xboneChoice->GetSelection()+1;
	Uint32 ps4Selected = m_ps4Choice->GetSelection()+1;

	atexture->SetPCDownscaleBias( pcSelected );
	atexture->SetXBoneDownscaleBias( xboneSelected );
	atexture->SetPS4DownscaleBias( ps4Selected );

	atexture->DropMipmaps( pcSelected-1 );

	if ( atexture->IsA<CBitmapTexture>() )
	{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		IMaterial::RecompileMaterialsUsingTexture( Cast< CBitmapTexture >( atexture ) );
#endif
	}

	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}
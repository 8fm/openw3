/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "..\engine\redGuiComboBox.h"
#include "..\engine\redGuiLabel.h"
#include "..\engine\redGuiPanel.h"
#include "..\engine\redGuiImage.h"
#include "..\engine\redGuiManager.h"
#include "..\engine\redGuiAdvancedSlider.h"
#include "..\engine\redGuiGraphicContext.h"
#include "..\engine\redGuiGridLayout.h"
#include "..\engine\redGuiSpin.h"
#include "debugWindowDynamicTextures.h"
#include "..\engine\renderFrame.h"
#include "..\engine\redGuiCheckBox.h"

namespace DebugWindows
{
	CDebugWindowDynamicTextures::CDynamicTexturesImage::CDynamicTexturesImage( Uint32 x, Uint32 y, Uint32 width, Uint32 height ) 
		: RedGui::CRedGuiControl( x, y, width, height )
		, m_textureRef( GpuApi::TextureRef::Null() )
		, m_minColorFactor( 0.0f )
		, m_maxColorFactor( 1.0f )
		, m_maxMips( 1 )
		, m_maxSlices( 1 )
		, m_channelSelector( Vector::ONES )
	{
		/* intentionally empty */
	}

	CDebugWindowDynamicTextures::CDynamicTexturesImage::~CDynamicTexturesImage()
	{
		m_textureRef = GpuApi::TextureRef::Null();
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::Draw()
	{
		if( m_textureRef != GpuApi::TextureRef::Null() )
		{
			CRenderFrame* frame = GRedGui::GetInstance().GetRenderManager()->GetGraphicContext()->GetRenderTarget();
			if( frame != nullptr )
			{
				frame->AddDebugDynamicTexture( GetAbsoluteLeft(), GetAbsoluteTop(), GetWidth(), GetHeight(), 
					m_textureRef, m_currentMip, m_currentSlice, m_minColorFactor, m_maxColorFactor, m_channelSelector );
			}
		}
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::OnMouseWheel( Int32 delta )
	{
		if( delta > 0 )
		{
			if( m_currentSlice > 0 )
			{
				--m_currentSlice;
				EventChangeState( this );
			}
		}
		else
		{
			if( m_currentSlice < m_maxSlices - 1 )
			{
				++m_currentSlice;
				EventChangeState( this );
			}
		}
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::ChangeChannelSelector( EChannel channel, Bool value )
	{
		switch ( channel )
		{
		case EChannel_R:
			m_channelSelector.X = value ? 1.f : 0.f;
			break;
		case EChannel_G:
			m_channelSelector.Y = value ? 1.f : 0.f;
			break;
		case EChannel_B:
			m_channelSelector.Z = value ? 1.f : 0.f;
			break;
		case EChannel_A:
			m_channelSelector.W = value ? 1.f : 0.f;
			break;
		}
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::OnMouseButtonClick(const Vector2& mousePosition, RedGui::EMouseButton button)
	{
		if( button == RedGui::MB_Left )
		{
			m_currentSlice = (m_currentSlice+1)%m_maxSlices;
			EventChangeState( this );
		}
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::Reset()
	{
		m_minColorFactor = 0.0f;
		m_maxColorFactor = 1.0f;
		m_maxMips = 1;
		m_maxSlices = 1;
		m_currentMip = 0;
		m_currentSlice = 0;
	}

	void CDebugWindowDynamicTextures::CDynamicTexturesImage::SetTextureRef( GpuApi::TextureRef textureRef )
	{
		m_textureRef = textureRef;

		
		EventChangeState( this );
	}

	CDebugWindowDynamicTextures::CDebugWindowDynamicTextures()
		: RedGui::CRedGuiWindow( 200, 200, 600, 600 )
	{
		SetCaption( TXT("Dynamic textures") );
		CreateControls();
	}

	CDebugWindowDynamicTextures::~CDebugWindowDynamicTextures()
	{
		/* intentionally empty */
	}

	void CDebugWindowDynamicTextures::OnWindowOpened( CRedGuiControl* control )
	{
		m_textureType->ClearAllItems();
		const Uint32 textureCount = GpuApi::GetNumDynamicTextures();
		for( Uint32 i=0; i<textureCount; ++i )
		{
			String textureName = ANSI_TO_UNICODE( GpuApi::GetDynamicTextureName( i ) );
			m_textureType->AddItem( textureName );
		}
	}

	void CDebugWindowDynamicTextures::OnWindowClosed( CRedGuiControl* control )
	{
		m_textureImage->SetTextureRef( GpuApi::TextureRef::Null() );
	}

	void CDebugWindowDynamicTextures::CreateControls()
	{
		RedGui::CRedGuiPanel* topPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 30 );
		topPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		topPanel->SetBorderVisible( false );
		topPanel->SetBackgroundColor( Color::CLEAR );
		topPanel->SetDock( RedGui::DOCK_Top );
		AddChild( topPanel );
		{
			RedGui::CRedGuiLabel* dynamicTextureLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
			dynamicTextureLabel->SetMargin( Box2( 5, 8, 5, 5 ) );
			dynamicTextureLabel->SetDock( RedGui::DOCK_Left );
			dynamicTextureLabel->SetText( TXT("Dynamic texture type:") );
			topPanel->AddChild( dynamicTextureLabel );

			m_textureType = new RedGui::CRedGuiComboBox( 0, 0, 100, 20 );
			m_textureType->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_textureType->SetDock( RedGui::DOCK_Fill );
			m_textureType->EventSelectedIndexChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyDynamicTextureChanged );
			topPanel->AddChild( m_textureType );
		}

		RedGui::CRedGuiPanel* informationPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 200 );
		informationPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		informationPanel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
		informationPanel->SetDock( RedGui::DOCK_Top );
		AddChild( informationPanel );
		{
			RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 200 );
			layout->SetDock( RedGui::DOCK_Fill );
			layout->SetDimensions( 2, 1 );
			informationPanel->AddChild( layout );
			{
				// slot 1,1
				RedGui::CRedGuiGridLayout* informationLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 200 );
				informationLayout->SetDock( RedGui::DOCK_Fill );
				informationLayout->SetDimensions( 1, 4 );
				layout->AddChild( informationLayout );
				{
					m_formatLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					m_formatLabel->SetText( TXT("Format: ") );
					informationLayout->AddChild( m_formatLabel );

					m_typeLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					m_typeLabel->SetText( TXT("Type: ") );
					informationLayout->AddChild( m_typeLabel );

					m_sizeLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					m_sizeLabel->SetText( TXT("Size: ") );
					informationLayout->AddChild( m_sizeLabel );

					m_mipLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					m_mipLabel->SetText( TXT("Current mip: ") );
					informationLayout->AddChild( m_mipLabel );
				}

				// slot 2,1
				RedGui::CRedGuiGridLayout* optionsLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 200 );
				optionsLayout->SetDock( RedGui::DOCK_Fill );
				optionsLayout->SetDimensions( 1, 4 );
				layout->AddChild( optionsLayout );
				{
					RedGui::CRedGuiPanel* firstPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 50 );
					firstPanel->SetBorderVisible( false );
					optionsLayout->AddChild( firstPanel );
					{
						RedGui::CRedGuiLabel* minColorFactorLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 40 );
						minColorFactorLabel->SetDock( RedGui::DOCK_Left );
						minColorFactorLabel->SetMargin( Box2( 5, 5, 5, 5 ) );
						minColorFactorLabel->SetText( TXT("Min color factor") );
						firstPanel->AddChild( minColorFactorLabel );

						m_minColorFactorSpin = new RedGui::CRedGuiSpin( 0, 0, 45, 20 );
						m_minColorFactorSpin->SetBorderVisible( false );
						m_minColorFactorSpin->SetMargin( Box2( 3, 5, 3, 20 ) );
						m_minColorFactorSpin->SetDock( RedGui::DOCK_Right );
						m_minColorFactorSpin->SetMinValue( 0 );
						m_minColorFactorSpin->SetMaxValue( 65536 );
						m_minColorFactorSpin->SetValue( 2 );
						m_minColorFactorSpin->EventValueChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventValueChangedMinColorFactor );
						firstPanel->AddChild( m_minColorFactorSpin );

						m_minColorFactor = new RedGui::CRedGuiAdvancedSlider( 0, 0, 100, 40 );
						m_minColorFactor->SetMinValue( -2.0f );
						m_minColorFactor->SetMaxValue( 2.0f );
						m_minColorFactor->SetStepValue( 0.1f );
						m_minColorFactor->SetValue( 0.0f );
						m_minColorFactor->SetDock( RedGui::DOCK_Fill );
						m_minColorFactor->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_minColorFactor->EventScroll.Bind( this, &CDebugWindowDynamicTextures::NotifySliderChanged );
						firstPanel->AddChild( m_minColorFactor );
					}

					RedGui::CRedGuiPanel* secondPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 50 );
					secondPanel->SetBorderVisible( false );
					optionsLayout->AddChild( secondPanel );
					{
						RedGui::CRedGuiLabel* maxColorFactorLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 40 );
						maxColorFactorLabel->SetDock( RedGui::DOCK_Left );
						maxColorFactorLabel->SetMargin( Box2( 5, 5, 5, 5 ) );
						maxColorFactorLabel->SetText( TXT("Max color factor") );
						secondPanel->AddChild( maxColorFactorLabel );

						m_maxColorFactorSpin = new RedGui::CRedGuiSpin( 0, 0, 45, 20 );
						m_maxColorFactorSpin->SetBorderVisible( false );
						m_maxColorFactorSpin->SetMargin( Box2( 3, 5, 3, 25 ) );
						m_maxColorFactorSpin->SetDock( RedGui::DOCK_Right );
						m_maxColorFactorSpin->SetMinValue( 0 );
						m_maxColorFactorSpin->SetMaxValue( 65536 );
						m_maxColorFactorSpin->SetValue( 2 );
						m_maxColorFactorSpin->EventValueChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventValueChangedMaxColorFactor );
						secondPanel->AddChild( m_maxColorFactorSpin );

						m_maxColorFactor = new RedGui::CRedGuiAdvancedSlider( 0, 0, 100, 40 );
						m_maxColorFactor->SetMinValue( -2.0f );
						m_maxColorFactor->SetMaxValue( 2.0f );
						m_maxColorFactor->SetStepValue( 0.1f );
						m_maxColorFactor->SetValue( 1.0f );
						m_maxColorFactor->SetDock( RedGui::DOCK_Fill );
						m_maxColorFactor->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_maxColorFactor->EventScroll.Bind( this, &CDebugWindowDynamicTextures::NotifySliderChanged );
						secondPanel->AddChild( m_maxColorFactor );
					}

					RedGui::CRedGuiPanel* thirdPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 50 );
					thirdPanel->SetBorderVisible( false );
					optionsLayout->AddChild( thirdPanel );
					{
						RedGui::CRedGuiLabel* mipsLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
						mipsLabel->SetDock( RedGui::DOCK_Left );
						mipsLabel->SetMargin( Box2( 5, 15, 5, 5 ) );
						mipsLabel->SetText( TXT("Mips") );
						thirdPanel->AddChild( mipsLabel );

						m_mips = new RedGui::CRedGuiComboBox( 0, 0, 100, 20 );
						m_mips->SetMargin( Box2( 5, 15, 5, 15 ) );
						m_mips->SetDock( RedGui::DOCK_Fill );
						m_mips->EventSelectedIndexChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyMipChanged );
						thirdPanel->AddChild( m_mips );
					}
					
					RedGui::CRedGuiPanel* channelsPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 50 );
					channelsPanel->SetBorderVisible( false );
					optionsLayout->AddChild( channelsPanel );
					{
						RedGui::CRedGuiLabel* channelsLabel = new RedGui::CRedGuiLabel( 0, 0, 50, 20 );
						channelsLabel->SetDock( RedGui::DOCK_Left );
						channelsLabel->SetMargin( Box2( 5, 5, 5, 5 ) );
						channelsLabel->SetText( TXT("Visible channels:") );
						channelsPanel->AddChild( channelsLabel );

						m_channelRCheckBox = new RedGui::CRedGuiCheckBox( 0, 0, 50, 30 );
						m_channelRCheckBox->SetText( TXT("R") );
						m_channelRCheckBox->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_channelRCheckBox->SetDock( RedGui::DOCK_Left );
						m_channelRCheckBox->EventCheckedChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventChannelCheckBoxClicked );
						channelsPanel->AddChild( m_channelRCheckBox );

						m_channelGCheckBox = new RedGui::CRedGuiCheckBox( 0, 0, 50, 30 );
						m_channelGCheckBox->SetText( TXT("G") );
						m_channelGCheckBox->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_channelGCheckBox->SetDock( RedGui::DOCK_Left );
						m_channelGCheckBox->EventCheckedChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventChannelCheckBoxClicked );
						channelsPanel->AddChild( m_channelGCheckBox );

						m_channelBCheckBox = new RedGui::CRedGuiCheckBox( 0, 0, 50, 30 );
						m_channelBCheckBox->SetText( TXT("B") );
						m_channelBCheckBox->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_channelBCheckBox->SetDock( RedGui::DOCK_Left );
						m_channelBCheckBox->EventCheckedChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventChannelCheckBoxClicked );
						channelsPanel->AddChild( m_channelBCheckBox );

						m_channelACheckBox = new RedGui::CRedGuiCheckBox( 0, 0, 50, 30 );
						m_channelACheckBox->SetText( TXT("A") );
						m_channelACheckBox->SetMargin( Box2( 5, 5, 5, 5 ) );
						m_channelACheckBox->SetDock( RedGui::DOCK_Fill );
						m_channelACheckBox->EventCheckedChanged.Bind( this, &CDebugWindowDynamicTextures::NotifyEventChannelCheckBoxClicked );
						channelsPanel->AddChild( m_channelACheckBox );

						m_controlsToChannelsMap.Insert( m_channelRCheckBox, EChannel_R );
						m_controlsToChannelsMap.Insert( m_channelGCheckBox, EChannel_G );
						m_controlsToChannelsMap.Insert( m_channelBCheckBox, EChannel_B );
						m_controlsToChannelsMap.Insert( m_channelACheckBox, EChannel_A );
					}
				}
			}
		}

		m_textureImage = new CDynamicTexturesImage( 0, 0, 100, 30 );
		m_textureImage->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_textureImage->SetDock( RedGui::DOCK_Fill );
		m_textureImage->EventChangeState.Bind( this, &CDebugWindowDynamicTextures::NotifyTextureStateChanged );
		AddChild( m_textureImage );

		m_channelRCheckBox->SetChecked( true );
		m_channelGCheckBox->SetChecked( true );
		m_channelBCheckBox->SetChecked( true );
		m_channelACheckBox->SetChecked( false );
	}

	void CDebugWindowDynamicTextures::NotifyTextureStateChanged( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		m_mipLabel->SetText( String::Printf( TXT("Current slice: %d / %d"), m_textureImage->GetCurrentSlice() + 1, m_textureImage->GetMaxSlices() ) );
	}

	String CDebugWindowDynamicTextures::GpiApiFormatToStr( GpuApi::eTextureFormat format )
	{
		switch ( format )
		{
		case GpuApi::TEXFMT_A8: return TXT("A8");
		case GpuApi::TEXFMT_L8: return TXT("L8");
		case GpuApi::TEXFMT_R8G8B8X8: return TXT("RGBX8");
		case GpuApi::TEXFMT_R8G8B8A8: return TXT("RGBA8");
		case GpuApi::TEXFMT_A8L8: return TXT("A8L8");
		case GpuApi::TEXFMT_Uint_16_norm: return TXT("Uint16Norm");
		case GpuApi::TEXFMT_Uint_16: return TXT("Uint16");
		case GpuApi::TEXFMT_Uint_32: return TXT("Uint32");
		case GpuApi::TEXFMT_R16G16_Uint: return TXT("RG16");
		case GpuApi::TEXFMT_Float_R10G10B10A2: return TXT("RGB10A2");
	#ifdef RED_PLATFORM_DURANGO
		case GpuApi::TEXFMT_R10G10B10_6E4_A2_FLOAT: return TXT("R10G10B10_6E4_A2_FLOAT");
	#endif
		case GpuApi::TEXFMT_Float_R16G16B16A16: return TXT("RGBA16F");
		case GpuApi::TEXFMT_Float_R11G11B10: return TXT("R11G11B10F");
		case GpuApi::TEXFMT_Float_R16G16: return TXT("RG16F");
		case GpuApi::TEXFMT_Float_R32G32: return TXT("RG32F");
		case GpuApi::TEXFMT_Float_R32G32B32A32: return TXT("RGBA32F");
		case GpuApi::TEXFMT_Float_R32: return TXT("R32F");
		case GpuApi::TEXFMT_Float_R16: return TXT("R16F");
		case GpuApi::TEXFMT_D24S8: return TXT("D24S8");
		case GpuApi::TEXFMT_D24FS8: return TXT("D24FS8");
		case GpuApi::TEXFMT_BC1: return TXT("BC1");
		case GpuApi::TEXFMT_BC2: return TXT("BC2");
		case GpuApi::TEXFMT_BC3: return TXT("BC3");
		case GpuApi::TEXFMT_BC4: return TXT("BC4");
		case GpuApi::TEXFMT_BC5: return TXT("BC5");
		case GpuApi::TEXFMT_BC6H: return TXT("BC6H");
		case GpuApi::TEXFMT_BC7: return TXT("BC7");
		case GpuApi::TEXFMT_R8_Uint: return TXT("R8_Uint");
		}

		return TXT("Unknown");
	}

	String CDebugWindowDynamicTextures::GpiApiTexTypeToStr( GpuApi::eTextureType type )
	{
		switch ( type )
		{
		case GpuApi::TEXTYPE_2D: return TXT("2D");
		case GpuApi::TEXTYPE_CUBE: return TXT("CUBE");
		case GpuApi::TEXTYPE_ARRAY: return TXT("ARRAY");
		}

		return TXT("Unknown");
	}

	void CDebugWindowDynamicTextures::NotifyDynamicTextureChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		m_textureImage->Reset();

		// get texture info
		GpuApi::TextureRef textureRef = GpuApi::GetDynamicTextureRef( value ); 
		m_textureImage->SetTextureRef( textureRef );
		if( textureRef != GpuApi::TextureRef::Null() )
		{
			GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( textureRef );
			m_textureImage->SetMaxMips( desc.initLevels );
			m_textureImage->SetMaxSlices( desc.sliceNum );
			m_textureImage->SetMinColorFactor( 0.0f );
			m_textureImage->SetMaxColorFactor( 1.0f );

			// update gui
			m_formatLabel->SetText( TXT("Format: ") + GpiApiFormatToStr( desc.format ) );
			m_typeLabel->SetText( TXT("Type: ") + GpiApiTexTypeToStr( desc.type ) );
			m_sizeLabel->SetText( String::Printf( TXT("Size: %dx%d"), desc.width, desc.height ) );
			m_minColorFactor->SetValue( 0.0f );
			m_maxColorFactor->SetValue( 1.0f );

			m_mips->ClearAllItems();
			for( Uint32 i=0; i<desc.initLevels; ++i )
			{
				m_mips->AddItem( String::Printf( TXT("Mip %d"), i ) );
			}
			m_mips->SetSelectedIndex( 0 );
		}
	}

	void CDebugWindowDynamicTextures::NotifySliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_minColorFactor )
		{
			if ( value > m_textureImage->GetMaxColorFactor() ) 
			{
				m_textureImage->SetMaxColorFactor( value + 0.1f );
				m_maxColorFactor->SetValue( value + 0.1f );
			}
			m_textureImage->SetMinColorFactor( value );
		}
		else if( sender == m_maxColorFactor )
		{
			if ( value < m_textureImage->GetMinColorFactor() ) 
			{
				m_textureImage->SetMinColorFactor( value - 0.1f );
				m_minColorFactor->SetValue( value - 0.1f );
			}
			m_textureImage->SetMaxColorFactor( value );
		}
	}

	void CDebugWindowDynamicTextures::NotifyMipChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		m_textureImage->SetCurrentMip( value );
	}

	void CDebugWindowDynamicTextures::NotifyEventValueChangedMinColorFactor( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		m_minColorFactor->SetMinValue( (Float)-value );
		m_minColorFactor->SetMaxValue( (Float)value );
	}

	void CDebugWindowDynamicTextures::NotifyEventValueChangedMaxColorFactor( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		m_maxColorFactor->SetMinValue( (Float)-value );
		m_maxColorFactor->SetMaxValue( (Float)value );
	}

	void CDebugWindowDynamicTextures::NotifyEventChannelCheckBoxClicked( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();
		if ( m_controlsToChannelsMap.KeyExist( sender ) )
		{
			m_textureImage->ChangeChannelSelector( m_controlsToChannelsMap.GetRef( sender ), value );
		}
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
